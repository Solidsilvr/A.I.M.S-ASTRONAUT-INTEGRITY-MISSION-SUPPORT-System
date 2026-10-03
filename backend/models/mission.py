from datetime import datetime, timezone
from pydantic import BaseModel

from models.astronaut import (
    AstronautTelemetry,
    EVAPosition,
)

class MissionEvent(BaseModel):
    timestamp: datetime
    event_type: str
    astronaut_id: str | None = None
    message: str
    severity: str = "INFO"


class AstronautState(BaseModel):
    telemetry: AstronautTelemetry
    position: EVAPosition

    # Individual integrity dimensions
    health_integrity: int
    eva_integrity: int = 100
    comms_integrity: int = 100

    # Combined A.I.M.S. integrity
    overall_integrity: int = 100

    health_status: str = "NORMAL"
    eva_status: str = "NORMAL"
    comms_status: str = "NORMAL"

    overall_status: str = "NORMAL"

    alerts: list[str] = []

class CommunicationsState(BaseModel):
    signal_strength: float = 98.7
    packet_loss: float = 0.2
    latency_seconds: float = 1.28

    link_status: str = "STABLE"
    integrity: int = 100

    last_packet_at: datetime | None = None
    last_packet_id: str | None = None

class MissionState(BaseModel):
    mission_id: str
    mission_day: int
    started_at: datetime

    crew: dict[str, AstronautState]

    communications: CommunicationsState

    overall_status: str = "NORMAL"

    events: list[MissionEvent] = []