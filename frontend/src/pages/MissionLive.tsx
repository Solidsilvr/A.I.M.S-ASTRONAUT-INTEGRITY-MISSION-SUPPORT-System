import type {
  MissionState,
} from "../types/mission";


interface MissionLiveProps {
  mission: MissionState;
}


export function MissionLive({
  mission,
}: MissionLiveProps) {

  const crew =
    Object.values(mission.crew);

  const latestEvent =
    mission.events.length > 0
      ? mission.events[
          mission.events.length - 1
        ]
      : null;


  const averageIntegrity =
    Math.round(
      crew.reduce(
        (total, astronaut) =>
          total + astronaut.overall_integrity,
        0,
      ) / crew.length,
    );


  return (
    <section>

      <div className="mission-title-row">

        <div>
          <p className="eyebrow">
            PUBLIC LUNAR MISSION TELEMETRY
          </p>

          <h1>Mission Live</h1>

          <p className="page-description">
            Real-time astronaut integrity and
            lunar mission overview.
          </p>
        </div>

        <div className="mission-status-block">
          <span>MISSION STATUS</span>

          <strong>
            {mission.overall_status}
          </strong>
        </div>

      </div>


      <div className="mission-grid">

        <article className="mission-panel">

          <span className="panel-label">
            A.I.M.S. INTEGRITY
          </span>

          <strong className="integrity-number">
            {averageIntegrity}
          </strong>

          <span className="panel-secondary">
            CREW AVERAGE
          </span>

        </article>


        <article className="mission-panel">

          <span className="panel-label">
            CREW
          </span>

          <strong className="panel-value">
            {crew.length}
          </strong>

          <span className="panel-secondary">
            ACTIVE PERSONNEL
          </span>

        </article>


        <article className="mission-panel">

          <span className="panel-label">
            EARTH LINK
          </span>

          <strong className="panel-value">
            {mission.communications.link_status}
          </strong>

          <span className="panel-secondary">
            {mission.communications.latency_seconds}
            {" SEC ONE-WAY"}
          </span>

        </article>

      </div>


      <div className="latest-event">

        <span className="panel-label">
          LATEST MISSION EVENT
        </span>

        {latestEvent ? (
          <>
            <strong>
              {latestEvent.event_type}
            </strong>

            <p>
              {latestEvent.message}
            </p>
          </>
        ) : (
          <p>
            No mission events recorded.
          </p>
        )}

      </div>


      <p className="simulation-notice">
        PUBLIC TELEMETRY FEED — SIMULATED DATA
        FOR DEMONSTRATION
      </p>

    </section>
  );
}