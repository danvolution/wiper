#pragma once
#include "common.h"

typedef struct {
  time_t time;
  uint16_t testIndex;
  uint16_t stepIndex;
  uint16_t endPauseRemaining;
} TestUnitData;

TestUnitData* CreateTestUnit();
void DestroyTestUnit(TestUnitData* data);
time_t TestUnitGetTime(TestUnitData* data);