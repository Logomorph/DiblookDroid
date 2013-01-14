#include "processing.h"
#include <stdlib.h>

//TODO add new operation here as well

extern "C" {
enum Operations {
	Invert = 0,
	Canny = 1,
	Canny_Hough = 2,
	Canny_Hough_Ransac = 3,
	Zebra_Crossing = 4
};
JNIEXPORT void JNICALL Java_com_awk_pics_MainActivity_processImage(JNIEnv * env,
		jobject obj, jobject bitmap, jlong operation) {
	AndroidBitmapInfo info;
	void* pixels;
	int ret;
	static int init;

	if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
		LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
		return;
	}
	if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
		LOGE("Bitmap format is not RGBA_8888 ! %c");
		return;
	}

	if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
		LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
	}

	/*if (operation == Invert) {
		processingInvert(info, pixels);
	}*/
	uint8_t* buffer;
	switch (operation) {
	case Invert:
		processingInvert(info, pixels);
		break;
	case Canny:
		buffer = new uint8_t[info.width * info.height];
		copyImageToBuffer(info, pixels,buffer);
		optimizedCanny(info, buffer);
		copyBufferToImage(info,pixels,buffer);
		break;
	case Canny_Hough:
		//TODO add calls to canny, hough, ransac and check for color switches along detected lines
		buffer = new uint8_t[info.width * info.height];
		copyImageToBuffer(info, pixels,buffer);
		optimizedCanny(info, buffer);
		processingHough(info, buffer);
		drawHoughLines(info, pixels);
		break;
	case Canny_Hough_Ransac:
		buffer = new uint8_t[info.width * info.height];
		copyImageToBuffer(info, pixels,buffer);
		optimizedCanny(info, buffer);
		processingHough(info, buffer);
		processingRansacZebra(info, pixels);
		break;
	case Zebra_Crossing:


		//TODO add calls to canny, hough, ransac and check for color switches along detected lines
		buffer = new uint8_t[info.width * info.height];
		copyImageToBuffer(info, pixels,buffer);
		optimizedCanny(info, buffer);
		processingHough(info, buffer);
		processingRansac(info, pixels);
		drawZebraCrossingFrame(info,pixels);
		//copyBufferToImage(info,pixels,buffer);
		break;
	}

	AndroidBitmap_unlockPixels(env, bitmap);
}
}
