#include <pebble.h>
#include "wiper_layer.h"

typedef struct {
  int16_t divider;
  uint16_t leftShade;
  uint16_t rightShade;
} LineShade;

#define ROTATION_INCREMENT_DURATION 60  // milliseconds
#define WIPE_FINISHED_DURATION (ROTATION_INCREMENT_DURATION * 3)  // milliseconds
#define ROTATION_INCREMENT 20           // degrees
#define NUM_SHADES 3

#define LEFT_WIPER_DEGREE 270
#define RIGHT_WIPER_DEGREE 90
#define WIPER_SWEEP_DEGREES 180

#define BOLT_DIAMETER 6

static uint16_t _shades[NUM_SHADES] = { 50, 75, 100 };

static GPoint _wiperCenterPoint = {1, 123};
static GSize _wiperOffset = {-52, -117};
static GPoint _boltCenterPoint = {71, 6};

static GRect _wipeRect;
static LineShade *_lineShades = NULL;

static void boltLayerUpdateProc(Layer *layer, GContext *ctx);
static void wipeLayerUpdateProc(Layer *layer, GContext *ctx);
static void rotationTimerCallback(void *callback_data);
static int16_t getWiperX(int16_t yPos, int32_t angleDegree);
static void drawHorizontalLine(GContext *ctx, int16_t yPos, int16_t startX, int16_t endX, uint16_t shade, bool drawLeftToRight);
static void greyPixelDistribution(uint16_t shade, int16_t *drawNPixels, int16_t *everyNPixels);

WiperLayerData* CreateWiperLayer(Layer *relativeLayer, LayerRelation relation, GRect wipeRect) {
  WiperLayerData *data = malloc(sizeof(WiperLayerData));
  if (data != NULL) {
    memset(data, 0, sizeof(WiperLayerData));
    
    // Wipe layer
    _wipeRect = wipeRect;
    _lineShades = malloc(sizeof(LineShade) * (_wipeRect.size.h + 1));
    memset(_lineShades, 0, sizeof(LineShade) * (_wipeRect.size.h + 1));
    data->wipeLayer = layer_create(_wipeRect);
    layer_set_update_proc(data->wipeLayer, wipeLayerUpdateProc);
    AddLayer(relativeLayer, data->wipeLayer, relation);
    
    // Wiper layer
    CreateRotBitmapGroup(&data->wiper.group, relativeLayer, relation, NULL, RESOURCE_ID_IMAGE_WIPER, GCompOpAssign);
    rot_bitmap_set_src_ic(data->wiper.group.layer, _wiperCenterPoint);

    GRect rotFrame = layer_get_frame((Layer*) data->wiper.group.layer);    
    rotFrame = layer_get_frame((Layer*) data->wiper.group.layer);    
    rotFrame.origin.x = _wiperOffset.w;
    rotFrame.origin.y = _wiperOffset.h;
    layer_set_frame((Layer*) data->wiper.group.layer, rotFrame);

    rot_bitmap_layer_set_angle(data->wiper.group.layer, PEBBLE_ANGLE_FROM_DEGREE(LEFT_WIPER_DEGREE));
    data->wiper.group.angle = LEFT_WIPER_DEGREE;
    
    // Wiper bolt layer
    data->boltLayer = layer_create(GRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT));
    layer_set_update_proc(data->boltLayer, boltLayerUpdateProc);
    AddLayer(relativeLayer, data->boltLayer, relation);
  }
  
  return data;
}

void DrawWiperLayer(WiperLayerData *data) {
}

void RunWiper(WiperLayerData *data, WiperFinishedCallback finishedCallback, void *wiperFinishedCallbackData) {
  ClearWiper(data);
  
  data->finishedCallback = finishedCallback;
  data->wiperFinishedCallbackData = wiperFinishedCallbackData;
  
  data->wiper.rotationIncrement = ROTATION_INCREMENT * ((data->wiper.group.angle == LEFT_WIPER_DEGREE) ? -1 : 1);
  data->wiper.rotationAmount = WIPER_SWEEP_DEGREES;
  data->wiper.endAngle = (data->wiper.group.angle == LEFT_WIPER_DEGREE) ? RIGHT_WIPER_DEGREE : LEFT_WIPER_DEGREE;
  data->shadeIndex = 0;
  data->wiper.rotationTimer = app_timer_register(ROTATION_INCREMENT_DURATION, (AppTimerCallback) rotationTimerCallback, (void*) data);
}

