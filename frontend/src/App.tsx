import {
  useEffect,
  useState,
} from "react";

import {
  Navigation,
  type Page,
} from "./components/Navigation";

import { MissionHeader } from "./components/MissionHeader";

import { MissionLive } from "./pages/MissionLive";
import { Crew } from "./pages/Crew";
import { LunarMap } from "./pages/LunarMap";
import { Comms } from "./pages/Comms";
import { MissionLog } from "./pages/MissionLog";

import {
  getMission,
  connectMissionWebSocket,
  runSimulationTick,
} from "./api/mission";

import type {
  MissionState,
} from "./types/mission";

import { DemoControls } from "./components/DemoControls";

function App() {
  const [activePage, setActivePage] =
    useState<Page>("mission");

  const [mission, setMission] =
    useState<MissionState | null>(null);

  const [error, setError] =
    useState<string | null>(null);


  useEffect(() => {
  let socket: WebSocket | null = null;

  async function initializeMission() {
    try {
      const data = await getMission();

      setMission(data);
      setError(null);

      socket = connectMissionWebSocket(
        (updatedMission) => {
          setMission(updatedMission);
        },
      );

    } catch (err) {
      console.error(err);

      setError(
        "Unable to connect to A.I.M.S. mission API.",
      );
    }
  }

  initializeMission();

  return () => {
    socket?.close();
  };
}, []);

  useEffect(() => {

  const simulationTimer =
    window.setInterval(
      async () => {

        try {

          await runSimulationTick();

        } catch (error) {

          console.error(
            "A.I.M.S. simulation update failed:",
            error,
          );

        }

      },
      5000,
    );


  return () => {
    window.clearInterval(
      simulationTimer,
    );
  };

}, []);


  function renderPage(currentMission: MissionState) {
  switch (activePage) {
    case "crew":
      return <Crew mission={currentMission} />;

    case "map":
      return <LunarMap mission={currentMission} />;

    case "comms":
      return <Comms mission={currentMission} />;

    case "log":
      return <MissionLog mission={currentMission} />;

    default:
      return (
        <MissionLive mission={currentMission} />
      );
  }
}


  if (error) {
    return (
      <div className="aims-container system-message">
        <p className="eyebrow">
          CONNECTION ERROR
        </p>

        <h1>A.I.M.S. Offline</h1>

        <p className="page-description">
          {error}
        </p>
      </div>
    );
  }


  if (!mission) {
    return (
      <div className="aims-container system-message">
        <p className="eyebrow">
          INITIALIZING
        </p>

        <h1>A.I.M.S.</h1>

        <p className="page-description">
          Acquiring mission telemetry...
        </p>
      </div>
    );
  }

console.log("AIMS MISSION STATE:", mission);
  return (
    <div className={`aims-app condition-${mission.overall_status.toLowerCase()}`}>

      <MissionHeader mission={mission} />

      <Navigation
        activePage={activePage}
        onChange={setActivePage}
      />
      <DemoControls />
      <main className="aims-container page-content">
        {renderPage(mission)}
      </main>

    </div>
  );
}


export default App;