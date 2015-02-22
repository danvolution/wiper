#include <pebble.h>
#include "common.h"

static uint16_t getImageHypotenuse(uint32_t imageResourceId);
static GSize getRotBitmapOffset(uint32_t imageResourceId);

void AddLayer(Layer* relativeLayer, Layer* newLayer, LayerRelation relation) {
  switch (relation) {
    case ABOVE_SIBLING:
      layer_insert_above_sibling(newLayer, relativeLayer);
    break;
    
    case BELOW_SIBLING:
      layer_insert_below_sibling(newLayer, relativeLayer);
      break;
    
    default:
      layer_add_child(relativeLayer, newLayer);      
      break;
  }
}

void CreateBitmapGroup(BitmapGroup *group, GRect frame, Layer *relativeLayer, LayerRelation relation, GCompOp compostingMode) {
  group->bitmap = NULL;
  group->createdBitmap = false;
  group->resourceId = 0;
  group->layer = bitmap_layer_create(frame);
  bitmap_layer_set_compositing_mode(group->layer, compostingMode);
  AddLayer(relativeLayer, (Layer*) group->layer, relation);    
}

void CreateRotBitmapGroup(RotBitmapGroup *group, Layer *relativeLayer, LayerRelation relation, GBitmap *bitmap, 
                          uint32_t imageResourceId, GCompOp compostingMode) {
  
  // Create the bitmap if NULL passed for the bitmap parameter
  if (bitmap == NULL) {
    group->bitmap = gbitmap_create_with_resource(imageResourceId);
    group->createdBitmap = true;
    
  } else {
    group->bitmap = bitmap;
    group->createdBitmap = false;
  }
  
  group->resourceId = imageResourceId;
  group->angle = 0;
  group->layer = rot_bitmap_layer_create(group->bitmap);
  rot_bitmap_set_compositing_mode(group->layer, compostingMode);
  AddLayer(relativeLayer, (Layer*) group->layer, relation);
}

// Returns whether the bitmap was changed.
bool BitmapGroupSetBitmap(BitmapGroup *group, GBitmap *bitmap, uint32_t imageResourceId) {
  bool imageChanged = false;
  
  if (group->resourceId != imageResourceId) {
    imageChanged = true;
    
    if (group->bitmap != NULL && group->createdBitmap) {
      gbitmap_destroy(group->bitmap);
    }
    
    group->bitmap = NULL;
    group->resourceId = 0;

    // Create the new bitmap if NULL passed for the bitmap parameter
    if (bitmap == NULL) {
      group->bitmap = gbitmap_create_with_resource(imageResourceId);
      group->createdBitmap = true;
      
    } else {
      group->bitmap = bitmap;
      group->createdBitmap = false;
    }
    
    // Set the new bitmap on the BitmapLayer
    group->resourceId = imageResourceId;
    bitmap_layer_set_bitmap((BitmapLayer*) group->layer, group->bitmap);
  }
  
  return imageChanged;
}

// Returns new GRect frame for the RotBitmapLayer. Frame and bounds will be adjusted to
// new image, however the frame will most likely not be in the right position.
GRect RotBitmapGroupChangeBitmap(RotBitmapGroup *group, GBitmap *bitmap, uint32_t imageResourceId) {
  if (group->bitmap != NULL && group->createdBitmap) {
    gbitmap_destroy(group->bitmap);
  }

  group->bitmap = NULL;
  group->resourceId = 0;

  // Create the new bitmap if NULL passed for the bitmap parameter
  if (bitmap == NULL) {
    group->bitmap = gbitmap_create_with_resource(imageResourceId);
    group->createdBitmap = true;
    
  } else {
    group->bitmap = bitmap;
    group->createdBitmap = false;
  }
  
  // Set the new bitmap on the RotBitmapLayer
  group->resourceId = imageResourceId;
  bitmap_layer_set_bitmap((BitmapLayer*) group->layer, group->bitmap);

  // Get the hypotenuse of the new image which is what the RotBitmapLayer uses
  // for the frame width & height.
  uint16_t hypotenuse = getImageHypotenuse(imageResourceId);

  // Adjust the frame size
  GRect rotFrame = layer_get_frame((Layer*) group->layer);   
  rotFrame.size.w = hypotenuse;
  rotFrame.size.h = hypotenuse;
  layer_set_frame((Layer*) group->layer, rotFrame);    

  // Adjust the bounds
  layer_set_bounds((Layer*) group->layer, GRect(0, 0, hypotenuse, hypotenuse));

  // Set the center point for the new bitmap. This causes the RotBitmapLayer to
  // center the new bitmap.
  rot_bitmap_set_src_ic(group->layer, grect_center_point(&group->bitmap->bounds));
  
  return rotFrame;
}

