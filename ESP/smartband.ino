/*
  ESP32 Smartband Prototype
  -------------------------
  Hardware : ESP32 DevKit, MPU6050, (MAX30102 optional), 1.3" I2C OLED (SH1106)

  Features
    - Heart rate / SpO2 / PPG (real MAX30102 OR simulated)
    - Acceleration, G-force, linear force, activity level, injury detection
    - Critical heart-rate / SpO2 events
    - Data logging to flash (LittleFS, CSV files) + history restored on boot
    - Graphs on the OLED, one section per dataset (HR, SpO2, Motion, Mood, Tasks)
    - Recurring to-do reminders
    - Routine health checkup task (15 s guided measurement)
    - Mood prompts (1-5), logged and plotted
    - Periodic "data review" event that shows all graphs automatically

  Wiring: SDA -> GPIO 21 | SCL -> GPIO 22 | VCC -> 3.3V | GND -> GND
          Button (optional extra): GPIO 27 to GND. By default the onboard BOOT
          button (GPIO 0) is used.

  Libraries: U8g2 (olikraus). LittleFS and Preferences are built into the ESP32 core.
             SparkFun MAX3010x only if SIMULATE_VITALS = 0.

  EVENTS ARE NOW STARTED BY THE BOOT BUTTON (no automatic cycle).
  Set AUTO_EVENTS to 1 to also run the timed schedule.

  BUTTON (BOOT button)
    Main screen : tap = open EVENT MENU
    Event menu  : tap = next item          | hold = START the selected event
                  (reminders, health checkup, mood, graphs, and the simulated
                   critical vitals: high HR, low HR, low SpO2, normal)
    Reminder    : tap = DONE               | hold = LATER (snooze)
    Mood prompt : tap = change 1..5        | hold = SAVE
    Checkup     : tap = START              | hold = LATER
    Graphs      : tap = next section       | hold = exit
    Do not hold BOOT while pressing reset (it enters flash mode).

  SERIAL COMMANDS (115200 baud, line ending = Newline)
    y / n        same as tap / hold
    1-5          answer mood prompt directly
    s0..s4       simulation: 0 auto, 1 normal, 2 tachy, 3 brady, 4 low SpO2
    m  c  g      trigger mood prompt / health checkup / graphs now
    l            list the three reminders
    d            dump all log files (copy into a spreadsheet)
    x            erase all logs
    h            help
*/

#include <Wire.h>
#include <U8g2lib.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <stdarg.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// ============================ CONFIG ============================
#define SIMULATE_VITALS 1     // 1 = simulated HR/SpO2/PPG, 0 = real MAX30102
#define AUTO_EVENTS 0         // 0 = events start only from the BOOT-button menu
                              // 1 = events also fire on the timed schedule

#if AUTO_EVENTS
#define LATER_TXT "Later"
#else
#define LATER_TXT "Skip"
#endif

#define SDA_PIN 21
#define SCL_PIN 22
#define LED_PIN 2             // on while any critical alert is active
#define BTN_PIN 0             // BOOT button (active LOW). Use 27 for an external button.

// ---- A.I.M.S. dashboard link (WiFi) ----
#define ENABLE_AIMS_LINK 1                 // 1 = WiFi API for aims.html, 0 = no WiFi
#define AIMS_PUSH 0                        // 1 = also POST the data to AIMS_URL below
const char* WIFI_SSID   = "KFON BROADBAND.";
const char* WIFI_PASS   = "maryh@123";
// Only used when AIMS_PUSH = 1 (a backend that accepts POSTs)
const char* AIMS_URL    = "http://192.168.1.50:8000/api/esp/telemetry";
const char* AIMS_KEY    = "";              // same value as AIMS_ESP_KEY on the backend (optional)
const char* CREW_ID     = "crew-1";        // crewman wearing this band
// Body part the band is worn on. One of:
// head, torso, left_arm, right_arm, left_leg, right_leg
const char* SENSOR_PART = "right_arm";

#if !SIMULATE_VITALS
#include "MAX30105.h"
#include "spo2_algorithm.h"
#endif

// Most 1.3" I2C OLEDs are SH1106. If yours is SSD1306, use the second line.
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

// ---- MPU6050 (read directly, works with clone chips) ----
#define MPU_ADDR 0x68
#define ACCEL_LSB_PER_G 2048.0        // +/-16g range

// ---- Injury detection (TESTING VALUES - very sensitive) ----
const float EFFECTIVE_MASS_KG      = 1.0;
const float INJURY_FORCE_THRESHOLD = 12.0;   // N
const unsigned long ALERT_HOLD_MS  = 5000;

// ---- Critical vital-sign thresholds ----
const int HR_HIGH_CRIT  = 120;
const int HR_LOW_CRIT   = 50;
const int SPO2_LOW_CRIT = 90;
const int HR_HYST       = 5;
const int SPO2_HYST     = 2;
const int CONFIRM_SEC   = 3;

// ---- Activity classification ----
const float ACTIVITY_WALK_G = 0.10;
const float ACTIVITY_RUN_G  = 0.60;

// ---- Scheduling (DEMO values in seconds; "real" suggestions in comments) ----
const unsigned long LOG_INTERVAL_SEC     = 10;    // real: 60
const unsigned long GRAPH_INTERVAL_SEC   = 100;   // real: 3600 (hourly data review)
const unsigned long GRAPH_SECTION_SEC    = 6;     // seconds per graph section
const unsigned long MOOD_INTERVAL_SEC    = 150;   // real: 4 * 3600
const unsigned long CHECKUP_INTERVAL_SEC = 300;   // real: 24 * 3600
const unsigned long CHECKUP_SEC          = 15;    // measurement length
const unsigned long SNOOZE_SEC           = 20;    // real: 600
const unsigned long REMINDER_TIMEOUT_MS  = 30000;
const unsigned long MOOD_TIMEOUT_MS      = 45000;
const unsigned long CHECKUP_PROMPT_TIMEOUT_MS = 45000;
const unsigned long RESULT_SHOW_MS       = 10000;
const unsigned long POPUP_GAP_MS         = 3000;  // pause between popups
const unsigned long LONG_PRESS_MS        = 800;
const unsigned long MENU_TIMEOUT_MS      = 15000; // menu closes if left alone

// ---- Checkup "normal at rest" ranges ----
const int CK_HR_MIN = 50, CK_HR_MAX = 100, CK_SPO2_MIN = 95;

// ---- Storage ----
#define HIST_LEN 64                              // points kept per graph
const size_t MAX_FILE_BYTES = 150000;            // a file restarts when it gets this big
#define MAX_TASKS 3                              // exactly three reminders

// ============================ TYPES =============================
struct Series {
  float v[HIST_LEN];
  int n;
};

struct Task {
  char name[32];
  unsigned long intervalSec;
  unsigned long nextDue;
  bool pending;
  bool active;
};

enum Screen {
  SCR_MAIN, SCR_MENU, SCR_REMINDER, SCR_MOOD,
  SCR_CHECK_PROMPT, SCR_CHECK_RUN, SCR_CHECK_RESULT, SCR_GRAPHS
};

// ============================ GLOBALS ===========================
// Vitals
int displayHR = 0, displaySpO2 = 0;
bool fingerPresent = false;
uint32_t ppgWave[128];

// Vital alerts
bool hrHighAlert = false, hrLowAlert = false, spo2LowAlert = false;
int hrHighCnt = 0, hrLowCnt = 0, spo2LowCnt = 0;
int totalEvents = 0;

// Motion
float gravX = 0, gravY = 9.80665, gravZ = 0;
bool gravityInit = false;
float accelMag_g = 1.0, linAccel = 0, linForce = 0, activityLevel_g = 0;
float peakForce = 0, intervalPeak = 0;
bool injuryAlert = false;
unsigned long alertStart = 0;

// Data history (graphs)
Series serHR, serSpO2, serAct, serMood, serTasks;

// Storage
bool fsOk = false;
unsigned int session = 0;
unsigned long lastLogSec = 0;

