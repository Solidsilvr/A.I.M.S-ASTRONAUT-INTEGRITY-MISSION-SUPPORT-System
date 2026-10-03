import type {
  MissionState,
} from "../types/mission";

const API_URL =
  import.meta.env.VITE_API_URL ||
  "http://127.0.0.1:8000";

const WS_URL =
  import.meta.env.VITE_WS_URL ||
  "ws://127.0.0.1:8000";


export async function getMission():
  Promise<MissionState> {

  const response = await fetch(
    `${API_URL}/mission`,
  );

  if (!response.ok) {
    throw new Error(
      `Mission API failed: ${response.status}`,
    );
  }

  return response.json();
}


export function connectMissionWebSocket(
  onMissionUpdate: (
    mission: MissionState,
  ) => void,
): WebSocket {

  const socket = new WebSocket(
    `${WS_URL}/ws/mission`,
  );

  socket.onopen = () => {
    console.log(
      "A.I.M.S. WebSocket connected",
    );
  };

  socket.onmessage = (event) => {
    const mission: MissionState =
      JSON.parse(event.data);

    onMissionUpdate(mission);
  };

  socket.onerror = (error) => {
    console.error(
      "A.I.M.S. WebSocket error:",
      error,
    );
  };

  return socket;
}


export async function runSimulationTick():
  Promise<void> {

  const response = await fetch(
    `${API_URL}/simulation/tick`,
    {
      method: "POST",
    },
  );

  if (!response.ok) {
    throw new Error(
      "Simulation tick failed",
    );
  }
}


/* =========================
   DEMO CONTROLS
   ========================= */

async function postDemoEndpoint(
  endpoint: string,
): Promise<void> {

  const response = await fetch(
    `${API_URL}${endpoint}`,
    {
      method: "POST",
    },
  );

  if (!response.ok) {
    throw new Error(
      `Demo action failed: ${response.status}`,
    );
  }
}


export function triggerOxygenEmergency() {
  return postDemoEndpoint(
    "/simulate/oxygen-emergency/EVA-01",
  );
}


export function triggerEVAEmergency() {
  return postDemoEndpoint(
    "/simulate/eva-range-emergency/EVA-02",
  );
}


export function triggerCommsDegraded() {
  return postDemoEndpoint(
    "/simulate/comms-degraded",
  );
}


export function triggerCommsCritical() {
  return postDemoEndpoint(
    "/simulate/comms-critical",
  );
}


export function restoreNominal() {
  return postDemoEndpoint(
    "/simulation/restore",
  );
}