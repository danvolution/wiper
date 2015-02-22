#include <pebble.h>
#include "test_unit.h"

#define JAN_01_2015_00_00_00 1420070400
#define JAN_01_2015_12_34_00 1420115640
#define JAN_01_2015_10_09_00 1420106940

#define TEST_COUNT 2 //60

typedef struct {
  time_t startTime;
  uint16_t stepSeconds;
  uint16_t stepCount;
  uint16_t endPauseCount;
} TestData;

static TestData _testData[TEST_COUNT];

TestUnitData* CreateTestUnit() {
  TestUnitData* data = malloc(sizeof(TestUnitData));
  if (data != NULL) {
    memset(data, 0, sizeof(TestUnitData));
        
    uint16_t testIndex = 0;
    
//     for (int index = 0; index < 60; index++) {
//       _testData[testIndex].startTime = JAN_01_2015_00_00_00 + (index * 3660);
//       _testData[testIndex].stepSeconds = 0;
//       _testData[testIndex].stepCount = 0;
//       _testData[testIndex].endPauseCount = 7;
//       testIndex++;
//     }

    _testData[testIndex].startTime = JAN_01_2015_10_09_00;
    _testData[testIndex].stepSeconds = 0;
    _testData[testIndex].stepCount = 0;
    _testData[testIndex].endPauseCount = 5;
    testIndex++;

    _testData[testIndex].startTime = JAN_01_2015_10_09_00 + 60;
    _testData[testIndex].stepSeconds = 0;
    _testData[testIndex].stepCount = 0;
    _testData[testIndex].endPauseCount = 600;
    testIndex++;
    
    // Normal. Increment by 61 minutes so hour and minute changes.
//     _testData[testIndex].startTime = JAN_01_2015_00_00_00;
//     _testData[testIndex].stepSeconds = 3660;
//     _testData[testIndex].stepCount = 300;
//     _testData[testIndex].endPauseCount = 0;
//     testIndex++;

//     _testData[testIndex].startTime = JAN_01_2015_00_00_00;
//     _testData[testIndex].stepSeconds = 300;
//     _testData[testIndex].stepCount = 5;
//     testIndex++;
  }
  
  return data;
}

void DestroyTestUnit(TestUnitData* data) {
  if (data != NULL) {
    free(data);
  }
}

time_t TestUnitGetTime(TestUnitData *data) {
  // Check if we need to roll over to next test.
  if (data->stepIndex > _testData[data->testIndex].stepCount) {
    // Run through the pause count
    if (data->endPauseRemaining > 0) {
      data->endPauseRemaining--;
      return data->time;
    }
    
    data->stepIndex = 0;
    data->testIndex++;
    if (data->testIndex >= TEST_COUNT) {
      data->testIndex = 0;
    }
  }
  
  // Check if first step of TestData
  if (data->stepIndex == 0) {
    data->time = _testData[data->testIndex].startTime;
    data->endPauseRemaining = _testData[data->testIndex].endPauseCount;
    
  } else {
    data->time += _testData[data->testIndex].stepSeconds;
  }
  
  data->stepIndex++;
    
  return data->time;
}