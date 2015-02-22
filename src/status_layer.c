#include <pebble.h>
#include "status_layer.h"
  
static char _batteryText[20];
static char _bluetoothConnected[] = "Connected";
static char _bluetoothDisconnected[50];

static void setDisconnectedString();
static void setPercentage(uint8_t chargePercent);
  
StatusLayerData* CreateStatusLayer(Layer *relativeLayer, LayerRelation relation) {
  StatusLayerData *data = malloc(sizeof(StatusLayerData));
  if (data != NULL) {
    memset(data, 0, sizeof(StatusLayerData));
    
    data->textLayerBattery = text_layer_create(GRect(107, SCREEN_HEIGHT - 18, 33, 18));
  	text_layer_set_font(data->textLayerBattery, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  	text_layer_set_text_alignment(data->textLayerBattery, GTextAlignmentRight);
    text_layer_set_text_color(data->textLayerBattery, GColorWhite);
    text_layer_set_background_color(data->textLayerBattery, GColorClear);
    AddLayer(relativeLayer, (Layer*) data->textLayerBattery, relation);
    
    data->textLayerBluetooth = text_layer_create(GRect(3, SCREEN_HEIGHT - 18, 104, 18));
   	text_layer_set_font(data->textLayerBluetooth, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  	text_layer_set_text_alignment(data->textLayerBluetooth, GTextAlignmentLeft);
    text_layer_set_text_color(data->textLayerBluetooth, GColorWhite);
    text_layer_set_background_color(data->textLayerBluetooth, GColorClear);
    AddLayer(relativeLayer, (Layer*) data->textLayerBluetooth, relation);
  }
  
  return data;
}

void DrawStatusLayer(StatusLayerData *data, uint16_t hour, uint16_t minute) {
  
}

void DestroyStatusLayer(StatusLayerData *data) {
  if (data != NULL) {
    if (data->textLayerBattery != NULL) {
      text_layer_destroy(data->textLayerBattery);
      data->textLayerBattery = NULL;
    }
    
    if (data->textLayerBluetooth != NULL) {
      text_layer_destroy(data->textLayerBluetooth);
      data->textLayerBluetooth = NULL;
    }
    
    free(data);
  }
}

void UpdateBatteryStatus(StatusLayerData *data, BatteryChargeState charge_state) {
  setPercentage(charge_state.charge_percent);
  text_layer_set_text(data->textLayerBattery, _batteryText);
}

void ShowBatteryStatus(StatusLayerData *data, bool show) {
  layer_set_hidden((Layer*) data->textLayerBattery, (show == false));
}

void UpdateBluetoothStatus(StatusLayerData *data, bool connected) {
  if (!connected) {
    setDisconnectedString();
  }
  
  text_layer_set_text(data->textLayerBluetooth, connected ? _bluetoothConnected : _bluetoothDisconnected);  
}

void ShowBluetoothStatus(StatusLayerData *data, bool show) {
  layer_set_hidden((Layer*) data->textLayerBluetooth, (show == false));
}

static void setDisconnectedString() {
  char *sys_locale = setlocale(LC_ALL, "");
  
  if (strcmp("fr_FR", sys_locale) == 0) {
    strcpy(_bluetoothDisconnected, "Déconnecté");
    
  } else if (strcmp("de_DE", sys_locale) == 0) {
    strcpy(_bluetoothDisconnected, "Verbindung getrennt");
    
  } else if (strcmp("es_ES", sys_locale) == 0) {
    strcpy(_bluetoothDisconnected, "Desconectado");
    
  } else if (strcmp("zh_CN", sys_locale) == 0) {
    strcpy(_bluetoothDisconnected, "已断开连接");
    
  } else {
    strcpy(_bluetoothDisconnected, "Disconnected");
  }  
}

static void setPercentage(uint8_t chargePercent) {
  char *sys_locale = setlocale(LC_ALL, "");
  
  if (strcmp("fr_FR", sys_locale) == 0 || strcmp("de_DE", sys_locale) == 0) {
    snprintf(_batteryText, sizeof(_batteryText), "%d %%", chargePercent);
    
  } else { // en_US, es_ES, zh_CN
    snprintf(_batteryText, sizeof(_batteryText), "%d%%", chargePercent);
  }    
}