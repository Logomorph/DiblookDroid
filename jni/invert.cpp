/*
 * invert.cpp
 *
 *  Created on: Nov 17, 2012
 *      Author: Logomorph
 */

#include "processing.h"
#include "string"

void processingInvert(AndroidBitmapInfo &info, void *pixels) {
	LOGI("Inverting...");
	for (int y = 0; y < info.height; y++) {
		rgba * line = (rgba *) pixels;
		for (int x = 0; x < info.width; x++) {
			line[x].red = (uint8_t) (255) - line[x].red;
			line[x].blue = (uint8_t) (255) - line[x].blue;
			line[x].green = (uint8_t) (255) - line[x].green;
		}
		pixels = (char *) pixels + info.stride;
	}

	LOGI("Done!");
}
