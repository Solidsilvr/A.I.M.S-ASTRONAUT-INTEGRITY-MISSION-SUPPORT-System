import math

from models.astronaut import EVAPosition


WARNING_RADIUS = 750
CRITICAL_RADIUS = 1000


def calculate_distance(
    x1: float,
    y1: float,
    x2: float,
    y2: float,
) -> float:

    return math.sqrt(
        (x2 - x1) ** 2 +
        (y2 - y1) ** 2
    )


def distance_from_base(
    position: EVAPosition,
) -> float:

    return calculate_distance(
        0,
        0,
        position.x,
        position.y,
    )


def evaluate_eva_status(
    distance: float,
) -> str:

    if distance >= CRITICAL_RADIUS:
        return "CRITICAL"

    if distance >= WARNING_RADIUS:
        return "WARNING"

    return "NORMAL"

def update_crew_proximity(crew):
    """
    Calculate distance from habitat and nearest
    crewmember for every astronaut.
    """

    for astronaut_id, astronaut in crew.items():

        position = astronaut.position

        base_distance = distance_from_base(
            position
        )

        position.distance_from_base = round(
            base_distance,
            1,
        )

        position.eva_status = evaluate_eva_status(
            base_distance
        )

        nearest_id = None
        nearest_distance = None

        for other_id, other in crew.items():

            if other_id == astronaut_id:
                continue

            distance = calculate_distance(
                position.x,
                position.y,
                other.position.x,
                other.position.y,
            )

            if (
                nearest_distance is None
                or distance < nearest_distance
            ):
                nearest_distance = distance
                nearest_id = other_id

        position.nearest_crew_id = nearest_id

        if nearest_distance is not None:
            position.nearest_crew_distance = round(
                nearest_distance,
                1,
            )

    return crew