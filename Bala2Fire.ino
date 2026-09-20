#define M5STACK_MPU6886
#include <M5Stack.h>
#include "freertos/FreeRTOS.h"
#include "imu_filter.h"
#include "MadgwickAHRS.h"
#include "bala.h"
#include "pid.h"
#include "calibration.h"
#include <WiFi.h>
#include <WebServer.h>
#include "esp_system.h"


extern uint8_t bala_img[41056];
static void PIDTask(void *arg);
static void draw_waveform();

static float angle_point = -1.5;

float kp = 24.0f, ki = 0.0f, kd = 90.0f;
float s_kp = 15.0f, s_ki = 0.075f, s_kd = 0.0f;

bool calibration_mode = false;

Bala bala;

PID pid(angle_point, kp, ki, kd);
PID speed_pid(0, s_kp, s_ki, s_kd);


static const char *AP_SSID = "Bala2Fire";
static const char *AP_PASS = "bala2fire";   

#define MAX_SPEED_CMD   12.0f     
#define MAX_TURN_CMD    250.0f    
#define CMD_TIMEOUT_MS  300      

WebServer server(80);

volatile float    g_cmd_speed   = 0;  
volatile float    g_cmd_turn    = 0;   
volatile uint32_t g_last_cmd_ms = 0;

