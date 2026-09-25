// ST7735 rendering. Knows nothing about game rules, only what it is told to draw.
#pragma once

#include <stdint.h>

void displayBegin();
int  displayWidth();
int  displayHeight();

void displayMessage(int x, int y, const char *text, uint16_t color);

void displayScene(int holeX, int holeY, int score, int ballX, int ballY);
void displayRedraw();

// r = 0 hides the ball.
void displayMoveBall(int x, int y, int r);
void displaySetHoleRim(uint16_t color);
