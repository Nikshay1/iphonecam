function snapshot(line) {
  if (typeof line !== "string" || line.length > 16384) return null
  var value
  try { value = JSON.parse(line) } catch (e) { return null }
  if (!value || typeof value !== "object" || Array.isArray(value)) return null
  var cameras = Array.isArray(value.cameras) ? value.cameras.slice(0, 16).filter(function(c) {
    return c && Number.isInteger(c.id) && c.id >= 0 && c.id < 16 && typeof c.label === "string"
  }).map(function(c) { return { id: c.id, label: c.label.slice(0, 80) } }) : []
  return {
    connected: value.connected === true, enabled: value.enabled === true,
    streaming: value.streaming === true, lensControls: value.lensControls === true,
    lensBusy: value.lensBusy === true, autoConnect: value.autoConnect === true,
    status: String(value.status || "offline").slice(0, 30),
    message: String(value.message || "").slice(0, 500),
    phoneName: String(value.phoneName || "").slice(0, 80),
    resolution: ["640x480", "1280x720", "1920x1080"].indexOf(value.resolution) >= 0 ? value.resolution : "1280x720",
    activeCamera: Number.isInteger(value.activeCamera) ? value.activeCamera : -1,
    cameras: cameras
  }
}

function lensLabel(state) {
  for (var i = 0; i < state.cameras.length; i++)
    if (state.cameras[i].id === state.activeCamera) return state.cameras[i].label
  return ""
}

if (typeof module !== "undefined") module.exports = { snapshot: snapshot, lensLabel: lensLabel }
