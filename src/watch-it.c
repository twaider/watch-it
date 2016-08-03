#include <pebble.h>

#define COLORS PBL_IF_COLOR_ELSE(true, false)
#define ANTIALIASING true

#define SAFEMODE_ON 0
#define SAFEMODE_OFF 6

typedef struct {
  int hours;
  int minutes;
} Time;

static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_date_layer, *s_hour_layer, *s_minute_layer,
    *s_icontext_layer, *s_icon_layer;

static GFont s_small_font, s_time_font, s_icon_font;

static GPoint s_center;
static char s_last_hour[8], s_last_minute[8], s_last_date[16],
    s_icon_content[8], s_icon_text[16];
static int text1_color, text2_color;

static bool weather_units_conf = false, weather_safemode_conf = true,
            weather_data_conf = false, connection_status = false,
            weather_on_conf = false, text_color_on_conf = false;

// Set battery text and icon
static void set_battery(int battery_level) {
  if (battery_level / 10 <= 10 && battery_level / 10 > 7) {
    snprintf(s_icon_content, sizeof(s_icon_content), "%s", "z");
  } else if (battery_level / 10 <= 7 && battery_level / 10 > 3) {
    snprintf(s_icon_content, sizeof(s_icon_content), "%s", "x");
  } else if (battery_level / 10 <= 3 && battery_level / 10 > 0) {
    snprintf(s_icon_content, sizeof(s_icon_content), "%s", "y");
  }
  snprintf(s_icon_text, sizeof(s_icon_text), "%d\n %%", battery_level);
}

// Battery handler
static void battery_handler(BatteryChargeState state) {
  if (!weather_on_conf || !weather_data_conf) {
    set_battery(state.charge_percent);
  }
}

