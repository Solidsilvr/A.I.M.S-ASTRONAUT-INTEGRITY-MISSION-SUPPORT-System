import type {
  MissionState,
} from "../types/mission";

import { getStatusClass } from "../utils/status";

interface CommsProps {
  mission: MissionState;
}

export function Comms({
  mission,
}: CommsProps) {
  const comms = mission.communications;

  return (
    <section>
      <div className="comms-heading">
        <div>
          <p className="eyebrow">
            SPACE ↔ GROUND
          </p>

          <h1>Communications</h1>

          <p className="page-description">
            Simulated Moon-to-Earth telemetry
            link and communications integrity.
          </p>
        </div>

        <div className="comms-heading-status">
          <span>LINK STATUS</span>

          <strong
            className={getStatusClass(
              comms.link_status,
            )}
          >
            {comms.link_status}
          </strong>
        </div>
      </div>

      <div className="comms-metrics">
        <MetricCard
          label="SIGNAL"
          value={`${comms.signal_strength}%`}
          detail="SIGNAL STRENGTH"
        />

        <MetricCard
          label="PACKET LOSS"
          value={`${comms.packet_loss}%`}
          detail="CURRENT LOSS"
        />

        <MetricCard
          label="LATENCY"
          value={`${comms.latency_seconds}s`}
          detail="ONE-WAY SIMULATED"
        />

        <MetricCard
          label="INTEGRITY"
          value={`${comms.integrity}`}
          detail="COMMS SCORE"
        />
      </div>

      <div className="comms-link-panel">
        <span className="section-label">
          MOON ↔ EARTH LINK
        </span>

        <div className="space-link">
          <div className="link-endpoint">
            <div className="moon-body" />

            <strong>MOON</strong>
            <span>LUNAR CREW</span>
          </div>

          <div className="link-path">
            <div className="link-line" />

            <div
              className={`packet ${
                getStatusClass(
                  comms.link_status,
                )
              }`}
            />

            <span className="latency-label">
              {comms.latency_seconds}s
              {" ONE-WAY"}
            </span>
          </div>

          <div className="link-endpoint">
            <div className="earth-body">
              <span />
            </div>

            <strong>EARTH</strong>
            <span>MISSION NETWORK</span>
          </div>
        </div>
      </div>

      <div className="comms-detail-grid">
        <article className="comms-detail-panel">
          <span className="section-label">
            LINK TELEMETRY
          </span>

          <CommsRow
            label="LINK STATE"
            value={comms.link_status}
            status={comms.link_status}
          />

          <CommsRow
            label="SIGNAL"
            value={`${comms.signal_strength}%`}
          />

          <CommsRow
            label="PACKET LOSS"
            value={`${comms.packet_loss}%`}
          />

          <CommsRow
            label="ONE-WAY LATENCY"
            value={`${comms.latency_seconds} SEC`}
          />

          <CommsRow
            label="INTEGRITY"
            value={`${comms.integrity} / 100`}
          />
        </article>

        <article className="comms-detail-panel">
          <span className="section-label">
            PUBLIC COMMS FEED
          </span>

          <div className="comms-feed">
            <p>
              <span>MOON</span>
              A.I.M.S. telemetry packet
              received.
            </p>

            <p>
              <span>EARTH</span>
              Mission network link active.
            </p>

            <p>
              <span>SYSTEM</span>
              Communications integrity:
              {" "}
              {comms.integrity}/100.
            </p>

            <p>
              <span>SYSTEM</span>
              One-way propagation simulation:
              {" "}
              {comms.latency_seconds} seconds.
            </p>
          </div>
        </article>
      </div>

      <p className="simulation-notice">
        PUBLIC TELEMETRY FEED — SIMULATED DATA
        FOR DEMONSTRATION
      </p>
    </section>
  );
}

interface MetricCardProps {
  label: string;
  value: string;
  detail: string;
}

function MetricCard({
  label,
  value,
  detail,
}: MetricCardProps) {
  return (
    <article className="comms-metric-card">
      <span>{label}</span>

      <strong>{value}</strong>

      <small>{detail}</small>
    </article>
  );
}

interface CommsRowProps {
  label: string;
  value: string;
  status?: string;
}

function CommsRow({
  label,
  value,
  status,
}: CommsRowProps) {
  return (
    <div className="comms-row">
      <span>{label}</span>

      <strong
        className={
          status
            ? getStatusClass(status)
            : undefined
        }
      >
        {value}
      </strong>
    </div>
  );
}