// Tasks & scheduler
Task tasks[MAX_TASKS];
bool pendingCheckup = false, pendingMood = false, pendingGraphs = false;
unsigned long nextCheckupDue = 0, nextMoodDue = 0, nextGraphsDue = 0;

// UI state
Screen screen = SCR_MAIN;
unsigned long screenStartMs = 0, lastPopupEndMs = 0;
int curTask = -1;
int moodSel = 3;
int graphSection = 0;
unsigned long graphSecStartMs = 0;

// Event menu (items 0-2 map to the three reminders in order)
int menuSel = 0;
#if SIMULATE_VITALS
#define MENU_COUNT 10
const char* menuLabels[MENU_COUNT] = {
  "Drink water", "Exercise", "Fluid checkups", "Health checkup", "Mood check-in",
  "View graphs", "Sim: High HR", "Sim: Low HR", "Sim: Low SpO2", "Sim: Normal"
};
#else
#define MENU_COUNT 6
const char* menuLabels[MENU_COUNT] = {
  "Drink water", "Exercise", "Fluid checkups", "Health checkup", "Mood check-in",
  "View graphs"
};
#endif

// Checkup state
unsigned long ckStartMs = 0;
long ckSumHR = 0, ckSumSpO2 = 0;
float ckSumAct = 0;
int ckCount = 0, ckActCount = 0;
int ckAvgHR = 0, ckAvgSpO2 = 0;
char ckResult[24] = "";

unsigned long lastMotionMs = 0, lastDisplayMs = 0, lastSerialMs = 0, lastTickMs = 0;

#if SIMULATE_VITALS
float simHR = 72, simSpO2 = 98;
float targetHR = 72, targetSpO2 = 98;
float ppgPhase = 0;
int simMode = 1;                               // 1 = normal; scenarios start from the menu
unsigned long lastSimSampleMs = 0;
#else
MAX30105 particleSensor;
const uint32_t FINGER_IR_THRESHOLD = 50000;
#define BUFFER_LEN 100
#define UPDATE_EVERY 25
uint32_t irBuffer[BUFFER_LEN];
uint32_t redBuffer[BUFFER_LEN];
int bufferCount = 0, newSamples = 0;
int32_t spo2 = 0, heartRate = 0;
int8_t validSPO2 = 0, validHR = 0;
#endif

// ============================ HELPERS ===========================
unsigned long nowSec() { return millis() / 1000UL; }

bool anyAlert() { return injuryAlert || hrHighAlert || hrLowAlert || spo2LowAlert; }

const char* moodLabel(int m) {
  static const char* labels[5] = {"Awful", "Low", "Okay", "Good", "Great"};
  if (m < 1) m = 1;
  if (m > 5) m = 5;
  return labels[m - 1];
}

void seriesPush(Series &s, float x) {
  if (s.n < HIST_LEN) {
    s.v[s.n++] = x;
  } else {
    memmove(s.v, s.v + 1, sizeof(float) * (HIST_LEN - 1));
    s.v[HIST_LEN - 1] = x;
  }
}

void seriesStats(Series &s, float &mn, float &mx, float &avg) {
  mn = 1e9; mx = -1e9;
  float sum = 0;
  for (int i = 0; i < s.n; i++) {
    if (s.v[i] < mn) mn = s.v[i];
    if (s.v[i] > mx) mx = s.v[i];
    sum += s.v[i];
  }
  avg = s.n ? sum / s.n : 0;
}

// ============================ STORAGE ===========================
// Append one CSV line to a file (writes the header for new files).
void appendLine(const char* path, const char* header, const char* fmt, ...) {
  if (!fsOk) return;

  bool fresh = !LittleFS.exists(path);
  if (!fresh) {
    File t = LittleFS.open(path, "r");
    if (t) {
      size_t sz = t.size();
      t.close();
      if (sz > MAX_FILE_BYTES) {
        LittleFS.remove(path);
        fresh = true;
        Serial.printf("[LOG] %s was full - restarted\n", path);
      }
    }
  }

  File f = LittleFS.open(path, "a");
  if (!f) return;
  if (fresh) f.println(header);

  char line[128];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(line, sizeof(line), fmt, ap);
  va_end(ap);
  f.println(line);
  f.close();
}

void logEventCsv(const char* type, const char* detail) {
  appendLine("/events.csv", "session,t_sec,type,detail",
             "%u,%lu,%s,%s", session, nowSec(), type, detail);
}

void logTaskCsv(const char* name, const char* result) {
  appendLine("/tasks.csv", "session,t_sec,task,result",
             "%u,%lu,%s,%s", session, nowSec(), name, result);
}

// Periodic vitals/motion sample -> flash + graph history
void logVitalsSample() {
  int hr = (fingerPresent && displayHR > 0) ? displayHR : 0;
  int o2 = (fingerPresent && displaySpO2 > 0) ? displaySpO2 : 0;

  appendLine("/vitals.csv", "session,t_sec,hr,spo2,peak_force_N,activity_g",
             "%u,%lu,%d,%d,%.2f,%.3f", session, nowSec(), hr, o2, intervalPeak, activityLevel_g);

  if (hr > 0) seriesPush(serHR, hr);
  if (o2 > 0) seriesPush(serSpO2, o2);
  seriesPush(serAct, activityLevel_g);
  intervalPeak = 0;
}

// Reload the most recent history from flash so graphs survive reboots
void restoreHistory() {
  char line[128];

  File f = LittleFS.open("/vitals.csv", "r");
  if (f) {
    while (f.available()) {
      int n = f.readBytesUntil('\n', line, sizeof(line) - 1);
      line[n] = 0;
      if (n < 5 || line[0] < '0' || line[0] > '9') continue;
      unsigned s; unsigned long t; int hr, o2; float pk, act;
      if (sscanf(line, "%u,%lu,%d,%d,%f,%f", &s, &t, &hr, &o2, &pk, &act) == 6) {
        if (hr > 0) seriesPush(serHR, hr);
        if (o2 > 0) seriesPush(serSpO2, o2);
        seriesPush(serAct, act);
      }
    }
    f.close();
  }

  f = LittleFS.open("/mood.csv", "r");
  if (f) {
    while (f.available()) {
      int n = f.readBytesUntil('\n', line, sizeof(line) - 1);
      line[n] = 0;
      if (n < 5 || line[0] < '0' || line[0] > '9') continue;
      unsigned s; unsigned long t; int m;
      if (sscanf(line, "%u,%lu,%d", &s, &t, &m) == 3) seriesPush(serMood, m);
    }
    f.close();
  }

  f = LittleFS.open("/tasks.csv", "r");
  if (f) {
    while (f.available()) {
      int n = f.readBytesUntil('\n', line, sizeof(line) - 1);
      line[n] = 0;
      if (n < 5 || line[0] < '0' || line[0] > '9') continue;
      char* r = strrchr(line, ',');
      if (!r) continue;
      if (strncmp(r + 1, "done", 4) == 0) seriesPush(serTasks, 1);
      else if (strncmp(r + 1, "missed", 6) == 0) seriesPush(serTasks, 0);
    }
    f.close();
  }
}

void dumpFile(const char* path) {
  File f = LittleFS.open(path, "r");
  if (!f) { Serial.printf("(no file %s yet)\n", path); return; }
  Serial.printf("----- %s (%u bytes) -----\n", path, (unsigned)f.size());
  while (f.available()) Serial.write(f.read());
  f.close();
  Serial.println();
}

void eraseLogs() {
  const char* files[] = {"/vitals.csv", "/events.csv", "/mood.csv", "/tasks.csv", "/checkups.csv"};
  for (int i = 0; i < 5; i++) LittleFS.remove(files[i]);
  serHR.n = serSpO2.n = serAct.n = serMood.n = serTasks.n = 0;
  Serial.println("[LOG] All logs erased");
}

