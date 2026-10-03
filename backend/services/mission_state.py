from datetime import datetime, timezone

from models.mission import (
    MissionState,
    MissionEvent,
    AstronautState,
    CommunicationsState,
)

from simulation.telemetry import (
    CREW,
    generate_telemetry,
    generate_crew_telemetry,
    generate_oxygen_emergency,
)

from integrity.health import calculate_health_integrity

from simulation.eva import (
    generate_initial_position,
    move_position,
)

from integrity.proximity import (
    update_crew_proximity,
)

from integrity.overall import (
    calculate_eva_integrity,
    calculate_overall_integrity,
    determine_overall_status,
)

from integrity.communications import (
    calculate_comms_integrity,
)

from simulation.communications import (
    generate_nominal_link,
    generate_degraded_link,
    generate_critical_link,
)

def build_astronaut_state(
    telemetry,
    position=None,
):
    health = calculate_health_integrity(
        telemetry
    )

    if position is None:
        position = generate_initial_position(
            telemetry.astronaut_id
        )

    return AstronautState(
        telemetry=telemetry,
        position=position,

        health_integrity=health[
            "health_integrity"
        ],

        health_status=health["status"],

        alerts=health["alerts"],
    )


def create_initial_crew():
    telemetry = generate_crew_telemetry()

    return {
        astronaut_id: build_astronaut_state(data)
        for astronaut_id, data in telemetry.items()
    }

def create_initial_communications():
    link = generate_nominal_link()

    result = calculate_comms_integrity(
        link["signal_strength"],
        link["packet_loss"],
    )

    return CommunicationsState(
        signal_strength=link["signal_strength"],
        packet_loss=link["packet_loss"],
        latency_seconds=link["latency_seconds"],
        link_status="STABLE",
        integrity=result["comms_integrity"],
    )

# --------------------------------------------------
# Persistent mission state
# --------------------------------------------------

mission_state = MissionState(
    mission_id="AIMS-LUNAR-01",
    mission_day=14,
    started_at=datetime.now(timezone.utc),

    crew=update_crew_proximity(
    create_initial_crew()
),
    communications=create_initial_communications(),
    overall_status="NORMAL",

    events=[
        MissionEvent(
            timestamp=datetime.now(timezone.utc),
            event_type="SYSTEM",
            message="A.I.M.S. crew monitoring initialized.",
            severity="INFO",
        )
    ],
)


def get_mission_state():
    return mission_state


def update_overall_status():
    statuses = [
        astronaut.overall_status
        for astronaut in mission_state.crew.values()
    ]

    if "CRITICAL" in statuses:
        mission_state.overall_status = "CRITICAL"

    elif "WARNING" in statuses:
        mission_state.overall_status = "WARNING"

    else:
        mission_state.overall_status = "NORMAL"


def update_normal_telemetry():
    """
    Generate a new nominal telemetry cycle
    for the complete crew while preserving
    each astronaut's EVA position.
    """

    crew_telemetry = generate_crew_telemetry()

    for astronaut_id, telemetry in crew_telemetry.items():
        current_position = mission_state.crew[astronaut_id].position

        mission_state.crew[astronaut_id] = build_astronaut_state(
            telemetry,
            current_position,
        )

    refresh_integrity()

    return mission_state.crew


def trigger_oxygen_emergency(
    astronaut_id: str = "EVA-01",
):
    """
    Trigger a persistent oxygen emergency for
    one selected astronaut.
    """

    if astronaut_id not in CREW:
        return None

    telemetry = generate_oxygen_emergency(
        astronaut_id
    )

    current_position = (
    mission_state
    .crew[astronaut_id]
    .position
)

    astronaut_state = build_astronaut_state(
    telemetry,
    current_position,
)

    mission_state.crew[astronaut_id] = (
        astronaut_state
    )

    mission_state.events.append(
        MissionEvent(
            timestamp=datetime.now(timezone.utc),
            event_type="HEALTH_ALERT",
            astronaut_id=astronaut_id,
            message=(
                "Critical oxygen saturation detected: "
                f"{telemetry.spo2}%"
            ),
            severity="CRITICAL",
        )
    )

    refresh_integrity()

    return astronaut_state

def update_eva_positions():
    """
    Move all EVA crew members and recalculate
    proximity information.
    """

    for astronaut in mission_state.crew.values():

        astronaut.position = move_position(
            astronaut.position
        )

    refresh_integrity()

    return {
        astronaut_id: astronaut.position
        for astronaut_id, astronaut
        in mission_state.crew.items()
    }

def trigger_eva_range_emergency(
    astronaut_id: str = "EVA-02",
):
    """
    Move an astronaut beyond the prototype
    EVA critical safety radius.
    """

    if astronaut_id not in CREW:
        return None

    astronaut = mission_state.crew[astronaut_id]

    # Force position beyond 1 km from habitat
    astronaut.position.x = 1050.0
    astronaut.position.y = 250.0

    refresh_integrity()

    mission_state.events.append(
        MissionEvent(
            timestamp=datetime.now(timezone.utc),
            event_type="EVA_ALERT",
            astronaut_id=astronaut_id,
            message=(
                "Astronaut exceeded critical "
                "EVA safety radius."
            ),
            severity="CRITICAL",
        )
    )

    return astronaut

