#include <pebble.h>

// static pointer to a Window variable, to access later in init()
static Window *s_main_window;

// Declare font globally
static GFont s_time_font;
static GFont s_weather_font;

// use a TextLayer element to add to the Window
static TextLayer *s_time_layer;

// Adding Layer to hold weather information
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

// start TickTimerService event service. struct tm contains the current time
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
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
	
	// Create the TextLayer with specific bounds
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
	text_layer_set_text(s_weather_layer, "Loading...");

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
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
	
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

static void init() {
	// Create main Window element and assign to pointer
	s_main_window = window_create();
	
	// set handlers to manage the elements inside the Window
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});
	
	window_set_background_color(s_main_window, GColorBlack);
	
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