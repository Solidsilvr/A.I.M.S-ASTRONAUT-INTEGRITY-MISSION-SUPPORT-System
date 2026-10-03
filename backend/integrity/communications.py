def calculate_comms_integrity(
    signal_strength: float,
    packet_loss: float,
) -> dict:
    """
    Calculate communication integrity using
    prototype signal and packet-loss thresholds.
    """

    score = 100
    alerts = []

    # Signal strength penalties
    if signal_strength < 40:
        score -= 50
        alerts.append("CRITICAL SIGNAL DEGRADATION")

    elif signal_strength < 70:
        score -= 25
        alerts.append("WEAK SIGNAL")

    elif signal_strength < 90:
        score -= 10

    # Packet-loss penalties
    if packet_loss >= 20:
        score -= 50
        alerts.append("CRITICAL PACKET LOSS")

    elif packet_loss >= 10:
        score -= 25
        alerts.append("HIGH PACKET LOSS")

    elif packet_loss >= 5:
        score -= 10

    score = max(0, score)

    # Critical subsystem conditions override score.
    if signal_strength < 40 or packet_loss >= 20:
        status = "CRITICAL"

    elif signal_strength < 70 or packet_loss >= 10:
        status = "WARNING"

    else:
        status = "NORMAL"

    return {
        "comms_integrity": score,
        "status": status,
        "alerts": alerts,
    }