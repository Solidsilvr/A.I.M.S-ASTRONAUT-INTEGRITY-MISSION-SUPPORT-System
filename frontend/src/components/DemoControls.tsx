import {
  useState,
} from "react";

import {
  triggerOxygenEmergency,
  triggerEVAEmergency,
  triggerCommsDegraded,
  triggerCommsCritical,
  restoreNominal,
} from "../api/mission";


export function DemoControls() {

  const [open, setOpen] =
    useState(false);

  const [busy, setBusy] =
    useState(false);


  async function runAction(
    action: () => Promise<void>,
  ) {

    try {

      setBusy(true);

      await action();

    } catch (error) {

      console.error(
        "Demo action failed:",
        error,
      );

    } finally {

      setBusy(false);

    }
  }


  return (
    <div className="demo-controls">

      <button
        className="demo-trigger"
        onClick={() =>
          setOpen(!open)
        }
      >
        DEMO CONTROLS
      </button>


      {open && (

        <div className="demo-menu">

          <span className="demo-label">
            SIMULATION EVENTS
          </span>


          <button
            disabled={busy}
            onClick={() =>
              runAction(
                triggerOxygenEmergency,
              )
            }
          >
            OXYGEN EMERGENCY
            <small>EVA-01</small>
          </button>


          <button
            disabled={busy}
            onClick={() =>
              runAction(
                triggerEVAEmergency,
              )
            }
          >
            EVA RANGE BREACH
            <small>EVA-02</small>
          </button>


          <button
            disabled={busy}
            onClick={() =>
              runAction(
                triggerCommsDegraded,
              )
            }
          >
            COMMS DEGRADED
          </button>


          <button
            className="demo-critical"
            disabled={busy}
            onClick={() =>
              runAction(
                triggerCommsCritical,
              )
            }
          >
            COMMS FAILURE
          </button>


          <button
            className="demo-restore"
            disabled={busy}
            onClick={() =>
              runAction(
                restoreNominal,
              )
            }
          >
            RESTORE NOMINAL
          </button>

        </div>

      )}

    </div>
  );
}