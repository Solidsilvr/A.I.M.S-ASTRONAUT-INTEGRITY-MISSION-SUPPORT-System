import {
  useEffect,
  useState,
} from "react";

import type {
  MissionState,
} from "../types/mission";

import {
  getStatusClass,
} from "../utils/status";


interface MissionHeaderProps {
  mission: MissionState;
}


export function MissionHeader({
  mission,
}: MissionHeaderProps) {

  const [now, setNow] =
    useState(new Date());


  useEffect(() => {

    const timer = window.setInterval(
      () => {
        setNow(new Date());
      },
      1000,
    );

    return () => {
      window.clearInterval(timer);
    };

  }, []);


  const met =
    calculateMET(
      mission.started_at,
      now,
    );


  const utc =
    now.toLocaleTimeString(
      "en-GB",
      {
        timeZone: "UTC",
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit",
        hour12: false,
      },
    );


  const kennedy =
    now.toLocaleTimeString(
      "en-US",
      {
        timeZone: "America/New_York",
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit",
      },
    );


  /*
   * A.I.M.S. demo-only simulated
   * Coordinated Lunar Time display.
   *
   * This is NOT presented as an
   * established lunar timezone.
   */
  const simulatedCLT =
  calculateSimulatedCLT(now);


  return (
    <header className="mission-header">

      <div className="aims-container mission-header-inner">

        <div className="brand">

          <span className="brand-name">
            A.I.M.S.
          </span>

          <span className="brand-subtitle">
            ASTRONAUT INTEGRITY & MISSION SUPPORT
          </span>

        </div>


        <div className="mission-meta">

          <MetaItem
            label="MISSION DAY"
            value={
              String(
                mission.mission_day,
              ).padStart(3, "0")
            }
          />

          <MetaItem
            label="MET"
            value={met}
          />

          <MetaItem
            label="UTC"
            value={utc}
          />

          <MetaItem
            label="CLT(S)"
            value={simulatedCLT}
          />

          <MetaItem
            label="KSC"
            value={kennedy}
          />


          <div className="meta-item">

            <span>EARTH LINK</span>

            <strong
              className={getStatusClass(
                mission.communications
                  .link_status,
              )}
            >
              {
                mission.communications
                  .link_status
              }
            </strong>

          </div>


          <div className="meta-item">

            <span>STATUS</span>

            <strong
              className={getStatusClass(
                mission.overall_status,
              )}
            >
              {mission.overall_status}
            </strong>

          </div>


          <div className="live-status">

            <span className="live-dot" />

            LIVE / SIMULATED

          </div>

        </div>

      </div>

    </header>
  );
}


interface MetaItemProps {
  label: string;
  value: string;
}


function MetaItem({
  label,
  value,
}: MetaItemProps) {

  return (
    <div className="meta-item">

      <span>{label}</span>

      <strong>{value}</strong>

    </div>
  );
}


function calculateMET(
  startedAt: string,
  now: Date,
): string {

  const start =
    new Date(startedAt).getTime();

  const elapsed =
    Math.max(
      0,
      now.getTime() - start,
    );


  const totalSeconds =
    Math.floor(
      elapsed / 1000,
    );


  const days =
    Math.floor(
      totalSeconds / 86400,
    );


  const hours =
    Math.floor(
      (totalSeconds % 86400) /
      3600,
    );


  const minutes =
    Math.floor(
      (totalSeconds % 3600) /
      60,
    );


  const seconds =
    totalSeconds % 60;


  const clock =
    [
      hours,
      minutes,
      seconds,
    ]
      .map((value) =>
        String(value).padStart(
          2,
          "0",
        ),
      )
      .join(":");


  if (days > 0) {
    return `${days}D ${clock}`;
  }


  return clock;
}
function calculateSimulatedCLT(
  now: Date,
): string {

  // Fictional lunar-time offset used
  // only for the A.I.M.S. simulation.
  const CLT_OFFSET_MINUTES =
    7 * 60 + 42;

  const cltTime =
    new Date(
      now.getTime() +
      CLT_OFFSET_MINUTES *
      60 *
      1000,
    );

  return cltTime.toLocaleTimeString(
    "en-GB",
    {
      timeZone: "UTC",
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
      hour12: false,
    },
  );
}