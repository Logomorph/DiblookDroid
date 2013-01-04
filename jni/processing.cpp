#include "string"
#include "math.h"
#include <time.h>

#include "processing.h"
#include "heap.h"

#define kernelSize 5
#define p 0.1

void optimizedCanny(AndroidBitmapInfo &info, void *pixels) {
	time_t start,end;
	time (&start);
	LOGI("Applying Canny...");
	void *src = pixels;

	uint32_t w = info.width;
	void* lpSrc = pixels;
	rgba* line;		/* buffer for line */
	void* tempLine;

	//Gaussian filtering
	double sigma = 0.8;

	double *kernel_1D = new double[kernelSize];

	int x0 = kernelSize/2;
	int y0 = kernelSize/2;

	double sum = 0.0;

	for(int x = 0; x < kernelSize; x++) {
		kernel_1D[x] = (1.0/(2*3.14*sigma*sigma)) * exp(-((x-x0)*(x-x0))/(2.0*sigma*sigma));
		sum+=kernel_1D[x];
	}

	double value;
	int index;

	//general purpose reusables
	int *array_one = new int[info.height * info.width];
	int *array_two = new int[info.height * info.width];
	int *array_three = new int[info.height * info.width];
	int *array_four = new int[info.height * info.width];

	int *image = new int[info.height * info.width];
	LOGI("Start Gaussian Filtering");
	//I*G(X)
	for(int i = 0; i < info.height; i++) {

		line = (rgba *) lpSrc;
		for(int j = x0; j < info.width - x0; j++) {
			index = i*w+j;
			value = 0;
			for(int x = -x0; x <= x0; x++) {
				value += kernel_1D[x+x0] * line[j+x].red;
			}
			image[index] = line[j].red;
			array_one[index] = value/sum;
		}
		lpSrc = (char *) lpSrc + info.stride;
	}
	//re-initialize source pointer
	lpSrc = pixels;

	//I*G(Y)
	for(int i = x0; i < info.height - x0; i++) {
		for(int j = 0; j < info.width; j++) {
			value = 0;
			for(int x = -x0; x <= x0; x++) {
				value += kernel_1D[x+x0] * image[(i+x) * w + j];
			}
			array_two[i*w+j] = value/sum;
		}
	}
	LOGI("Start Sobel Convolution");
	//gradient magnitudes
	int sx[3][3] = {-1, 0, 1,
					-2, 0, 2,
					-1, 0, 1};

	int sy[3][3] = {-1, -2, -1,
					0, 0, 0,
					1, 2, 1};

	double valuex, valuey;

	for(int i = 1; i < info.height - 1; i++) {
		for(int j = 1; j < info.width - 1; j++) {
			index = i*w+j;
			valuex = 0;
			valuey = 0;
			for(int x = -1; x <= 1; x++) {
				for (int y = -1; y <= 1; y++) {
					valuex += sx[x+1][y+1] * array_two[(i + x) * w + j + y];
					valuey += sy[x+1][y+1] * array_two[(i + x) * w + j + y];
				}
			}

			array_three[index] = valuex; //igx
			array_four[index] = valuey;		 //igy

			//array_one[i*w+j] = sqrt(valuex*valuex + valuey*valuey)/(4*sqrt(2.0));
			//array_two[i*w+j] = atan2(valuey, valuex);
		}
	}

	LOGI("Start computing orientation");

	for(int i = 0; i < info.height; i++) {
		for(int j = 0; j < info.width; j++) {
			index = i*w+j;
			array_one[index] = sqrt(array_three[index]*array_three[index] + array_four[index]*array_four[index])/(4*sqrt(2.0));
			array_two[index] = atan2(array_four[index], array_three[index]);
		}
	}
	LOGI("Start non-maxima suppression");
	double pi = 4.0*atan(1.0);
	//non-maxima suppresion
	double pi8 = pi/8.0;
	double pi4 = pi/4.0;
	double pi2 = pi/2.0;
	int nrG0 = 0;	//used for adaptive thresholding

	int histogram[256];
	memset(histogram, 0, 256 * sizeof(int));
	/*
	 * array_one - magnitude
	 * array_two - orientation
	 * array_three - maximas
	 */
	for(int i = 1; i < info.height-1; i++) {
		for(int j = 1; j < info.width-1; j++) {

			index = i*w+j;

			//adaptive thresholding
			if (array_one[index] == 0) {
				nrG0++;
			}

			//compute maxima of current point
			if ((array_two[index] >= -pi8 && array_two[index] <= pi8) ||
				(array_two[index] >= pi - pi8 && array_two[index] <= -pi) ||
				(array_two[index] >= -pi && array_two[index] <= -pi + pi8)) {

				if (array_one[index] >= array_one[index+1] && array_one[index] >= array_one[index-1]) {
					array_three[index] = array_one[index];
				} else {
					array_three[index] = 0;
				}

			} else if ((array_two[index] >= pi8 && array_two[i*w+j] <= pi8 + pi4) ||
				(array_two[index] >= -pi + pi8 && array_two[index] <= -pi + pi8 + pi4)) {

					if (array_one[index] >= array_one[(i+1)*w+j+1] && array_one[index] >= array_one[(i-1)*w+j-1]) {
						array_three[index] = array_one[index];
					} else {
						array_three[index] = 0;
					}

			} else if ((array_two[index] >= pi2 - pi8 && array_two[index] <= pi2 + pi8) ||
				(array_two[index] >= -pi2 - pi8 && array_two[index] <= -pi2 + pi8)) {

					if (array_one[index] >= array_one[(i+1)*w+j] && array_one[index] >= array_one[(i-1)*w+j]) {
						array_three[index] = array_one[index];
					} else {
						array_three[index] = 0;
					}

			} else {
				if (array_one[index] >= array_one[(i+1)*w+j-1] && array_one[index] >= array_one[(i-1)*w+j+1]) {
					array_three[index] = array_one[index];
				} else {
					array_three[index] = 0;
				}
			}

			//add to histogram
			histogram[(int)array_three[index]]++;

		}
	}

	LOGI("Start adaptive thresholding");
	double nrNonEdgePixels;
	nrNonEdgePixels = (1-p)*((info.height - 2) * (info.width - 2) - histogram[0]);

	int sumPix = 0;
	int th = 0;

	while(sumPix < nrNonEdgePixels && th < 256) {
		th++;
		sumPix += histogram[th];
	}

	/*
	 * array_three - maxima
	 * array_four - thresh
	 */
	for(int i = 1; i < info.height - 1; i++) {
		line = (rgba *) pixels;

		for(int j = 1; j < info.width - 1; j++) {
			index = i*w+j;
			if (array_three[index] > th) {
				array_four[index] = 255;
			} else if (array_three[index] > 0.4 * th) {
				array_four[index] = 128;
			} else {
				array_four[index] = 0;
			}

			line[j].red = array_four[index];
			line[j].green = array_four[index];
			line[j].blue = array_four[index];
		}
		pixels = (char *) pixels + info.stride;
	}

	double dif;

	time (&end);
	dif = difftime (end,start);

	pixels = src;
	LOGI("Finished int %.2lf seconds", dif);


}
/*
 *	Applies the Canny edge detection filter on the image. It assumes a 8 bit grayscale image.
 */