// ============================ TASKS =============================
bool addTask(const char* name, unsigned long intervalSec) {
  for (int i = 0; i < MAX_TASKS; i++) {
    if (!tasks[i].active) {
      strncpy(tasks[i].name, name, sizeof(tasks[i].name) - 1);
      tasks[i].name[sizeof(tasks[i].name) - 1] = 0;
      for (char* c = tasks[i].name; *c; c++) if (*c == ',') *c = ';';   // keep CSV valid
      tasks[i].intervalSec = intervalSec;
      tasks[i].nextDue = nowSec() + intervalSec;
      tasks[i].pending = false;
      tasks[i].active = true;
      return true;
    }
  }
  return false;
}

void listTasks() {
  Serial.println("--- Reminders ---");
  for (int i = 0; i < MAX_TASKS; i++) {
    if (!tasks[i].active) continue;
#if AUTO_EVENTS
    long inSec = (long)tasks[i].nextDue - (long)nowSec();
    Serial.printf("%d. %s  (every %lu min, next in %lds%s)\n", i + 1, tasks[i].name,
                  tasks[i].intervalSec / 60, inSec > 0 ? inSec : 0, tasks[i].pending ? ", DUE" : "");
#else
    Serial.printf("%d. %s  (start from the BOOT-button menu)\n", i + 1, tasks[i].name);
#endif
  }
#if AUTO_EVENTS
  long ck = (long)nextCheckupDue - (long)nowSec();
  long md = (long)nextMoodDue - (long)nowSec();
  Serial.printf("Health checkup: next in %lds | Mood prompt: next in %lds\n",
                ck > 0 ? ck : 0, md > 0 ? md : 0);
#endif
}

// ====================== SCREEN / EVENT FLOW =====================
void closeToMain() {
  screen = SCR_MAIN;
  lastPopupEndMs = millis();
}

// A critical alert always takes over; interrupted events stay pending
void abortToMain() {
  if (screen == SCR_CHECK_RUN) pendingCheckup = true;
  if (screen == SCR_GRAPHS) pendingGraphs = true;
  closeToMain();
}

void startReminder(int i) {
  curTask = i;
  screen = SCR_REMINDER;
  screenStartMs = millis();
  Serial.printf("[REMINDER] %s  (tap/y = done, hold/n = later)\n", tasks[i].name);
}

void taskDone(int i) {
  tasks[i].pending = false;
  tasks[i].nextDue = nowSec() + tasks[i].intervalSec;
  seriesPush(serTasks, 1);
  logTaskCsv(tasks[i].name, "done");
  logEventCsv("task_done", tasks[i].name);
  Serial.printf("[TASK] Done: %s\n", tasks[i].name);
  closeToMain();
}

void taskSnooze(int i) {
  tasks[i].pending = false;
  tasks[i].nextDue = nowSec() + SNOOZE_SEC;
  logEventCsv("task_snoozed", tasks[i].name);
  Serial.printf("[TASK] Snoozed: %s\n", tasks[i].name);
  closeToMain();
}

void taskMissed(int i) {
  tasks[i].pending = false;
  tasks[i].nextDue = nowSec() + tasks[i].intervalSec;
  seriesPush(serTasks, 0);
  logTaskCsv(tasks[i].name, "missed");
  logEventCsv("task_missed", tasks[i].name);
  Serial.printf("[TASK] Missed: %s\n", tasks[i].name);
  closeToMain();
}

// ---- Mood ----
void startMood() {
  moodSel = 3;
  screen = SCR_MOOD;
  screenStartMs = millis();
  Serial.println("[MOOD] How are you feeling? tap = change, hold = save, or type 1-5 (1 awful ... 5 great)");
}

void confirmMood() {
  appendLine("/mood.csv", "session,t_sec,mood", "%u,%lu,%d", session, nowSec(), moodSel);
  logEventCsv("mood", moodLabel(moodSel));
  seriesPush(serMood, moodSel);
  Serial.printf("[MOOD] Logged %d (%s)\n", moodSel, moodLabel(moodSel));
  pendingMood = false;
  nextMoodDue = nowSec() + MOOD_INTERVAL_SEC;
  closeToMain();
}

void skipMood() {
  logEventCsv("mood_skipped", "timeout");
  Serial.println("[MOOD] Skipped");
  pendingMood = false;
  nextMoodDue = nowSec() + MOOD_INTERVAL_SEC;
  closeToMain();
}

// ---- Routine health checkup ----
void startCheckupPrompt() {
  screen = SCR_CHECK_PROMPT;
  screenStartMs = millis();
  Serial.println("[CHECKUP] Routine health checkup due (tap/y = start, hold/n = later)");
}

void checkupLater() {
  pendingCheckup = false;
  nextCheckupDue = nowSec() + SNOOZE_SEC;
  logEventCsv("checkup_postponed", "later");
  Serial.println("[CHECKUP] Postponed");
  closeToMain();
}

void checkupMissed() {
  pendingCheckup = false;
  nextCheckupDue = nowSec() + CHECKUP_INTERVAL_SEC;
  seriesPush(serTasks, 0);
  logTaskCsv("Health checkup", "missed");
  logEventCsv("checkup_missed", "timeout");
  Serial.println("[CHECKUP] Missed");
  closeToMain();
}

void startCheckupRun() {
  screen = SCR_CHECK_RUN;
  ckStartMs = millis();
  ckSumHR = ckSumSpO2 = 0;
  ckSumAct = 0;
  ckCount = ckActCount = 0;
  Serial.printf("[CHECKUP] Measuring for %lu s - sit still and relax\n", CHECKUP_SEC);
}

void cancelCheckup() {
  pendingCheckup = false;
  nextCheckupDue = nowSec() + SNOOZE_SEC;
  logEventCsv("checkup_cancelled", "user");
  Serial.println("[CHECKUP] Cancelled");
  closeToMain();
}

void finishCheckup() {
  float avgAct = ckActCount ? ckSumAct / ckActCount : 0;

  if (ckCount < 3) {
    ckAvgHR = 0; ckAvgSpO2 = 0;
    strcpy(ckResult, "No valid data");
  } else {
    ckAvgHR = ckSumHR / ckCount;
    ckAvgSpO2 = ckSumSpO2 / ckCount;
    if (ckAvgSpO2 < CK_SPO2_MIN)      strcpy(ckResult, "Low SpO2 - review");
    else if (ckAvgHR > CK_HR_MAX)     strcpy(ckResult, "High HR - review");
    else if (ckAvgHR < CK_HR_MIN)     strcpy(ckResult, "Low HR - review");
    else                              strcpy(ckResult, "Normal");
  }

  appendLine("/checkups.csv", "session,t_sec,avg_hr,avg_spo2,avg_activity_g,result",
             "%u,%lu,%d,%d,%.3f,%s", session, nowSec(), ckAvgHR, ckAvgSpO2, avgAct, ckResult);
  logTaskCsv("Health checkup", "done");
  logEventCsv("checkup_done", ckResult);
  seriesPush(serTasks, 1);

  Serial.printf("[CHECKUP] Done: HR %d, SpO2 %d%% -> %s\n", ckAvgHR, ckAvgSpO2, ckResult);

  pendingCheckup = false;
  nextCheckupDue = nowSec() + CHECKUP_INTERVAL_SEC;
  screen = SCR_CHECK_RESULT;
  screenStartMs = millis();
}

// ---- Graph showcase ----
void startGraphs() {
  screen = SCR_GRAPHS;
  graphSection = 0;
  graphSecStartMs = millis();
  logEventCsv("graphs_shown", "review");
  Serial.println("[GRAPHS] Data review (tap/y = next, hold/n = exit)");
}

void endGraphs() {
  pendingGraphs = false;
  nextGraphsDue = nowSec() + GRAPH_INTERVAL_SEC;
  closeToMain();
}

void nextGraphSection() {
  graphSection++;
  graphSecStartMs = millis();
  if (graphSection >= 5) endGraphs();
}

// ---- Event menu ----
void openMenu() {
  screen = SCR_MENU;
  screenStartMs = millis();
}

