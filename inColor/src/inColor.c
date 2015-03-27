/*
  inSquare

  Copyright 2015 Theo Browne
  Author: Theo Browne (with help from Greg Bartell)
  Website: TheoBrowne.com
  Github: Github.com/TheoBr
*/

#include <pebble.h>

#define KEY_MODE 0
  
static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_time_layer_top;
static TextLayer *s_time_layer_bottom;
static GFont s_time_font_top;
static GFont s_time_font_bottom;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
GColor top_layer_text_color;
GColor bottom_layer_text_color;
GColor background_color;

static void update_time() {
  
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00";
  static char bufferbottom[] = "00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 24 hour format
    strftime(buffer, sizeof("00"), "%H", tick_time);
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof("00"), "%I", tick_time);
  }
  
  strftime(bufferbottom, sizeof("00"), "%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer_top, buffer);
  text_layer_set_text(s_time_layer_bottom, bufferbottom);
}

static void canvas_update_proc(Layer *this_layer, GContext *ctx) {

  int mode = persist_read_int(KEY_MODE);
  GRect bounds = layer_get_bounds(this_layer);

  // Get the center of the screen (non full-screen)
  GPoint center = GPoint(bounds.size.w / 2, (bounds.size.h / 2));

  // Draw the 'stalk'

  GColor frameColor;
  GColor innerColor;

  if(mode == 0){
      frameColor = GColorWhite;
      innerColor = GColorBlack;
  }else if(mode == 1){
      frameColor = GColorBlack;
      innerColor = GColorWhite;
  }else{
    frameColor = GColorWhite;
    innerColor = GColorBlack;
  }

  graphics_context_set_fill_color(ctx, frameColor);
  graphics_fill_rect(ctx, GRect(14, 6, 116, 156), 0, GCornerNone);

  graphics_context_set_fill_color(ctx, innerColor);
  graphics_fill_rect(ctx, GRect(17, 9, 110, 150), 0, GCornerNone);
}

static void bluetooth_connection_callback(bool connected){
  if(!connected){
    vibes_long_pulse(); //I think this should work but it's hard to debug
  }
}

static void in_recv_handler(DictionaryIterator *iterator, void *context)
{
  //Get Tuple
  Tuple *t = dict_read_first(iterator);
  if(t)
  {
    switch(t->key)
    {
    case KEY_MODE:
      //It's the KEY_MODE key
      if(strcmp(t->value->cstring, "0") == 0)
      {
        top_layer_text_color = GColorWhite;
        bottom_layer_text_color = GColorWhite;
        background_color = GColorBlack;

        persist_write_int(KEY_MODE, 0);
      }else if(strcmp(t->value->cstring, "1") == 0){
        top_layer_text_color = GColorBlack;
        bottom_layer_text_color = GColorBlack;
        background_color = GColorWhite;

        persist_write_int(KEY_MODE, 1);
      }else if(strcmp(t->value->cstring, "2") == 0){
        top_layer_text_color = GColorBlack;
        bottom_layer_text_color = GColorWhite;

        persist_write_int(KEY_MODE, 2);
      }
        layer_mark_dirty(s_canvas_layer);
        window_set_background_color(s_main_window, background_color);
        text_layer_set_text_color(s_time_layer_top, top_layer_text_color);
        text_layer_set_text_color(s_time_layer_bottom, bottom_layer_text_color);
        bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
      break;
    }
  }
}

static void main_window_load(Window *window) {

  int mode = persist_read_int(KEY_MODE);
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
  //Create GBitmap, then set to created BitmapLayer
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  s_time_font_top = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_Font_72));
  s_time_font_bottom = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_Font_Light_72));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

  //Creates Canvas Layer for the background stuff!
  s_canvas_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));;
  layer_add_child(window_layer, s_canvas_layer);

  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  
  // Create time TextLayerTop
  s_time_layer_top = text_layer_create(GRect(0, 4, 144, 82));
  text_layer_set_background_color(s_time_layer_top, GColorClear);
  text_layer_set_text(s_time_layer_top, "00");
  
  //Create time TextLayerBottom
  s_time_layer_bottom = text_layer_create(GRect(0, 76, 144, 156));
  text_layer_set_background_color(s_time_layer_bottom, GColorClear);
  text_layer_set_text(s_time_layer_bottom, "00");

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer_top));
  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer_bottom));
  
  // Make sure the time is displayed from the start
  update_time();

  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer_top, s_time_font_top);
  text_layer_set_text_alignment(s_time_layer_top, GTextAlignmentCenter);
  
  // Same for bottom layer
  text_layer_set_font(s_time_layer_bottom, s_time_font_bottom);
  text_layer_set_text_alignment(s_time_layer_bottom, GTextAlignmentCenter);

  #ifdef PBL_COLOR
    // Define the colors
    top_layer_text_color = GColorFromRGB(255,255,255);
    bottom_layer_text_color = GColorFromRGB(255,255,255);
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND_COLOR_IMAGE);
  #else
    if(mode == 0){
      top_layer_text_color = GColorWhite;
      bottom_layer_text_color = GColorWhite;
      background_color = GColorBlack;
      //s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND_IMAGE);
    }else if(mode == 1){
      top_layer_text_color = GColorBlack;
      bottom_layer_text_color = GColorBlack;
      background_color = GColorWhite;
    }else if(mode == 2){
      top_layer_text_color = GColorBlack;
      bottom_layer_text_color = GColorWhite;
      background_color = GColorBlack;
    }
  #endif

  text_layer_set_text_color(s_time_layer_top, top_layer_text_color);
  text_layer_set_text_color(s_time_layer_bottom, bottom_layer_text_color);
  window_set_background_color(window, background_color);
  //bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);

}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer_top);
  text_layer_destroy(s_time_layer_bottom);
  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);
  layer_destroy(s_canvas_layer);

  // Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  
  fonts_unload_custom_font(s_time_font_top);
  fonts_unload_custom_font(s_time_font_bottom);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}
  
static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  app_message_register_inbox_received((AppMessageInboxReceived) in_recv_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  bluetooth_connection_service_subscribe(bluetooth_connection_callback);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
  bluetooth_connection_service_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
