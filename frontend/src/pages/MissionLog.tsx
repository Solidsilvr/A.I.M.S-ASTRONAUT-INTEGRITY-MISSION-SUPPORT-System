import {
  useMemo,
  useState,
} from "react";

import type {
  MissionState,
  MissionEvent,
} from "../types/mission";

import {
  getStatusClass,
} from "../utils/status";


interface MissionLogProps {
  mission: MissionState;
}


type LogFilter =
  | "ALL"
  | "CREW"
  | "EVA"
  | "HEALTH"
  | "COMMS"
  | "ALERTS";


const filters: LogFilter[] = [
  "ALL",
  "CREW",
  "EVA",
  "HEALTH",
  "COMMS",
  "ALERTS",
];


export function MissionLog({
  mission,
}: MissionLogProps) {

  const [activeFilter, setActiveFilter] =
    useState<LogFilter>("ALL");


  const events = useMemo(() => {

    return [...mission.events]
      .reverse()
      .filter((event) =>
        eventMatchesFilter(
          event,
          activeFilter,
        ),
      );

  }, [
    mission.events,
    activeFilter,
  ]);


  return (
    <section>

      <div className="log-heading">

        <div>

          <p className="eyebrow">
            MISSION HISTORY
          </p>

          <h1>Mission Log</h1>

          <p className="page-description">
            Chronological health, EVA,
            communications and system events
            recorded by A.I.M.S.
          </p>

        </div>


        <div className="log-event-count">

          <span>EVENTS</span>

          <strong>
            {mission.events.length}
          </strong>

        </div>

      </div>


      <div className="log-filters">

        {filters.map((filter) => (

          <button
            key={filter}
            className={
              activeFilter === filter
                ? "log-filter active"
                : "log-filter"
            }
            onClick={() =>
              setActiveFilter(filter)
            }
          >
            {filter}
          </button>

        ))}

      </div>


      <div className="mission-log-list">

        {events.length > 0 ? (

          events.map(
            (event, index) => (

              <LogEntry
                key={
                  `${event.timestamp}-${index}`
                }
                event={event}
              />

            ),
          )

        ) : (

          <div className="log-empty">

            <span className="section-label">
              NO EVENTS
            </span>

            <p>
              No mission events match the
              selected filter.
            </p>

          </div>

        )}

      </div>


      <p className="simulation-notice">
        PUBLIC TELEMETRY FEED — SIMULATED DATA
        FOR DEMONSTRATION
      </p>

    </section>
  );
}


interface LogEntryProps {
  event: MissionEvent;
}


function LogEntry({
  event,
}: LogEntryProps) {

  const date =
    new Date(event.timestamp);


  const time =
    date.toLocaleTimeString(
      [],
      {
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit",
      },
    );


  return (
    <article className="log-entry">

      <div className="log-time">

        <strong>{time}</strong>

        <span>
          {date.toLocaleDateString()}
        </span>

      </div>


      <div
        className={`log-indicator ${
          getStatusClass(
            event.severity,
          )
        }`}
      >
        <span />
      </div>


      <div className="log-content">

        <div className="log-entry-header">

          <strong>
            {formatEventType(
              event.event_type,
            )}
          </strong>


          <span
            className={getStatusClass(
              event.severity,
            )}
          >
            {event.severity}
          </span>

        </div>


        <p>
          {event.message}
        </p>


        {event.astronaut_id && (

          <small>
            CREW MEMBER ·{" "}
            {event.astronaut_id}
          </small>

        )}

      </div>

    </article>
  );
}


function eventMatchesFilter(
  event: MissionEvent,
  filter: LogFilter,
): boolean {

  if (filter === "ALL") {
    return true;
  }


  const type =
    event.event_type.toUpperCase();

  const severity =
    event.severity.toUpperCase();


  switch (filter) {

    case "CREW":
      return event.astronaut_id !== null;


    case "EVA":
      return type.includes("EVA");


    case "HEALTH":
      return type.includes("HEALTH");


    case "COMMS":
      return type.includes("COMMS");


    case "ALERTS":
      return (
        severity === "WARNING" ||
        severity === "CRITICAL"
      );


    default:
      return true;
  }
}


function formatEventType(
  eventType: string,
): string {

  return eventType
    .replaceAll("_", " ");

}