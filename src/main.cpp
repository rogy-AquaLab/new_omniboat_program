#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

// ==== WiFi設定 ====
const char *ssid = "ESP32_RC";
const char *password = "12345678";

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

// コントローラからの各種命令
const int ORDER_STOP = 0;
const int ORDER_MOVE_FORWARD = 1;
const int ORDER_MOVE_BACKWARD = 2;
const int ORDER_MOVE_TURN_L = 3;
const int ORDER_MOVE_TURN_R = 4;
const int ORDER_MOVE_SPIN_L = 5;
const int ORDER_MOVE_SPIN_R = 6;

// ==== 加速設定 ====
const int maxSpeed = 255;
const int turnSpeed = 0;
const int accelStep = 5;
const int accelInterval = 5;

// ==== 状態変数 (プログラム内で更新されていく変数) ====
WebServer server(80);         // UIを表示するためのWebサーバーを管理するのに必要
int order = ORDER_STOP;       // コントローラからの直近の命令
int m1_current = 0;           // モータ1の現在の出力値
int m2_current = 0;           // モータ2の現在の出力値
unsigned long lastUpdate = 0; // 最終更新時刻 [ms]

void updateMotor() {
  int m1_target;
  int m2_target;

  // コントローラからの命令(`order`)をもとに目標値(`target`)を設定
  switch (order) {
  case ORDER_STOP:
    m1_target = 0;
    m2_target = 0;
    break;
  case ORDER_MOVE_FORWARD:
    m1_target = maxSpeed;
    m2_target = maxSpeed;
    break;
  case ORDER_MOVE_BACKWARD:
    m1_target = -maxSpeed;
    m2_target = -maxSpeed;
    break;
  case ORDER_MOVE_TURN_L:
    m1_target = turnSpeed;
    m2_target = maxSpeed;
    break;
  case ORDER_MOVE_TURN_R:
    m1_target = maxSpeed;
    m2_target = turnSpeed;
    break;
  case ORDER_MOVE_SPIN_L:
    m1_target = -maxSpeed;
    m2_target = maxSpeed;
    break;
  case ORDER_MOVE_SPIN_R:
    m1_target = maxSpeed;
    m2_target = -maxSpeed;
    break;
  }

  // 現在の出力値(`current`)を目標値(`target`)に近づけるように台形加速を行う。
  // `target`と`current`の差が`accelStep`より大きい => current = current +/- accelStep
  // `target`と`current`の差が`accelStep`より小さい => current = target
  //
  // constrain(x, a, b) ... xを[a, b]の範囲に収める (constrain(-23, 0, 100) = 0)
  // abs(x)             ... 絶対値 (abs(-10) = 10)
  int m1_step = constrain(m1_target - m1_current, -accelStep, accelStep); // Motor 1
  int m2_step = constrain(m2_target - m2_current, -accelStep, accelStep); // Motor 2
  m1_current += m1_step;
  m2_current += m2_step;

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

void onStop() {
  order = ORDER_STOP;
  Serial.println("Stop");
  handleRoot();
}

void onForward() {
  order = ORDER_MOVE_FORWARD;
  Serial.println("Forward");
  handleRoot();
}

void onBackward() {
  order = ORDER_MOVE_BACKWARD;
  Serial.println("Backward");
  handleRoot();
}

void onLeft() {
  order = ORDER_MOVE_TURN_L;
  Serial.println("Left Turn");
  handleRoot();
}

void onRight() {
  order = ORDER_MOVE_TURN_R;
  Serial.println("Right Turn");
  handleRoot();
}

void onSpinL() {
  order = ORDER_MOVE_SPIN_L;
  Serial.println("Spin Left");
  handleRoot();
}

void onSpinR() {
  order = ORDER_MOVE_SPIN_R;
  Serial.println("Spin Right");
  handleRoot();
}

void setupRoutes() {
  server.on("/", handleRoot);

  server.on("/forward", onForward);
  server.on("/back", onBackward);
  server.on("/left", onLeft);
  server.on("/right", onRight);
  server.on("/spinL", onSpinL);
  server.on("/spinR", onSpinR);
  server.on("/stop", onStop);
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

  if (millis() - lastUpdate >= accelInterval) {
    updateMotor();
    lastUpdate = millis();
  }
}
