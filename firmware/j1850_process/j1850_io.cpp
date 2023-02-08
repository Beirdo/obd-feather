#include <Arduino.h>

#include "project.h"
#include "j1850_io.h"

static const timing_t vpw_timing[INDEX_COUNT] = {
  {200, 182, 218, 163, 239},    // SOF
  {280, 261, -1, 239, -1},      // EOF
  {300, 280, 5000, 238, 32767}, // BRK (actually <= 1.0s for rx_max)
  {300, 280, -1, 280, -1},      // IFS
  {64, 49, 79, 34, 96},         // INACT_0
  {128, 112, 145, 96, 163},     // INACT_1
  {128, 112, 145, 96, 163},     // ACT_0
  {64, 49, 79, 34, 96},         // ACT_1
};

static const timing_t pwm_timing[INDEX_COUNT] = {
  {48, 47, 49, 42, 54},     // SOF
  {72, 70, -1, 63, -1},     // EOF
  {39, 37, 41, 35, 43},     // BRK
  {96, 93, -1, 84, -1},     // IFS
  {8, 6, 8, 4, 10},         // INACT_0
  {16, 14, 16, 12, 18},     // INACT_1
  {16, 14, 16, 12, 18},     // ACT_0
  {8, 6, 8, 4, 10},         // ACT_1
};