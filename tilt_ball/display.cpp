#include "display.h"

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#include "config.h"

static Adafruit_ST7735 tft = Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST);

// Updates are composed off-screen and pushed in one go, so no pixel flickers.
// The patch must fit the ball plus its per-frame travel, and the whole hole.
constexpr int PATCH_HALF = (BALL_R + 8 > HOLE_R + 1) ? BALL_R + 8 : HOLE_R + 1;
constexpr int PATCH = 2 * PATCH_HALF + 1;
static GFXcanvas16 patch(PATCH, PATCH);

// What is currently on screen.
static int holeX, holeY;
static uint16_t holeRim = HOLE_RIM;
static int score;
static int ballX, ballY, ballR;

// Draws the scene onto g, whose top-left corner is screen point (ox, oy).
static void drawScene(Adafruit_GFX &g, int ox, int oy) {
  g.fillScreen(BG_COLOR);
  g.fillCircle(holeX - ox, holeY - oy, HOLE_R, HOLE_COLOR);
  g.drawCircle(holeX - ox, holeY - oy, HOLE_R, holeRim);

  g.setTextWrap(false);
  g.setTextSize(1);
  g.setTextColor(TEXT_COLOR);
  g.setCursor(2 - ox, 1 - oy);
  g.print(score);

  if (ballR > 0) g.fillCircle(ballX - ox, ballY - oy, ballR, BALL_COLOR);
}

static void pushPatch(int x0, int y0) {
  x0 = constrain(x0, 0, tft.width() - PATCH);
  y0 = constrain(y0, 0, tft.height() - PATCH);
  drawScene(patch, x0, y0);
  tft.drawRGBBitmap(x0, y0, patch.getBuffer(), PATCH, PATCH);
}

static void pushPatchAround(int cx, int cy) {
  pushPatch(cx - PATCH_HALF, cy - PATCH_HALF);
}

void displayBegin() {
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setSPISpeed(40000000);  // drop to 27 MHz if artifacts appear
  tft.setRotation(1);         // 160x128 landscape
}

int displayWidth()  { return tft.width(); }
int displayHeight() { return tft.height(); }

void displayMessage(int x, int y, const char *text, uint16_t color) {
  tft.fillScreen(BG_COLOR);
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.setCursor(x, y);
  tft.print(text);
}

void displayScene(int hx, int hy, int s, int bx, int by) {
  holeX = hx;
  holeY = hy;
  holeRim = HOLE_RIM;
  score = s;
  ballX = bx;
  ballY = by;
  ballR = BALL_R;
  displayRedraw();
}

void displayRedraw() {
  drawScene(tft, 0, 0);
}

void displayMoveBall(int nx, int ny, int nr) {
  if (nx == ballX && ny == ballY && nr == ballR) return;

  // Bounding box of the old and the new ball.
  int x0 = min(ballX - ballR, nx - nr), x1 = max(ballX + ballR, nx + nr);
  int y0 = min(ballY - ballR, ny - nr), y1 = max(ballY + ballR, ny + nr);
  int oldX = ballX, oldY = ballY;

  ballX = nx; ballY = ny; ballR = nr;

  int w = x1 - x0 + 1, h = y1 - y0 + 1;
  if (w <= PATCH && h <= PATCH) {
    pushPatch(x0 - (PATCH - w) / 2, y0 - (PATCH - h) / 2);
  } else {
    pushPatchAround(oldX, oldY);
    pushPatchAround(nx, ny);
  }
}

void displaySetHoleRim(uint16_t color) {
  holeRim = color;
  pushPatchAround(holeX, holeY);
}
