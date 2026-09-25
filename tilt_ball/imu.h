// MPU6050 tilt sensing.
#pragma once

bool imuBegin();

// Keeps calibration; the filter restarts from the current orientation.
bool imuReconnect();

// Board must lie still. Always applies the result; returns false if the board moved.
bool imuCalibrate();

// Tilt relative to the calibrated level: gravity components along sensor X/Y,
// i.e. the sine of the tilt, -1..1.
bool imuUpdate(float dt, float &tiltX, float &tiltY);
