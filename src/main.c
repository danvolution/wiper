#include <pebble.h>
#include "common.h"
#include "time_layer.h"
#include "message_layer.h"
#include "status_layer.h"
  
#ifdef RUN_TEST
#include "test_unit.h"
#endif

#define KEY_BLUETOOTH_VIBRATE 0
#define KEY_USAGE_DURATION 1
#define KEY_LAST_USAGE_RECORD_DAY 100
  
#define MESSAGE_SETTINGS_DURATION 1500
#define MESSAGE_BLUETOOTH_DURATION 5000

typedef struct {
  int32_t bluetoothVibrate;
} Settings;

static Window *_mainWindow = NULL;
static TimeLayerData *_timeData = NULL;
static MessageLayerData *_messageData = NULL;
static StatusLayerData *_statusData = NULL;
static Settings _settings;
static AppTimer *_messageTimer = NULL;
static AppTimer *_fiveMinuteTimer = NULL;

// Message window strings
static const char *_settingsReceivedMsg = "Settings received!";
static const char *_bluetoothDisconnectMsg = "Bluetooth connection lost!";

#ifdef RUN_TEST
static TestUnitData* _testUnitData = NULL;
#endif

static void init();
static void deinit();
static void main_window_load(Window *window);
static void main_window_unload(Window *window);
static void timer_handler(struct tm *tick_time, TimeUnits units_changed);
static void bluetooth_service_handler(bool connected);
static void battery_service_handler(BatteryChargeState charge_state);
static void inbox_received_callback(DictionaryIterator *iterator, void *context);
static void inbox_dropped_callback(AppMessageResult reason, void *context);
static void outbox_sent_callback(DictionaryIterator *values, void *context);
static void outbox_failed_callback(DictionaryIterator *failed, AppMessageResult reason, void *context);
static void loadSettings(Settings *settings);
static void saveSettings(Settings *settings);
static void sendUsageDuration(uint16_t durationMinutes);
static int32_t readPersistentInt(const uint32_t key, int32_t defaultValue);
static void showMessage(const char *text, uint32_t duration);
static void messageTimerCallback(void *callback_data);
static void fiveMinuteTimerCallback(void *callback_data);
static struct tm* getTime(struct tm *real_time);
static void drawWatchFace(struct tm *tick_time);

int main(void) {
  init();
  app_event_loop();
  deinit();
}