// Starts the chosen event right away (popups open through dispatchEvents)
void runMenuItem(int i) {
  Serial.printf("[MENU] %s\n", menuLabels[i]);
  logEventCsv("menu", menuLabels[i]);
  closeToMain();
  lastPopupEndMs = millis() - POPUP_GAP_MS;     // skip the pause between popups

  if (i >= 0 && i <= 2) {
    if (tasks[i].active) tasks[i].pending = true;
  } else if (i == 3) {
    pendingCheckup = true;
  } else if (i == 4) {
    pendingMood = true;
  } else if (i == 5) {
    pendingGraphs = true;
  }
#if SIMULATE_VITALS
  else if (i == 6) simMode = 2;                 // high heart rate
  else if (i == 7) simMode = 3;                 // low heart rate
  else if (i == 8) simMode = 4;                 // low SpO2
  else if (i == 9) simMode = 1;                 // back to normal
#endif
}

// ---- Input: act 1 = short press (tap), 2 = long press (hold) ----
void handleAction(int act) {
  switch (screen) {
    case SCR_MAIN:
      if (anyAlert()) return;
      if (act == 1) openMenu();
      break;
    case SCR_MENU:
      if (act == 1) { menuSel = (menuSel + 1) % MENU_COUNT; screenStartMs = millis(); }
      else runMenuItem(menuSel);
      break;
    case SCR_REMINDER:
      if (curTask >= 0) { if (act == 1) taskDone(curTask); else taskSnooze(curTask); }
      break;
    case SCR_MOOD:
      if (act == 1) { moodSel = moodSel % 5 + 1; screenStartMs = millis(); }
      else confirmMood();
      break;
    case SCR_CHECK_PROMPT:
      if (act == 1) startCheckupRun(); else checkupLater();
      break;
    case SCR_CHECK_RUN:
      if (act == 2) cancelCheckup();
      break;
    case SCR_CHECK_RESULT:
      closeToMain();
      break;
    case SCR_GRAPHS:
      if (act == 1) nextGraphSection(); else endGraphs();
      break;
  }
}

// Popup timeouts and automatic screen progress
void updateScreen() {
  unsigned long now = millis();
  switch (screen) {
    case SCR_MENU:
      if (now - screenStartMs > MENU_TIMEOUT_MS) closeToMain();
      break;
    case SCR_REMINDER:
      if (now - screenStartMs > REMINDER_TIMEOUT_MS && curTask >= 0) taskMissed(curTask);
      break;
    case SCR_MOOD:
      if (now - screenStartMs > MOOD_TIMEOUT_MS) skipMood();
      break;
    case SCR_CHECK_PROMPT:
      if (now - screenStartMs > CHECKUP_PROMPT_TIMEOUT_MS) checkupMissed();
      break;
    case SCR_CHECK_RUN:
      if (now - ckStartMs >= CHECKUP_SEC * 1000UL) finishCheckup();
      break;
    case SCR_CHECK_RESULT:
      if (now - screenStartMs > RESULT_SHOW_MS) closeToMain();
      break;
    case SCR_GRAPHS:
      if (now - graphSecStartMs > GRAPH_SECTION_SEC * 1000UL) nextGraphSection();
      break;
    default: break;
  }
}

// Marks events as due when their time comes (called once per second)
void tickScheduler() {
  unsigned long t = nowSec();
  for (int i = 0; i < MAX_TASKS; i++) {
    if (tasks[i].active && !tasks[i].pending && t >= tasks[i].nextDue) tasks[i].pending = true;
  }
  if (!pendingCheckup && t >= nextCheckupDue) pendingCheckup = true;
  if (!pendingMood && t >= nextMoodDue) pendingMood = true;
  if (!pendingGraphs && t >= nextGraphsDue) pendingGraphs = true;
}

// Opens the next due event when the band is idle and no critical alert is active
void dispatchEvents() {
  if (screen != SCR_MAIN || anyAlert()) return;
  if (millis() - lastPopupEndMs < POPUP_GAP_MS) return;

  if (pendingCheckup) { startCheckupPrompt(); return; }
  for (int i = 0; i < MAX_TASKS; i++) {
    if (tasks[i].active && tasks[i].pending) { startReminder(i); return; }
  }
  if (pendingMood)   { startMood(); return; }
  if (pendingGraphs) { startGraphs(); return; }
}

// ===================== SERIAL COMMANDS / BUTTON =================
void printHelp() {
  Serial.println("Commands: y/n (tap/hold) | 1-5 mood | s0-s4 sim scenario | m mood | c checkup | g graphs");
  Serial.println("          l list reminders | d dump logs | x erase logs | h help");
}

void processCommand(char* cmd) {
  while (*cmd == ' ') cmd++;
  if (!*cmd) return;

  if (screen == SCR_MOOD && cmd[1] == 0 && cmd[0] >= '1' && cmd[0] <= '5') {
    moodSel = cmd[0] - '0';
    confirmMood();
    return;
  }
  if (!strcmp(cmd, "y")) { handleAction(1); return; }
  if (!strcmp(cmd, "n")) { handleAction(2); return; }

#if SIMULATE_VITALS
  if (cmd[0] == 's' && cmd[1] >= '0' && cmd[1] <= '4' && cmd[2] == 0) {
    simMode = cmd[1] - '0';
    const char* names[] = {"AUTO cycle", "NORMAL", "TACHYCARDIA", "BRADYCARDIA", "LOW SpO2"};
    Serial.printf("[SIM] Scenario: %s\n", names[simMode]);
    return;
  }
#endif

  if (!strcmp(cmd, "m")) { pendingMood = true;    Serial.println("[MOOD] Prompt queued"); return; }
  if (!strcmp(cmd, "c")) { pendingCheckup = true; Serial.println("[CHECKUP] Queued"); return; }
  if (!strcmp(cmd, "g")) { pendingGraphs = true;  Serial.println("[GRAPHS] Queued"); return; }
  if (!strcmp(cmd, "l")) { listTasks(); return; }
  if (!strcmp(cmd, "h")) { printHelp(); return; }
  if (!strcmp(cmd, "x")) { eraseLogs(); return; }
  if (!strcmp(cmd, "d")) {
    dumpFile("/vitals.csv"); dumpFile("/events.csv"); dumpFile("/mood.csv");
    dumpFile("/tasks.csv");  dumpFile("/checkups.csv");
    return;
  }

  Serial.println("Unknown command. Send 'h' for help.");
}

void handleSerial() {
  static char buf[64];
  static int len = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      buf[len] = 0;
      if (len > 0) processCommand(buf);
      len = 0;
    } else if (len < (int)sizeof(buf) - 1) {
      buf[len++] = c;
    }
  }
}

// Returns 0 = nothing, 1 = short press, 2 = long press
int pollButton() {
  static bool lastRaw = HIGH, stable = HIGH, longFired = false;
  static unsigned long lastChange = 0, downAt = 0;

  bool raw = digitalRead(BTN_PIN);
  unsigned long now = millis();
  if (raw != lastRaw) { lastRaw = raw; lastChange = now; }

  if (now - lastChange > 30 && raw != stable) {
    stable = raw;
    if (stable == LOW) { downAt = now; longFired = false; }
    else if (!longFired) return 1;
  }
  if (stable == LOW && !longFired && now - downAt > LONG_PRESS_MS) {
    longFired = true;
    return 2;
  }
  return 0;
}

// ========================= SIMULATION ===========================
#if SIMULATE_VITALS

void updateSimTargets() {
  int m = simMode;
  if (m == 0) {                                // auto cycle, 110 s total
    unsigned long t = (millis() / 1000) % 110;
    if (t < 20)       m = 1;
    else if (t < 40)  m = 2;
    else if (t < 55)  m = 1;
    else if (t < 75)  m = 4;
    else if (t < 90)  m = 1;
    else              m = 3;
  }
  switch (m) {
    case 1: targetHR = 72;  targetSpO2 = 98; break;
    case 2: targetHR = 135; targetSpO2 = 97; break;
    case 3: targetHR = 42;  targetSpO2 = 97; break;
    case 4: targetHR = 95;  targetSpO2 = 86; break;
  }
}

