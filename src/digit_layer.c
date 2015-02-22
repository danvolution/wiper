#include <pebble.h>
#include "digit_layer.h"
  
#define BLOCK_IMAGE_RESOURCE_ID RESOURCE_ID_IMAGE_BLOCK_WHITE_9x9
#define BLOCK_WIDTH 9
#define BLOCK_HEIGHT 9
#define BLOCK_DIVIDER 0
  
#define SPOT_DURATION 75

static GRect _blockDefinition[NUM_BLOCKS] = {
  { {0, 0}, {BLOCK_WIDTH, BLOCK_HEIGHT} },
  { {BLOCK_WIDTH + BLOCK_DIVIDER, 0}, {BLOCK_WIDTH, BLOCK_HEIGHT} },
  { {(BLOCK_WIDTH + BLOCK_DIVIDER) * 2, 0}, {BLOCK_WIDTH, BLOCK_HEIGHT} },
  { {0, BLOCK_HEIGHT + BLOCK_DIVIDER}, {BLOCK_WIDTH, BLOCK_HEIGHT} },
  { {BLOCK_WIDTH + BLOCK_DIVIDER, BLOCK_HEIGHT + BLOCK_DIVIDER}, {BLOCK_WIDTH, BLOCK_HEIGHT} },
  { {(BLOCK_WIDTH + BLOCK_DIVIDER) * 2, BLOCK_HEIGHT + BLOCK_DIVIDER}, {BLOCK_WIDTH, BLOCK_HEIGHT} },
  { {0, (BLOCK_HEIGHT + BLOCK_DIVIDER) * 2}, {BLOCK_WIDTH, BLOCK_HEIGHT} },
  { {BLOCK_WIDTH + BLOCK_DIVIDER, (BLOCK_HEIGHT + BLOCK_DIVIDER) * 2}, {BLOCK_WIDTH, BLOCK_HEIGHT} },
  { {(BLOCK_WIDTH + BLOCK_DIVIDER) * 2, (BLOCK_HEIGHT + BLOCK_DIVIDER) * 2}, {BLOCK_WIDTH, BLOCK_HEIGHT} },
  { {0, (BLOCK_HEIGHT + BLOCK_DIVIDER) * 3}, {BLOCK_WIDTH, BLOCK_HEIGHT} },
  { {BLOCK_WIDTH + BLOCK_DIVIDER, (BLOCK_HEIGHT + BLOCK_DIVIDER) * 3}, {BLOCK_WIDTH, BLOCK_HEIGHT} },
  { {(BLOCK_WIDTH + BLOCK_DIVIDER) * 2, (BLOCK_HEIGHT + BLOCK_DIVIDER) * 3}, {BLOCK_WIDTH, BLOCK_HEIGHT} },
  { {0, (BLOCK_HEIGHT + BLOCK_DIVIDER) * 4}, {BLOCK_WIDTH, BLOCK_HEIGHT} },
  { {BLOCK_WIDTH + BLOCK_DIVIDER, (BLOCK_HEIGHT + BLOCK_DIVIDER) * 4}, {BLOCK_WIDTH, BLOCK_HEIGHT} },
  { {(BLOCK_WIDTH + BLOCK_DIVIDER) * 2, (BLOCK_HEIGHT + BLOCK_DIVIDER) * 4}, {BLOCK_WIDTH, BLOCK_HEIGHT} }
};

static uint16_t _numberDefinition[10][NUM_BLOCKS] = {
/*{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 } */
  { 1, 1, 1, 1, 0, 1, 1, 0, 1, 1,  0,  1,  1,  1,  1 }, // 0
  { 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,  0,  1,  0,  0,  1 }, // 1
  { 1, 1, 1, 0, 0, 1, 1, 1, 1, 1,  0,  0,  1,  1,  1 }, // 2
  { 1, 1, 1, 0, 0, 1, 1, 1, 1, 0,  0,  1,  1,  1,  1 }, // 3
  { 1, 0, 1, 1, 0, 1, 1, 1, 1, 0,  0,  1,  0,  0,  1 }, // 4
  { 1, 1, 1, 1, 0, 0, 1, 1, 1, 0,  0,  1,  1,  1,  1 }, // 5
  { 1, 1, 1, 1, 0, 0, 1, 1, 1, 1,  0,  1,  1,  1,  1 }, // 6
  { 1, 1, 1, 0, 0, 1, 0, 0, 1, 0,  0,  1,  0,  0,  1 }, // 7
  { 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,  0,  1,  1,  1,  1 }, // 8
  { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0,  0,  1,  1,  1,  1 }  // 9
};

static uint16_t _randomSpots[NUM_BLOCKS] = {
  2, 6, 10, 0, 13, 5, 4, 12, 11, 7, 14, 1, 8, 9, 3
};

static void destoryBlockAnimation(Block *block);
static void spotTimerCallback(void *callback_data);
static void digitClear(DigitLayerData *data);
static void digitSpots(DigitLayerData *data);

