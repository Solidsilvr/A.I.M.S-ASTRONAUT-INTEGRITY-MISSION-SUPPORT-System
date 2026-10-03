import random


def generate_nominal_link():
    """
    Generate normal Moon-Earth communications data.
    """

    return {
        "signal_strength": round(
            random.uniform(95.0, 99.5),
            1,
        ),
        "packet_loss": round(
            random.uniform(0.0, 1.0),
            1,
        ),
        "latency_seconds": round(
            random.uniform(1.25, 1.35),
            2,
        ),
    }


def generate_degraded_link():
    """
    Generate a deliberately degraded
    communications state for testing.
    """

    return {
        "signal_strength": 52.0,
        "packet_loss": 14.0,
        "latency_seconds": 1.42,
    }


def generate_critical_link():
    """
    Simulate severe Moon-Earth link degradation.
    """

    return {
        "signal_strength": 28.0,
        "packet_loss": 24.0,
        "latency_seconds": 1.85,
    }