void processingCanny(AndroidBitmapInfo &info, void *pixels) {
	LOGI("Applying Canny...");
	LOGI("Height: %d\nWidth: %d\n", info.height, info.width);

	uint32_t w = info.stride;
	uint32_t dwWidth = info.width;
	uint32_t dwHeight = info.height;
	void* lpSrc = pixels;
	rgba* line;		/* buffer for line */
	void* tempLine;

	//Gaussian filtering
	double sigma = 0.9;

	double *kernel_1D = new double[kernelSize];

	int x0 = kernelSize/2;
	int y0 = kernelSize/2;

	double sum = 0.0;

	for(int x = 0; x < kernelSize; x++) {
		kernel_1D[x] = (1.0/(2*3.14*sigma*sigma)) * exp(-((x-x0)*(x-x0))/(2.0*sigma*sigma));
		sum+=kernel_1D[x];
	}

	double value;

	//general purpose reusables
	/*double *array_one = new double[info.height * info.width];
	double *array_two = new double[info.height * info.width];
	double *array_three = new double[info.height * info.width];
	double *array_four = new double[info.height * info.width];*/
	double *gaussian = new double[info.height * w];
	double *image = new double[info.height * w];
	double *igx = new double[info.height * w];
	double *igy = new double[info.height * w];

	//I*G(X)
	for(int i = 0; i < info.height; i++) {
		line = (rgba *) lpSrc;
		for(int j = kernelSize/2; j < info.width - kernelSize/2; j++) {
			value = 0;
			for(int x = -kernelSize/2; x <= kernelSize/2; x++) {
				value += kernel_1D[x+kernelSize/2] * line[j+x].red;
			}
			image[i*w+j] = value/sum;
		}
		lpSrc = (char *) lpSrc + info.stride;
	}
	//re-initialize source pointer
	lpSrc = pixels;

	//I*G(Y)
	for(int i = kernelSize/2; i < info.height - kernelSize/2; i++) {
		for(int j = 0; j < info.width; j++) {
			value = 0;
			for(int x = -kernelSize/2; x <= kernelSize/2; x++) {
				value += kernel_1D[x+kernelSize/2] * image[(i+x) * w + j];
			}
			gaussian[i*w+j] = value/sum;
		}
	}
	//gradient magnitudes
	int sx[3][3] = {-1, 0, 1,
					-2, 0, 2,
					-1, 0, 1};

	int sy[3][3] = {-1, -2, -1,
					0, 0, 0,
					1, 2, 1};


	double valuex, valuey;

	for(int i = 1; i < dwHeight - 1; i++) {
		for(int j = 1; j < dwWidth - 1; j++) {
			valuex = 0;
			valuey = 0;
			for(int x = -1; x <= 1; x++) {
				for (int y = -1; y <= 1; y++) {
					valuex += sx[x+1][y+1] * gaussian[(i + x) * w + j + y];
					valuey += sy[x+1][y+1] * gaussian[(i + x) * w + j + y];
				}
			}

			igx[i * w + j] = valuex;
			igy[i*w+j] = valuey;
		}
	}
	free(gaussian);

	double *magn = new double[dwHeight * w];
	double *orient = new double[dwHeight * w];

	for(int i = 0; i < dwHeight; i++) {
		for(int j = 0; j < dwWidth; j++) {
			magn[i*w+j] = sqrt(igx[i*w+j]*igx[i*w+j] + igy[i*w+j]*igy[i*w+j])/(4*sqrt(2.0));
			orient[i*w+j] = atan2(igy[i*w+j], igx[i*w+j]);
		}
	}

	free(igx);
	free(igy);
	double *maxima = new double[dwHeight * w];

	double pi = 4.0*atan(1.0);
	//non-maxima suppresion
	double pi8 = pi/8.0;
	double pi4 = pi/4.0;
	double pi2 = pi/2.0;
	for(int i = 1; i < dwHeight-1; i++) {
		for(int j = 1; j < dwWidth-1; j++) {

			if ((orient[i*w+j] >= -pi8 && orient[i*w+j] <= pi8) ||
				(orient[i*w+j] >= pi - pi8 && orient[i*w+j] <= -pi) ||
				(orient[i*w+j] >= -pi && orient[i*w+j] <= -pi + pi8)) {

				if (magn[i*w+j] >= magn[i*w+j+1] && magn[i*w+j] >= magn[i*w+j-1]) {
					maxima[i*w+j] = magn[i*w+j];
				} else {
					maxima[i*w+j] = 0;
				}

			} else if ((orient[i*w+j] >= pi8 && orient[i*w+j] <= pi8 + pi4) ||
				(orient[i*w+j] >= -pi + pi8 && orient[i*w+j] <= -pi + pi8 + pi4)) {

					if (magn[i*w+j] >= magn[(i+1)*w+j+1] && magn[i*w+j] >= magn[(i-1)*w+j-1]) {
						maxima[i*w+j] = magn[i*w+j];
					} else {
						maxima[i*w+j] = 0;
					}

			} else if ((orient[i*w+j] >= pi2 - pi8 && orient[i*w+j] <= pi2 + pi8) ||
				(orient[i*w+j] >= -pi2 - pi8 && orient[i*w+j] <= -pi2 + pi8)) {

					if (magn[i*w+j] >= magn[(i+1)*w+j] && magn[i*w+j] >= magn[(i-1)*w+j]) {
						maxima[i*w+j] = magn[i*w+j];
					} else {
						maxima[i*w+j] = 0;
					}

			} else {
				if (magn[i*w+j] >= magn[(i+1)*w+j-1] && magn[i*w+j] >= magn[(i-1)*w+j+1]) {
					maxima[i*w+j] = magn[i*w+j];
				} else {
					maxima[i*w+j] = 0;
				}
			}
		}
	}
	//adaptive thresholding
	int nrG0 = 0;
	for(int i = 0; i < dwHeight; i++) {
		for(int j = 0; j < dwWidth; j++) {
			if (magn[i*w+j] == 0) nrG0++;
		}
	}

	/* Free variables */
	free(magn);
	free(orient);

	double nrNonEdgePixels;

	int histogram[256];

	for(int i = 0; i < 256; i++) {
		histogram[i] = 0;
	}
	LOGI("After maxing histogram 0");
	int current;
	for(int i = 1; i < dwHeight - 1; i++) {
		for(int j = 1; j < dwWidth - 1; j++) {
			current = (int)maxima[i*w+j];
			histogram[current]++;
		}
	}

	nrNonEdgePixels = (1-p)*((dwHeight - 2) * (dwWidth - 2) - histogram[0]);

	int sumPix = 0;
	int th = 0;

	while(sumPix < nrNonEdgePixels && th < 256) {
		th++;
		sumPix += histogram[th];
	}

	double *thresh = new double[dwHeight * w];
	for(int i = 1; i < dwHeight - 1; i++) {
		line = (rgba *) pixels;

		for(int j = 1; j < dwWidth - 1; j++) {
			if (maxima[i*w+j] > th) {
				thresh[i*w+j] = 255;
			} else if (maxima[i*w+j] > 0.4 * th) {
				thresh[i*w+j] = 128;
			} else {
				thresh[i*w+j] = 0;
			}

			line[j].red = thresh[i*w+j];
			line[j].green = thresh[i*w+j];
			line[j].blue = thresh[i*w+j];
		}
		pixels = (char *) pixels + info.stride;
	}
	free(maxima);
	free(thresh);


	LOGI("Done!");
}