DigitLayerData* CreateDigitLayer(Layer *relativeLayer, LayerRelation relation, GPoint origin) {
  DigitLayerData* data = malloc(sizeof(DigitLayerData));
  if (data != NULL) {
    memset(data, 0, sizeof(DigitLayerData));
    
    data->digit = -1;
    data->bitmap = gbitmap_create_with_resource(BLOCK_IMAGE_RESOURCE_ID);
    data->origin = origin;
    data->layer = layer_create(GRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT));
    AddLayer(relativeLayer, data->layer, relation);
    
    for (int blockIndex = 0; blockIndex < TOTAL_NUM_BLOCKS; blockIndex++) {
      CreateRotBitmapGroup(&data->blocks[blockIndex].group, data->layer, CHILD, data->bitmap, 
                           BLOCK_IMAGE_RESOURCE_ID, GCompOpAssign);
      
      layer_set_hidden((Layer*) data->blocks[blockIndex].group.layer, true);
    }
  }
      
  return data;
}

void DrawDigitLayer(DigitLayerData *data, uint16_t hour, uint16_t minute) {
}

void ConstructDigit(DigitLayerData *data, uint16_t digit, DigitFinishedCallback finishedCallback, void *digitFinishedCallbackData) {
  DeconstructDigit(data, NULL, NULL);
  data->digit = digit;
  data->finishedCallback = finishedCallback;
  data->digitFinishedCallbackData = digitFinishedCallbackData;
  digitSpots(data);
}

static void digitSpots(DigitLayerData *data) {
  data->startSpotIndex = rand() % NUM_BLOCKS;
  data->spotIndex = data->startSpotIndex;
  data->spotTimer = app_timer_register(SPOT_DURATION, spotTimerCallback, (void*) data);
}

void DeconstructDigit(DigitLayerData *data, DigitFinishedCallback finishedCallback, void *digitFinishedCallbackData) {
  // Clean up spot timer
  if (data->spotTimer != NULL) {
    app_timer_cancel(data->spotTimer);
    data->spotTimer = NULL;
  }
  
  data->finishedCallback = finishedCallback;
  data->digitFinishedCallbackData = digitFinishedCallbackData;
  
  // Clean up previous animations
  for (int blockIndex = 0; blockIndex < TOTAL_NUM_BLOCKS; blockIndex++) {
    destoryBlockAnimation(&data->blocks[blockIndex]);
  }

  digitClear(data);
  
  data->digit = -1;
}

static void digitClear(DigitLayerData *data) {
  for (int blockIndex = 0; blockIndex < TOTAL_NUM_BLOCKS; blockIndex++) {
    layer_set_hidden((Layer*) data->blocks[blockIndex].group.layer, true);
  }
  
  if (data->finishedCallback != NULL) {
    data->finishedCallback(data->digitFinishedCallbackData);
  }
}

void DestroyDigitLayer(DigitLayerData *data) {
  if (data != NULL) {
    if (data->spotTimer != NULL) {
      app_timer_cancel(data->spotTimer);
      data->spotTimer = NULL;
    }
    
    for (int blockIndex = 0; blockIndex < TOTAL_NUM_BLOCKS; blockIndex++) {
      destoryBlockAnimation(&data->blocks[blockIndex]);
      DestroyRotBitmapGroup(&data->blocks[blockIndex].group);
    }
    
    if (data->layer != NULL) {
      layer_remove_from_parent(data->layer);
      layer_destroy(data->layer);
      data->layer = NULL;
    }
    
    if (data->bitmap != NULL) {
      gbitmap_destroy(data->bitmap);
      data->bitmap = NULL;
    }
    
    free(data);
  }
}

static void destoryBlockAnimation(Block *block) {
  if (block->animation != NULL) {
    if (animation_is_scheduled((Animation*) block->animation)) {
      animation_unschedule((Animation*) block->animation);
    }

    property_animation_destroy(block->animation);
    block->animation = NULL;
  }
}

static void spotTimerCallback(void *callback_data) {
  DigitLayerData* data = (DigitLayerData*) callback_data;
  data->spotTimer = NULL;
  
  uint16_t *blocks = _numberDefinition[data->digit];
  int16_t blockIndex = _randomSpots[data->spotIndex];
  
  if (blocks[blockIndex] == 1) {
    GRect blockFrame = RotRectFromBitmapRect(&data->blocks[blockIndex].group, _blockDefinition[blockIndex]);
    blockFrame.origin.x += data->origin.x;
    blockFrame.origin.y += data->origin.y;
    
    layer_set_frame((Layer*) data->blocks[blockIndex].group.layer, blockFrame);
    layer_set_hidden((Layer*) data->blocks[blockIndex].group.layer, false);
  }
    
  data->spotIndex++;
  if (data->spotIndex == NUM_BLOCKS) {
    data->spotIndex = 0;
  }
  
  if (data->spotIndex != data->startSpotIndex) {
    data->spotTimer = app_timer_register(SPOT_DURATION, spotTimerCallback, (void*) data);
    
  } else if (data->finishedCallback != NULL) {
    data->finishedCallback(data->digitFinishedCallbackData);
  }
}
