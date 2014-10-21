#include <pebble.h>
#pragma once

Window *window;
BitmapLayer *face_layer;
TextLayer *time_layer;
Layer *outline_layer;
GBitmap *face_icon;

int randomTimer_time = 2100000;
AppTimer *randomTimer;
int pubhour = 0, face_invert = 0;
bool vibrated = false, wasCharging = false;