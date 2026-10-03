def calculate_eva_integrity(
    distance_from_base: float,
) -> int:
    """
    Calculate EVA integrity from distance
    to the lunar habitat.

    Prototype thresholds only.
    """

    if distance_from_base >= 1000:
        return 40

    if distance_from_base >= 750:
        return 70

    if distance_from_base >= 500:
        return 90

    return 100


def calculate_overall_integrity(
    health_integrity: int,
    eva_integrity: int,
    comms_integrity: int,
) -> int:
    """
    Weighted A.I.M.S. integrity score.

    Health: 60%
    EVA:    25%
    Comms:  15%
    """

    score = (
        health_integrity * 0.60
        + eva_integrity * 0.25
        + comms_integrity * 0.15
    )

    return round(score)


def determine_overall_status(
    health_status: str,
    eva_status: str,
    comms_status: str,
    overall_integrity: int,
) -> str:
    """
    Determine operational state.

    Critical subsystem states override
    the weighted numerical score.
    """

    subsystem_statuses = [
        health_status,
        eva_status,
        comms_status,
    ]

    if "CRITICAL" in subsystem_statuses:
        return "CRITICAL"

    if "WARNING" in subsystem_statuses:
        return "WARNING"

    if overall_integrity < 60:
        return "CRITICAL"

    if overall_integrity < 85:
        return "WARNING"

    return "NORMAL"