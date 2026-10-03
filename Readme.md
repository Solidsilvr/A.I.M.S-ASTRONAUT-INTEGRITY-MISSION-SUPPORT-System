# A.I.M.S. — Astronaut Integrity & Mission Support

**A.I.M.S.** is a real-time simulated lunar mission monitoring platform designed to visualize astronaut health, EVA safety, crew proximity, Moon–Earth communications, and overall mission integrity through a public-facing mission dashboard.

Built as a hackathon project, A.I.M.S. demonstrates how multiple streams of mission telemetry can be combined into a single integrity monitoring system with live alerts and interactive emergency simulations.

> **PUBLIC TELEMETRY FEED — SIMULATED DATA FOR DEMONSTRATION**

---

## 🚀 Features

### 🧑‍🚀 Crew Health Monitoring
A.I.M.S. continuously monitors simulated astronaut telemetry including:

- Heart rate
- Blood oxygen saturation (SpO₂)
- Body temperature
- Sleep duration
- Exercise/activity
- Individual health integrity
- Animated heart-rate visualization
- Active health and EVA alerts

### 🌕 EVA Safety & Lunar Map
The lunar operations display tracks crew members relative to the habitat.

- Live simulated astronaut movement
- Distance from lunar base
- Nearest crew member and separation
- 250 m, 500 m, 1 km, and 2 km safety rings
- EVA warning and critical zones
- Animated lunar radar/sonar visualization
- Automatic EVA range alerts

### 📡 Moon–Earth Communications
A.I.M.S. monitors a simulated Moon-to-Earth communications link.

- Signal strength
- Packet loss
- One-way latency
- Communications integrity
- Stable, degraded, and critical link states
- Animated Moon ↔ Earth packet visualization

### ⚠️ Mission Integrity Engine
Subsystem integrity is combined into an overall astronaut integrity score:

```text
Crew Integrity =
Health Integrity × 60%
+ EVA Safety × 25%
+ Communications Integrity × 15%
```

Critical subsystem conditions override the numerical score so that a severe safety event cannot be hidden by a weighted average.

Mission conditions are represented using semantic states:

- 🟢 **NORMAL**
- 🟡 **WARNING**
- 🔴 **CRITICAL**

The dashboard dynamically changes its visual state based on the current mission condition.

---

## 🎛️ Interactive Emergency Simulation

A.I.M.S. includes built-in demo controls for triggering mission events in real time.

Available simulations include:

- Oxygen emergency
- EVA safety-radius breach
- Communications degradation
- Critical communications failure
- Restore nominal mission conditions

Events propagate through the application using WebSockets, allowing the dashboard, crew cards, lunar map, communications system, integrity scores, and mission log to react immediately.

---

## 🖥️ Mission Dashboard

The interface contains five primary mission views:

### MISSION LIVE
Mission-wide status, crew integrity, Earth link status, mission timing, and latest events.

### CREW
Detailed astronaut telemetry, integrity scores, EVA information, animated heart monitoring, and active alerts.

### LUNAR MAP
Live crew positioning around the lunar habitat with safety zones and radar-style visualization.

### COMMS
Moon–Earth link status, signal quality, packet loss, latency, and communications integrity.

### MISSION LOG
Chronological mission events with filters for health, EVA, communications, crew, and alerts.

---

## 🛰️ Live Mission Timing

The persistent mission header displays:

- Mission Day
- Mission Elapsed Time (MET)
- UTC
- Simulated Coordinated Lunar Time — `CLT(S)`
- Kennedy Space Center local time
- Earth-link status
- Overall mission condition

`CLT(S)` is explicitly simulated for demonstration and is not presented as an official lunar timezone.

---

## 🛠️ Technology Stack

### Frontend

- React
- TypeScript
- Vite
- CSS
- WebSocket client

### Backend

- Python
- FastAPI
- Pydantic
- Uvicorn
- REST API
- WebSockets

---

## 🏗️ Architecture

```text
AIMS/
├── backend/
│   ├── integrity/
│   │   ├── health.py
│   │   ├── proximity.py
│   │   ├── overall.py
│   │   └── communications.py
│   ├── models/
│   ├── services/
│   ├── simulation/
│   ├── main.py
│   └── requirements.txt
│
└── frontend/
    ├── src/
    │   ├── api/
    │   ├── components/
    │   ├── pages/
    │   ├── types/
    │   └── utils/
    ├── package.json
    └── vite.config.ts
```

The **FastAPI backend is the source of truth** for mission state and integrity calculations.

React acts as the visualization layer, receiving mission updates through REST and WebSocket connections.

---

## ▶️ Running Locally

### Backend

From the `backend` directory:

```bash
pip install -r requirements.txt
uvicorn main:app --reload
```

The API will run at:

```text
http://127.0.0.1:8000
```

FastAPI documentation:

```text
http://127.0.0.1:8000/docs
```

### Frontend

From the `frontend` directory:

```bash
npm install
npm run dev
```

The Vite development server will normally run at:

```text
http://localhost:5173
```

Configure the frontend environment with:

```env
VITE_API_URL=http://127.0.0.1:8000
VITE_WS_URL=ws://127.0.0.1:8000
```

---

## 🧪 Simulation Notice

All astronaut telemetry, EVA positions, communications metrics, alerts, mission events, and lunar-time displays in A.I.M.S. are **simulated for demonstration purposes**.

The project is not an operational aerospace, medical, or astronaut safety system.

---

## 🌌 Project Goal

A.I.M.S. explores a simple question:

**How can complex astronaut health, EVA, proximity, and communications telemetry be transformed into an understandable real-time picture of mission integrity?**

The result is a single public-facing interface where mission conditions can be understood at a glance while still allowing users to explore the individual systems contributing to overall crew safety.

---

**A.I.M.S. — Astronaut Integrity & Mission Support**

*Monitoring the mission. Protecting the crew.*