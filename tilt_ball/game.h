// Ball physics, hole, score.
#pragma once

void gameBegin(int width, int height);

// Tilt as returned by imuUpdate(). Never blocks: animations advance across calls.
void gameStep(float tiltX, float tiltY, float dt);
