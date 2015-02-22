#pragma once
#include "common.h"
  
typedef void (*WiperFinishedCallback)(void *callback_data);

typedef struct {
  Layer *boltLayer;
  Layer *wipeLayer;
  RotAnimation wiper;
  uint16_t shadeIndex;
  WiperFinishedCallback finishedCallback;
  void *wiperFinishedCallbackData;
} WiperLayerData;

WiperLayerData* CreateWiperLayer(Layer *relativeLayer, LayerRelation relation, GRect wipeRect);
void DrawWiperLayer(WiperLayerData *data);
void RunWiper(WiperLayerData *data, WiperFinishedCallback finishedCallback, void *wiperFinishedCallbackData);
void ClearWiper(WiperLayerData *data);
void DestroyWiperLayer(WiperLayerData *data);