void ClearWiper(WiperLayerData *data) {
  if (data->wiper.rotationTimer != NULL) {
    app_timer_cancel(data->wiper.rotationTimer);
    data->wiper.rotationTimer = NULL;
  
    // Wiper was in motion. Set wiper to angle degree it was headed to.
    rot_bitmap_layer_set_angle(data->wiper.group.layer, PEBBLE_ANGLE_FROM_DEGREE(data->wiper.endAngle));
    data->wiper.group.angle = data->wiper.endAngle;
  }

  if (_lineShades != NULL) {
    memset(_lineShades, 0, sizeof(LineShade) * (_wipeRect.size.h + 1));
  }
  
  data->finishedCallback = NULL;
  data->wiperFinishedCallbackData = NULL;
  
  layer_mark_dirty(data->wipeLayer);
}

void DestroyWiperLayer(WiperLayerData *data) {
  if (data != NULL) {
    if (data->wiper.rotationTimer != NULL) {
      app_timer_cancel(data->wiper.rotationTimer);
      data->wiper.rotationTimer = NULL;
    }
    
    DestroyRotBitmapGroup(&data->wiper.group);
    
    if (data->boltLayer != NULL) {
      layer_remove_from_parent(data->boltLayer);
      layer_destroy(data->boltLayer);
      data->boltLayer = NULL;
    }
    
    if (data->wipeLayer != NULL) {
      layer_remove_from_parent(data->wipeLayer);
      layer_destroy(data->wipeLayer);
      data->wipeLayer = NULL;
    }
    
    if (_lineShades != NULL) {
      free(_lineShades);
      _lineShades = NULL;
    }
    
    free(data);
  }
}

static void rotationTimerCallback(void *callback_data) {
  WiperLayerData *data = (WiperLayerData*) callback_data;
  data->wiper.rotationTimer = NULL;
  bool wipeFinished = false;
  bool previouslyMovingRight = (data->wiper.rotationIncrement < 0);
  uint16_t previousShade = _shades[data->shadeIndex];
    
  data->wiper.rotationAmount -= abs(data->wiper.rotationIncrement);
  if (data->wiper.rotationAmount > 0) {
    data->wiper.group.angle += data->wiper.rotationIncrement;
    if (data->wiper.group.angle < 0) {
      data->wiper.group.angle += 360;
      
    } else if (data->wiper.group.angle > 360) {
      data->wiper.group.angle -= 360;    
    }
  } else {
    // Done with a wipe.
    wipeFinished = true;
    data->wiper.group.angle = data->wiper.endAngle;
    data->shadeIndex++;
    
    // Start next wipe if not on last shade.
    if (data->shadeIndex < NUM_SHADES) {
      data->wiper.rotationIncrement = ROTATION_INCREMENT * ((data->wiper.group.angle == LEFT_WIPER_DEGREE) ? -1 : 1);
      data->wiper.rotationAmount = WIPER_SWEEP_DEGREES;
      data->wiper.endAngle = (data->wiper.group.angle == LEFT_WIPER_DEGREE) ? RIGHT_WIPER_DEGREE : LEFT_WIPER_DEGREE;
    }
  }

  rot_bitmap_layer_set_angle(data->wiper.group.layer, PEBBLE_ANGLE_FROM_DEGREE(data->wiper.group.angle));

  for (int line = 0; line < _wipeRect.size.h + 1; line++) {
    int16_t xPos = getWiperX(_wipeRect.origin.y + line, data->wiper.group.angle);
    _lineShades[line].divider = xPos;
    if (xPos < 0) {
      // Set shade right if moving left. Otherwise, the wiper is moving right but hasn't reached the wipe area yet.
      if (previouslyMovingRight == false) {
        _lineShades[line].rightShade = previousShade;
      }
    } else if (xPos >= SCREEN_WIDTH) {
      // Set shade left if moving right. Otherwise, the wiper is moving left but hasn't reached the wipe area yet.
      if (previouslyMovingRight) {
        _lineShades[line].leftShade = previousShade;
      }
    } else if (previouslyMovingRight) {
      // Moving right, set shade left
      _lineShades[line].leftShade = previousShade;
      
    } else {
      // Moving left, set shade right
      _lineShades[line].rightShade = previousShade;
    }
  }
  
  layer_mark_dirty(data->wipeLayer);
  
  if (data->wiper.rotationAmount > 0) {
    data->wiper.rotationTimer = app_timer_register((wipeFinished ? WIPE_FINISHED_DURATION : ROTATION_INCREMENT_DURATION), (AppTimerCallback) rotationTimerCallback, (void*) data);
    
  } else if (data->finishedCallback != NULL) {
    data->finishedCallback(data->wiperFinishedCallbackData);
  }
}

