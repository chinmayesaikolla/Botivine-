// Mobile Manipulator Control using ESP32

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include <vector>
#include <sstream>

#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1
#define FORWARD 1
#define BACKWARD -1
#define STOP 0

const int PWMFreq = 1000;
const int PWMResolution = 8;
const int PWMRightChannel = 4;
const int PWMLeftChannel = 5;

const char* ssid = "MobileManipulator";
const char* password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

struct MOTOR_PINS {
  int pinEn;
  int pinIN1;
  int pinIN2;
};

std::vector<MOTOR_PINS> motorPins = {
  {22, 16, 17}, // RIGHT MOTOR
  {23, 18, 19}  // LEFT MOTOR
};

Servo servos[6];
int servoPins[6] = {32, 33, 25, 26, 27, 14};
int servoAngles[6] = {90, 90, 90, 90, 90, 90};

const char* htmlPage PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
  body { font-family: Arial; text-align: center; background: #f2f2f2; }
  h2 { color: teal; }
  .slider { width: 80%; }
</style>
</head><body>
<h2>Mobile Manipulator Control</h2>

<h3>Car Movement</h3>
<button ontouchstart='send("MoveCar",1)' ontouchend='send("MoveCar",0)'>Forward</button>
<button ontouchstart='send("MoveCar",2)' ontouchend='send("MoveCar",0)'>Backward</button>
<button ontouchstart='send("MoveCar",3)' ontouchend='send("MoveCar",0)'>Left</button>
<button ontouchstart='send("MoveCar",4)' ontouchend='send("MoveCar",0)'>Right</button>

<h3>Speed Control</h3>
Right Speed: <input type="range" min="0" max="255" value="150" class="slider" id="speedRight" oninput='send("SpeedR",value)'><br>
Left Speed: <input type="range" min="0" max="255" value="150" class="slider" id="speedLeft" oninput='send("SpeedL",value)'><br>

<h3>Manipulator Control</h3>
<script>
  const joints = ["Base","Shoulder","Elbow","Wrist1","Wrist2","Gripper"];
  for (let i = 0; i < joints.length; i++) {
    document.write(joints[i] + ': <input type="range" min="0" max="180" value="90" class="slider" oninput="send(\"S\"+i,this.value)"><br>');
  }
</script>

<script>
let socket;
function initWebSocket() {
  socket = new WebSocket("ws://" + window.location.hostname + "/ws");
  socket.onopen = () => console.log("Connected");
  socket.onclose = () => setTimeout(initWebSocket, 2000);
}
function send(key, val) {
  socket.send(key + "," + val);
}
window.onload = initWebSocket;
</script></body></html>
)rawliteral";

void rotateMotor(int motor, int dir) {
  digitalWrite(motorPins[motor].pinIN1, dir == FORWARD);
  digitalWrite(motorPins[motor].pinIN2, dir == BACKWARD);
  if (dir == STOP) {
    digitalWrite(motorPins[motor].pinIN1, LOW);
    digitalWrite(motorPins[motor].pinIN2, LOW);
  }
}

void moveCar(int val) {
  switch(val) {
    case 1: rotateMotor(RIGHT_MOTOR, FORWARD); rotateMotor(LEFT_MOTOR, FORWARD); break;
    case 2: rotateMotor(RIGHT_MOTOR, BACKWARD); rotateMotor(LEFT_MOTOR, BACKWARD); break;
    case 3: rotateMotor(RIGHT_MOTOR, FORWARD); rotateMotor(LEFT_MOTOR, BACKWARD); break;
    case 4: rotateMotor(RIGHT_MOTOR, BACKWARD); rotateMotor(LEFT_MOTOR, FORWARD); break;
    default: rotateMotor(RIGHT_MOTOR, STOP); rotateMotor(LEFT_MOTOR, STOP); break;
  }
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len) {
      std::string msg((char*)data, len);
      std::istringstream ss(msg);
      std::string key, val;
      getline(ss, key, ','); getline(ss, val);
      int value = atoi(val.c_str());

      if (key == "MoveCar") moveCar(value);
      else if (key == "SpeedR") ledcWrite(PWMRightChannel, value);
      else if (key == "SpeedL") ledcWrite(PWMLeftChannel, value);
      else if (key[0] == 'S') {
        int idx = key[1] - '0';
        if (idx >= 0 && idx < 6) {
          servos[idx].write(value);
          servoAngles[idx] = value;
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  Serial.println("WiFi started, IP: " + WiFi.softAPIP().toString());

  for (int i = 0; i < 6; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(servoAngles[i]);
  }

  for (int i = 0; i < motorPins.size(); i++) {
    pinMode(motorPins[i].pinEn, OUTPUT);
    pinMode(motorPins[i].pinIN1, OUTPUT);
    pinMode(motorPins[i].pinIN2, OUTPUT);
  }

  ledcSetup(PWMRightChannel, PWMFreq, PWMResolution);
  ledcAttachPin(motorPins[RIGHT_MOTOR].pinEn, PWMRightChannel);

  ledcSetup(PWMLeftChannel, PWMFreq, PWMResolution);
  ledcAttachPin(motorPins[LEFT_MOTOR].pinEn, PWMLeftChannel);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", htmlPage);
  });
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
}

void loop() {
  ws.cleanupClients();
}
