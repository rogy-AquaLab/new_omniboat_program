#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

// ==== WiFi設定 ====
const char* ssid = "ESP32_RC";
const char* password = "12345678";

// ==== モーターピン ====
const int M1_IN1 = 19;
const int M1_IN2 = 18;
const int M2_IN1 = 17;
const int M2_IN2 = 16;

// ==== 電圧監視ピン ====
const int V_ESP = 33;
const int V_Motor = 32;

// ==== LEDピン ====
const int LED_Program = 26;
const int LED_Wifi = 5;

// ==== PWM設定 ====
const int pwmFreq = 20000;
const int pwmResolution = 8;
const int M1_CH1 = 0;
const int M1_CH2 = 1;
const int M2_CH1 = 2;
const int M2_CH2 = 3;

// ==== 加速設定 ====
const int maxSpeed = 255;
const int turnSpeed = 0;
const int accelStep = 5;
const int accelInterval = 5;

int m1_target = 0;
int m2_target = 0;
int m1_current = 0;
int m2_current = 0;

unsigned long lastUpdate = 0;

// ===== 台形加速処理 =====
void updateMotor() {

  if (millis() - lastUpdate < accelInterval) return;
  lastUpdate = millis();

  // M1
  if (m1_current < m1_target) {
    m1_current += accelStep;
    if (m1_current > m1_target) m1_current = m1_target;
  }
  else if (m1_current > m1_target) {
    m1_current -= accelStep;
    if (m1_current < m1_target) m1_current = m1_target;
  }

  // M2
  if (m2_current < m2_target) {
    m2_current += accelStep;
    if (m2_current > m2_target) m2_current = m2_target;
  }
  else if (m2_current > m2_target) {
    m2_current -= accelStep;
    if (m2_current < m2_target) m2_current = m2_target;
  }

  int m1_pwm = constrain(abs(m1_current), 0, 255);
  int m2_pwm = constrain(abs(m2_current), 0, 255);

  // 出力
  if (m1_current >= 0) {
    ledcWrite(M1_CH1, m1_pwm);
    ledcWrite(M1_CH2, 0);
  } else {
    ledcWrite(M1_CH1, 0);
    ledcWrite(M1_CH2, m1_pwm);
  }

  if (m2_current >= 0) {
    ledcWrite(M2_CH1, m2_pwm);
    ledcWrite(M2_CH2, 0);
  } else {
    ledcWrite(M2_CH1, 0);
    ledcWrite(M2_CH2, m2_pwm);
  }
}

// ===== 動作関数 =====
void stopAll() {
  m1_target = 0;
  m2_target = 0;
  Serial.println("Stop");
}

void forward() {
  m1_target = maxSpeed;
  m2_target = maxSpeed;
  Serial.println("Forward");
}

void backward() {
  m1_target = -maxSpeed;
  m2_target = -maxSpeed;
  Serial.println("Backward");
}

void rightTurn() {
  m1_target = maxSpeed;
  m2_target = turnSpeed;
  Serial.println("Right Turn");
}

void leftTurn() {
  m1_target = turnSpeed;
  m2_target = maxSpeed;
  Serial.println("Left Turn");
}

void spinRight() {
  m1_target = maxSpeed;
  m2_target = -maxSpeed;
  Serial.println("Spin Right");
}

void spinLeft() {
  m1_target = -maxSpeed;
  m2_target = maxSpeed;
  Serial.println("Spin Left");
}

// ===== Web UI =====
void handleRoot() {

  digitalWrite(LED_Wifi, HIGH);
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "</head><body>";
  html += "<h2>ESP32 WiFi RC</h2>";

  html += "<p>";
  html += "<a href='/forward'><button style='width:100px;height:50px;'>前進</button></a><br><br>";
  html += "<a href='/left'><button style='width:100px;height:50px;'>左折</button></a>";
  html += "<a href='/stop'><button style='width:100px;height:50px;'>停止</button></a>";
  html += "<a href='/right'><button style='width:100px;height:50px;'>右折</button></a><br><br>";
  html += "<a href='/back'><button style='width:100px;height:50px;'>後進</button></a><br><br>";
  html += "<a href='/spinL'><button style='width:120px;height:50px;'>その場左回転</button></a><br><br>";
  html += "<a href='/spinR'><button style='width:120px;height:50px;'>その場右回転</button></a>";
  html += "</p>";

  html += "</body></html>";

  server.send(200, "text/html; charset=UTF-8", html);
  digitalWrite(LED_Wifi, LOW);
}

void setupRoutes() {

  server.on("/", handleRoot);

  server.on("/forward", [](){ forward(); handleRoot(); });
  server.on("/back", [](){ backward(); handleRoot(); });
  server.on("/left", [](){ leftTurn(); handleRoot(); });
  server.on("/right", [](){ rightTurn(); handleRoot(); });
  server.on("/spinL", [](){ spinLeft(); handleRoot(); });
  server.on("/spinR", [](){ spinRight(); handleRoot(); });
  server.on("/stop", [](){ stopAll(); handleRoot(); });
}

void setup() {

  Serial.begin(9600);

  pinMode(V_ESP, INPUT);
  pinMode(V_Motor, INPUT);
  pinMode(LED_Program, OUTPUT);
  pinMode(LED_Wifi, OUTPUT);

  ledcSetup(M1_CH1, pwmFreq, pwmResolution);
  ledcSetup(M1_CH2, pwmFreq, pwmResolution);
  ledcSetup(M2_CH1, pwmFreq, pwmResolution);
  ledcSetup(M2_CH2, pwmFreq, pwmResolution);

  ledcAttachPin(M1_IN1, M1_CH1);
  ledcAttachPin(M1_IN2, M1_CH2);
  ledcAttachPin(M2_IN1, M2_CH1);
  ledcAttachPin(M2_IN2, M2_CH2);

  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());

  setupRoutes();
  server.begin();
}

void loop() {
  server.handleClient();
  updateMotor();
}