static const char CONTROL_PAGE[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
<title>Bala2Fire Control</title>
<style>
  html,body{margin:0;height:100%;background:#111;color:#eee;font-family:sans-serif;
    display:flex;flex-direction:column;align-items:center;justify-content:center;
    touch-action:none;user-select:none;}
  h1{font-size:18px;font-weight:normal;color:#8fd;margin:10px 0;}
  #pad{width:240px;height:240px;border-radius:50%;background:#222;
    border:2px solid #444;position:relative;margin:10px;}
  #stick{width:70px;height:70px;border-radius:50%;background:#4af;
    position:absolute;left:85px;top:85px;box-shadow:0 0 12px #4af8;}
  .row{display:flex;gap:10px;margin:8px 0;}
  .btn{width:64px;height:64px;border-radius:12px;background:#333;color:#fff;
    font-size:22px;display:flex;align-items:center;justify-content:center;
    border:1px solid #555;}
  .btn:active{background:#4af;}
  #status{font-size:12px;color:#888;margin-top:6px;}
</style>
</head>
<body>
<h1>Bala2Fire — drag joystick or hold buttons</h1>

<div id="pad"><div id="stick"></div></div>

<div class="row">
  <div style="width:64px"></div>
  <div class="btn" id="fwd">&#9650;</div>
  <div style="width:64px"></div>
</div>
<div class="row">
  <div class="btn" id="left">&#9664;</div>
  <div class="btn" id="stop">&#9679;</div>
  <div class="btn" id="right">&#9654;</div>
</div>
<div class="row">
  <div style="width:64px"></div>
  <div class="btn" id="back">&#9660;</div>
  <div style="width:64px"></div>
</div>

<div id="status">not sending</div>

<script>
let sendX = 0, sendY = 0; // -100..100
const statusEl = document.getElementById('status');

function sendCmd(x, y){
  sendX = Math.max(-100, Math.min(100, x));
  sendY = Math.max(-100, Math.min(100, y));
}

let lastSentZero = true;
setInterval(() => {
  const isZero = (sendX === 0 && sendY === 0);
  if (isZero && lastSentZero) return; 
  fetch(`/cmd?x=${sendX|0}&y=${sendY|0}`).catch(()=>{});
  statusEl.textContent = isZero ? 'stopped' : `x=${sendX|0} y=${sendY|0}`;
  lastSentZero = isZero;
}, 66);


const pad = document.getElementById('pad');
const stick = document.getElementById('stick');
const PAD_R = 120, STICK_R = 35;
let dragging = false;

function padCenterFromEvent(e){
  const r = pad.getBoundingClientRect();
  const t = e.touches ? e.touches[0] : e;
  return { x: t.clientX - (r.left + PAD_R), y: t.clientY - (r.top + PAD_R) };
}
function updateStick(dx, dy){
  const dist = Math.min(PAD_R - STICK_R, Math.hypot(dx, dy));
  const ang = Math.atan2(dy, dx);
  const sx = Math.cos(ang) * dist, sy = Math.sin(ang) * dist;
  stick.style.left = (PAD_R - STICK_R + sx) + 'px';
  stick.style.top  = (PAD_R - STICK_R + sy) + 'px';
  
  const nx = (sx / (PAD_R - STICK_R)) * 100;
  const ny = (-sy / (PAD_R - STICK_R)) * 100;
  sendCmd(nx, ny);
}
function resetStick(){
  stick.style.left = '85px'; stick.style.top = '85px';
  sendCmd(0, 0);
}
function start(e){ dragging = true; move(e); e.preventDefault(); }
function move(e){
  if (!dragging) return;
  const p = padCenterFromEvent(e);
  updateStick(p.x, p.y);
  e.preventDefault();
}
function end(e){ dragging = false; resetStick(); e.preventDefault(); }

pad.addEventListener('mousedown', start);
window.addEventListener('mousemove', move);
window.addEventListener('mouseup', end);
pad.addEventListener('touchstart', start, {passive:false});
pad.addEventListener('touchmove', move, {passive:false});
pad.addEventListener('touchend', end, {passive:false});


function bindHold(id, x, y){
  const el = document.getElementById(id);
  const on  = (e) => { sendCmd(x, y); e.preventDefault(); };
  const off = (e) => { sendCmd(0, 0); e.preventDefault(); };
  el.addEventListener('mousedown', on);
  el.addEventListener('mouseup', off);
  el.addEventListener('mouseleave', off);
  el.addEventListener('touchstart', on, {passive:false});
  el.addEventListener('touchend', off, {passive:false});
}
bindHold('fwd',   0,  70);
bindHold('back',  0, -70);
bindHold('left', -70,  0);
bindHold('right', 70,  0);
document.getElementById('stop').addEventListener('click', ()=>sendCmd(0,0));
</script>
</body>
</html>
)HTML";

static void handleRoot() {
  server.send_P(200, "text/html", CONTROL_PAGE);
}

static void handleCmd() {
  
  if (server.hasArg("x") && server.hasArg("y")) {
    int x = server.arg("x").toInt();
    int y = server.arg("y").toInt();
    x = constrain(x, -100, 100);
    y = constrain(y, -100, 100);
    g_cmd_speed   = (y / 100.0f) * MAX_SPEED_CMD;
    g_cmd_turn    = (x / 100.0f) * MAX_TURN_CMD;
    g_last_cmd_ms = millis();
  }
  server.send(200, "text/plain", "ok");
}

void setup(){
  
  M5.begin(true, false, false, false);
  Serial.begin(115200);
  M5.IMU.Init();

  int16_t x_offset, y_offset, z_offset;
  float angle_center;
  calibrationInit();

  if (M5.BtnB.isPressed()) {
    calibrationGryo();
    calibration_mode = true;
  }

  if (M5.BtnC.isPressed()) {
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Charge mode");
    while (1) {
      if (M5.Power.isCharging()) {
        M5.Lcd.println("Start charging...");
        while(1) {
          if (M5.Power.isChargeFull())
            M5.Lcd.println("Charge completed!");
          delay(5000);
        }
      }
      delay(500);
    }
  }

  calibrationGet(&x_offset, &y_offset, &z_offset, &angle_center);
  Serial.printf("x: %d, y: %d, z:%d, angle: %.2f", x_offset, y_offset, z_offset, angle_center);
  angle_point = angle_center;
  pid.SetPoint(angle_point);

  SemaphoreHandle_t i2c_mutex;;
  i2c_mutex = xSemaphoreCreateMutex();
  bala.SetMutex(&i2c_mutex);

  ImuTaskStart(x_offset, y_offset, z_offset, &i2c_mutex);
  xTaskCreatePinnedToCore(PIDTask, "pid_task", 8 * 1024, NULL, 4, NULL, 1);

  M5.Lcd.drawJpg(bala_img, 41056);

  if (calibration_mode) {
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("calibration mode");
  }


  Serial.print("Reset reason: ");
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   Serial.println("Power-on reset"); break;
    case ESP_RST_BROWNOUT:  Serial.println("BROWNOUT RESET (power sag!)"); break;
    case ESP_RST_PANIC:     Serial.println("Software panic / crash"); break;
    case ESP_RST_INT_WDT:   Serial.println("Interrupt watchdog timeout"); break;
    case ESP_RST_TASK_WDT:  Serial.println("Task watchdog timeout"); break;
    case ESP_RST_WDT:       Serial.println("Other watchdog timeout"); break;
    case ESP_RST_SW:        Serial.println("Software reset (esp_restart)"); break;
    case ESP_RST_DEEPSLEEP: Serial.println("Woke from deep sleep"); break;
    case ESP_RST_EXT:       Serial.println("External pin reset"); break;
    default:                Serial.println("Unknown"); break;
  }

  WiFi.mode(WIFI_AP);

  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("AP started. SSID: %s  IP: %s\n", AP_SSID, ip.toString().c_str());

  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);
  server.begin();
  g_last_cmd_ms = millis(); 
}

void loop() {
  static uint32_t next_show_time = 0;
  
  vTaskDelay(pdMS_TO_TICKS(5));

  server.handleClient();

  if(millis() > next_show_time) {
    draw_waveform();
    next_show_time = millis() + 10;
  }

  M5.update();

  if (M5.BtnA.wasPressed()) {
    angle_point += 0.25;
    pid.SetPoint(angle_point);
  }

  if (M5.BtnB.wasPressed()) {
    if (calibration_mode) {
      calibrationSaveCenterAngle(angle_point);
    }
  }

  if (M5.BtnC.wasPressed()) {
    angle_point -= 0.25;
    pid.SetPoint(angle_point);
  }
}

static void PIDTask(void *arg) {
  float bala_angle;
  float motor_speed = 0;
  int16_t pwm_speed;
  int16_t pwm_output;
  int16_t pwm_angle;
  int32_t encoder = 0;
  int32_t last_encoder = 0;
  uint32_t last_ticks = 0;

  pid.SetOutputLimits(1023, -1023);
  pid.SetDirection(-1);
  speed_pid.SetIntegralLimits(40, -40);
  speed_pid.SetOutputLimits(1023, -1023);
  speed_pid.SetDirection(1);

  for(;;) {
    vTaskDelayUntil(&last_ticks, pdMS_TO_TICKS(5));

    bala_angle = getAngle();

    bala.UpdateEncoder();
    encoder = bala.wheel_left_encoder + bala.wheel_right_encoder;

    motor_speed = 0.8 * motor_speed + 0.2 * (encoder - last_encoder);
    last_encoder = encoder;

    float speed_target = 0.0f;
    float turn_cmd = 0.0f;
    if (millis() - g_last_cmd_ms < CMD_TIMEOUT_MS) {
      speed_target = g_cmd_speed;
      turn_cmd = g_cmd_turn;
    }
  

    if(fabs(bala_angle) < 70) {
      pwm_angle = (int16_t)pid.Update(bala_angle);
      pwm_speed = (int16_t)speed_pid.Update(motor_speed - speed_target);
      pwm_output = pwm_speed + pwm_angle;
      if(pwm_output > 1023) { pwm_output = 1023; }
      if(pwm_output < -1023) { pwm_output = -1023; }

      
      int16_t pwm_left  = constrain((int32_t)pwm_output - (int32_t)turn_cmd, -1023, 1023);
      int16_t pwm_right = constrain((int32_t)pwm_output + (int32_t)turn_cmd, -1023, 1023);
      bala.SetSpeed(pwm_left, pwm_right);
      
    } else {
      pwm_angle = 0;
      bala.SetSpeed(0, 0);
      bala.SetEncoder(0, 0);
      speed_pid.SetIntegral(0);
    }
  }
}

static void draw_waveform() {
  #define MAX_LEN 120
  #define X_OFFSET 100
  #define Y_OFFSET 95
  #define X_SCALE 3
  static int16_t val_buf[MAX_LEN] = {0};
  static int16_t pt = MAX_LEN - 1;
  val_buf[pt] = constrain((int16_t)(getAngle() * X_SCALE), -50, 50);
  if (--pt < 0) {
    pt = MAX_LEN - 1;
  }
  for (int i = 1; i < (MAX_LEN); i++) {
    uint16_t now_pt = (pt + i) % (MAX_LEN);
    M5.Lcd.drawLine(i + X_OFFSET, val_buf[(now_pt + 1) % MAX_LEN] + Y_OFFSET, i + 1 + X_OFFSET, val_buf[(now_pt + 2) % MAX_LEN] + Y_OFFSET, TFT_BLACK);
    if (i < MAX_LEN - 1) {
      M5.Lcd.drawLine(i + X_OFFSET, val_buf[now_pt] + Y_OFFSET, i + 1 + X_OFFSET, val_buf[(now_pt + 1) % MAX_LEN] + Y_OFFSET, TFT_GREEN);
    }
  }
}