void simulateVitalsStep() {
  updateSimTargets();
  simHR   += constrain(targetHR - simHR, -8.0f, 8.0f);
  simSpO2 += constrain(targetSpO2 - simSpO2, -1.5f, 1.5f);
  displayHR   = constrain((int)round(simHR) + (int)random(-1, 2), 30, 220);
  displaySpO2 = constrain((int)round(simSpO2) + (int)random(-1, 2), 70, 100);
  fingerPresent = true;
}

void simulatePPG() {
  if (millis() - lastSimSampleMs < 40) return;
  lastSimSampleMs = millis();

  ppgPhase += (simHR / 60.0f) * 0.04f;
  if (ppgPhase >= 1.0f) ppgPhase -= 1.0f;

  float x1 = (ppgPhase - 0.18f) / 0.07f;
  float x2 = (ppgPhase - 0.45f) / 0.09f;
  float v = exp(-x1 * x1) + 0.35f * exp(-x2 * x2);
  float noise = (random(-100, 100) / 100.0f) * 0.02f;
  float irf = 80000.0f + (v + noise) * 6000.0f;

  memmove(ppgWave, ppgWave + 1, sizeof(uint32_t) * 127);
  ppgWave[127] = (uint32_t)irf;
}

#else  // ------------------- REAL SENSOR -------------------

void readPPG() {
  particleSensor.check();

  while (particleSensor.available()) {
    uint32_t red = particleSensor.getFIFORed();
    uint32_t ir  = particleSensor.getFIFOIR();
    particleSensor.nextSample();

    fingerPresent = (ir > FINGER_IR_THRESHOLD);

    memmove(ppgWave, ppgWave + 1, sizeof(uint32_t) * 127);
    ppgWave[127] = ir;

    if (!fingerPresent) {
      bufferCount = 0; newSamples = 0;
      displayHR = 0; displaySpO2 = 0;
      continue;
    }

    if (bufferCount < BUFFER_LEN) {
      irBuffer[bufferCount] = ir;
      redBuffer[bufferCount] = red;
      bufferCount++;
    } else {
      memmove(irBuffer,  irBuffer + 1,  sizeof(uint32_t) * (BUFFER_LEN - 1));
      memmove(redBuffer, redBuffer + 1, sizeof(uint32_t) * (BUFFER_LEN - 1));
      irBuffer[BUFFER_LEN - 1] = ir;
      redBuffer[BUFFER_LEN - 1] = red;
    }
    newSamples++;

    if (bufferCount == BUFFER_LEN && newSamples >= UPDATE_EVERY) {
      newSamples = 0;
      maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_LEN, redBuffer,
                                             &spo2, &validSPO2, &heartRate, &validHR);
      if (validHR && heartRate > 40 && heartRate < 200)
        displayHR = (displayHR == 0) ? heartRate : (displayHR * 2 + heartRate) / 3;
      if (validSPO2 && spo2 >= 80 && spo2 <= 100)
        displaySpO2 = (displaySpO2 == 0) ? spo2 : (displaySpO2 * 2 + spo2) / 3;
    }
  }
}

#endif

// ======================= CRITICAL EVENTS ========================
void logEvent(const char* name, int value, bool triggered) {
  unsigned long s = millis() / 1000;
  char detail[56];
  snprintf(detail, sizeof(detail), "%s=%d", name, value);

  if (triggered) {
    totalEvents++;
    Serial.printf("[%02lu:%02lu] >>> CRITICAL EVENT #%d: %s (value=%d)\n",
                  s / 60, s % 60, totalEvents, name, value);
    logEventCsv("alert", detail);
  } else {
    Serial.printf("[%02lu:%02lu] <<< RESOLVED: %s (value=%d)\n",
                  s / 60, s % 60, name, value);
    logEventCsv("resolved", detail);
  }
}

void evalAlert(bool cond, bool clearCond, int &cnt, bool &active, const char* name, int value) {
  if (!active) {
    if (cond) {
      if (++cnt >= CONFIRM_SEC) {
        active = true;
        cnt = 0;
        logEvent(name, value, true);
      }
    } else {
      cnt = 0;
    }
  } else if (clearCond) {
    active = false;
    cnt = 0;
    logEvent(name, value, false);
  }
}

void checkVitals() {
  if (!fingerPresent || displayHR == 0 || displaySpO2 == 0) {
    hrHighCnt = hrLowCnt = spo2LowCnt = 0;
    hrHighAlert = hrLowAlert = spo2LowAlert = false;
    return;
  }
  evalAlert(displayHR >= HR_HIGH_CRIT, displayHR < HR_HIGH_CRIT - HR_HYST,
            hrHighCnt, hrHighAlert, "HIGH HEART RATE (bpm)", displayHR);
  evalAlert(displayHR <= HR_LOW_CRIT, displayHR > HR_LOW_CRIT + HR_HYST,
            hrLowCnt, hrLowAlert, "LOW HEART RATE (bpm)", displayHR);
  evalAlert(displaySpO2 < SPO2_LOW_CRIT, displaySpO2 >= SPO2_LOW_CRIT + SPO2_HYST,
            spo2LowCnt, spo2LowAlert, "LOW SpO2 (%)", displaySpO2);
}

// ============================ MOTION ============================
void readMotion() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return;
  if (Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)6) != 6) return;

  int16_t rawX = (Wire.read() << 8) | Wire.read();
  int16_t rawY = (Wire.read() << 8) | Wire.read();
  int16_t rawZ = (Wire.read() << 8) | Wire.read();

  const float toMs2 = 9.80665 / ACCEL_LSB_PER_G;
  float ax = rawX * toMs2;
  float ay = rawY * toMs2;
  float az = rawZ * toMs2;

  accelMag_g = sqrt(ax * ax + ay * ay + az * az) / 9.80665;

  if (!gravityInit) { gravX = ax; gravY = ay; gravZ = az; gravityInit = true; }
  const float alpha = 0.95;
  gravX = alpha * gravX + (1 - alpha) * ax;
  gravY = alpha * gravY + (1 - alpha) * ay;
  gravZ = alpha * gravZ + (1 - alpha) * az;

  float lx = ax - gravX, ly = ay - gravY, lz = az - gravZ;
  linAccel = sqrt(lx * lx + ly * ly + lz * lz);
  linForce = EFFECTIVE_MASS_KG * linAccel;

  activityLevel_g = 0.9 * activityLevel_g + 0.1 * (linAccel / 9.80665);

  if (linForce > peakForce) peakForce = linForce;
  if (linForce > intervalPeak) intervalPeak = linForce;

  if (!injuryAlert && linForce > INJURY_FORCE_THRESHOLD) {
    injuryAlert = true;
    alertStart = millis();
    totalEvents++;
    Serial.printf(">>> CRITICAL EVENT #%d: INJURY DETECTED! Force = %.1f N (%.2f g total)\n",
                  totalEvents, linForce, accelMag_g);
    char detail[40];
    snprintf(detail, sizeof(detail), "force=%.1fN", linForce);
    logEventCsv("injury", detail);
  }
}

const char* activityLabel() {
  if (activityLevel_g > ACTIVITY_RUN_G)  return "Active";
  if (activityLevel_g > ACTIVITY_WALK_G) return "Walking";
  return "Still";
}

// Runs once per second
void tickSecond() {
#if SIMULATE_VITALS
  simulateVitalsStep();
#endif
  checkVitals();
#if AUTO_EVENTS
  tickScheduler();
#endif

  // collect samples while a checkup is running
  if (screen == SCR_CHECK_RUN) {
    if (fingerPresent && displayHR > 0 && displaySpO2 > 0) {
      ckSumHR += displayHR;
      ckSumSpO2 += displaySpO2;
      ckCount++;
    }
    ckSumAct += activityLevel_g;
    ckActCount++;
  }

  // periodic data logging
  if (nowSec() - lastLogSec >= LOG_INTERVAL_SEC) {
    lastLogSec = nowSec();
    logVitalsSample();
  }
}