def refresh_integrity():
    update_crew_proximity(
        mission_state.crew
    )

    comms_result = (
        calculate_comms_integrity(
            mission_state.communications.signal_strength,
            mission_state.communications.packet_loss,
        )
    )

    mission_state.communications.integrity = (
        comms_result[
            "comms_integrity"
        ]
    )

    if (
        comms_result["status"]
        == "CRITICAL"
    ):
        mission_state.communications.link_status = (
            "CRITICAL"
        )

    elif (
        comms_result["status"]
        == "WARNING"
    ):
        mission_state.communications.link_status = (
            "DEGRADED"
        )

    else:
        mission_state.communications.link_status = (
            "STABLE"
        )


    for astronaut in (
        mission_state.crew.values()
    ):

        astronaut.eva_integrity = (
            calculate_eva_integrity(
                astronaut.position.distance_from_base
            )
        )

        astronaut.eva_status = (
            astronaut.position.eva_status
        )

        astronaut.comms_integrity = (
            mission_state.communications.integrity
        )

        astronaut.comms_status = (
            comms_result["status"]
        )


        # -------------------------
        # REMOVE OLD EVA ALERTS
        # -------------------------

        astronaut.alerts = [
            alert
            for alert
            in astronaut.alerts
            if not alert.startswith(
                "EVA:"
            )
        ]


        # -------------------------
        # ADD CURRENT EVA ALERT
        # -------------------------

        if (
            astronaut.eva_status
            == "CRITICAL"
        ):
            astronaut.alerts.append(
                "EVA: CRITICAL SAFETY RADIUS BREACH"
            )

        elif (
            astronaut.eva_status
            == "WARNING"
        ):
            astronaut.alerts.append(
                "EVA: APPROACHING SAFETY LIMIT"
            )


        astronaut.overall_integrity = (
            calculate_overall_integrity(
                astronaut.health_integrity,
                astronaut.eva_integrity,
                astronaut.comms_integrity,
            )
        )

        astronaut.overall_status = (
            determine_overall_status(
                astronaut.health_status,
                astronaut.eva_status,
                astronaut.comms_status,
                astronaut.overall_integrity,
            )
        )


    update_overall_status()

# Calculate complete integrity state
# when the backend starts.
refresh_integrity()

def apply_link_state(link):
    """
    Apply communications telemetry and
    recalculate mission integrity.
    """

    mission_state.communications.signal_strength = (
        link["signal_strength"]
    )

    mission_state.communications.packet_loss = (
        link["packet_loss"]
    )

    mission_state.communications.latency_seconds = (
        link["latency_seconds"]
    )

    refresh_integrity()

    return mission_state.communications

def update_nominal_communications():
    return apply_link_state(
        generate_nominal_link()
    )


def trigger_degraded_communications():
    communications = apply_link_state(
        generate_degraded_link()
    )

    mission_state.events.append(
        MissionEvent(
            timestamp=datetime.now(timezone.utc),
            event_type="COMMS_ALERT",
            message=(
                "Moon-Earth communications "
                "link degraded."
            ),
            severity="WARNING",
        )
    )

    return communications


def trigger_critical_communications():
    communications = apply_link_state(
        generate_critical_link()
    )

    mission_state.events.append(
        MissionEvent(
            timestamp=datetime.now(timezone.utc),
            event_type="COMMS_ALERT",
            message=(
                "Critical Moon-Earth "
                "communications degradation."
            ),
            severity="CRITICAL",
        )
    )

    return communications

def restore_nominal_state():

    # -------------------------
    # RESET CREW TELEMETRY
    # -------------------------

    crew_telemetry = (
        generate_crew_telemetry()
    )

    for astronaut_id, telemetry in (
        crew_telemetry.items()
    ):

        position = (
            generate_initial_position(
                astronaut_id
            )
        )

        mission_state.crew[
            astronaut_id
        ] = build_astronaut_state(
            telemetry,
            position,
        )


    # -------------------------
    # RESET COMMUNICATIONS
    # -------------------------

    link = generate_nominal_link()

    mission_state.communications.signal_strength = (
        link["signal_strength"]
    )

    mission_state.communications.packet_loss = (
        link["packet_loss"]
    )

    mission_state.communications.latency_seconds = (
        link["latency_seconds"]
    )


    # -------------------------
    # RECALCULATE EVERYTHING
    # -------------------------

    refresh_integrity()


    # -------------------------
    # LOG RESTORE EVENT
    # -------------------------

    mission_state.events.append(
        MissionEvent(
            timestamp=datetime.now(
                timezone.utc
            ),
            event_type="SYSTEM",
            message=(
                "Mission simulation restored "
                "to nominal conditions."
            ),
            severity="INFO",
        )
    )


    return mission_state

def run_simulation_tick():
    update_normal_telemetry()
    update_eva_positions()
    update_nominal_communications()

    return mission_state