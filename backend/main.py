from fastapi import (
    FastAPI,
    HTTPException,
    WebSocket,
    WebSocketDisconnect,
)
from services.mission_state import (
    get_mission_state,
    update_normal_telemetry,
    trigger_oxygen_emergency,
    update_eva_positions,
    trigger_eva_range_emergency,
    update_nominal_communications,
    trigger_degraded_communications,
    trigger_critical_communications,
    run_simulation_tick,
    restore_nominal_state,
)
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI(
    title="A.I.M.S. API",
    description="Astronaut Integrity & Mission Support",
    version="1.0.0",
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=[
        "http://localhost:5173",
        "http://127.0.0.1:5173",
        "https://a-i-m-s-astronaut-integrity-mission.vercel.app",
    ],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/")
def root():
    return {
        "system": "A.I.M.S.",
        "status": "ONLINE",
        "message": (
            "Astronaut Integrity & Mission Support API"
        ),
    }


@app.get("/mission")
def mission():
    return get_mission_state()


@app.post("/telemetry/update")
async def telemetry_update():
    crew = update_normal_telemetry()
    await broadcast_mission_state()
    return {
        "crew": crew,
        "mission_status": (
            get_mission_state().overall_status
        ),
    }


@app.post(
    "/simulate/oxygen-emergency/{astronaut_id}"
)
async def oxygen_emergency(
    astronaut_id: str,
):
    astronaut = trigger_oxygen_emergency(
        astronaut_id
    )

    if astronaut is None:
        raise HTTPException(
            status_code=404,
            detail="Astronaut not found",
        )

    await broadcast_mission_state()

    return {
        "event": "OXYGEN_EMERGENCY",
        "astronaut": astronaut,
        "mission_status":
            get_mission_state().overall_status,
    }

@app.post("/eva/update")
async def eva_update():

    positions = update_eva_positions()
    await broadcast_mission_state()
    return {
        "positions": positions
    }

@app.post(
    "/simulate/eva-range-emergency/{astronaut_id}"
)
async def eva_range_emergency(
    astronaut_id: str,
):
    astronaut = (
        trigger_eva_range_emergency(
            astronaut_id
        )
    )

    if astronaut is None:
        raise HTTPException(
            status_code=404,
            detail="Astronaut not found",
        )

    await broadcast_mission_state()

    return {
        "event": "EVA_RANGE_EMERGENCY",
        "astronaut": astronaut,
    }

@app.post("/communications/update")
async def communications_update():

    communications = (
        update_nominal_communications()
    )
    await broadcast_mission_state()
    return {
        "communications": communications,
        "mission_status": (
            get_mission_state().overall_status
        ),
    }


@app.post("/simulate/comms-degraded")
async def simulate_comms_degraded():
    communications = (
        trigger_degraded_communications()
    )

    await broadcast_mission_state()

    return {
        "event": "COMMS_DEGRADED",
        "communications": communications,
        "mission_status":
            get_mission_state().overall_status,
    }


@app.post("/simulate/comms-critical")
async def simulate_comms_critical():
    communications = (
        trigger_critical_communications()
    )

    await broadcast_mission_state()

    return {
        "event": "COMMS_CRITICAL",
        "communications": communications,
        "mission_status":
            get_mission_state().overall_status,
    }

connected_clients: list[WebSocket] = []


@app.websocket("/ws/mission")
async def mission_websocket(
    websocket: WebSocket,
):
    await websocket.accept()

    connected_clients.append(websocket)

    try:
        await websocket.send_json(
            get_mission_state().model_dump(
                mode="json"
            )
        )

        while True:
            await websocket.receive_text()

    except WebSocketDisconnect:
        if websocket in connected_clients:
            connected_clients.remove(websocket)


async def broadcast_mission_state():
    mission_data = (
        get_mission_state().model_dump(
            mode="json"
        )
    )

    disconnected = []

    for websocket in connected_clients:
        try:
            await websocket.send_json(
                mission_data
            )
        except Exception:
            disconnected.append(websocket)

    for websocket in disconnected:
        if websocket in connected_clients:
            connected_clients.remove(
                websocket
            )

@app.post("/simulation/tick")
async def simulation_tick():
    mission = run_simulation_tick()

    await broadcast_mission_state()

    return {
        "status": "SIMULATION_UPDATED",
        "mission_status": mission.overall_status,
    }

@app.post("/simulation/restore")
async def simulation_restore():

    mission = (
        restore_nominal_state()
    )

    await broadcast_mission_state()

    return {
        "status": "NOMINAL_RESTORED",
        "mission_status":
            mission.overall_status,
    }
