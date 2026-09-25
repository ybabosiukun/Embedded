// Pins and tuning.
#pragma once

#include <Adafruit_ST77xx.h>

// Pins
constexpr int8_t TFT_CS   = 10;
constexpr int8_t TFT_DC   = 9;
constexpr int8_t TFT_RST  = 8;
constexpr int8_t TFT_MOSI = 11;
constexpr int8_t TFT_SCLK = 12;

constexpr int MPU_SDA = 39;
constexpr int MPU_SCL = 40;

// Ball physics
constexpr float G_PX     = 500.0f;  // px/s^2 at 90 deg tilt
constexpr float DAMPING  = 1.2f;    // 1/s
constexpr float BOUNCE   = 0.5f;    // 0 = sticks to walls, 1 = no energy loss
constexpr float DEADZONE = 1.5f;    // deg
constexpr int   BALL_R   = 5;

// Hole
constexpr int   HOLE_R        = 8;
constexpr float CAPTURE_SPEED = 180.0f;  // px/s, faster balls roll over the hole
constexpr int   MIN_HOLE_DIST = 40;      // px from screen center

// Funnel around the hole; HOLE_PULL = 0 disables it
constexpr float HOLE_PULL_R = HOLE_R + BALL_R + 4;
constexpr float HOLE_PULL   = 600.0f;    // px/s^2 at the center

constexpr unsigned long SINK_STEP_MS = 40;
constexpr unsigned long CELEBRATE_MS = 250;

// Colors
constexpr uint16_t BALL_COLOR   = ST77XX_YELLOW;
constexpr uint16_t BG_COLOR     = ST77XX_BLACK;
constexpr uint16_t HOLE_COLOR   = 0x2104;  // dark gray
constexpr uint16_t HOLE_RIM     = ST77XX_WHITE;
constexpr uint16_t HOLE_WIN_RIM = ST77XX_GREEN;
constexpr uint16_t TEXT_COLOR   = ST77XX_WHITE;

constexpr int SCORE_W = 40, SCORE_H = 10;  // top-left corner

// Sensor
constexpr uint8_t MPU_DLPF   = 3;     // 0 = off, 1 = 184 Hz, 3 = 44 Hz, 6 = 5 Hz
constexpr float   FILTER_TAU = 0.4f;  // s, how fast the accelerometer corrects gyro drift

constexpr float CALIB_MAX_GYRO_STD = 2.0f;  // deg/s, above that the board was moving
constexpr int   CALIB_ATTEMPTS     = 5;     // then the last result is used anyway

constexpr unsigned long IMU_LOST_MS  = 300;
constexpr unsigned long IMU_RETRY_MS = 500;

// Sensor-to-screen axes: flip these if the ball goes the wrong way
constexpr bool SWAP_AXES = false;
constexpr bool INVERT_X  = true;
constexpr bool INVERT_Y  = false;