// ============================ DISPLAY ===========================
void drawCentered(const char* s, int y) {
  int x = (128 - (int)display.getStrWidth(s)) / 2;
  if (x < 0) x = 0;
  display.drawStr(x, y, s);
}

void drawTitleBar(const char* txt) {
  display.drawBox(0, 0, 128, 12);
  display.setDrawColor(0);
  drawCentered(txt, 10);
  display.setDrawColor(1);
}

void showMessage(const char* msg) {
  display.clearBuffer();
  const char* nl = strchr(msg, '\n');
  if (nl) {
    char line1[24];
    size_t n = min((size_t)(nl - msg), sizeof(line1) - 1);
    strncpy(line1, msg, n);
    line1[n] = 0;
    display.drawStr(0, 20, line1);
    display.drawStr(0, 34, nl + 1);
  } else {
    display.drawStr(0, 20, msg);
  }
  display.sendBuffer();
}

// ---- Main dashboard ----
void drawMain() {
  char buf[32];
  display.clearBuffer();

  const char* alerts[4];
  int n = 0;
  if (injuryAlert)  alerts[n++] = "!! INJURY ALERT !!";
  if (spo2LowAlert) alerts[n++] = "! LOW OXYGEN SpO2 !";
  if (hrHighAlert)  alerts[n++] = "! HIGH HEART RATE !";
  if (hrLowAlert)   alerts[n++] = "! LOW HEART RATE !";

  if (n > 0) {
    const char* txt = alerts[(millis() / 1500) % n];
    int x = (128 - (int)display.getStrWidth(txt)) / 2;
    if ((millis() / 250) % 2 == 0) {
      display.drawBox(0, 0, 128, 12);
      display.setDrawColor(0);
      display.drawStr(x, 10, txt);
      display.setDrawColor(1);
    } else {
      display.drawStr(x, 10, txt);
    }
  } else {
    display.drawStr(0, 10, activityLabel());
#if SIMULATE_VITALS
    display.drawStr(54, 10, "SIM");
#endif
    snprintf(buf, sizeof(buf), "%.2fg", accelMag_g);
    display.drawStr(92, 10, buf);
  }

  if (fingerPresent && displayHR > 0)   snprintf(buf, sizeof(buf), "HR:%3d", displayHR);
  else if (fingerPresent)               snprintf(buf, sizeof(buf), "HR:...");
  else                                  snprintf(buf, sizeof(buf), "HR: --");
  display.drawStr(0, 24, buf);

  if (fingerPresent && displaySpO2 > 0) snprintf(buf, sizeof(buf), "SpO2:%d%%", displaySpO2);
  else if (fingerPresent)               snprintf(buf, sizeof(buf), "SpO2:...");
  else                                  snprintf(buf, sizeof(buf), "SpO2: --");
  display.drawStr(64, 24, buf);

  snprintf(buf, sizeof(buf), "F:%4.1fN", linForce);
  display.drawStr(0, 36, buf);
  snprintf(buf, sizeof(buf), "Pk:%4.1fN", peakForce);
  display.drawStr(64, 36, buf);

  if (fingerPresent) {
    uint32_t mn = 0xFFFFFFFF, mx = 0;
    for (int i = 0; i < 128; i++) {
      if (ppgWave[i] == 0) continue;
      if (ppgWave[i] < mn) mn = ppgWave[i];
      if (ppgWave[i] > mx) mx = ppgWave[i];
    }
    if (mx > mn) {
      int prevY = -1;
      for (int x = 0; x < 128; x++) {
        if (ppgWave[x] == 0) { prevY = -1; continue; }
        int y = 63 - (int)((ppgWave[x] - mn) * 22UL / (mx - mn));
        if (prevY >= 0) display.drawLine(x - 1, prevY, x, y);
        prevY = y;
      }
    }
  } else {
    display.drawStr(10, 56, "Place finger...");
  }
  display.sendBuffer();
}

// ---- Reminder popup ----
void drawReminder() {
  display.clearBuffer();
  drawTitleBar("REMINDER");
  if (curTask >= 0) {
    const char* nm = tasks[curTask].name;
    if (display.getStrWidth(nm) <= 124) {
      drawCentered(nm, 34);
    } else {
      // too wide for one line: split at the last space that fits
      char l1[32], l2[32];
      strncpy(l1, nm, sizeof(l1) - 1);
      l1[sizeof(l1) - 1] = 0;
      l2[0] = 0;
      for (int i = (int)strlen(l1) - 1; i > 0; i--) {
        if (l1[i] == ' ') {
          l1[i] = 0;
          if (display.getStrWidth(l1) <= 124) { strcpy(l2, nm + i + 1); break; }
          l1[i] = ' ';
        }
      }
      drawCentered(l1, 28);
      drawCentered(l2, 40);
    }
  }
  drawCentered("To-do task due", 52);
  drawCentered("Tap:Done Hold:" LATER_TXT, 63);
  display.sendBuffer();
}

// ---- Mood popup ----
void drawFace(int cx, int cy, int mood) {
  display.drawCircle(cx, cy, 14);
  display.drawDisc(cx - 5, cy - 4, 1);
  display.drawDisc(cx + 5, cy - 4, 1);
  switch (mood) {
    case 1:
      display.drawLine(cx - 7, cy + 9, cx - 4, cy + 4); display.drawLine(cx - 4, cy + 4, cx + 4, cy + 4);
      display.drawLine(cx + 4, cy + 4, cx + 7, cy + 9); break;
    case 2:
      display.drawLine(cx - 6, cy + 8, cx - 3, cy + 5); display.drawLine(cx - 3, cy + 5, cx + 3, cy + 5);
      display.drawLine(cx + 3, cy + 5, cx + 6, cy + 8); break;
    case 3:
      display.drawLine(cx - 5, cy + 6, cx + 5, cy + 6); break;
    case 4:
      display.drawLine(cx - 6, cy + 4, cx - 3, cy + 7); display.drawLine(cx - 3, cy + 7, cx + 3, cy + 7);
      display.drawLine(cx + 3, cy + 7, cx + 6, cy + 4); break;
    default:
      display.drawLine(cx - 7, cy + 3, cx - 4, cy + 8); display.drawLine(cx - 4, cy + 8, cx + 4, cy + 8);
      display.drawLine(cx + 4, cy + 8, cx + 7, cy + 3); break;
  }
}

void drawMood() {
  display.clearBuffer();
  drawTitleBar("HOW ARE YOU FEELING?");
  drawFace(28, 36, moodSel);
  display.drawStr(58, 28, moodLabel(moodSel));
  for (int i = 1; i <= 5; i++) {
    int x = 60 + (i - 1) * 12;
    if (i == moodSel) display.drawDisc(x, 40, 4);
    else display.drawCircle(x, 40, 3);
  }
  display.drawStr(58, 54, "1 low  5 great");
  display.drawStr(0, 63, "Tap:next  Hold:save");
  display.sendBuffer();
}

// ---- Checkup screens ----
void drawCheckupPrompt() {
  display.clearBuffer();
  drawTitleBar("HEALTH CHECKUP");
  drawCentered("Routine check is due", 28);
  drawCentered("Sit still and relax", 42);
  drawCentered("Tap:Start Hold:" LATER_TXT, 63);
  display.sendBuffer();
}

void drawCheckupRun() {
  char buf[32];
  unsigned long elapsed = millis() - ckStartMs;
  int pct = (int)min(elapsed * 100UL / (CHECKUP_SEC * 1000UL), 100UL);
  int left = (int)CHECKUP_SEC - (int)(elapsed / 1000);
  if (left < 0) left = 0;

  display.clearBuffer();
  drawTitleBar("MEASURING...");
  snprintf(buf, sizeof(buf), "HR:%d  SpO2:%d%%", displayHR, displaySpO2);
  drawCentered(buf, 28);
  display.drawFrame(10, 36, 108, 8);
  display.drawBox(12, 38, (104 * pct) / 100, 4);
  snprintf(buf, sizeof(buf), "Stay still... %ds", left);
  drawCentered(buf, 56);
  display.sendBuffer();
}

