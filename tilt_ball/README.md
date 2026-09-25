# Tilt Ball

Tilt the board to roll a ball into a hole. Each hit scores a point, the ball returns to the center and the hole moves to a new random spot. A slow ball near the rim gets pulled in; a fast one rolls over.

## Hardware

- ESP32-S3
- MPU6050 accelerometer/gyroscope
- ST7735 1.8" TFT, 128x160

| Module  | Pins                                          |
| ------- | --------------------------------------------- |
| Display | MOSI 11, SCK 12, CS 10, DC 9, RES 8, BL/VCC 3.3V |
| MPU6050 | SDA 39, SCL 40, VCC 3.3V                      |

Libraries: Adafruit GFX, Adafruit ST7735 and ST7789.

## Playing

1. Power up and hold the board level for about a second while it calibrates. That pose becomes "level". If the board moves, it asks you to hold still and retries.
2. Tilt to roll the ball into the hole. The score is in the top-left corner.
3. If the sensor disconnects, the game pauses with a message and resumes once it is back.

## Code

| File            | Role                                             |
| --------------- | ------------------------------------------------ |
| `config.h`      | Pins and all tuning values                       |
| `imu.*`         | MPU6050: calibration and gravity-vector filter   |
| `display.*`     | Flicker-free rendering                           |
| `game.*`        | Ball physics, hole funnel, score, sink animation |
| `tilt_ball.ino` | Setup, main loop, sensor-loss handling           |

If the ball rolls the wrong way, flip `SWAP_AXES`, `INVERT_X` or `INVERT_Y` in `config.h`.
