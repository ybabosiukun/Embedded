#include "game.h"

#include <Arduino.h>

#include "config.h"
#include "display.h"

enum class Phase { Rolling, Sinking, Celebrating };

static int W, H;

static float bx, by;          // px
static float vx = 0, vy = 0;  // px/s

static int holeX, holeY;
static int score = 0;

static Phase phase = Phase::Rolling;
static unsigned long phaseT;  // ms
static int sinkR;

static void placeHole() {
  int margin = HOLE_R + 3;
  for (int tries = 0; tries < 100; tries++) {
    holeX = random(margin, W - margin);
    holeY = random(margin + SCORE_H, H - margin);
    int dx = holeX - W / 2, dy = holeY - H / 2;
    if (dx * dx + dy * dy >= MIN_HOLE_DIST * MIN_HOLE_DIST) return;
  }
}

static void newRound() {
  bx = W / 2.0f;
  by = H / 2.0f;
  vx = vy = 0;
  placeHole();
  phase = Phase::Rolling;
  displayScene(holeX, holeY, score, (int)round(bx), (int)round(by));
}

static void tiltAcceleration(float tx, float ty, float &accX, float &accY) {
  static const float deadzone = sinf(DEADZONE * DEG_TO_RAD);
  if (fabsf(tx) < deadzone) tx = 0;
  if (fabsf(ty) < deadzone) ty = 0;

  float sx = -G_PX * tx;
  float sy = -G_PX * ty;

  accX = SWAP_AXES ? sy : sx;
  accY = SWAP_AXES ? sx : sy;
  if (INVERT_X) accX = -accX;
  if (INVERT_Y) accY = -accY;
}

// Funnel: pull toward the center, strongest at the center, zero at HOLE_PULL_R.
static void holePull(float &accX, float &accY) {
  float dx = holeX - bx, dy = holeY - by;
  float d = sqrtf(dx * dx + dy * dy);
  if (d >= HOLE_PULL_R || d < 0.01f) return;
  float a = HOLE_PULL * (1 - d / HOLE_PULL_R);
  accX += a * dx / d;
  accY += a * dy / d;
}

static void moveBall(float tx, float ty, float dt) {
  float accX, accY;
  tiltAcceleration(tx, ty, accX, accY);
  holePull(accX, accY);

  vx += accX * dt;
  vy += accY * dt;
  vx -= vx * DAMPING * dt;
  vy -= vy * DAMPING * dt;
  bx += vx * dt;
  by += vy * dt;

  if (bx < BALL_R)          { bx = BALL_R;          vx = -vx * BOUNCE; }
  if (bx > W - 1 - BALL_R)  { bx = W - 1 - BALL_R;  vx = -vx * BOUNCE; }
  if (by < BALL_R)          { by = BALL_R;          vy = -vy * BOUNCE; }
  if (by > H - 1 - BALL_R)  { by = H - 1 - BALL_R;  vy = -vy * BOUNCE; }
}

static bool inHole() {
  float dx = bx - holeX, dy = by - holeY;
  float speed = sqrtf(vx * vx + vy * vy);
  return dx * dx + dy * dy < (float)(HOLE_R * HOLE_R) && speed < CAPTURE_SPEED;
}

void gameBegin(int width, int height) {
  W = width;
  H = height;
  newRound();
}

void gameStep(float tiltX, float tiltY, float dt) {
  unsigned long now = millis();

  switch (phase) {
    case Phase::Rolling:
      moveBall(tiltX, tiltY, dt);
      displayMoveBall((int)round(bx), (int)round(by), BALL_R);
      if (inHole()) {
        score++;
        sinkR = BALL_R;
        displayMoveBall(holeX, holeY, sinkR);
        phase = Phase::Sinking;
        phaseT = now;
      }
      break;

    case Phase::Sinking:
      if (now - phaseT < SINK_STEP_MS) break;
      phaseT = now;
      if (sinkR > 0) {
        sinkR--;
        displayMoveBall(holeX, holeY, sinkR);
      } else {
        displaySetHoleRim(HOLE_WIN_RIM);
        phase = Phase::Celebrating;
      }
      break;

    case Phase::Celebrating:
      if (now - phaseT >= CELEBRATE_MS) newRound();
      break;
  }
}
