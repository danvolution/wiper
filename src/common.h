#pragma once
//#define RUN_TEST true 
//#define LOGGING_ON true

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168
  
// Delay animations to give the watchface animation to finish when it is first launched.
#define FIRST_DISPLAY_ANIMATION_DELAY 500
  
#define PEBBLE_ANGLE_PER_DEGREE (TRIG_MAX_ANGLE / 360)

// Convert degree to Pebble angle
#define PEBBLE_ANGLE_FROM_DEGREE(degree) (degree * PEBBLE_ANGLE_PER_DEGREE)
  
#ifdef LOGGING_ON
  #define MY_APP_LOG(level, fmt, args...)                                \
    app_log(level, __FILE_NAME__, __LINE__, fmt, ## args)
#else
  #define MY_APP_LOG(level, fmt, args...)
#endif

typedef enum { CHILD, ABOVE_SIBLING, BELOW_SIBLING } LayerRelation;

typedef struct {
  BitmapLayer *layer;
  GBitmap *bitmap;
  uint32_t resourceId;
  bool createdBitmap;
} BitmapGroup;

typedef struct {
  RotBitmapLayer *layer;
  GBitmap *bitmap;
  uint32_t resourceId;
  bool createdBitmap;
  int32_t angle;        // Angle in degrees
} RotBitmapGroup;
  
typedef struct {
  RotBitmapGroup group;
  PropertyAnimation *animation;
  AppTimer *rotationTimer;
  int32_t rotationAmount;
  int32_t rotationIncrement;
  int32_t endAngle;        // Angle in degrees
} RotAnimation;

void AddLayer(Layer *relativeLayer, Layer *newLayer, LayerRelation relation);
void CreateBitmapGroup(BitmapGroup *group, GRect frame, Layer *relativeLayer, LayerRelation relation, GCompOp compostingMode);
bool BitmapGroupSetBitmap(BitmapGroup *group, GBitmap *bitmap, uint32_t imageResourceId);
void DestroyBitmapGroup(BitmapGroup *group);
void CreateRotBitmapGroup(RotBitmapGroup *group, Layer *relativeLayer, LayerRelation relation, GBitmap *bitmap, uint32_t imageResourceId, GCompOp compostingMode);
GRect RotBitmapGroupChangeBitmap(RotBitmapGroup *group, GBitmap *bitmap, uint32_t imageResourceId);
GRect RotRectFromBitmapRect(RotBitmapGroup *group, GRect bitmapRect);
GRect BitmapRectFromRotRect(RotBitmapGroup *group, GRect rotRect);
void DestroyRotBitmapGroup(RotBitmapGroup *group);
