/*
 * helpers.h
 *
 *  Created on: Jan 16, 2013
 *      Author: Logomorph
 */

#ifndef HELPERS_H_
#define HELPERS_H_

struct ImageRegion {
	uint8_t *image;
	int iW,iH;
	int left, top, right, bottom;

	ImageRegion(uint8_t *img, int w, int h, int l, int r, int t, int b) :
		image(img),
		iW(w),
		iH(h),
		left(l),
		right(r),
		top(t),
		bottom(b)
	{
	}
};

#endif /* HELPERS_H_ */
