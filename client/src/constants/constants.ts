const urlParts = window.location.href.split(":");
export const API_ENDPOINT = urlParts[0] + ":" + urlParts[1] + ":3000";

export const REFRESH_INTERVAL_MS = 500;