GRect RotRectFromBitmapRect(RotBitmapGroup *group, GRect bitmapRect) {
  GSize offset = getRotBitmapOffset(group->resourceId);
  GRect rotRect = layer_get_frame((Layer*) group->layer);
  return GRect(bitmapRect.origin.x - offset.w, bitmapRect.origin.y - offset.h, rotRect.size.w, rotRect.size.h);
}

GRect BitmapRectFromRotRect(RotBitmapGroup *group, GRect rotRect) {
  GSize offset = getRotBitmapOffset(group->resourceId);
  GRect bitmapRect = group->bitmap->bounds;
  return GRect(rotRect.origin.x + offset.w, rotRect.origin.y + offset.h, bitmapRect.size.w, bitmapRect.size.h);
}

void DestroyBitmapGroup(BitmapGroup *group) {
  if (group != NULL) {
    if (group->bitmap != NULL && group->createdBitmap) {
      gbitmap_destroy(group->bitmap);
    }
    
    group->bitmap = NULL;
    group->resourceId = 0;
    
    if (group->layer != NULL) {
      layer_remove_from_parent((Layer*) group->layer);
      bitmap_layer_destroy(group->layer);
      group->layer = NULL;
    }
  }
}

void DestroyRotBitmapGroup(RotBitmapGroup *group) {
  if (group != NULL) {
    if (group->bitmap != NULL && group->createdBitmap) {
      gbitmap_destroy(group->bitmap);
    }
    
    group->bitmap = NULL;
    group->resourceId = 0;
    
    if (group->layer != NULL) {
      layer_remove_from_parent((Layer*) group->layer);
      rot_bitmap_layer_destroy(group->layer);
      group->layer = NULL;
    }
  }
}

static uint16_t getImageHypotenuse(uint32_t imageResourceId) {
  uint16_t hypotenuse = 0;
  
  switch (imageResourceId) {
    case RESOURCE_ID_IMAGE_BLOCK_WHITE_9x9:
      hypotenuse = 12;    // 12.73
      break;
    
    case RESOURCE_ID_IMAGE_AM:
    case RESOURCE_ID_IMAGE_PM:
      hypotenuse = 16;    // 16.64
      break;
    
    case RESOURCE_ID_IMAGE_WIPER:
      hypotenuse = 133;    // 133.63
      break;
    
    default:
      break;
  }
  
  return hypotenuse;
}

// Get the offset of the unrotated bitmap inside the RotBitmap
static GSize getRotBitmapOffset(uint32_t imageResourceId) {
  GSize offset = { 0, 0 };
  
  switch (imageResourceId) {
    case RESOURCE_ID_IMAGE_BLOCK_WHITE_9x9:
      offset = GSize(2, 2);
      break;
    
    case RESOURCE_ID_IMAGE_AM:
    case RESOURCE_ID_IMAGE_PM:
      offset = GSize(1, 3);
      break;
    
    case RESOURCE_ID_IMAGE_WIPER:
      offset = GSize(0, 60);
      break;
     
    default:
      break;
  }
  
  return offset;
}