static void boltLayerUpdateProc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, _boltCenterPoint, BOLT_DIAMETER);
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, _boltCenterPoint, 1);
}

static void wipeLayerUpdateProc(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorBlack);
  
  for (int line = 0; line < _wipeRect.size.h + 1; line++) {
    if (_lineShades[line].divider > 0 && _lineShades[line].leftShade != 0) {
      drawHorizontalLine(ctx, line, 0, _lineShades[line].divider - 1, _lineShades[line].leftShade, true);
    }
    
    if (_lineShades[line].divider < (SCREEN_WIDTH - 1) && _lineShades[line].rightShade != 0) {
      drawHorizontalLine(ctx, line, _lineShades[line].divider + 1, SCREEN_WIDTH - 1, _lineShades[line].rightShade, false);
    }
  }
}

static int16_t getWiperX(int16_t yPos, int32_t angleDegree) {
  if (angleDegree == LEFT_WIPER_DEGREE) {
    return -1;
    
  } else if (angleDegree == RIGHT_WIPER_DEGREE) {
    return SCREEN_WIDTH;
  }
  
  int16_t wiperLength = 0;
  int16_t wiperX = 0;
  int32_t angle = PEBBLE_ANGLE_FROM_DEGREE(angleDegree);
  
  wiperLength = (int16_t)((int32_t)((yPos - _boltCenterPoint.y) * TRIG_MAX_RATIO) / (int32_t)(-cos_lookup(angle)));
  wiperX = (int16_t)(sin_lookup(angle) * (int32_t)wiperLength / TRIG_MAX_RATIO) + _boltCenterPoint.x;
  
  if (wiperX < 0) {
    return -1;
    
  } else if (wiperX >= SCREEN_WIDTH) {
    return SCREEN_WIDTH;
    
  } else {
    return wiperX;
  }
}

static void drawHorizontalLine(GContext *ctx, int16_t yPos, int16_t startX, int16_t endX, uint16_t shade, bool drawLeftToRight) {
  if (shade >= 100) {
    graphics_draw_line(ctx, GPoint(startX, yPos), GPoint(endX, yPos));
    return;
  } 

  int16_t drawNPixels;
  int16_t everyNPixels;

  greyPixelDistribution(shade, &drawNPixels, &everyNPixels);

  int startPos = (drawLeftToRight ? startX : endX) + (yPos % everyNPixels);
  int endPos = (drawLeftToRight ? endX : startX);

  while (true) {
    if ((drawLeftToRight && startPos > endPos) || (!drawLeftToRight && startPos < endPos)) {
      // Done. Break.
      break;
    }

    if (drawNPixels > 1) {
      graphics_draw_line(ctx, GPoint(startPos, yPos), GPoint(startPos + drawNPixels - 1, yPos));
      
    } else {
      graphics_draw_pixel(ctx, GPoint(startPos, yPos));
    }
    
    startPos += (everyNPixels * (drawLeftToRight ? 1 : -1));
  }
}

static void greyPixelDistribution(uint16_t shade, int16_t *drawNPixels, int16_t *everyNPixels) {
  switch (shade) {
    case 10:
      *drawNPixels = 1;
      *everyNPixels = 10;
      break;
    
    case 20:
      *drawNPixels = 1;
      *everyNPixels = 5;
      break;
    
    case 25:
      *drawNPixels = 1;
      *everyNPixels = 4;
      break;
    
    case 30:
      *drawNPixels = 3;
      *everyNPixels = 10;
      break;
    
    case 33:
      *drawNPixels = 1;
      *everyNPixels = 3;
      break;
    
    case 40:
      *drawNPixels = 2;
      *everyNPixels = 5;
      break;
    
    case 50:
      *drawNPixels = 1;
      *everyNPixels = 2;
      break;
    
    case 60:
      *drawNPixels = 3;
      *everyNPixels = 5;
      break;
    
    case 66:
      *drawNPixels = 2;
      *everyNPixels = 3;
      break;
    
    case 70:
      *drawNPixels = 7;
      *everyNPixels = 10;
      break;
    
    case 75:
      *drawNPixels = 3;
      *everyNPixels = 4;
      break;
    
    case 80:
      *drawNPixels = 4;
      *everyNPixels = 5;
      break;
    
    case 90:
      *drawNPixels = 9;
      *everyNPixels = 10;
      break;
    
    case 100:
      *drawNPixels = 1;
      *everyNPixels = 1;
      break;
    
    default:
      *drawNPixels = 1;
      *everyNPixels = 2;
      break;
  }
}
