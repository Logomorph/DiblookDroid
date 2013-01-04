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

#define  LOG_TAG    "diblook"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
} rgba;

void processingInvert(AndroidBitmapInfo &info,void *pixels);
void processingCanny(AndroidBitmapInfo &info,void *pixels);
void optimizedCanny(AndroidBitmapInfo &info,void *pixels);
void processingHough(AndroidBitmapInfo &info,void *pixels);

void drawLine(AndroidBitmapInfo &info, void *pixels, int start_x, int start_y, int end_x, int end_y);

#endif /* PROCESSING_H_ */
