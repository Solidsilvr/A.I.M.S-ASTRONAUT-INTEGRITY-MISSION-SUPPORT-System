export interface Telemetry {
  astronaut_id: string;
  name: string;

  heart_rate: number;
  spo2: number;
  temperature: number;
  sleep_hours: number;
  exercise_minutes: number;
}


export interface EVAPosition {
  x: number;
  y: number;

  distance_from_base: number;

  nearest_crew_id: string | null;
  nearest_crew_distance: number | null;

  eva_status: string;
}


export interface AstronautState {
  telemetry: Telemetry;
  position: EVAPosition;

  health_integrity: number;
  eva_integrity: number;
  comms_integrity: number;

  overall_integrity: number;

  health_status: string;
  eva_status: string;
  comms_status: string;

  overall_status: string;

  alerts: string[];
}


export interface CommunicationsState {
  signal_strength: number;
  packet_loss: number;
  latency_seconds: number;

  link_status: string;
  integrity: number;

  last_packet_at: string | null;
  last_packet_id: string | null;
}


export interface MissionEvent {
  timestamp: string;
  event_type: string;

  astronaut_id: string | null;

  message: string;
  severity: string;
}


export interface MissionState {
  mission_id: string;
  mission_day: number;
  started_at: string;

  crew: Record<string, AstronautState>;

  communications: CommunicationsState;

  overall_status: string;

  events: MissionEvent[];
}