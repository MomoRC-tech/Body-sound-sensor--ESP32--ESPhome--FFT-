// Node-RED Function: Parse Body Sound Spectrum JSON and prepare InfluxDB payload
// Use with Home Assistant 'server-state-changed' node. If that node outputs object msg.data.new_state.state,
// set msg.payload = msg.data.new_state.state via a preceding Change node OR adapt raw extraction below.
// Optionally supply latest CPU load via flow context (flow.get('cpu_load_latest')) rather than msg.cpu_load.
// This function mirrors logic in examples/node_red_flow.json but can be used standalone.

// CONFIGURABLE CONSTANTS
const MEASUREMENT = "body_sound";
const SENSOR_TAG = "body_sound_1";   // change if multiple sensors
const LOCATION_TAG = "basement";     // customize location
const SCHEMA_SUPPORTED = 1;           // current known schema_version

function parseSpectrum(raw) {
  if (!raw || raw === 'unknown' || raw === 'unavailable') return null;
  let j;
  try { j = JSON.parse(raw); } catch (e) { node.error("JSON parse error: " + e, { raw }); return null; }

  // Optional schema_version gating
  if (j.schema_version && j.schema_version > SCHEMA_SUPPORTED) {
    node.warn(`Future schema_version ${j.schema_version} > supported ${SCHEMA_SUPPORTED}`);
  }

  const fields = {};

  // Bands -> band_0..band_N
  if (Array.isArray(j.bands)) {
    j.bands.forEach((val, idx) => {
      fields[`band_${idx}`] = Number(val);
    });
  }

  // Core numeric metadata
  fields.rms = Number(j.rms || 0);
  fields.peak_hz = Number(j.peak_hz || 0);
  fields.fs = Number(j.fs || 0);
  fields.fft_size = Number(j.n || 0);
  fields.max_analysis_hz = Number(j.max_analysis_hz || 0);
  fields.bin_hz = Number(j.bin_hz || 0);

  // Timing metadata (store monotonic + epoch if available)
  fields.ts_ms = Number(j.ts_ms || 0);
  if (j.epoch_ms) fields.epoch_ms = Number(j.epoch_ms);
  fields.win_ms = Number(j.win_ms || 0);
  fields.hop_ms = Number(j.hop_ms || 0);
  fields.seq = Number(j.seq || 0);

  // Optional CPU load passed separately (so we don't need a second flow)
  // Prefer cached CPU if available
  const cpuCached = flow.get('cpu_load_latest');
  if (typeof cpuCached !== 'undefined') {
    fields.cpu_load = Number(cpuCached);
  } else if (typeof msg.cpu_load !== 'undefined') {
    fields.cpu_load = Number(msg.cpu_load);
  }

  return fields;
}

// MAIN EXECUTION
const fields = parseSpectrum(msg.payload);
if (!fields) return null;

// Tags for filtering/grouping in Grafana queries
const tags = {
  sensor: SENSOR_TAG,
  location: LOCATION_TAG
};

// Influx line protocol via array payload (for node-red-contrib-influxdb)
msg.payload = [{
  measurement: MEASUREMENT,
  fields,
  tags
}];

return msg;