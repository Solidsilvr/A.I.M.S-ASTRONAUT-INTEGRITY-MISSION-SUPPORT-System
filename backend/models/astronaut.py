from pydantic import BaseModel


class AstronautTelemetry(BaseModel):
    astronaut_id: str
    name: str

    heart_rate: int
    spo2: float
    temperature: float
    sleep_hours: float
    exercise_minutes: int

class EVAPosition(BaseModel):
    x: float
    y: float

    distance_from_base: float = 0

    nearest_crew_id: str | None = None
    nearest_crew_distance: float | None = None

    eva_status: str = "NORMAL"