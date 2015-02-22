#pragma once
#include "common.h"
#include "digit_layer.h"
#include "wiper_layer.h"

typedef struct {
  Layer *layer;
  DigitLayerData *digitData[4];
  RotAnimation amPm;
  int16_t lastUpdateMinute;
  WiperLayerData *wiperData;
} TimeLayerData;

TimeLayerData* CreateTimeLayer(Layer* relativeLayer, LayerRelation relation);
void DrawTimeLayer(TimeLayerData *data, uint16_t hour, uint16_t minute);
void DestroyTimeLayer(TimeLayerData *data);
