import type {
  MissionState,
  AstronautState,
} from "../types/mission";

import { getStatusClass } from "../utils/status";

interface LunarMapProps {
  mission: MissionState;
}

const MAP_RADIUS_METERS = 2000;

export function LunarMap({
  mission,
}: LunarMapProps) {
  const crewEntries = Object.entries(
    mission.crew,
  );

  return (
    <section>
      <div className="map-heading">
        <div>
          <p className="eyebrow">
            EVA OPERATIONS
          </p>

          <h1>Lunar Map</h1>

          <p className="page-description">
            Live crew positioning relative to
            the lunar habitat.
          </p>
        </div>

        <div className="map-scale">
          <span>MAP RADIUS</span>
          <strong>2 KM</strong>
        </div>
      </div>

      <div className="lunar-map-layout">
        <div className="lunar-map">
          <div className="map-grid" />

          <RangeRing
            distance={2000}
            className="ring-2000"
          />

          <RangeRing
            distance={1000}
            className="ring-1000"
          />

          <RangeRing
            distance={500}
            className="ring-500"
          />

          <RangeRing
            distance={250}
            className="ring-250"
          />
          <div className="sonar-pulse sonar-pulse-one" />
<div className="sonar-pulse sonar-pulse-two" />
<div className="sonar-pulse sonar-pulse-three" />

<div className="sonar-sweep" />
          <div className="habitat-marker">
            <span className="habitat-dot" />
            <strong>HABITAT</strong>
            <small>0, 0</small>
          </div>

          {crewEntries.map(
            ([astronautId, astronaut]) => (
              <CrewMarker
                key={astronautId}
                astronautId={astronautId}
                astronaut={astronaut}
              />
            ),
          )}
        </div>

        <aside className="map-crew-panel">
          <span className="section-label">
            EVA CREW
          </span>

          {crewEntries.map(
            ([astronautId, astronaut]) => (
              <div
                className="map-crew-row"
                key={astronautId}
              >
                <div>
                  <strong>
                    {astronaut.telemetry.name}
                  </strong>

                  <span>{astronautId}</span>
                </div>

                <div className="map-crew-distance">
                  <strong>
                    {
                      astronaut.position
                        .distance_from_base
                    }{" "}
                    M
                  </strong>

                  <span
                    className={getStatusClass(
                      astronaut.eva_status,
                    )}
                  >
                    {astronaut.eva_status}
                  </span>
                </div>
              </div>
            ),
          )}
        </aside>
      </div>

      <p className="simulation-notice">
        PUBLIC TELEMETRY FEED — SIMULATED DATA
        FOR DEMONSTRATION
      </p>
    </section>
  );
}

interface RangeRingProps {
  distance: number;
  className: string;
}

function RangeRing({
  distance,
  className,
}: RangeRingProps) {
  return (
    <div
      className={`range-ring ${className}`}
    >
      <span>{formatDistance(distance)}</span>
    </div>
  );
}

interface CrewMarkerProps {
  astronautId: string;
  astronaut: AstronautState;
}

function CrewMarker({
  astronautId,
  astronaut,
}: CrewMarkerProps) {
  const { x, y } = astronaut.position;

  const left =
    50 +
    (x / MAP_RADIUS_METERS) * 50;

  const top =
    50 -
    (y / MAP_RADIUS_METERS) * 50;

  return (
    <div
      className="crew-map-marker"
      style={{
        left: `${left}%`,
        top: `${top}%`,
      }}
    >
      <span
        className={`crew-map-dot ${
          getStatusClass(
            astronaut.eva_status,
          )
        }`}
      />

      <div className="crew-map-label">
        <strong>{astronautId}</strong>
        <span>
          {astronaut.telemetry.name}
        </span>
      </div>
    </div>
  );
}

function formatDistance(
  distance: number,
): string {
  if (distance >= 1000) {
    return `${distance / 1000} KM`;
  }

  return `${distance} M`;
}