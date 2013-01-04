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

	switch (operation) {
	case Invert:
		processingInvert(info, pixels);
		break;
	case Canny:
		//processingCanny(info, pixels);
		optimizedCanny(info, pixels);
		break;
	case Canny_Hough:
		//drawLine(info, pixels, 0,0,400,400);
		//optimizedCanny(info, pixels);
		processingHough(info, pixels);
		break;
	case Canny_Hough_Ransac:
		drawLine(info, pixels, 0,0,102,100);
		break;
	case Zebra_Crossing:
		//TODO add calls to canny, hough, ransac and check for color switches along detected lines
		break;

	}

	AndroidBitmap_unlockPixels(env, bitmap);
}
}
