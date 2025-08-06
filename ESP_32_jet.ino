#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

Servo esc;
Servo servo1;
Servo servo2;

WebServer server(80);

int throttle = 1000;
int servo1_pos = 90;
int servo2_pos = 90;

// Access Point credentials
const char* ssid = "ESP32-Jet";
const char* password = "12345678";

// Web UI with slider and joystick
String HTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Jet Control</title>
  <style>
    body { font-family: sans-serif; text-align: center; }
    input[type=range] { width: 80%; }
    #joystickArea { width: 200px; height: 200px; background: #eee; margin: auto; position: relative; border-radius: 10px; touch-action: none; }
    #stick { width: 40px; height: 40px; background: #333; border-radius: 50%; position: absolute; left: 80px; top: 80px; touch-action: none; }
  </style>
</head>
<body>
<h2>ESP32 RC Jet Controller</h2>

Throttle: <input type="range" min="1000" max="2000" value="1000" id="throttleSlider" oninput="updateThrottle(this.value)">
<p id="tval">1000</p>

<h3>Servo Joystick</h3>
<div id="joystickArea">
  <div id="stick"></div>
</div>

<script>
function updateThrottle(val) {
  document.getElementById('tval').innerText = val;
  fetch("/throttle?val=" + val);
}

// Joystick handler
let stick = document.getElementById('stick');
let area = document.getElementById('joystickArea');
let dragging = false;

area.addEventListener('touchstart', e => { dragging = true; }, false);
area.addEventListener('touchend', e => { dragging = false; resetStick(); }, false);
area.addEventListener('touchmove', e => {
  if (!dragging) return;
  e.preventDefault();
  let rect = area.getBoundingClientRect();
  let touch = e.touches[0];
  let x = touch.clientX - rect.left;
  let y = touch.clientY - rect.top;
  x = Math.max(0, Math.min(x, rect.width));
  y = Math.max(0, Math.min(y, rect.height));
  stick.style.left = (x - 20) + "px";
  stick.style.top = (y - 20) + "px";

  let sx = Math.round((x / rect.width) * 180);
  let sy = Math.round((1 - y / rect.height) * 180);
  fetch(`/joystick?sx=${sx}&sy=${sy}`);
}, false);

function resetStick() {
  stick.style.left = "80px";
  stick.style.top = "80px";
}
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", HTML);
}

void handleThrottle() {
  if (server.hasArg("val")) {
    throttle = server.arg("val").toInt();
    throttle = constrain(throttle, 1000, 2000);
    esc.writeMicroseconds(throttle);
    server.send(200, "text/plain", "Throttle set");
  } else {
    server.send(400, "text/plain", "Missing throttle value");
  }
}

void handleJoystick() {
  if (server.hasArg("sx")) {
    servo1_pos = constrain(server.arg("sx").toInt(), 0, 180);
    servo1.write(servo1_pos);
  }
  if (server.hasArg("sy")) {
    servo2_pos = constrain(server.arg("sy").toInt(), 0, 180);
    servo2.write(servo2_pos);
  }
  server.send(200, "text/plain", "Joystick updated");
}

void setup() {
  Serial.begin(115200);
  esc.attach(21);
  servo1.attach(16);
  servo2.attach(17);

  esc.writeMicroseconds(throttle);
  servo1.write(servo1_pos);
  servo2.write(servo2_pos);

  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/throttle", handleThrottle);
  server.on("/joystick", handleJoystick);
  server.begin();
}

void loop() {
  server.handleClient();
}