void processingHough(AndroidBitmapInfo &info, void *pixels) {
	int **H; /* Hough accumulator*/
	int hmax;
	int diag;/* Diagonal of image */
	void *src = pixels;
	/* Compute diagonal */
	diag = sqrt((double)info.height*info.height + info.width*info.width);

	/* Initialize Hough Accumulator */
	H = new int*[diag];
	for(int i = 0; i < diag; i++) {
		H[i] = new int[360];
	}

	//memset(H, 0,diag*360*sizeof(int));
	for(int i = 0; i < diag; i++) {
		for(int j = 0; j < 360; j++) {
			H[i][j] = 0;
		}
	}


	hmax = 0;
	int rho = 0;
	double angle;
	rgba * line;
	/* Compute the Hough Accumulator */
	for(int i = 0; i < info.height; i++) {
		line = (rgba *) pixels;
		for(int j = 0; j < info.width; j++) {
			//if edge point, meaning it's not black
			if(line[j].red > 0) {
				for(int teta = 0; teta < 360; teta++) {
					angle = (teta*3.14)/180.0;
					rho = j*cos(angle) + i*sin(angle);
					if (rho >= 0) {
						H[rho][teta]++;
						if (H[rho][teta] > hmax) {
							hmax = H[rho][teta];
						}
					}
				}

			}
		}
		pixels = (char *) pixels + info.stride;
	}

	/* Non maxima suppression */
	int n = 5; //5, 7
	int m = (n-1)/2;
	int houghSz = diag * 360;

	int *lm = new int[houghSz];
	int *ro = new int[houghSz];
	int *teta = new int[houghSz];
	bool max = true;
	int nrMax = 0;

	Heap heap;
	heap.A = new houghLM[houghSz];
	heap.maxSize = houghSz;
	heap.size = 0;
	houghLM current;

	for(int i = m; i < diag - m; i++) {
		for(int j = m; j < 360 - m; j++) {
			max = true;
			for(int k = -m; k <= m; k++) {
				for(int l = -m; l <=m; l++) {
					if (H[i+k][j+l] > H[i][j]) {
						max = false;
					}
				}
			}

			if (max) {
				//if (H[i][j] > 100) {
					/*
					lm[nrMax] = H[i][j];
					ro[nrMax] = i;
					teta[nrMax] = j;*/
					nrMax++;
					current.lm = H[i][j];
					current.ro = i;
					current.teta = j;
					heapInsert(&heap,current);
				//}
			}

		}
	}

	//sort
	/*
	int aux;

	for(int i = 0; i < nrMax-1; i++) {
		for(int j = i+1; j < nrMax; j++) {
			if (lm[i] < lm[j]) {
				aux = lm[i];
				lm[i] = lm[j];
				lm[j] = aux;

				aux = ro[i];
				ro[i] = ro[j];
				ro[j] = aux;

				aux = teta[i];
				teta[i] = teta[j];
				teta[j] = aux;
			}
		}
	}*/
	pixels = src;
	int k = 20; /* Number of lines to display */
	//k = nrMax;

	double x0 = 0;
	double y0 = 0;
	double x1 = 0;
	double y1 = 0;

	for (int i = 0; i < k; i++) {
		current = popMaxFromHeap(&heap);
		/*
		x0 = 0;
		y0 = ro[i]/sin(teta[i]*3.14/180.0);
		x1 = info.width;
		y1 = (ro[i] - x1*cos(teta[i]*3.14/180.0))/sin(teta[i]*3.14/180.0);*/
		x0 = 0;
		y0 = current.ro/sin(current.teta*3.14/180.0);
		x1 = info.width;
		y1 = (current.ro - x1*cos(current.teta*3.14/180.0))/sin(current.teta*3.14/180.0);
		drawLine(info, pixels,x0,y0,x1,y1);
	}


}