// App messages
static void inbox_received_callback(DictionaryIterator *iterator,
                                    void *context) {
  static int temperature;

  // Read tuples for data
  Tuple *weather_units_tuple = dict_find(iterator, MESSAGE_KEY_UNITS);
  Tuple *weather_on_tuple = dict_find(iterator, MESSAGE_KEY_WEATHER_ON);
  Tuple *weather_status_tuple = dict_find(iterator, MESSAGE_KEY_WEATHER_STATUS);
  Tuple *weather_safemode_tuple =
      dict_find(iterator, MESSAGE_KEY_WEATHER_SAFEMODE);
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *icon_tuple = dict_find(iterator, MESSAGE_KEY_ICON);
  Tuple *text1_color_tuple = dict_find(iterator, MESSAGE_KEY_TEXT1_COLOR);
  Tuple *text2_color_tuple = dict_find(iterator, MESSAGE_KEY_TEXT2_COLOR);
  Tuple *text_color_on_tuple = dict_find(iterator, MESSAGE_KEY_TEXT_COLOR_ON);

  // If we get weather option
  if (weather_on_tuple) {
    weather_on_conf = (bool)weather_on_tuple->value->int16;
    persist_write_bool(MESSAGE_KEY_WEATHER_ON, weather_on_conf);
  }

  if (weather_status_tuple) {
    weather_data_conf = (bool)weather_status_tuple->value->int16;
  }

  if (weather_safemode_tuple) {
    weather_safemode_conf = (bool)weather_safemode_tuple->value->int16;
    persist_write_bool(MESSAGE_KEY_WEATHER_SAFEMODE, weather_safemode_conf);
  }

  if (weather_units_tuple) {
    weather_units_conf = (bool)weather_units_tuple->value->int16;
    persist_write_bool(MESSAGE_KEY_UNITS, weather_units_conf);
  }

  // If all data is available, use it
  if (temp_tuple && icon_tuple) {
    APP_LOG(APP_LOG_LEVEL_INFO, "icon_tuple");
    // Assemble strings for temp and icon
    temperature = (float)temp_tuple->value->int32;

    if (weather_units_conf) {
      snprintf(s_icon_text, sizeof(s_icon_text), "%d\n°F", temperature);
    } else {
      snprintf(s_icon_text, sizeof(s_icon_text), "%d\n°C", temperature);
    }

    snprintf(s_icon_content, sizeof(s_icon_content), "%s",
             icon_tuple->value->cstring);
  }

  // If weather disabled or no data display battery
  battery_handler(battery_state_service_peek());

  // If text color and enabled
  if (text1_color_tuple && text2_color_tuple && text_color_on_tuple) {
    // Set text color on/off
    text_color_on_conf = (bool)text_color_on_tuple->value->int16;
    persist_write_bool(MESSAGE_KEY_TEXT_COLOR_ON, text_color_on_conf);
    // Set text1 color
    text1_color = (int)text1_color_tuple->value->int32;
    persist_write_int(MESSAGE_KEY_TEXT1_COLOR, text1_color);

    // Set text2 color
    text2_color = (int)text2_color_tuple->value->int32;
    persist_write_int(MESSAGE_KEY_TEXT2_COLOR, text2_color);
  }

  // Redraw
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator,
                                   AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

// Tick handled
static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  static bool in_interval = true;

  strftime(s_last_date, sizeof(s_last_date),
           PBL_IF_ROUND_ELSE("%a\n%d", "%a %d\n%Y"), tick_time);
  strftime(s_last_hour, sizeof(s_last_hour), clock_is_24h_style() ? "%H" : "%I",
           tick_time);
  strftime(s_last_minute, sizeof(s_last_minute), "%M", tick_time);

  if (weather_safemode_conf) {
    if (tick_time->tm_hour >= SAFEMODE_ON &&
        tick_time->tm_hour <= SAFEMODE_OFF) {
      in_interval = false;
      APP_LOG(APP_LOG_LEVEL_INFO, "in_interval");
    } else {
      in_interval = true;
    }
  }

  // Get weather update every 30 minutes
  if (tick_time->tm_min % 30 == 0 && weather_on_conf && in_interval) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }

  // Redraw
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GRect block = GRect(PBL_IF_ROUND_ELSE(0, 0), PBL_IF_ROUND_ELSE(125, 110),
                      bounds.size.w, 60);

  graphics_context_set_antialiased(ctx, ANTIALIASING);

  // Set date
  text_layer_set_text(s_date_layer, s_last_date);

  // Set time
  text_layer_set_text(s_hour_layer, s_last_hour);
  text_layer_set_text(s_minute_layer, s_last_minute);

  // Set icon and icon text layers
  text_layer_set_text(s_icontext_layer, s_icon_text);
  text_layer_set_text(s_icon_layer, s_icon_content);

  // If color screen set text color
  if (COLORS) {
    text_layer_set_text_color(s_hour_layer, GColorFromHEX(text1_color));
    text_layer_set_text_color(s_icon_layer, GColorFromHEX(text2_color));
    text_layer_set_text_color(s_icontext_layer, GColorFromHEX(text2_color));
    text_layer_set_text_color(s_date_layer, GColorFromHEX(text2_color));
  }

  // White clockface
  graphics_context_set_fill_color(ctx, GColorWhite);

  graphics_context_set_fill_color(ctx, COLORS ? GColorFromHEX(text1_color)
                                              : GColorBlack);
  graphics_context_set_stroke_color(ctx, COLORS ? GColorFromHEX(text1_color)
                                                : GColorBlack);
  graphics_fill_rect(ctx, block, 0, GCornerNone);
  graphics_draw_rect(ctx, block);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 2);

  for (int i = PBL_IF_ROUND_ELSE(32, 12);
       i < bounds.size.h - PBL_IF_ROUND_ELSE(65, 70); i++) {
    if (i % 4 == 0) {
      GRect v_line_1 = GRect(bounds.size.w / 2 - 1, i, 2, 2);

      graphics_context_set_fill_color(ctx, GColorDarkGray);
      graphics_context_set_stroke_color(ctx, GColorDarkGray);
      graphics_fill_rect(ctx, v_line_1, 0, GCornerNone);
      graphics_draw_rect(ctx, v_line_1);
    }
  }

  for (int i = bounds.size.h - PBL_IF_ROUND_ELSE(45, 50);
       i < bounds.size.h - 10; i++) {
    if (i % 4 == 0) {
      GRect v_line_2 = GRect(bounds.size.w / 2 - 1, i, 2, 2);

      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_context_set_stroke_color(ctx, GColorWhite);
      graphics_fill_rect(ctx, v_line_2, 0, GCornerNone);
      graphics_draw_rect(ctx, v_line_2);
    }
  }

  for (int i = 0; i < bounds.size.w; i++) {
    if (i % 4 == 0) {
      GRect h_line = GRect(i, PBL_IF_ROUND_ELSE(124, 109), 2, 2);

      graphics_context_set_fill_color(ctx, COLORS ? GColorFromHEX(text1_color)
                                                  : GColorBlack);
      graphics_context_set_stroke_color(ctx, COLORS ? GColorFromHEX(text1_color)
                                                    : GColorBlack);
      graphics_fill_rect(ctx, h_line, 0, GCornerNone);
      graphics_draw_rect(ctx, h_line);
    }
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // Set fonts
  s_time_font =
      fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PM_96));
  s_icon_font =
      fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ICON_24));
  s_small_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);

  s_center = grect_center_point(&window_bounds);

  s_canvas_layer = layer_create(window_bounds);

  layer_set_update_proc(s_canvas_layer, update_proc);

  layer_add_child(window_layer, s_canvas_layer);

  // Create time Layers
  s_hour_layer = text_layer_create(
      GRect(PBL_IF_ROUND_ELSE(10, 0), PBL_IF_ROUND_ELSE(20, 0),
            window_bounds.size.w / 2, window_bounds.size.h));

  s_minute_layer =
      text_layer_create(GRect(PBL_IF_ROUND_ELSE(window_bounds.size.w / 2 - 10,
                                                window_bounds.size.w / 2),
                              PBL_IF_ROUND_ELSE(20, 0),
                              window_bounds.size.w / 2, window_bounds.size.h));

  // Style the time text
  text_layer_set_background_color(s_hour_layer, GColorClear);
  text_layer_set_text_color(s_hour_layer, GColorBlueMoon);
  text_layer_set_text_alignment(s_hour_layer, GTextAlignmentCenter);

  text_layer_set_background_color(s_minute_layer, GColorClear);
  text_layer_set_text_color(s_minute_layer, GColorBlack);
  text_layer_set_text_alignment(s_minute_layer, GTextAlignmentCenter);

  // Create date Layer
  s_date_layer = text_layer_create(GRect(
      window_bounds.size.w / 2 + 12,
      PBL_IF_ROUND_ELSE(window_bounds.size.h - 51, window_bounds.size.h - 51),
      window_bounds.size.w / 2, 60));

  // Style the date text
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);

  // Create weather icon Layer
  s_icontext_layer = text_layer_create(GRect(
      PBL_IF_ROUND_ELSE(65, 45),
      PBL_IF_ROUND_ELSE(window_bounds.size.h - 51, window_bounds.size.h - 51),
      window_bounds.size.w / 2, 60));

  // Style the weather
  text_layer_set_background_color(s_icontext_layer, GColorClear);
  text_layer_set_text_color(s_icontext_layer, GColorWhite);
  text_layer_set_text_alignment(s_icontext_layer, GTextAlignmentLeft);

  // Create weather icon Layer
  s_icon_layer = text_layer_create(GRect(
      PBL_IF_ROUND_ELSE(33, 13),
      PBL_IF_ROUND_ELSE(window_bounds.size.h - 43, window_bounds.size.h - 43),
      window_bounds.size.w / 2, 60));

  // Style the weather icon
  text_layer_set_background_color(s_icon_layer, GColorClear);
  text_layer_set_text_color(s_icon_layer, GColorWhite);
  text_layer_set_text_alignment(s_icon_layer, GTextAlignmentLeft);

  text_layer_set_font(s_hour_layer, s_time_font);
  text_layer_set_font(s_minute_layer, s_time_font);
  text_layer_set_font(s_date_layer, s_small_font);
  text_layer_set_font(s_icontext_layer, s_small_font);
  text_layer_set_font(s_icon_layer, s_icon_font);

  // Add layers
  layer_add_child(window_get_root_layer(window),
                  text_layer_get_layer(s_hour_layer));
  layer_add_child(window_get_root_layer(window),
                  text_layer_get_layer(s_minute_layer));
  layer_add_child(window_get_root_layer(window),
                  text_layer_get_layer(s_date_layer));
  layer_add_child(window_get_root_layer(window),
                  text_layer_get_layer(s_icontext_layer));
  layer_add_child(window_get_root_layer(window),
                  text_layer_get_layer(s_icon_layer));
}

