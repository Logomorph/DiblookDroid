/*
 * processing.h
 *
 *  Created on: Nov 17, 2012
 *      Author: Logomorph
 */

#ifndef PROCESSING_H_
#define PROCESSING_H_

#include <jni.h>
#include <android/bitmap.h>
#include <android/log.h>
#include "queue"

#define  LOG_TAG    "diblook"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

using namespace std;

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
} rgba;

typedef struct Point{
	int x;
	int y;
	int value;  //pixel value

	Point() {
		this->x = 0;
		this->y = 0;
		this->value = 0;
	}

	Point(int x, int y, int value) {
		this->x = x;
		this->y = y;
		this->value = value;
	}

	void populate(int x, int y, int value) {
		this->x = x;
		this->y = y;
		this->value = value;
	}

} Point2D;

void processingInvert(AndroidBitmapInfo &info,void *pixels);
void processingCanny(AndroidBitmapInfo &info,void *pixels);
void optimizedCanny(AndroidBitmapInfo &info,void *pixels);
void processingHough(AndroidBitmapInfo &info,void *pixels);

/*
 * Helper functions for accessing pixels
 */
queue<Point2D> bfs(void* pixels, queue<Point2D> q, int x, int y, int pixelWidth, int stride);
void* getPixel(void* pixels,int dx, int dy, int stride, int width);

/*
 * Helper functions for copying the image
 */
void copyBufferToImage(AndroidBitmapInfo &info, void* pixels, uint8_t* buffer);
void copyImageToBuffer(AndroidBitmapInfo &info, void* pixels, uint8_t* buffer);

void drawLine(AndroidBitmapInfo &info, void *pixels, int start_x, int start_y, int end_x, int end_y);

#endif /* PROCESSING_H_ */
