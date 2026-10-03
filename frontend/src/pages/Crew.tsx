import type {
  MissionState,
  AstronautState,
} from "../types/mission";

import { getStatusClass } from "../utils/status";


interface CrewProps {
  mission: MissionState;
}


export function Crew({
  mission,
}: CrewProps) {

  const crewEntries =
    Object.entries(mission.crew);


  return (
    <section>

      <div className="crew-heading">

        <div>
          <p className="eyebrow">
            CREW INTEGRITY
          </p>

          <h1>Crew</h1>

          <p className="page-description">
            Live biometric, EVA and integrity
            telemetry for the lunar crew.
          </p>
        </div>

        <div className="crew-count">
          <span>ACTIVE CREW</span>
          <strong>{crewEntries.length}</strong>
        </div>

      </div>


      <div className="crew-grid">

        {crewEntries.map(
          ([astronautId, astronaut]) => (

            <CrewCard
              key={astronautId}
              astronautId={astronautId}
              astronaut={astronaut}
            />

          ),
        )}

      </div>


      <p className="simulation-notice">
        PUBLIC TELEMETRY FEED — SIMULATED DATA
        FOR DEMONSTRATION
      </p>

    </section>
  );
}


interface CrewCardProps {
  astronautId: string;
  astronaut: AstronautState;
}


function CrewCard({
  astronautId,
  astronaut,
}: CrewCardProps) {

  const telemetry = astronaut.telemetry;
  const position = astronaut.position;


  return (
    <article className="crew-card">

      <div className="crew-card-header">

        <div>
          <span className="crew-id">
            {astronautId}
          </span>

          <h2>
            {telemetry.name}
          </h2>
        </div>

        <span
          className={`crew-status ${
            getStatusClass(
              astronaut.overall_status,
            )
          }`}
        >
          ● {astronaut.overall_status}
        </span>

      </div>

      <div className="heart-monitor">

  <div className="heart-monitor-header">

    <span>
      HEART RATE
    </span>

    <strong
      className={getStatusClass(
        astronaut.health_status,
      )}
    >
      {telemetry.heart_rate} BPM
    </strong>

  </div>


  <div className="ecg-window">
  <div className="ecg-grid" />

  <div className="ecg-track">
    <svg
      viewBox="0 0 300 60"
      preserveAspectRatio="none"
    >
      <polyline
        points="
          0,30
          35,30
          48,29
          55,32
          63,30
          72,30
          82,8
          91,52
          101,20
          110,30
          145,30
          158,29
          165,32
          173,30
          182,30
          192,8
          201,52
          211,20
          220,30
          300,30
        "
      />
    </svg>

    <svg
      viewBox="0 0 300 60"
      preserveAspectRatio="none"
    >
      <polyline
        points="
          0,30
          35,30
          48,29
          55,32
          63,30
          72,30
          82,8
          91,52
          101,20
          110,30
          145,30
          158,29
          165,32
          173,30
          182,30
          192,8
          201,52
          211,20
          220,30
          300,30
        "
      />
    </svg>
  </div>
</div>

</div>
      <div className="crew-integrity">

        <span>OVERALL INTEGRITY</span>

        <strong>
          {astronaut.overall_integrity}
        </strong>

      </div>


      <div className="telemetry-section">

        <span className="section-label">
          BIOMETRICS
        </span>


        <TelemetryRow
          label="HEART RATE"
          value={`${telemetry.heart_rate} BPM`}
        />

        <TelemetryRow
          label="OXYGEN"
          value={`${telemetry.spo2}%`}
        />

        <TelemetryRow
          label="BODY TEMP"
          value={`${telemetry.temperature} °C`}
        />

        <TelemetryRow
          label="SLEEP"
          value={`${telemetry.sleep_hours} H`}
        />

        <TelemetryRow
          label="EXERCISE"
          value={`${telemetry.exercise_minutes} MIN`}
        />

      </div>


      <div className="integrity-section">

        <span className="section-label">
          INTEGRITY
        </span>


        <IntegrityRow
          label="HEALTH"
          value={astronaut.health_integrity}
          status={astronaut.health_status}
        />

        <IntegrityRow
          label="EVA"
          value={astronaut.eva_integrity}
          status={astronaut.eva_status}
        />

        <IntegrityRow
          label="COMMS"
          value={astronaut.comms_integrity}
          status={astronaut.comms_status}
        />

      </div>


      <div className="eva-section">

        <span className="section-label">
          EVA POSITION
        </span>


        <TelemetryRow
          label="BASE DISTANCE"
          value={`${position.distance_from_base} M`}
        />

        <TelemetryRow
          label="NEAREST CREW"
          value={
            position.nearest_crew_id ?? "—"
          }
        />

        <TelemetryRow
          label="SEPARATION"
          value={
            position.nearest_crew_distance !== null
              ? `${position.nearest_crew_distance} M`
              : "—"
          }
        />

      </div>


      {astronaut.alerts.length > 0 && (

        <div className="crew-alerts">

          <span className="section-label">
            ACTIVE ALERTS
          </span>

          {astronaut.alerts.map(
            (alert) => (
              <p key={alert}>
                {alert}
              </p>
            ),
          )}

        </div>

      )}

    </article>
  );
}


interface TelemetryRowProps {
  label: string;
  value: string;
}


function TelemetryRow({
  label,
  value,
}: TelemetryRowProps) {

  return (
    <div className="telemetry-row">

      <span>{label}</span>

      <strong>{value}</strong>

    </div>
  );
}


interface IntegrityRowProps {
  label: string;
  value: number;
  status: string;
}


function IntegrityRow({
  label,
  value,
  status,
}: IntegrityRowProps) {

  return (
    <div className="integrity-row">

      <span>{label}</span>

      <div>
        <strong>{value}</strong>

        <small
          className={getStatusClass(status)}
        >
          {status}
        </small>
      </div>

    </div>
  );
}