static void app_connection_handler(bool connected) {
  if (!connected) {
    battery_handler(battery_state_service_peek());
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "Pebble app %sconnected", connected ? "" : "dis");
}

// static void kit_connection_handler(bool connected) {
//   connection_status = connected;
//   APP_LOG(APP_LOG_LEVEL_INFO, "PebbleKit %sconnected", connected ? "" :
//   "dis");
// }

static void window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  // Destroy weather elements
  text_layer_destroy(s_hour_layer);
  text_layer_destroy(s_minute_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_icontext_layer);
  text_layer_destroy(s_icon_layer);
  fonts_unload_custom_font(s_icon_font);
  fonts_unload_custom_font(s_time_font);
}

/*********************************** App **************************************/

static void init() {
  srand(time(NULL));

  time_t t = time(NULL);
  struct tm *time_now = localtime(&t);
  tick_handler(time_now, MINUTE_UNIT);

  s_main_window = window_create();

  weather_on_conf = persist_exists(MESSAGE_KEY_WEATHER_ON)
                        ? persist_read_bool(MESSAGE_KEY_WEATHER_ON)
                        : false;
  weather_safemode_conf = persist_exists(MESSAGE_KEY_WEATHER_SAFEMODE)
                              ? persist_read_bool(MESSAGE_KEY_WEATHER_SAFEMODE)
                              : true;
  weather_units_conf = persist_exists(MESSAGE_KEY_UNITS)
                           ? persist_read_bool(MESSAGE_KEY_UNITS)
                           : false;
  text_color_on_conf = persist_exists(MESSAGE_KEY_TEXT_COLOR_ON)
                           ? persist_read_bool(MESSAGE_KEY_TEXT_COLOR_ON)
                           : false;
  text1_color = persist_exists(MESSAGE_KEY_TEXT1_COLOR)
                    ? persist_read_int(MESSAGE_KEY_TEXT1_COLOR)
                    : 0x0055FF;
  text2_color = persist_exists(MESSAGE_KEY_TEXT2_COLOR)
                    ? persist_read_int(MESSAGE_KEY_TEXT2_COLOR)
                    : 0xFFFFFF;

  window_set_window_handlers(s_main_window,
                             (WindowHandlers){
                                 .load = window_load, .unload = window_unload,
                             });

  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Register for battery level updates
  battery_state_service_subscribe(battery_handler);
  // Ensure battery level is displayed from the start
  battery_handler(battery_state_service_peek());

  connection_service_subscribe((ConnectionHandlers){
      .pebble_app_connection_handler = app_connection_handler,
      // .pebblekit_connection_handler = kit_connection_handler
  });

  app_connection_handler(connection_service_peek_pebble_app_connection());
  // app_connection_handler(connection_service_peek_pebblekit_connection())

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  app_message_open(app_message_inbox_size_maximum(),
                   app_message_outbox_size_maximum());
}

static void deinit() { window_destroy(s_main_window); }

int main() {
  init();
  app_event_loop();
  deinit();
}
