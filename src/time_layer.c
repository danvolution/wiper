#include <pebble.h>
#include "time_layer.h"
  
typedef enum { TS_WIPER, TS_DIGITS, TS_COLON_TOP, TS_COLON_BOTTOM, TS_AMPM } TimeState;

#define COLON_DRAW_DURATION 350
  
static GRect _colonTop = { {69, 73}, {6, 6} };
static GRect _colonBottom = { {69, 89}, {6, 6} };
static GRect _amPm = { {18, 43}, {14, 9} };

static int16_t _digits[4];
static AppTimer *_timeTimer = NULL;
static bool _drawColonTop = false;
static bool _drawColonBottom = false;
static TimeState _timeState;
  
static uint16_t getHour(uint16_t hour);
static void timeTimerCallback(void *callback_data);
static void timeLayerUpdateProc(Layer *layer, GContext *ctx);
static void wiperFinishedCallback(void *callback_data);
static void digitFinishedCallback(void *callback_data);
static void moveToNextTimeState();
static void clearTime(TimeLayerData *data);

TimeLayerData* CreateTimeLayer(Layer *relativeLayer, LayerRelation relation) {
  TimeLayerData* data = malloc(sizeof(TimeLayerData));
  if (data != NULL) {
    memset(data, 0, sizeof(TimeLayerData));
    data->lastUpdateMinute = -1;
    
    // Layer for drawing colon.
    data->layer = layer_create(GRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT));
    layer_set_update_proc(data->layer, timeLayerUpdateProc);
    AddLayer(relativeLayer, data->layer, relation);
    
    // Digit layers
    data->digitData[0] = CreateDigitLayer(data->layer, CHILD, GPoint(5, 62));
    data->digitData[1] = CreateDigitLayer(data->layer, CHILD, GPoint(37, 62));
    data->digitData[2] = CreateDigitLayer(data->layer, CHILD, GPoint(80, 62));
    data->digitData[3] = CreateDigitLayer(data->layer, CHILD, GPoint(112, 62));
    
    // AM/PM layer
    CreateRotBitmapGroup(&data->amPm.group, data->layer, CHILD, NULL, RESOURCE_ID_IMAGE_AM, GCompOpAssign);
    layer_set_hidden((Layer*) data->amPm.group.layer, true);
    GRect ampmFrame = RotRectFromBitmapRect(&data->amPm.group, _amPm);
    layer_set_frame((Layer*) data->amPm.group.layer, ampmFrame);
    
    // Wiper layer
    GRect wipeRect = { {0, _amPm.origin.y}, {SCREEN_WIDTH, (107 - _amPm.origin.y)} };
    data->wiperData = CreateWiperLayer(data->layer, CHILD, wipeRect);
  }

  return data;
}

void DestroyTimeLayer(TimeLayerData *data) {
  if (_timeTimer != NULL) {
    app_timer_cancel(_timeTimer);
    _timeTimer = NULL;
  }
  
  if (data != NULL) {
    DestroyWiperLayer(data->wiperData);
    data->wiperData = NULL;
      
    DestroyRotBitmapGroup(&data->amPm.group);
    
    DestroyDigitLayer(data->digitData[0]);
    data->digitData[0] = NULL;
    
    DestroyDigitLayer(data->digitData[1]);
    data->digitData[1] = NULL;
    
    DestroyDigitLayer(data->digitData[2]);
    data->digitData[2] = NULL;
    
    DestroyDigitLayer(data->digitData[3]);
    data->digitData[3] = NULL;
    
    if (data->layer != NULL) {
      layer_remove_from_parent(data->layer);
      layer_destroy(data->layer);
      data->layer = NULL;
    }
    
    free(data);
  }
}