void drawCheckupResult() {
  char buf[32];
  display.clearBuffer();
  drawTitleBar("CHECKUP RESULT");
  if (ckAvgHR > 0) {
    snprintf(buf, sizeof(buf), "HR %d   SpO2 %d%%", ckAvgHR, ckAvgSpO2);
    drawCentered(buf, 28);
  }
  drawCentered(ckResult, 42);
  drawCentered("Saved to log", 62);
  display.sendBuffer();
}

// ---- Graphs ----
void autoRange(float mn, float mx, float minSpan, float pad, float &lo, float &hi) {
  lo = mn - pad;
  hi = mx + pad;
  if (hi - lo < minSpan) {
    float c = (hi + lo) / 2;
    lo = c - minSpan / 2;
    hi = c + minSpan / 2;
  }
}

void drawChart(Series &s, float lo, float hi, int dec, bool bars, bool dots) {
  const int X0 = 24, X1 = 127, Y0 = 14, Y1 = 52;
  display.setFont(u8g2_font_5x7_tr);

  if (!bars) {
    char b[10];
    snprintf(b, sizeof(b), "%.*f", dec, hi);
    display.drawStr(0, Y0 + 6, b);
    snprintf(b, sizeof(b), "%.*f", dec, lo);
    display.drawStr(0, Y1, b);
  }
  display.drawVLine(X0, Y0, Y1 - Y0 + 1);
  display.drawHLine(X0, Y1, X1 - X0 + 1);

  float span = hi - lo;
  if (span <= 0) span = 1;
  int w = X1 - X0 - 3;
  int h = Y1 - Y0 - 3;

  if (bars) {                                   // task history: tall bar = done, small hollow = missed
    int cnt = min(s.n, 14);
    int start = s.n - cnt;
    for (int i = 0; i < cnt; i++) {
      int x = X0 + 3 + i * 7;
      if (s.v[start + i] > 0.5f) display.drawBox(x, Y0 + 2, 6, h + 1);
      else display.drawFrame(x, Y1 - 6, 6, 5);
    }
    return;
  }

  int px = -1, py = -1;
  for (int i = 0; i < s.n; i++) {
    int x = X0 + 2 + (s.n > 1 ? (int)((long)i * w / (s.n - 1)) : w / 2);
    float f = constrain((s.v[i] - lo) / span, 0.0f, 1.0f);
    int y = Y1 - 2 - (int)(f * h);
    if (px >= 0) display.drawLine(px, py, x, y);
    if (dots || s.n == 1) display.drawDisc(x, y, 1);
    px = x; py = y;
  }
}

void drawGraphScreen(int sec) {
  static const char* titles[5] = {"HEART RATE", "SpO2", "MOTION", "MOOD", "TASKS"};
  Series* all[5] = {&serHR, &serSpO2, &serAct, &serMood, &serTasks};
  Series &s = *all[sec];

  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);

  char idx[8];
  snprintf(idx, sizeof(idx), "%d/5", sec + 1);
  int idxW = display.getStrWidth(idx);
  display.drawStr(0, 9, titles[sec]);
  display.drawStr(128 - idxW, 9, idx);
  display.drawHLine(0, 11, 128);

  if (s.n == 0) {
    drawCentered("No data yet", 36);
    display.sendBuffer();
    return;
  }

  float mn, mx, avg;
  seriesStats(s, mn, mx, avg);
  float last = s.v[s.n - 1];

  char nowTxt[12], bottom[40];
  float lo = 0, hi = 1;
  int dec = 0;
  bool bars = false, dots = false;

  switch (sec) {
    case 0:
      autoRange(mn, mx, 20, 4, lo, hi);
      snprintf(nowTxt, sizeof(nowTxt), "%.0fbpm", last);
      snprintf(bottom, sizeof(bottom), "mn %.0f mx %.0f av %.0f", mn, mx, avg);
      break;
    case 1:
      autoRange(mn, mx, 8, 1, lo, hi);
      if (hi > 100) { lo -= hi - 100; hi = 100; }
      snprintf(nowTxt, sizeof(nowTxt), "%.0f%%", last);
      snprintf(bottom, sizeof(bottom), "mn %.0f mx %.0f av %.0f", mn, mx, avg);
      break;
    case 2:
      lo = 0; hi = max(0.2f, mx * 1.2f); dec = 2;
      snprintf(nowTxt, sizeof(nowTxt), "%.2fg", last);
      snprintf(bottom, sizeof(bottom), "mn %.2f mx %.2f av %.2f", mn, mx, avg);
      break;
    case 3:
      lo = 1; hi = 5; dots = true;
      snprintf(nowTxt, sizeof(nowTxt), "%s", moodLabel((int)round(last)));
      snprintf(bottom, sizeof(bottom), "avg %.1f   entries %d", avg, s.n);
      break;
    default: {
      int done = 0;
      for (int i = 0; i < s.n; i++) if (s.v[i] > 0.5f) done++;
      bars = true;
      snprintf(nowTxt, sizeof(nowTxt), "%d/%d", done, s.n);
      snprintf(bottom, sizeof(bottom), "done %d   missed %d", done, s.n - done);
      break;
    }
  }

  int nowW = display.getStrWidth(nowTxt);
  display.drawStr(128 - idxW - 6 - nowW, 9, nowTxt);

  drawChart(s, lo, hi, dec, bars, dots);

  display.setFont(u8g2_font_5x7_tr);
  display.drawStr(0, 62, bottom);
  display.setFont(u8g2_font_6x10_tf);
  display.sendBuffer();
}

// ---- Event menu (4 visible rows, selected row inverted, scrollbar on the right) ----
void drawMenu() {
  display.clearBuffer();
  drawTitleBar("Tap:next Hold:start");

  int top = menuSel - 1;
  if (top > MENU_COUNT - 4) top = MENU_COUNT - 4;
  if (top < 0) top = 0;

  for (int r = 0; r < 4 && top + r < MENU_COUNT; r++) {
    int item = top + r;
    int y = 24 + r * 12;
    if (item == menuSel) {
      display.drawBox(0, y - 9, 124, 11);
      display.setDrawColor(0);
      display.drawStr(3, y, menuLabels[item]);
      display.setDrawColor(1);
    } else {
      display.drawStr(3, y, menuLabels[item]);
    }
  }

  int thumb = 13 + (menuSel * 44) / (MENU_COUNT - 1);
  display.drawVLine(127, 13, 51);
  display.drawBox(126, thumb, 2, 6);
  display.sendBuffer();
}

void drawDisplay() {
  display.setFont(u8g2_font_6x10_tf);
  switch (screen) {
    case SCR_MAIN:         drawMain(); break;
    case SCR_MENU:         drawMenu(); break;
    case SCR_REMINDER:     drawReminder(); break;
    case SCR_MOOD:         drawMood(); break;
    case SCR_CHECK_PROMPT: drawCheckupPrompt(); break;
    case SCR_CHECK_RUN:    drawCheckupRun(); break;
    case SCR_CHECK_RESULT: drawCheckupResult(); break;
    case SCR_GRAPHS:       drawGraphScreen(graphSection); break;
  }
}

// ============================ SERIAL ============================
void printSerial() {
  Serial.printf("IR:%lu, HR:%d, SpO2:%d, G:%.2f, Lin:%.2f, F:%.2f, PK:%.2f, %s, ALERTS[inj:%d hrH:%d hrL:%d o2:%d]\n",
                (unsigned long)ppgWave[127], displayHR, displaySpO2,
                accelMag_g, linAccel, linForce, peakForce, activityLabel(),
                injuryAlert, hrHighAlert, hrLowAlert, spo2LowAlert);
}

