#include <pebble.h>
#include "main.h"
void refresh(int override);

AppTimer *defaultTimer;

TextLayer* text_layer_init(GRect location, bool is_date)
{
	TextLayer *layer = text_layer_create(location);
	text_layer_set_text_color(layer, GColorWhite);
	text_layer_set_background_color(layer, GColorBlack);
	text_layer_set_text_alignment(layer, GTextAlignmentCenter);
	text_layer_set_font(layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMPACT_32)));
	if(is_date){
		text_layer_set_font(layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	}
	return layer;
}

struct tm *get_tm(){
	struct tm *t;
  	time_t temp;        
  	temp = time(NULL);        
  	t = localtime(&temp);
	
	return t;
}

void default_callback(){
	refresh(2);
}

void random_timer_callback(){
	defaultTimer = app_timer_register(240000, default_callback, NULL);
	randomTimer = app_timer_register(randomTimer_time, random_timer_callback, NULL);
}

void outline_proc(Layer *l, GContext *ctx){
	graphics_draw_rect(ctx, GRect(0, 0, 144, 168));
	graphics_draw_rect(ctx, GRect(1, 1, 142, 166));
	//top left corner rounding
	graphics_draw_pixel(ctx, GPoint(2, 2));
	graphics_draw_pixel(ctx, GPoint(3, 2));
	graphics_draw_pixel(ctx, GPoint(2, 3));
	//top right corner rounding
	graphics_draw_pixel(ctx, GPoint(141, 2));
	graphics_draw_pixel(ctx, GPoint(140, 2));
	graphics_draw_pixel(ctx, GPoint(141, 3));
	//bottom right
	graphics_draw_pixel(ctx, GPoint(141, 165));
	graphics_draw_pixel(ctx, GPoint(141, 164));
	graphics_draw_pixel(ctx, GPoint(140, 165));
	//bottom left
	graphics_draw_pixel(ctx, GPoint(2, 165));
	graphics_draw_pixel(ctx, GPoint(2, 164));
	graphics_draw_pixel(ctx, GPoint(3, 165));

	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_draw_pixel(ctx, GPoint(100, 150));
	graphics_draw_pixel(ctx, GPoint(101, 150));
	graphics_draw_pixel(ctx, GPoint(100, 151));
}
	
uint32_t calculate_happiness(){
	if(!bluetooth_connection_service_peek()){
		return RESOURCE_ID_IMAGE_BTDISC;
	}

	BatteryChargeState state = battery_state_service_peek();
	if(state.is_charging){
		return RESOURCE_ID_IMAGE_CHARGING;
	}

	if(state.charge_percent == 100){
		if(state.is_plugged || state.is_charging){
			return RESOURCE_ID_IMAGE_FULLY_CHARGED;
		}
	}

	if(!state.is_charging &&  state.charge_percent > 80 && state.charge_percent < 110){
		return RESOURCE_ID_IMAGE_FULLNHAPPY;
	}

	struct tm *t;
	t = get_tm();
	if(t->tm_hour > 0 && t->tm_hour < 7){
		return RESOURCE_ID_IMAGE_SLEEPING;
	}

	return RESOURCE_ID_IMAGE_DEFAULT1;
}

void refresh(int override){
	gbitmap_destroy(face_icon);
	if(override == 0){
		face_icon = gbitmap_create_with_resource(calculate_happiness());
	}
	else if(override == 2){
		face_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DEFAULT1);
	}
	else{
		int toPick = rand() % 3;
		switch(toPick){
			case 0:
				face_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RANDOM1);
				break;
			case 1:
				face_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RANDOM2);
				break;
			case 2:
				face_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RANDOM3);
				break;
			default:
				face_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RANDOM1);
				break;
		}
	}
	bitmap_layer_set_bitmap(face_layer, face_icon);
	layer_mark_dirty(bitmap_layer_get_layer(face_layer));
}

void tick_handler(struct tm *t, TimeUnits units){
	pubhour = t->tm_hour;

	static char buffer[] = "00 00";
	if(clock_is_24h_style()){
		strftime(buffer, sizeof(buffer), "%H:%M", t);
   	}
   	else{
		strftime(buffer,sizeof(buffer), "%I:%M", t);
	}
	text_layer_set_text(time_layer, buffer);

	refresh(0);
}

void bt_handler(bool connected){
	if(!connected){
		vibes_short_pulse();
	}
	refresh(0);
}

void battery_handler(BatteryChargeState state){
	if(state.is_charging){
		if(!vibrated){
			vibes_double_pulse();
			vibrated = true;
		}
		wasCharging = true;
	}
	else{
		if(wasCharging){
			gbitmap_destroy(face_icon);
			face_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UPWC);
			bitmap_layer_set_bitmap(face_layer, face_icon);
			layer_mark_dirty(bitmap_layer_get_layer(face_layer));
			wasCharging = false;
			return;
		}
		vibrated = false;
	}
	refresh(0);
}

void shake_handler(AccelAxisType axis, int32_t direction){
	refresh(2);
}

void window_load(Window *window){	
	Layer *window_layer = window_get_root_layer(window);

	face_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
	bitmap_layer_set_bitmap(face_layer, face_icon);
	layer_add_child(window_layer, bitmap_layer_get_layer(face_layer));

	time_layer = text_layer_init(GRect(100, 150, 44, 18), true);
	layer_add_child(window_layer, text_layer_get_layer(time_layer));

	outline_layer = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(outline_layer, outline_proc);
	layer_add_child(window_layer, outline_layer);

	randomTimer = app_timer_register(randomTimer_time, random_timer_callback, NULL);
		
	srand(time(NULL));

	struct tm *t = get_tm();
	tick_handler(t, MINUTE_UNIT);
}

void window_unload(Window *window){
	bitmap_layer_destroy(face_layer);
	gbitmap_destroy(face_icon);
	text_layer_destroy(time_layer);
}

void init(){
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers){
		.load = window_load,
		.unload = window_unload,
	});
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	bluetooth_connection_service_subscribe(bt_handler);
	battery_state_service_subscribe(battery_handler);
	accel_tap_service_subscribe(shake_handler);
	
	window_stack_push(window, true);
}

void deinit(){
	window_destroy(window);
}

int main(){
	init();
	app_event_loop();
	deinit();
}