void DrawTimeLayer(TimeLayerData *data, uint16_t hour, uint16_t minute) {
  // Exit if this minute has already been handled.
  if (data->lastUpdateMinute == minute) {
    return;
  }
  
  // Remember whether first time called.
  bool firstDisplay = (data->lastUpdateMinute == -1); 
  data->lastUpdateMinute = minute;
  
  // If time change occurs while still animating, cancel previous animation and
  // start over with current time.
  bool interruptedTimer = false;
  if (_timeTimer != NULL) {
    app_timer_cancel(_timeTimer);
    _timeTimer = NULL;
    interruptedTimer = true;
    clearTime(data);
  }
  
  uint16_t trueHour = getHour(hour);
  if (clock_is_24h_style() == true) {
    _digits[0] = trueHour / 10;
    _digits[1] = trueHour % 10;
    
  } else {
    if (trueHour < 10) {
      _digits[0] = -1;
      _digits[1] = trueHour;
      
    } else {
      _digits[0] = 1;
      _digits[1] = trueHour % 10;
    }
    
    if (hour < 12 && data->amPm.group.resourceId != RESOURCE_ID_IMAGE_AM) {
      RotBitmapGroupChangeBitmap(&data->amPm.group, NULL, RESOURCE_ID_IMAGE_AM);
      
    } else if (hour > 11 && data->amPm.group.resourceId != RESOURCE_ID_IMAGE_PM) {
      RotBitmapGroupChangeBitmap(&data->amPm.group, NULL, RESOURCE_ID_IMAGE_PM);
    }
  }
  
  _digits[2] = minute / 10;
  _digits[3] = minute % 10;

  _timeState = TS_WIPER;
  if (firstDisplay || interruptedTimer) {
    _timeTimer = app_timer_register((firstDisplay ? FIRST_DISPLAY_ANIMATION_DELAY : 10), timeTimerCallback, (void*) data);
    
  } else {
    RunWiper(data->wiperData, wiperFinishedCallback, (void*) data);
  }
}

static void clearTime(TimeLayerData *data) {
  DeconstructDigit(data->digitData[0], NULL, NULL);
  DeconstructDigit(data->digitData[1], NULL, NULL);
  DeconstructDigit(data->digitData[2], NULL, NULL);
  DeconstructDigit(data->digitData[3], NULL, NULL);
  _drawColonTop = false;
  _drawColonBottom = false;
  layer_set_hidden((Layer*) data->amPm.group.layer, true);
  ClearWiper(data->wiperData);
}

static void wiperFinishedCallback(void *callback_data) {
  moveToNextTimeState((TimeLayerData*) callback_data);
}

static void digitFinishedCallback(void *callback_data) {
  _timeTimer = app_timer_register(COLON_DRAW_DURATION, timeTimerCallback, callback_data);
}

static void timeTimerCallback(void *callback_data) {
  _timeTimer = NULL;
  moveToNextTimeState((TimeLayerData*) callback_data);
}

static void moveToNextTimeState(TimeLayerData *data) {
  switch (_timeState) {
    case TS_WIPER:
      _timeState = TS_DIGITS;

      clearTime(data);

      bool setCallback = false;
      for (int digitIndex = 0; digitIndex < 4; digitIndex++) {
        if (_digits[digitIndex] != -1) {
          if (setCallback) {
            ConstructDigit(data->digitData[digitIndex], _digits[digitIndex], NULL, NULL);
            
          } else {
            ConstructDigit(data->digitData[digitIndex], _digits[digitIndex], digitFinishedCallback, data);
            setCallback = true;
          }
        }
      }
    
      break;
    
    case TS_DIGITS:
      _timeState = TS_COLON_TOP;
    
      _drawColonTop = true;
      layer_mark_dirty(data->layer);
    
      _timeTimer = app_timer_register(COLON_DRAW_DURATION, timeTimerCallback, (void*) data);
      break;
    
    case TS_COLON_TOP:
      _timeState = TS_COLON_BOTTOM;
    
      _drawColonBottom = true;
      layer_mark_dirty(data->layer);
    
      if (clock_is_24h_style() == false) {
        _timeTimer = app_timer_register(COLON_DRAW_DURATION, timeTimerCallback, (void*) data);
      }

      break;
    
    case TS_COLON_BOTTOM:
      _timeState = TS_AMPM;
    
      layer_set_hidden((Layer*) data->amPm.group.layer, false);
      break;
    
    default:
      break;
  }  
}

static void timeLayerUpdateProc(Layer *layer, GContext *ctx) {
  if (_drawColonTop) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, _colonTop, 0, GCornerNone);
    if (_drawColonBottom) {
      graphics_fill_rect(ctx, _colonBottom, 0, GCornerNone);
    }
  }
}

static uint16_t getHour(uint16_t hour) {
  if (clock_is_24h_style() == true) {
    return hour;
  }
  
  int hour12 = hour % 12;
  return (hour12 == 0) ? 12 : hour12;
}