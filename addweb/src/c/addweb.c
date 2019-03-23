#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

// static pointer to a Window variable, to access later in init()
static Window *s_main_window;

// Declare font globally
static GFont s_time_font;
static GFont s_weather_font;

// use a TextLayer element to add time and weather info to the Window
static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;

// Pointers for bitmap
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static void update_time() {
	// Get a tm structure
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	
	//Write the current hours and minutes into a buffer
	static char s_buffer[8];
	strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
	
	// Display this time on the TextLayer
	text_layer_set_text(s_time_layer, s_buffer);
}

// handler function
static void main_window_load(Window *window) {
	// Get information about the Window
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	
	// Create GBitmap - this needs to be before the TextLayer
	s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
	
	// Create BitmapLayer to display the GBitmap
	s_background_layer = bitmap_layer_create(bounds);
	
	// Set the bitmap onto the layer and add to the window
	bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
	layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
	
	// Create the time TextLayer with specific bounds
	s_time_layer = text_layer_create(
		GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
	
	// Improve the layout to be more like a watchface
	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_text_color(s_time_layer, GColorBlack);

	// Create temperature layer
	s_weather_layer = text_layer_create(
		GRect(0, PBL_IF_ROUND_ELSE(125,120), bounds.size.w, 25));
	
	// Style text for temperature layer
	text_layer_set_background_color(s_weather_layer, GColorClear);
	text_layer_set_text_color(s_weather_layer, GColorWhite);
	text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
	// text_layer_set_text(s_weather_layer, "Loading...");

  // Create GFonts for time and for weather
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_48));
	s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_20));

    // Apply fonts to TextLayer
    text_layer_set_font(s_time_layer, s_time_font);
    text_layer_set_font(s_weather_layer, s_weather_font);
	
	// text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));   //previous font
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	
	// Add child layers to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
	
	update_time();
	
}

// handler function
static void main_window_unload(Window *window) {
	// Destroy TextLayer
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_weather_layer);
	
	//Unload GFont
	fonts_unload_custom_font(s_time_font);
	fonts_unload_custom_font(s_weather_font);
	
	// Destroy GBitmap
	gbitmap_destroy(s_background_bitmap);
	
	// Destroy BitmapLayer
	bitmap_layer_destroy(s_background_layer);
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
	static char conditions_buffer[32];
	static char weather_layer_buffer[32];
	
	// Read tuples for data
	Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
	Tuple *conditions_tuple = dict_find(iterator, KEY_CONDITIONS);

	// If all data is available, use it
	if(temp_tuple && conditions_tuple) {
	  snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)temp_tuple->value->int32);
	  snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
	  
  	  // Assemble full string and display
  	  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
  	  text_layer_set_text(s_weather_layer, weather_layer_buffer);
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
	window_set_background_color(s_main_window, GColorBlack);
	
	// set handlers to manage the elements inside the Window
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});
	
	// Show the Window on the watch, with animated=true
	window_stack_push(s_main_window, true);
	
	// Register with TickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	
	// Make sure the time is displayed from the start
	update_time();
	
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