void drawLine(AndroidBitmapInfo &info, void *pixels, int start_x, int start_y, int end_x, int end_y) {

	double m = ((double) end_y - start_y) / ((double) end_x - start_x); //slope
	double b = start_y - (double) m * start_x;

	void* imageData;
	rgba* current;
	//LOGI("Attempting to draw (%d,%d) to (%d,%d)", start_x,start_y,end_x, end_y);
	int y = 0;

	if (start_x > info.width) {
		start_x = info.width;
		start_y = m*start_x+b;
	}

	if (start_x < 0 ) {
		start_x = 0;
		start_y = m*start_x+b;
	}
/*
	if (start_y > info.height) {
		start_y = info.height;
		start_x = (start_y - b )/m;
	}

	if (start_y < 0 ) {
		start_y = 0;
		start_x = (start_y - b )/m;
	}*/

	if (end_x > info.width) {
		end_x = info.width;
		end_y = m*end_x+b;
	}

	if (end_x < 0 ) {
		end_x = 0;
		end_y = m*end_x+b;
	}
/*
	if (end_y > info.height) {
		end_y = info.height;
		end_x = (end_y - b )/m;
	}

	if (end_y < 0 ) {
		end_y = 0;
		end_x = (end_y - b )/m;
	}*/


	//LOGE("Final points (%d,%d) to (%d,%d)",start_x,start_y,end_x, end_y);
	if (start_x > end_x) {
		//LOGE("Failed to draw (%d,%d) to (%d,%d)",start_x,start_y,end_x, end_y);
	}

	for (int x = start_x; x < end_x; x++) {
		y = m * x + b;
		if (y < info.height && y >= 0) {
			imageData = (char*) pixels + x*sizeof(rgba) + y * info.stride;
			current = (rgba *) imageData;
			current->red = (uint8_t) 255;
			current->green = (uint8_t) 0;
			current->blue = (uint8_t) 0;
		}
	}

}


