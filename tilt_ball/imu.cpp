#include "imu.h"

#include <Arduino.h>
#include <Wire.h>

#include "config.h"

constexpr uint8_t MPU_ADDR         = 0x68;
constexpr uint8_t REG_CONFIG       = 0x1A;
constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
constexpr uint8_t REG_PWR_MGMT_1   = 0x6B;

constexpr float ACC_LSB_PER_G    = 16384.0f;  // +-2 g range
constexpr float GYRO_LSB_PER_DPS = 131.0f;    // +-250 deg/s range

struct Vec3 {
  float x, y, z;
};

static Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static Vec3 operator*(Vec3 a, float k) { return {a.x * k, a.y * k, a.z * k}; }
static float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static Vec3 normalized(Vec3 a) {
  float len = sqrtf(dot(a, a));
  return len > 1e-6f ? a * (1.0f / len) : a;
}

struct Sample {
  Vec3 acc;   // g
  Vec3 gyro;  // deg/s
};

static Vec3 gravity  = {0, 0, 1};  // filtered, unit, sensor frame
static Vec3 gyroBias = {0, 0, 0};
static Vec3 zeroRows[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};  // rotates level gravity to +Z
static bool seedFilter = true;

static bool writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static int16_t read16() {
  // Two statements: evaluation order inside one expression is unspecified.
  uint8_t hi = Wire.read();
  uint8_t lo = Wire.read();
  return (int16_t)((hi << 8) | lo);
}

static bool readMPU(Sample &s) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(REG_ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)14) != 14) return false;

  s.acc.x = read16() / ACC_LSB_PER_G;
  s.acc.y = read16() / ACC_LSB_PER_G;
  s.acc.z = read16() / ACC_LSB_PER_G;
  read16();  // temperature
  s.gyro.x = read16() / GYRO_LSB_PER_DPS;
  s.gyro.y = read16() / GYRO_LSB_PER_DPS;
  s.gyro.z = read16() / GYRO_LSB_PER_DPS;
  return true;
}

// The sensor forgets its settings on power loss.
static bool wakeAndConfigure() {
  if (!writeReg(REG_PWR_MGMT_1, 0)) return false;
  if (!writeReg(REG_CONFIG, MPU_DLPF)) return false;
  delay(100);
  seedFilter = true;
  return true;
}

// Rodrigues rotation taking `level` to +Z.
static void setZero(Vec3 level) {
  const Vec3 a = normalized(level);
  const Vec3 v = cross(a, {0, 0, 1});
  const float c = a.z;

  if (c < -0.999f) {  // upside down
    zeroRows[0] = {1, 0, 0};
    zeroRows[1] = {0, -1, 0};
    zeroRows[2] = {0, 0, -1};
    return;
  }

  const float k = 1.0f / (1.0f + c);
  const float m[3][3] = {{0, -v.z, v.y}, {v.z, 0, -v.x}, {-v.y, v.x, 0}};
  float r[3][3];
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      float m2 = 0;
      for (int n = 0; n < 3; n++) m2 += m[i][n] * m[n][j];
      r[i][j] = (i == j ? 1.0f : 0.0f) + m[i][j] + k * m2;
    }
  }
  for (int i = 0; i < 3; i++) zeroRows[i] = {r[i][0], r[i][1], r[i][2]};
}

bool imuBegin() {
  Wire.begin(MPU_SDA, MPU_SCL);
  Wire.setClock(400000);
  return wakeAndConfigure();
}

bool imuReconnect() {
  return wakeAndConfigure();
}

bool imuCalibrate() {
  constexpr int N = 200;
  Vec3 sumAcc = {0, 0, 0}, sumGyro = {0, 0, 0}, sumGyroSq = {0, 0, 0};
  int ok = 0;
  for (int i = 0; i < N; i++) {
    Sample s;
    if (readMPU(s)) {
      sumAcc = sumAcc + s.acc;
      sumGyro = sumGyro + s.gyro;
      sumGyroSq = sumGyroSq + Vec3{s.gyro.x * s.gyro.x, s.gyro.y * s.gyro.y, s.gyro.z * s.gyro.z};
      ok++;
    }
    delay(5);
  }
  if (ok < N / 2) return false;

  const Vec3 mean = sumGyro * (1.0f / ok);
  const Vec3 var = sumGyroSq * (1.0f / ok) - Vec3{mean.x * mean.x, mean.y * mean.y, mean.z * mean.z};

  gyroBias = mean;
  setZero(sumAcc);
  gravity = normalized(sumAcc);
  seedFilter = false;

  return sqrtf(max(var.x + var.y + var.z, 0.0f)) <= CALIB_MAX_GYRO_STD;
}

bool imuUpdate(float dt, float &tiltX, float &tiltY) {
  Sample s;
  if (!readMPU(s)) return false;
  const Vec3 acc = normalized(s.acc);

  if (seedFilter) {
    gravity = acc;
    seedFilter = false;
  } else {
    // Complementary filter on the gravity vector. Gravity is fixed in the world,
    // so in the sensor frame it turns opposite to the board: dg = -w x g * dt.
    const Vec3 w = (s.gyro - gyroBias) * DEG_TO_RAD;
    const Vec3 predicted = gravity - cross(w, gravity) * dt;
    const float alpha = FILTER_TAU / (FILTER_TAU + dt);
    gravity = normalized(predicted * alpha + acc * (1.0f - alpha));
  }

  tiltX = dot(zeroRows[0], gravity);
  tiltY = dot(zeroRows[1], gravity);
  return true;
}
