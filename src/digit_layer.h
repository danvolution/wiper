#pragma once
#include "common.h"
  
#define TOTAL_NUM_BLOCKS 16
#define NUM_BLOCKS 15
#define EXTRA_BLOCK_INDEX 15

typedef void (*DigitFinishedCallback)(void *callback_data);

typedef struct {
  RotBitmapGroup group;
  PropertyAnimation *animation;
} Block;

typedef struct {
  Layer *layer;
  GPoint origin;
  Block blocks[TOTAL_NUM_BLOCKS];
  GBitmap *bitmap;
  int16_t digit;
  int16_t startSpotIndex;
  int16_t spotIndex;
  AppTimer *spotTimer;
  DigitFinishedCallback finishedCallback;
  void *digitFinishedCallbackData;
} DigitLayerData;

DigitLayerData* CreateDigitLayer(Layer *relativeLayer, LayerRelation relation, GPoint origin);
void DrawDigitLayer(DigitLayerData* data, uint16_t hour, uint16_t minute);
void ConstructDigit(DigitLayerData* data, uint16_t digit, DigitFinishedCallback finishedCallback, void *digitFinishedCallbackData);
void DeconstructDigit(DigitLayerData* data, DigitFinishedCallback finishedCallback, void *digitFinishedCallbackData);
void DestroyDigitLayer(DigitLayerData* data);