static void init() {
  srand(time(NULL));
  loadSettings(&_settings);
  
#ifdef RUN_TEST
  _testUnitData = CreateTestUnit();
#endif
  
  // Create main Window element and assign to pointer
  _mainWindow = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(_mainWindow, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch
  window_stack_push(_mainWindow, false);
  
#ifdef RUN_TEST
    tick_timer_service_subscribe(SECOND_UNIT, timer_handler);
#else
    tick_timer_service_subscribe(MINUTE_UNIT, timer_handler);
#endif
  
  // Register bluetooth service
  bluetooth_connection_service_subscribe(bluetooth_service_handler);
  
  // Register battery service
  battery_state_service_subscribe(battery_service_handler);
  
  // Register AppMessage callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  // Set usage timer
#ifndef RUN_TEST
  struct tm *localNow = getTime(NULL);
  int32_t day = readPersistentInt(KEY_LAST_USAGE_RECORD_DAY, -1);
  if (day != localNow->tm_yday) {
    _fiveMinuteTimer = app_timer_register(5 * 60 * 1000, fiveMinuteTimerCallback, NULL);
  }
#endif
}

static void deinit() {
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  animation_unschedule_all();
  
  if (_messageTimer != NULL) {
    app_timer_cancel(_messageTimer);
    _messageTimer = NULL;
  }

  if (_fiveMinuteTimer != NULL) {
    app_timer_cancel(_fiveMinuteTimer);
    _fiveMinuteTimer = NULL;
  }

#ifdef RUN_TEST
  if (_testUnitData != NULL) {
    DestroyTestUnit(_testUnitData);
    _testUnitData = NULL;
  }
#endif

  if (_mainWindow != NULL) {
    window_destroy(_mainWindow);
    _mainWindow = NULL;
  }
}

static void main_window_load(Window *window) {
  window_set_background_color(window, GColorBlack);
    
  _timeData = CreateTimeLayer(window_get_root_layer(_mainWindow), CHILD);
  _statusData = CreateStatusLayer(window_get_root_layer(_mainWindow), CHILD);
  
  // Initialize Bluetooth status
  bool connected = bluetooth_connection_service_peek();
  ShowBluetoothStatus(_statusData, !connected);
  UpdateBluetoothStatus(_statusData, connected);
  
  // Initialize battery status
  BatteryChargeState batteryState = battery_state_service_peek();
  ShowBatteryStatus(_statusData, (batteryState.is_charging || batteryState.is_plugged));
  UpdateBatteryStatus(_statusData, batteryState);
  
  drawWatchFace(getTime(NULL));
}

static void main_window_unload(Window *window) {
  if (_messageData != NULL) {
    DestroyMessageLayer(_messageData);
    _messageData = NULL;
  }
  
  DestroyStatusLayer(_statusData);
  _statusData = NULL;
  
  DestroyTimeLayer(_timeData);
  _timeData = NULL;
}

static void timer_handler(struct tm *tick_time, TimeUnits units_changed) {
  struct tm *localNow = getTime(tick_time);
  drawWatchFace(localNow);
  
#ifndef RUN_TEST
  // A new day has begun. Start usage timer for today.
  if ((units_changed & DAY_UNIT) != 0 && _fiveMinuteTimer == NULL) {
    _fiveMinuteTimer = app_timer_register(5 * 60 * 1000, fiveMinuteTimerCallback, NULL);
  }
#endif
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *tuple = dict_read_first(iterator);

  while (tuple != NULL) {
    switch (tuple->key) {
      case KEY_BLUETOOTH_VIBRATE:
        _settings.bluetoothVibrate = tuple->value->int32;
        MY_APP_LOG(APP_LOG_LEVEL_INFO, "Bluetooth vibrate %i", (int) _settings.bluetoothVibrate);
        break;
      
      default:
        MY_APP_LOG(APP_LOG_LEVEL_ERROR, "Key %i not recognized", (int) tuple->key);
        break;
    }

    tuple = dict_read_next(iterator);
  }
  
  saveSettings(&_settings);
  showMessage(_settingsReceivedMsg, MESSAGE_SETTINGS_DURATION);    
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
}

static void outbox_sent_callback(DictionaryIterator *values, void *context) {
  Tuple *tuple = dict_read_first(values);
  
  while (tuple != NULL) {
    switch (tuple->key) {
      case KEY_USAGE_DURATION:
        MY_APP_LOG(APP_LOG_LEVEL_INFO, "Successfully sent usage duration, %i minutes, to phone", (int) tuple->value->int32);
        break;
      
      default:
        MY_APP_LOG(APP_LOG_LEVEL_ERROR, "Key %i not recognized", (int) tuple->key);
        break;
    }

    tuple = dict_read_next(values);
  }
}

static void outbox_failed_callback(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  MY_APP_LOG(APP_LOG_LEVEL_INFO, "outbox_failed_callback");
}

static void bluetooth_service_handler(bool connected) {
  if (connected == false) {
    showMessage(_bluetoothDisconnectMsg, MESSAGE_BLUETOOTH_DURATION);
    if (_settings.bluetoothVibrate) {
      vibes_short_pulse(); 
    }
  }
  
  ShowBluetoothStatus(_statusData, !connected);
  UpdateBluetoothStatus(_statusData, connected);
}

static void battery_service_handler(BatteryChargeState charge_state) {
  ShowBatteryStatus(_statusData, (charge_state.is_charging || charge_state.is_plugged));
  UpdateBatteryStatus(_statusData, charge_state);
}

static void loadSettings(Settings *settings) {
  settings->bluetoothVibrate = readPersistentInt(KEY_BLUETOOTH_VIBRATE, 1);
  MY_APP_LOG(APP_LOG_LEVEL_INFO, "Load settings: bluetoothVibrate=%i", (int) settings->bluetoothVibrate);
}

static void saveSettings(Settings *settings) {
  persist_write_int(KEY_BLUETOOTH_VIBRATE, settings->bluetoothVibrate);
}

static int32_t readPersistentInt(const uint32_t key, int32_t defaultValue) {
  if (persist_exists(key)) {
    return persist_read_int(key);  
  }
  
  return defaultValue;
}

static void sendUsageDuration(uint16_t durationMinutes) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }
  
  Tuplet duration = TupletInteger(KEY_USAGE_DURATION, durationMinutes);
  dict_write_tuplet(iter, &duration);
  dict_write_end(iter);
  app_message_outbox_send();
}

static void messageTimerCallback(void *callback_data) {
  _messageTimer = NULL;
  if (_messageData != NULL) {
    DestroyMessageLayer(_messageData);
    _messageData = NULL;
  }
}

static void showMessage(const char *text, uint32_t duration) {
  if (_messageTimer != NULL) {
    if (app_timer_reschedule(_messageTimer, duration) == false) {
      _messageTimer = NULL;
    }
  } 
  
  if (_messageTimer == NULL) {
    _messageTimer = app_timer_register(duration, messageTimerCallback, NULL);
  }
  
  if (_messageData == NULL) {
    _messageData = CreateMessageLayer(window_get_root_layer(_mainWindow), CHILD);
  }
  
  DrawMessageLayer(_messageData, text);
}

static void fiveMinuteTimerCallback(void *callback_data) {
  _fiveMinuteTimer = NULL;
  
  // Only send the 5 minute usage data once a day.
  struct tm *localNow = getTime(NULL);
  persist_write_int(KEY_LAST_USAGE_RECORD_DAY, localNow->tm_yday);
  
  sendUsageDuration(5);
}

static void drawWatchFace(struct tm *tick_time) {
  uint16_t hour = tick_time->tm_hour;
  uint16_t minute = tick_time->tm_min;
  
  DrawStatusLayer(_statusData, hour, minute);
  DrawTimeLayer(_timeData, hour, minute);
}

static struct tm* getTime(struct tm *real_time) {
  struct tm *localTime;

#ifdef RUN_TEST
  time_t now = TestUnitGetTime(_testUnitData); 
  localTime = localtime(&now);
#else
  if (real_time != NULL) {
    localTime = real_time;
    
  } else {
    time_t now = time(NULL);
    localTime = localtime(&now);
  }
#endif
  
  return localTime;
}