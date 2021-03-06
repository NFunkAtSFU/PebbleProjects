#include <pebble.h>

// static pointer to a Window variable, to access later in init()
static Window *s_main_window;

// integer to store battery level percentage
static int s_battery_level;

// Declare font globally
static GFont s_time_font;

// use a TextLayer element to add to the Window
static TextLayer *s_time_layer;

// layer for the battery bar
static Layer *s_battery_layer;

// Pointers for bitmap
static BitmapLayer *s_background_layer, *s_bt_icon_layer;
static GBitmap *s_background_bitmap, *s_bt_icon_bitmap;

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
	
	// Find the width of the bar (total width = 114px)
	int width = (s_battery_level * 114) / 100;
	
	// Draw the background
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);
	
	// Draw the bar
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
	
}

// Set up Bluetooth service subscription
static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

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
	
	// Create GBitmap - this needs to appear before (under) the TextLayer
	s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
	
	// Create the Bluetooth icon GBitmap
	s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);

	// Create the BitmapLayer to display the GBitmap for bluetooth icon
	s_bt_icon_layer = bitmap_layer_create(GRect(59, 12, 30, 30));
	bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));
	
	// Create BitmapLayer to display the GBitmap for the background graphic
	s_background_layer = bitmap_layer_create(bounds);
	
	// Set the bitmap onto the layer and add to the window
	bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
	layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
	
	// Create the TextLayer with specific bounds
	s_time_layer = text_layer_create(
		GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
	
	// Create battery meter Layer
	s_battery_layer = layer_create(GRect(14, 54, 115, 2));
	layer_set_update_proc(s_battery_layer, battery_update_proc);
	
	// Improve the layout to be more like a watchface
	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_text_color(s_time_layer, GColorBlack);

  // Create GFont
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_48));

    // Apply the font to TextLayer
    text_layer_set_font(s_time_layer, s_time_font);
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	
	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
	
	// Add to Window
	layer_add_child(window_get_root_layer(window), s_battery_layer);
	
	// Show the correct state of the BT connection from the start
	bluetooth_callback(connection_service_peek_pebble_app_connection());
}

// handler function
static void main_window_unload(Window *window) {
	// Destroy TextLayer
	text_layer_destroy(s_time_layer);
	
	//Unload GFont
	fonts_unload_custom_font(s_time_font);
	
	// Destroy GBitmap
	gbitmap_destroy(s_background_bitmap);
	
	// Destroy BitmapLayer
	bitmap_layer_destroy(s_background_layer);
	
	// Destroy the battery layer
	layer_destroy(s_battery_layer);
	
	// Destroy bluetooth icon layer
	gbitmap_destroy(s_bt_icon_bitmap);
	bitmap_layer_destroy(s_bt_icon_layer);
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
	
	// Register for Bluetooth connection updates
	connection_service_subscribe((ConnectionHandlers) {
	  .pebble_app_connection_handler = bluetooth_callback
	});
	
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