// ======================= A.I.M.S. DASHBOARD LINK ================
// The band hosts a tiny REST API over WiFi:
//   GET /api/telemetry  ->  JSON with HR, SpO2, force, injury, ...
// aims.html polls it once per second (CORS is enabled).
#if ENABLE_AIMS_LINK
WebServer server(80);

void buildTelemetryJson(char* out, size_t n) {
  snprintf(out, n,
           "{\"crew_id\":\"%s\",\"hr\":%d,\"spo2\":%d,\"activity_g\":%.3f,\"accel_g\":%.3f,"
           "\"linear_force_n\":%.2f,\"peak_force_n\":%.2f,\"injury\":%s,\"injury_part\":\"%s\","
           "\"simulated\":%s,\"uptime_s\":%lu}",
           CREW_ID,
           fingerPresent ? displayHR : 0,
           fingerPresent ? displaySpO2 : 0,
           activityLevel_g, accelMag_g, linForce, peakForce,
           injuryAlert ? "true" : "false", SENSOR_PART,
           SIMULATE_VITALS ? "true" : "false", nowSec());
}

void sendCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
  server.sendHeader("Access-Control-Allow-Private-Network", "true");
  server.sendHeader("Cache-Control", "no-store");
}

void handleRoot() {
  sendCors();
  server.send(200, "text/plain", "A.I.M.S. band API - GET /api/telemetry");
}

void handleTelemetry() {
  char b[360];
  buildTelemetryJson(b, sizeof(b));
  sendCors();
  server.send(200, "application/json", b);
}

void handleNotFound() {
  sendCors();
  server.send(server.method() == HTTP_OPTIONS ? 204 : 404, "text/plain", "");
}

#if AIMS_PUSH
// Optional: also POST the same JSON to a backend (AIMS_URL) once per second
void postTelemetry() {
  HTTPClient http;
  http.setConnectTimeout(1500);
  http.setTimeout(1500);
  if (!http.begin(AIMS_URL)) return;
  http.addHeader("Content-Type", "application/json");
  if (AIMS_KEY[0]) http.addHeader("X-AIMS-Key", AIMS_KEY);
  char body[360];
  buildTelemetryJson(body, sizeof(body));
  http.POST((uint8_t*)body, strlen(body));
  http.end();
}
#endif

// Runs on core 0 so network work never delays sensor reading or the display
void aimsNetTask(void* arg) {
  bool serverUp = false;
  unsigned long lastPush = 0;
  unsigned long lastBegin = millis();
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!serverUp) {
        server.on("/", HTTP_GET, handleRoot);
        server.on("/api/telemetry", HTTP_GET, handleTelemetry);
        server.onNotFound(handleNotFound);
        server.begin();
        MDNS.begin("aims-band");
        serverUp = true;
        Serial.printf("[AIMS] API ready: http://%s/api/telemetry  (or http://aims-band.local/api/telemetry)\n",
                      WiFi.localIP().toString().c_str());
      }
      server.handleClient();
#if AIMS_PUSH
      if (millis() - lastPush >= 1000) { lastPush = millis(); postTelemetry(); }
#endif
      vTaskDelay(pdMS_TO_TICKS(5));
    } else {
      wl_status_t st = WiFi.status();
      Serial.printf("[AIMS] WiFi not connected: %s\n",
                    st == WL_NO_SSID_AVAIL ? "network name not found (check spelling, use a 2.4 GHz network)" :
                    st == WL_CONNECT_FAILED ? "connection failed (wrong password?)" :
                    "still connecting...");
      if (millis() - lastBegin > 20000) {      // retry from scratch every 20 s
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        lastBegin = millis();
      }
      vTaskDelay(pdMS_TO_TICKS(3000));
    }
  }
}

void startAimsLink() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[AIMS] Connecting to WiFi '%s'...\n", WIFI_SSID);
  xTaskCreatePinnedToCore(aimsNetTask, "aims_net", 8192, NULL, 1, NULL, 0);
}
#endif

// ============================ SETUP =============================
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  display.begin();
  display.setFont(u8g2_font_6x10_tf);
  showMessage("Starting...");

  // --- Storage ---
  fsOk = LittleFS.begin(true);               // true = format on first use
  if (fsOk) {
    Serial.printf("[LOG] LittleFS ready: %u of %u bytes used\n",
                  (unsigned)LittleFS.usedBytes(), (unsigned)LittleFS.totalBytes());
  } else {
    Serial.println("[LOG] LittleFS failed - logging disabled (graphs still work in RAM)");
  }
  Preferences prefs;
  prefs.begin("band", false);
  session = prefs.getUInt("session", 0) + 1;
  prefs.putUInt("session", session);
  prefs.end();
  Serial.printf("[LOG] Session #%u\n", session);
  if (fsOk) restoreHistory();

  // --- MPU6050 ---
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    Serial.println("MPU6050 not responding!");
    showMessage("MPU6050 FAIL");
    while (1) delay(10);
  }
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C); Wire.write(0x18);
  Wire.endTransmission();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1A); Wire.write(0x04);
  Wire.endTransmission();

  memset(ppgWave, 0, sizeof(ppgWave));

#if SIMULATE_VITALS
  fingerPresent = true;
  displayHR = 72;
  displaySpO2 = 98;
  randomSeed(micros());
  Serial.println("*** SIMULATED VITALS MODE ***");
#else
  Wire.beginTransmission(0x57);
  Wire.write(0xFF);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom((uint8_t)0x57, (uint8_t)1) != 1) {
    Serial.println("MAX30102 not responding on I2C!");
    showMessage("MAX30102 FAIL");
    while (1) delay(10);
  }
  Serial.printf("MAX30102 part ID: 0x%02X (expected 0x15)\n", Wire.read());
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 ID check failed - continuing anyway (clone chip?)");
  }
  Wire.setClock(400000);
  particleSensor.setup(60, 4, 2, 100, 411, 4096);
#endif

#if ENABLE_AIMS_LINK
  startAimsLink();
#endif

  // --- Default reminders & schedule ---
  addTask("Drink water", 120);                  // real: every 2 hours
  addTask("Exercise", 180);                     // real: every 4 hours
  addTask("Do routine fluid checkups", 240);    // real: every 6 hours
  nextCheckupDue = nowSec() + CHECKUP_INTERVAL_SEC;
  nextMoodDue    = nowSec() + MOOD_INTERVAL_SEC;
  nextGraphsDue  = nowSec() + GRAPH_INTERVAL_SEC;
  lastLogSec     = nowSec();

  logEventCsv("boot", SIMULATE_VITALS ? "simulated" : "real");
  printHelp();
  showMessage("Band ready\nTap BOOT: menu");
  delay(800);
}

// ============================ LOOP ==============================
void loop() {
  handleSerial();

#if SIMULATE_VITALS
  simulatePPG();
#else
  readPPG();
#endif

  unsigned long now = millis();

  if (now - lastMotionMs >= 20) {        // 50 Hz motion
    lastMotionMs = now;
    readMotion();
  }

  if (now - lastTickMs >= 1000) {        // 1 Hz: vitals, alerts, scheduler, logging
    lastTickMs = now;
    tickSecond();
  }

  int ev = pollButton();
  if (ev) handleAction(ev);

  if (anyAlert() && screen != SCR_MAIN) abortToMain();   // critical alerts take over
  updateScreen();
  dispatchEvents();

  if (now - lastDisplayMs >= 100) {      // 10 Hz display
    lastDisplayMs = now;
    drawDisplay();
  }

  if (now - lastSerialMs >= 1000) {      // 1 Hz serial data line
    lastSerialMs = now;
    printSerial();
  }

  if (injuryAlert && now - alertStart > ALERT_HOLD_MS) {
    injuryAlert = false;
    peakForce = 0;
    Serial.println("[EVENT] Injury alert cleared");
  }

  digitalWrite(LED_PIN, anyAlert() ? HIGH : LOW);
}
