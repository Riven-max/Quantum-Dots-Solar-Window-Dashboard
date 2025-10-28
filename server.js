const express = require("express");
const bodyParser = require("body-parser");
const cors = require("cors");
const fetch = require("node-fetch");

const app = express();
const PORT = 3000;
app.use(cors());
app.use(bodyParser.json());
app.use(express.static("public"));

let solarData = [];
let clients = [];

// ✅ Replace with your ESP32's local IP
const ESP32_IP = "http://localhost:3000";

// ==============================
// LIVE STREAM (SSE) CONNECTION
// ==============================
app.get("/api/stream", (req, res) => {
  res.setHeader("Content-Type", "text/event-stream");
  res.setHeader("Cache-Control", "no-cache");
  res.setHeader("Connection", "keep-alive");
  res.flushHeaders();
  clients.push(res);

  req.on("close", () => {
    clients = clients.filter(c => c !== res);
  });
});

function sendEvent(data) {
  clients.forEach(res => res.write(`data: ${JSON.stringify(data)}\n\n`));
}

// ==============================
// DATA HANDLING
// ==============================
app.post("/api/data/latest", (req, res) => {
  const entry = { ...req.body, timestamp: new Date() };
  solarData.push(entry);
  sendEvent({ device: "sensor", data: entry });
  res.json({ message: "Data received", entry });
});

app.get("/api/data/latest", (req, res) => {
  if (solarData.length === 0) return res.json({});
  res.json(solarData[solarData.length - 1]);
});

// ==============================
// BULB CONTROL (Unified)
// ==============================
app.post("/api/bulb", async (req, res) => {
  const { state } = req.body;
  try {
    const response = await fetch(`${ESP32_IP}/setBulb?state=${state}`, { timeout: 3000 });
    if (response.ok) {
      sendEvent({ device: "bulb", state });
      res.json({ success: true });
    } else {
      console.warn("ESP32 responded with error status");
      sendEvent({ device: "bulb", state, error: true });
      res.status(500).json({ success: false });
    }
  } catch (err) {
    console.error("ESP32 not reachable:", err);
    sendEvent({ device: "bulb", state, error: true });
    res.status(500).json({ success: false, message: "ESP32 unreachable" });
  }
});

app.post("/api/brightness", async (req, res) => {
  const { brightness } = req.body;
  try {
    const response = await fetch(`${ESP32_IP}/setBrightness?value=${brightness}`, { timeout: 3000 });
    if (response.ok) {
      sendEvent({ device: "bulb", brightness });
      res.json({ success: true });
    } else {
      console.warn("ESP32 responded with error status");
      sendEvent({ device: "bulb", brightness, error: true });
      res.status(500).json({ success: false });
    }
  } catch (err) {
    console.error("ESP32 not reachable:", err);
    sendEvent({ device: "bulb", brightness, error: true });
    res.status(500).json({ success: false, message: "ESP32 unreachable" });
  }
});


app.listen(PORT, () => console.log(`✅ Server running on http://localhost:${PORT}`));
