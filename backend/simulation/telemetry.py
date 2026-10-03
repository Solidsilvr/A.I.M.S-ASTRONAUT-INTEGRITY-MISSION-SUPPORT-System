import random

from models.astronaut import AstronautTelemetry


CREW = {
    "EVA-01": "Maya Chen",
    "EVA-02": "Alex Rivera",
    "EVA-03": "Sam Okafor",
}


def generate_telemetry(
    astronaut_id: str,
    name: str,
) -> AstronautTelemetry:
    """
    Generate nominal simulated biometric telemetry
    for one astronaut.
    """

    return AstronautTelemetry(
        astronaut_id=astronaut_id,
        name=name,
        heart_rate=random.randint(65, 90),
        spo2=round(random.uniform(95.0, 99.0), 1),
        temperature=round(random.uniform(36.4, 37.2), 1),
        sleep_hours=round(random.uniform(6.5, 8.0), 1),
        exercise_minutes=random.randint(60, 100),
    )


def generate_crew_telemetry():
    """
    Generate telemetry for the complete EVA crew.
    """

    return {
        astronaut_id: generate_telemetry(
            astronaut_id,
            name,
        )
        for astronaut_id, name in CREW.items()
    }


def generate_oxygen_emergency(
    astronaut_id: str = "EVA-01",
) -> AstronautTelemetry:
    """
    Simulate a critical oxygen anomaly for a
    selected astronaut.
    """

    name = CREW[astronaut_id]

    return AstronautTelemetry(
        astronaut_id=astronaut_id,
        name=name,
        heart_rate=108,
        spo2=88.0,
        temperature=36.9,
        sleep_hours=7.2,
        exercise_minutes=78,
    )