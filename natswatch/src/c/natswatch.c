#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

// static pointer to a Window variable, to access later in init()
static Window *s_main_window;

// integer to store battery level percentage
static int s_battery_level;

// use a TextLayer element to add to the Window
static TextLayer *s_time_layer;
static TextLayer *s_day_layer;  // this will be for the day of the week
static TextLayer *s_date_layer;  // to hold the date
static TextLayer *s_weather_layer; // for the weather conditions layer
static TextLayer *s_temp_layer;  // for the temperature layer
static TextLayer *s_bt_dis_layer;  // to show the letter b if bluetooth disconnects

// layer for the battery bar
static Layer *s_battery_layer;

static GFont s_time_font;

static void update_time() {
	// Get a tm structure
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	
	//Write the current hours and minutes into a buffer
	static char s_buffer[8];  // buffer for hours and minutes
	static char s_daybuffer[10];  // buffer for day of week
	static char s_datebuffer[16]; // buffer for month day
	
	strftime(s_daybuffer, sizeof(s_daybuffer), "%A", tick_time); // full day format
	text_layer_set_text(s_day_layer, s_daybuffer);
		
	strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%l:%M", tick_time); 
	text_layer_set_text(s_time_layer, s_buffer); // display this time on text_layer

	strftime(s_datebuffer, sizeof(s_datebuffer), "%B %e", tick_time); // date
	text_layer_set_text(s_date_layer, s_datebuffer);
	
}

// callback to store the current charge percentage
static void battery_callback(BatteryChargeState state) {
	// Record the new battery level
	s_battery_level = state.charge_percent;
	
	// Update meter
	layer_mark_dirty(s_battery_layer);
}

// Layer update procedure for drawing the battery meter
static void battery_update_proc(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
	
	// Find the width of the bar (168 px is width of Pebble 2)
	int width = (s_battery_level * 168) / 100;
	
	// Draw the background
	graphics_context_set_fill_color(ctx, GColorDarkGray);
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);
	
	// Draw the bar
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
	
}

// Set up Bluetooth service subscription
static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(text_layer_get_layer(s_bt_dis_layer), connected);

  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}

// handler function
static void main_window_load(Window *window) {
	// Get information about the Window
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HELSINKI_48));
	
	// Create the TextLayer with specific bounds
	s_day_layer = text_layer_create(
		GRect(0, 0, bounds.size.w, 32));
	s_time_layer = text_layer_create(
		GRect(0, 32, bounds.size.w, 70));
	s_date_layer = text_layer_create(
		GRect(0, 84, bounds.size.w, 38));
	s_temp_layer = text_layer_create(
		GRect(0, 118, 42, 40));
	s_weather_layer = text_layer_create(
		GRect(42, 122, 150, 36));
	s_bt_dis_layer = text_layer_create(
		GRect(124, 0, 18, 22));
	
	// Create battery meter Layer
	s_battery_layer = layer_create(GRect(0, 160, 180, 6));
	layer_set_update_proc(s_battery_layer, battery_update_proc);
	
	// Settings for the day layer
	text_layer_set_background_color(s_day_layer, GColorBlack);
	text_layer_set_text_color(s_day_layer, GColorClear);
	text_layer_set_font(s_day_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_alignment(s_day_layer, GTextAlignmentLeft);

	// Settings for the time layer
	text_layer_set_background_color(s_time_layer, GColorBlack);
	text_layer_set_text_color(s_time_layer, GColorClear);
	text_layer_set_font(s_time_layer, s_time_font);
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	
	// Settings for the date layer
	text_layer_set_background_color(s_date_layer, GColorBlack);
	text_layer_set_text_color(s_date_layer, GColorClear);
	text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
	
	// Settings for the weather layer
	text_layer_set_background_color(s_weather_layer, GColorBlack);
	text_layer_set_text_color(s_weather_layer, GColorClear);
	text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
	text_layer_set_text_alignment(s_weather_layer, GTextAlignmentLeft);
	// text_layer_set_text(s_weather_layer, "12C Cloudy");  // Placeholder
	
	// Settings for the temperature layer
	text_layer_set_background_color(s_temp_layer, GColorBlack);
	text_layer_set_text_color(s_temp_layer, GColorClear);
	text_layer_set_font(s_temp_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
	text_layer_set_text_alignment(s_temp_layer, GTextAlignmentLeft);
	
	// Settings for the Bluetooth layer
	text_layer_set_background_color(s_bt_dis_layer, GColorWhite);
	text_layer_set_text_color(s_bt_dis_layer, GColorBlack);
	text_layer_set_font(s_bt_dis_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
	text_layer_set_text_alignment(s_bt_dis_layer, GTextAlignmentLeft);
	text_layer_set_text(s_bt_dis_layer, "!B");  // Placeholder
	
	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_day_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_temp_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_bt_dis_layer));
	
	// Add to battery bar layer to Window
	layer_add_child(window_get_root_layer(window), s_battery_layer);
	
	// Show the correct state of the BT connection from the start
	bluetooth_callback(connection_service_peek_pebble_app_connection());
}

