#include <pebble.h>
#include "message_layer.h"

#define BORDER_WIDTH 2
#define TEXT_MARGIN 20
  
static void borderLayerUpdateProc(Layer *layer, GContext *ctx);

MessageLayerData* CreateMessageLayer(Layer *relativeLayer, LayerRelation relation) {
  MessageLayerData *data = malloc(sizeof(MessageLayerData));
  if (data != NULL) {
    memset(data, 0, sizeof(MessageLayerData));
    
    data->borderLayer = layer_create(GRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT));
    layer_set_update_proc(data->borderLayer, borderLayerUpdateProc);
    AddLayer(relativeLayer, data->borderLayer, relation);
    
	  data->textLayer = text_layer_create(GRect(TEXT_MARGIN, TEXT_MARGIN, SCREEN_WIDTH - (2 * TEXT_MARGIN), SCREEN_HEIGHT - (2 * TEXT_MARGIN)));
  	text_layer_set_font(data->textLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  	text_layer_set_text_alignment(data->textLayer, GTextAlignmentCenter);
    AddLayer(relativeLayer, (Layer*) data->textLayer, relation);
  }
  
  return data;
}

void DrawMessageLayer(MessageLayerData *data, const char *text) {
	text_layer_set_text(data->textLayer, text);  
}

void DestroyMessageLayer(MessageLayerData *data) {
  if (data != NULL) {
    if (data->textLayer != NULL) {
      text_layer_destroy(data->textLayer);
      data->textLayer = NULL;
    }
    
    if (data->borderLayer != NULL) {
      layer_destroy(data->borderLayer);
      data->borderLayer = NULL;
    }
    
    free(data);
  }
}

static void borderLayerUpdateProc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);

  graphics_fill_rect(ctx, GRect(TEXT_MARGIN - BORDER_WIDTH, TEXT_MARGIN - BORDER_WIDTH, 
                     SCREEN_WIDTH - (2 * TEXT_MARGIN) + (2 * BORDER_WIDTH), 
                     SCREEN_HEIGHT - (2 * TEXT_MARGIN) + (2 * BORDER_WIDTH)), 0, GCornerNone);
  
  graphics_context_set_stroke_color(ctx, GColorWhite);
  
  for (int pixel = TEXT_MARGIN - BORDER_WIDTH; pixel < SCREEN_WIDTH - TEXT_MARGIN + BORDER_WIDTH; pixel += 2) {
    graphics_draw_pixel(ctx, GPoint(pixel, TEXT_MARGIN - BORDER_WIDTH));
    graphics_draw_pixel(ctx, GPoint(pixel, SCREEN_HEIGHT - TEXT_MARGIN + BORDER_WIDTH));
  }
  
  for (int pixel = TEXT_MARGIN - BORDER_WIDTH; pixel < SCREEN_HEIGHT - TEXT_MARGIN + BORDER_WIDTH; pixel += 2) {
    graphics_draw_pixel(ctx, GPoint(TEXT_MARGIN - BORDER_WIDTH, pixel));
    graphics_draw_pixel(ctx, GPoint(SCREEN_WIDTH - TEXT_MARGIN + BORDER_WIDTH, pixel));
  }
}
