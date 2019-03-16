#include <pebble.h>

// static pointer to a Window variable, to access later in init()
static Window *s_main_window;

// use a TextLayer element to add to the Window
static TextLayer *s_time_layer;

static TextLayer *s_day_layer;  // this will be for the day of the week
//static TextLayer *s_date_layer;  // to hold the date

static void update_time() {
	// Get a tm structure
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	
	//Write the current hours and minutes into a buffer
	static char s_buffer[8];  // buffer for hours and minutes
	static char s_daybuffer[10];  // buffer for day of week
	//static char s_datebuffer[] = "00-00-00";
	
	strftime(s_daybuffer, sizeof(s_daybuffer), "%A", tick_time); // full day format
	text_layer_set_text(s_day_layer, s_daybuffer);
		
	strftime(s_buffer, sizeof(s_buffer), "%I:%M", tick_time);  // 12 hour format
	text_layer_set_text(s_time_layer, s_buffer); // display this time on text_layer

	//strftime(s_datebuffer, sizeof(s_datebuffer), "%d/%m/%y", tick_time); // date
	//text_layer_set_text(s_date_layer, s_datebuffer);
	
}

// start TickTimerService event service. struct tm contains the current time
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
}

// handler function
static void main_window_load(Window *window) {
	// Get information about the Window
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	
	// Create the TextLayer with specific bounds
	s_day_layer = text_layer_create(
		GRect(2, 2, bounds.size.w, 36));
	s_time_layer = text_layer_create(
		GRect(0, 36, bounds.size.w, 50));
	
	text_layer_set_background_color(s_day_layer, GColorBlack);
	text_layer_set_text_color(s_day_layer, GColorClear);
	text_layer_set_font(s_day_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_alignment(s_day_layer, GTextAlignmentLeft);
	//text_layer_set_text(s_day_layer, "Saturday");  // Placeholder
	
	// Improve the layout to be more like a watchface
	text_layer_set_background_color(s_time_layer, GColorBlack);
	text_layer_set_text_color(s_time_layer, GColorClear);
	text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	
	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_day_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
}

// handler function
static void main_window_unload(Window *window) {
	// Destroy TextLayer
	text_layer_destroy(s_day_layer);
	text_layer_destroy(s_time_layer);
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