// handler function
static void main_window_unload(Window *window) {
	// Destroy TextLayer
	text_layer_destroy(s_day_layer);
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
	text_layer_destroy(s_weather_layer);
	text_layer_destroy(s_temp_layer);
	text_layer_destroy(s_bt_dis_layer);
	
	// Destroy the battery layer
	layer_destroy(s_battery_layer);
}

// start TickTimerService event service. struct tm contains the current time
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
	
	// Get weather update every 30 minutes
	if(tick_time->tm_min % 30 == 0) {
		// Begin dictionary
		DictionaryIterator *iter;
		app_message_outbox_begin(&iter);
		
		// Add a key-value pair
		dict_write_uint8(iter, 0,0);
		
		// Send the message!
		app_message_outbox_send();
	}
}

// setting up callback functions for AppMessage
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	
	// Store incoming information from javascript weather
	static char temperature_buffer[8];
	static char temp_layer_buffer[8];
	
	static char conditions_buffer[32];
	static char weather_layer_buffer[32];


	// Read tuples for data
	Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
	Tuple *conditions_tuple = dict_find(iterator, KEY_CONDITIONS);

	// If conditions data is available, use it
	if(conditions_tuple) {
	  snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
	  
  	  // Assemble full string and display
  	  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s", conditions_buffer);
  	  text_layer_set_text(s_weather_layer, weather_layer_buffer);
	}
	
	// If temperature data is available, use it
	if(temp_tuple) {
	  snprintf(temperature_buffer, sizeof(temperature_buffer), "%d", (int)temp_tuple->value->int32);
	  
  	  // Assemble full string and display
  	  snprintf(temp_layer_buffer, sizeof(temp_layer_buffer), "%s", temperature_buffer);
  	  text_layer_set_text(s_temp_layer, temp_layer_buffer);
	}
}

// set up three callbacks for error messages
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed");
}
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
	// Create main Window element and assign to pointer
	s_main_window = window_create();
	
	// set handlers to manage the elements inside the Window
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});
	
	// Show the Window on the watch, with animated=true
	window_stack_push(s_main_window, true);
	
	// Register with TickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	
	// subscribe to updates for the battery level
	battery_state_service_subscribe(battery_callback);
	
	// Ensure battery level is displayed from the start
	battery_callback(battery_state_service_peek());
	
	// Make sure the time is displayed from the start
	update_time();
	
	// Register for Bluetooth connection updates
	connection_service_subscribe((ConnectionHandlers) {
	  .pebble_app_connection_handler = bluetooth_callback
	});
	
	// register callbacks for AppMessage
	app_message_register_inbox_received(inbox_received_callback);
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);
	
	// Open AppMessage
	const int inbox_size = 128;
	const int outbox_size = 128;
	app_message_open(inbox_size, outbox_size);
}

static void deinit() {
	// every create function should be paired with destroy
	// Destroy Window
	window_destroy(s_main_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}