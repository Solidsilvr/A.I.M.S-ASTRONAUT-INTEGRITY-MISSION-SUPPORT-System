export function getStatusClass(
  status: string,
): string {
  switch (status.toUpperCase()) {

    case "CRITICAL":
      return "status-critical";

    case "WARNING":
    case "DEGRADED":
      return "status-warning";

    case "OFFLINE":
      return "status-offline";

    case "NORMAL":
    case "STABLE":
      return "status-normal";

    case "INFO":
      return "status-info";

    default:
      return "status-info";
  }
}