from models.astronaut import AstronautTelemetry


def calculate_health_integrity(telemetry: AstronautTelemetry) -> dict:
    """
    Calculate an explainable health-integrity score.

    Every abnormal biometric applies a visible penalty.
    Critical measurements can also override the final state.
    """

    score = 100
    alerts = []

    # Oxygen saturation
    if telemetry.spo2 < 90:
        score -= 40
        alerts.append("CRITICAL LOW OXYGEN")

    elif telemetry.spo2 < 95:
        score -= 20
        alerts.append("LOW OXYGEN")

    # Heart rate
    if telemetry.heart_rate < 50 or telemetry.heart_rate > 120:
        score -= 20
        alerts.append("ABNORMAL HEART RATE")

    # Body temperature
    if telemetry.temperature < 35.5 or telemetry.temperature > 38.0:
        score -= 20
        alerts.append("ABNORMAL BODY TEMPERATURE")

    # Sleep
    if telemetry.sleep_hours < 6:
        score -= 10
        alerts.append("INSUFFICIENT SLEEP")

    # Exercise
    if telemetry.exercise_minutes < 30:
        score -= 10
        alerts.append("LOW ACTIVITY")

    score = max(0, score)

    # Determine health state
    if telemetry.spo2 < 90 or score < 60:
        status = "CRITICAL"

    elif score < 85:
        status = "WARNING"

    else:
        status = "NORMAL"

    return {
        "health_integrity": score,
        "status": status,
        "alerts": alerts,
    }