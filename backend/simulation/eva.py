import random

from models.astronaut import EVAPosition


INITIAL_POSITIONS = {
    "EVA-01": (180.0, 120.0),
    "EVA-02": (420.0, 310.0),
    "EVA-03": (-260.0, 190.0),
}


def generate_initial_position(
    astronaut_id: str,
) -> EVAPosition:

    x, y = INITIAL_POSITIONS[astronaut_id]

    return EVAPosition(
        x=x,
        y=y,
    )


def move_position(
    position: EVAPosition,
) -> EVAPosition:
    """
    Simulate a small astronaut movement during EVA.
    """

    movement_x = random.uniform(-20, 20)
    movement_y = random.uniform(-20, 20)

    return EVAPosition(
        x=round(position.x + movement_x, 1),
        y=round(position.y + movement_y, 1),
    )