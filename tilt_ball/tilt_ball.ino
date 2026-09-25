/*
  Tilt ball: ESP32-S3 + MPU6050 + ST7735 1.8" 128x160.
  Libraries: Adafruit GFX, Adafruit ST7735 and ST7789.

  Display: MOSI 11, SCK 12, CS 10, DC 9, RES 8, BL/VCC -> 3.3V
  MPU6050: SDA 39, SCL 40, VCC -> 3.3V

  Hold the board level for ~1 s after power-up: that pose becomes "level".
*/

#include "config.h"
#include "imu.h"
#include "display.h"
#include "game.h"

unsigned long lastT;      // us
unsigned long lastImuOk;  // ms

// Blocks until the sensor answers.
void waitForImu(const char *message) {
  displayMessage(10, 60, message, ST77XX_RED);
  Serial.println("MPU6050 not responding, check wiring");
  while (!imuReconnect()) delay(IMU_RETRY_MS);
  Serial.println("MPU6050 back online");
}

void calibrate() {
  displayMessage(20, 60, "Calibrating...", TEXT_COLOR);
  for (int attempt = 1; !imuCalibrate(); attempt++) {
    if (attempt >= CALIB_ATTEMPTS) {
      Serial.println("Board kept moving, using last calibration");
      return;
    }
    displayMessage(20, 60, "Hold still...", TEXT_COLOR);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  displayBegin();
  if (!imuBegin()) waitForImu("MPU6050 not found");
  calibrate();

  randomSeed(esp_random());
  gameBegin(displayWidth(), displayHeight());

  lastT = micros();
  lastImuOk = millis();
}

void loop() {
  unsigned long now = micros();
  float dt = (now - lastT) / 1e6f;
  if (dt < 0.008f) return;      // ~120 Hz
  lastT = now;
  if (dt > 0.05f) dt = 0.05f;   // no huge step after a stall

  float tiltX, tiltY;
  if (imuUpdate(dt, tiltX, tiltY)) {
    lastImuOk = millis();
    gameStep(tiltX, tiltY, dt);
    return;
  }

  if (millis() - lastImuOk >= IMU_LOST_MS) {
    waitForImu("MPU6050 lost");
    displayRedraw();
    lastImuOk = millis();
    lastT = micros();
  }
}
