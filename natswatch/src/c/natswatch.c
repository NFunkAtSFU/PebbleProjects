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
static TextLayer *s_weather_layer; // for the weather layer

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

// start TickTimerService event service. struct tm contains the current time
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
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
	
	// Find the width of the bar (total watch width = 160px)
	int width = (s_battery_level * 160) / 100;
	
	// Draw the background
	graphics_context_set_fill_color(ctx, GColorDarkGray);
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);
	
	// Draw the bar
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
	
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
		GRect(0, 84, bounds.size.w, 34));
	s_weather_layer = text_layer_create(
		GRect(0, 120, bounds.size.w, 24));
	
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
	text_layer_set_background_color(s_weather_layer, GColorClear);
	text_layer_set_text_color(s_weather_layer, GColorBlack);
	text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
	text_layer_set_text_alignment(s_weather_layer, GTextAlignmentLeft);
	text_layer_set_text(s_weather_layer, "12C Cloudy");  // Placeholder
	
	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_day_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
	
	// Add to battery bar layer to Window
	layer_add_child(window_get_root_layer(window), s_battery_layer);
}

// handler function
static void main_window_unload(Window *window) {
	// Destroy TextLayer
	text_layer_destroy(s_day_layer);
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
	text_layer_destroy(s_weather_layer);
	
	// Destroy the battery layer
	layer_destroy(s_battery_layer);
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