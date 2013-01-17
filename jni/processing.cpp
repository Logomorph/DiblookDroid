#include "string"
#include <algorithm>
#include <math.h>
#include <time.h>
#include "queue"
#include <vector>
#include <stdlib.h>

#include "processing.h"
#include "heap.h"

#define kernelSize 5
#define p 0.1
#define WEAK_EDGE 128
#define STRONG_EDGE 255


#define houghDegreeNumber 360
#define houghIncrement 1
#define houghThreshold 8
#define ransacThreshold 30
#define zebraLines 5

#define PI 3.14159265

using namespace std;

bool showRansacLines = false;

int topY = -1;
int bottomY = -1;

//collection of the hough lines
vector<houghLM> hough_lines;

//region of interest boundaries, x is constant along the y axis
int region_top;
int region_bottom;

void drawLines(AndroidBitmapInfo &info, void* pixels, vector<houghLM> lines);


/*
 * Use this now
 */
void optimizedCanny(AndroidBitmapInfo &info, uint8_t *image) {
	timespec start, end;

	LOGI("Start Canny");
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	double value;
	int index;

	//general purpose reusables
	int *array_one = new int[info.height * info.width];
	int *array_two = new int[info.height * info.width];
	int *array_three = new int[info.height * info.width];
	int *array_four = new int[info.height * info.width];

	region_top = info.height / 2;
	region_bottom = info.height - 5;

	double valuex, valuey;
	int w = info.width;

	//store this so we don't need to compute it each time
	value = 4 * sqrt(2.0);

	for(int i = region_top; i < region_bottom - 1; i++) {

		for(int j = 1; j < info.width-1; j++) {
			index = i*w+j;

			valuex = 2 * (image[index+1] - image[index-1]) -
						  image[index-w-1] - image[index+w-1]
						  + image[index-w+1] + image[index+w+1];

			valuey =  2 * (image[index+w] - image[index-w]) -
					  image[index-w-1] - image[index-w+1]
					  + image[index+w-1] + image[index+w+1];

			array_one[index] = sqrt(valuex*valuex + valuey*valuey)/value;
			array_two[index] = atan2(valuey, valuex);

		}
	}

	LOGI("Start non-maxima suppression");
	double pi = 4.0*atan(1.0);

	/*
	 * Perform non maxima supression
	 */
	double pi8 = pi/8.0;
	double pi4 = pi/4.0;
	double pi2 = pi/2.0;
	double pi78 = pi - pi8;
	double pi38 = pi4 + pi8;
	double pi58 = pi - pi38;

	int nrG0 = 0;	//used for adaptive thresholding

	int histogram[256];
	memset(histogram, 0, 256 * sizeof(int));
	/*
	 * array_one - magnitude
	 * array_two - orientation
	 * array_three - maximas
	 */
	for(int i = region_top; i < region_bottom-1; i++) {
		for(int j = 1; j < info.width-1; j++) {

			index = i*w+j;

			//adaptive thresholding
			if (array_one[index] == 0) {
				nrG0++;
			}

			//compute maxima of current point
			if ((array_two[index] >= -pi8 && array_two[index] <= pi8) ||
				(array_two[index] >= pi78 && array_two[index] <= -pi) ||
				(array_two[index] >= -pi && array_two[index] <= -pi78)) {

				if (array_one[index] >= array_one[index+1] && array_one[index] >= array_one[index-1]) {
					array_three[index] = array_one[index];
				} else {
					array_three[index] = 0;
				}

			} else if ((array_two[index] >= pi8 && array_two[i*w+j] <= pi38) ||
				(array_two[index] >= -pi78 && array_two[index] <= -pi58)) {

					if (array_one[index] >= array_one[(i+1)*w+j+1] && array_one[index] >= array_one[(i-1)*w+j-1]) {
						array_three[index] = array_one[index];
					} else {
						array_three[index] = 0;
					}

			} else if ((array_two[index] >= pi38 && array_two[index] <= pi58) ||
				(array_two[index] >= -pi58 && array_two[index] <= -pi38)) {

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
	nrNonEdgePixels = (1-p)*((region_bottom - region_top - 2) * (info.width - 2) - histogram[0]);

	int sumPix = 0;
	int th = 0;

	while(sumPix < nrNonEdgePixels && th < 256) {
		th++;
		sumPix += histogram[th];
	}

	th*=1.8;

	/*
	 * array_three - maxima
	 * array_four - thresh
	 */
	for(int i = region_top; i < region_bottom ; i++) {

		for(int j = 0; j < info.width; j++) {
			index = i*w+j;

			if ( i == 0 || j == 0 || i == (info.height - 1) || j  == (info.width - 1)) {
				array_four[index] = 0;
			} else if (array_three[index] > th) {
				array_four[index] = 255;
			} else if (array_three[index] > 0.4 * th) {
				array_four[index] = 128;
			} else {
				array_four[index] = 0;
			}
		}
	}

	/*
	 *	Perform edge linking through hysteresis
	 */
	queue<Point2D> q;
	Point2D currentP;
	for (int i = region_top; i < region_bottom-1; i++) {
		for (int j = 1; j < info.width-1; j++) {

			index = j + i * info.width;
			image[index] = array_four[index];
			if (array_four[index] == STRONG_EDGE) {
				currentP.populate(j,i,array_four[index]);
				q.push(currentP);
				q = bfs(array_four,q,currentP.x, currentP.y,1, info.width);
			}
		}
	}

	/*
	 * Delete weak edges which are not linked
	 */

	for (int i = region_top; i < region_bottom; i++) {
		for (int j = 0; j < info.width; j++) {

			index = j + i * info.width;

			if (array_four[index] == WEAK_EDGE) {
				array_four[index] = 0;
			}

			image[index] = array_four[index];
		}
	}


	delete array_one;
	delete array_two;
	delete array_three;
	delete array_four;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
	LOGI("Canny done. Took: %ll", (end.tv_nsec - start.tv_nsec));
}

void processingRansac(AndroidBitmapInfo &info, void* pixels) {
	if(hough_lines.size() == 0) {
		LOGE("Run Hough before running RANSAC");
		return;
	}


	//drawLines(info, pixels, hough_lines);

	srand(time(NULL));

	int max_inter = 0;
	LOGI("No intersection %d", max_inter);
	int loops = 50;
	vector<int> indices;
	vector<int> max_indices;
	int x0,y0,x1,y1;

	while(loops > 0) {
		int i1 = rand() % hough_lines.size();
		int i2 = rand() % hough_lines.size();

		while(i2 == i1)
			i2 = rand() % hough_lines.size();

		indices.clear();

		Point2D intersection = intersectionOfLines(hough_lines[i1].ro, hough_lines[i1].teta, hough_lines[i2].ro, hough_lines[i2].teta);

		for(size_t i=0;i<hough_lines.size();i++) {

			const houghLM& line = hough_lines[i];
			if(isOnLine(intersection.x, intersection.y, line.ro, line.teta)) {
				indices.push_back(i);
			}

		}

		if((int)indices.size()>max_inter) {
			max_inter = (int)indices.size();
			max_indices.clear();
			max_indices = indices;
			LOGI("moar intersections! %d", max_inter);

		}

		if((int)max_indices.size() >= ransacThreshold ) {
			LOGI("Candidate found!");
			break;
		}

		indices.clear();
		loops--;
	}

	drawZebraCrossingFrame(info, pixels);

	LOGI("Intersections! %d", max_inter);
	for(size_t i =0;i< max_indices.size(); i++) {
		const houghLM &current = hough_lines[max_indices[i]];
		x0 = 0;
		y0 = current.ro/sin(current.teta*3.14/180.0);
		x1 = info.width;
		y1 = (current.ro - x1*cos(current.teta*3.14/180.0))/sin(current.teta*3.14/180.0);
		//LOGI("Attempting to draw %d %d %d %d", x0, y0, x1,y1);
		drawZebraEdge(info, pixels,x0,y0,x1,y1, Color::Red());
		//drawLineBressenham(info, pixels,x0,y0,x1,y1, Color::Green());
	}

	//drawLines(info, pixels, hough_lines);
	indices.clear();
}

void processingRansacZebra(AndroidBitmapInfo &info, void* pixels) {
	if(hough_lines.size() == 0) {
		LOGE("Run Hough before running RANSAC");
		return;
	}
	//drawLines(info, pixels, hough_lines);

	srand(time(NULL));

	int max_inter = 0;
	LOGI("No intersection %d", max_inter);
	int loops = 50;
	vector<int> indices;
	vector<int> max_indices;
	int x0,y0,x1,y1,i1,i2;

	while(loops > 0) {
		i1 = rand() % hough_lines.size();
		i2 = rand() % hough_lines.size();

		while(i2 == i1)
			i2 = rand() % hough_lines.size();

		indices.clear();

		Point2D intersection = intersectionOfLines(hough_lines[i1].ro, hough_lines[i1].teta, hough_lines[i2].ro, hough_lines[i2].teta);

		for(size_t i=0;i<hough_lines.size();i++) {

			const houghLM& line = hough_lines[i];
			if(isOnLine(intersection.x, intersection.y, line.ro, line.teta)) {
				indices.push_back(i);
			}

		}

		if((int)indices.size()>max_inter) {
			max_inter = (int)indices.size();
			max_indices.clear();
			max_indices = indices;
			LOGI("More intersections! %d", max_inter);

		}

		if((int)max_indices.size() >= ransacThreshold ) {
			LOGI("Candidate found!");
			break;
		}

		indices.clear();
		loops--;
	}

	LOGI("Intersections! %d", max_inter);
	for(size_t i =0;i< max_indices.size(); i++) {
		houghLM &current = hough_lines[max_indices[i]];
		x0 = 0;
		y0 = current.ro/sin(current.teta*3.14/180.0);
		x1 = info.width;
		y1 = (current.ro - x1*cos(current.teta*3.14/180.0))/sin(current.teta*3.14/180.0);
		//LOGI("Attempting to draw %d %d %d %d", x0, y0, x1,y1);
		//drawZebraEdge(info, pixels,x0,y0,x1,y1, Color::Green());
		drawLineBressenham(info, pixels,x0,y0,x1,y1, Color::Green());
	}

	indices.clear();
}
void processingHough(AndroidBitmapInfo &info, uint8_t *pixelsBuffer) {

	LOGI("Start Hough");
	float **H; /* Hough accumulator*/
	int hmax;
	int diag;/* Diagonal of image */
	/* Compute diagonal */
	diag = region_bottom - region_top;
	diag = sqrt((double)diag*diag + info.width*info.width);
	LOGI("Entering correct Hough. Diag is %d", diag);

	/* Initialize Hough Accumulator */
	H = new float*[diag];
	for(int i = 0; i < diag; i++) {
		H[i] = new float[houghDegreeNumber];
	}

	//memset(H, 0,diag*360*sizeof(int));
	for(int i = 0; i < diag; i++) {
		for(int j = 0; j < houghDegreeNumber; j++) {
			H[i][j] = 0;
		}
	}
	int index;

	hmax = 0;
	int rho = 0;
	double angle;
	rgba * line;
	/* Compute the Hough Accumulator */
	for(int i = region_top; i < region_bottom; i++) {
		for(int j = 0; j < info.width; j++) {
			//if edge point, meaning it's not black
			index = i * info.width + j;
			if(pixelsBuffer[index] > 0) {
				for(int teta = 0; teta < houghDegreeNumber; teta+=houghIncrement) {
					angle = (teta*3.14)/180.0;
					rho = j*cos(angle) + (i-region_top) *sin(angle);
					if (rho >= 0) {
						/*if (teta < 45 || teta > 315) {
							H[rho][teta]+=1 + cos(angle);
						} else {
							H[rho][teta]+=1;
						}*/
						H[rho][teta]+= cos(angle);
						if (H[rho][teta] > hmax) {
							hmax = H[rho][teta];
						}
					}
				}

			}
		}
	}

	/* Non maxima suppression */
	int n = 5; //5, 7
	int m = (n-1)/2;

	int houghSz = diag * houghDegreeNumber;

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
		for(int j = m; j < houghDegreeNumber - m; j++) {
			//check for a threshold
			if (H[i][j] > houghThreshold) {
				max = true;
				for(int k = -m; k <= m; k++) {
					for(int l = -m; l <=m; l++) {
						if (H[i+k][j+l] > H[i][j]) {
							max = false;
						}
					}
				}

				if (max) {
					nrMax++;
					current.lm = H[i][j];
					current.ro = i;
					current.teta = j;
					heapInsert(&heap,current);
				}
			}

		}
	}

	LOGI("Hough found %d local maximas.", nrMax);

	int k = 50; /* Number of lines to display */
	//k = nrMax;
	if (k > nrMax) {
		k = nrMax;
	}

	hough_lines.clear();

	for (int i = 0; i < k; i++) {
		current = popMaxFromHeap(&heap);

		hough_lines.push_back(current);
	}

	LOGI("End Hough");
}

void drawLines(AndroidBitmapInfo &info, void* pixels, vector<houghLM> lines) {
	houghLM current;
	int x0,y0,x1,y1;

	for (int i = 0; i < hough_lines.size(); i++) {
		current = lines.at(i);
		x0 = 0;
		y0 = current.ro/sin(current.teta*3.14/180.0);
		x1 = info.width;
		y1 = (current.ro - x1*cos(current.teta*3.14/180.0))/sin(current.teta*3.14/180.0);
		drawLineBressenham(info, pixels ,x0,y0,x1,y1, Color::Red());
	}
}

void drawHoughLines(AndroidBitmapInfo &info, void* pixels) {
	drawLines(info, pixels, hough_lines);
}

void drawZebraEdge(AndroidBitmapInfo &info, void* pixels, int start_x, int start_y, int end_x, int end_y, Color c) {
	start_y += region_top;
	end_y += region_top;
	int delta_x = end_x-start_x;
	delta_x = (delta_x > 0) ? delta_x : -delta_x;
	int delta_y = end_y-start_y;
	delta_y = (delta_y > 0) ? delta_y : -delta_y;


	int sign_x = (start_x - end_x > 0 ) ? -1 : 1;
	int sign_y = (start_y - end_y > 0 ) ? -1 : 1;
	int err = delta_x - delta_y;
	void* imageData;
	rgba* current;
	rgba* left1, *left2, *right1, *right2;
	int current_error;
	//LOGI("%d %d %d %d",start_x, end_x,  start_y, end_y);
	do {

		if (start_x < info.width-5 && start_x >= 5 && start_y < info.height && start_y >= region_top ) {
			imageData = (char*) pixels + start_x*sizeof(rgba) + start_y * info.stride;

			current = (rgba *) imageData;

			imageData = (char*) imageData +  sizeof(rgba);
			right1 = (rgba *) imageData;
			imageData = (char*) imageData +  3 * sizeof(rgba);
			right2 = (rgba *) imageData;

			imageData = (char*) imageData - 5 * sizeof(rgba);
			left1 = (rgba *) imageData;
			imageData = (char*) imageData - sizeof(rgba);
			left2 = (rgba *) imageData;

			//if (abs(left1->red -left2->red) < 30 && abs(right1->red -right2->red) < 30 )  {

			//if ((left2->red > 120 || right2->red > 120 ) &&   abs(left2->red - right2->red) > 50 ) {
			//if ((left2->red > 100 || right2->red > 100 ) &&   abs(left2->red - right2->red) > 30 ) {

			/*if (((left2->red > 120 && (left2->red - right2->red > 30 ))) ||
					((right2->red > 120 && (right2->red - left2->red > 30 )))) {
				*/
			if  (((abs(left2->red - right2->red) > 30 ) && ((left2->red > 120) || (right2->red > 120)))
			 && (start_y >= topY && start_y <= bottomY)) {
				current->red = (uint8_t) c.r;
				current->green = (uint8_t) c.g;
				current->blue = (uint8_t) c.b;

				}
			} else {
				if (showRansacLines) {
					current->red = (uint8_t) 0;
					current->green = (uint8_t) 255;
					current->blue = (uint8_t) 0;
				}
			}
		//}
		if (start_x == end_x && start_y == end_y) {
			break;
		}

		current_error = 2 * err;

		if (current_error > - delta_y) {
			err -= delta_y;
			start_x += sign_x;
		}

		if (current_error < delta_x) {
			err += delta_x;
			start_y += sign_y;
		}

	} while (true);
}

void drawLineBressenham(AndroidBitmapInfo &info, void *pixels, int start_x, int start_y, int end_x, int end_y, Color c) {
	//make up for roi
	start_y += region_top;
	end_y += region_top;
	int delta_x = end_x-start_x;
	delta_x = (delta_x > 0) ? delta_x : -delta_x;
	int delta_y = end_y-start_y;
	delta_y = (delta_y > 0) ? delta_y : -delta_y;

	int sign_x = (start_x - end_x > 0 ) ? -1 : 1;
	int sign_y = (start_y - end_y > 0 ) ? -1 : 1;
	int err = delta_x - delta_y;
	void* imageData;
	rgba* current;
	int current_error;
	//LOGI("%d %d %d %d",start_x, end_x,  start_y, end_y);
	do {


		if (start_x < info.width && start_x >= 0 && start_y < info.height && start_y >= 0 ) {
			imageData = (char*) pixels + start_x*sizeof(rgba) + start_y * info.stride;
			current = (rgba *) imageData;
			current->red = (uint8_t) c.r;
			current->green = (uint8_t) c.g;
			current->blue = (uint8_t) c.b;
		}
		if (start_x == end_x && start_y == end_y) {
			break;
		}

		current_error = 2 * err;

		if (current_error > - delta_y) {
			err -= delta_y;
			start_x += sign_x;
		}

		if (current_error < delta_x) {
			err += delta_x;
			start_y += sign_y;
		}

	} while (true);




}

queue<Point2D> bfs(int* pixels, queue<Point2D> q, int x, int y, int pixelWidth, int stride) {
	Point2D current = q.front();
	q.pop();
	Point2D neigh;

	int index;
	int pointValue;

	for (int dx = -1; dx <= 1; dx++) {
		for (int dy = -1; dy <= 1; dy++) {
			index = current.y + dy * stride + current.x + dx;
			pointValue = pixels[index];
			if (pointValue == WEAK_EDGE) {
				pixels[index] = STRONG_EDGE;
				neigh.populate(current.x+dx, current.y+dy, pointValue);
				q.push(neigh);
			}
		}
	}

	return q;
}

void* getPixel(void* pixels,int dx, int dy, int stride, int width) {
	return (char*) pixels + dx * width + dy * stride;
}

void copyImageToBuffer(AndroidBitmapInfo &info, void* pixels, uint8_t* buffer) {

	void* lpSrc = pixels;
	rgba * line;
	for (int y = 0; y < info.height; y++) {
		line = (rgba *) lpSrc;
		for (int x = 0; x < info.width; x++) {
			buffer[y*info.width+x] = line[x].red;
		}
		lpSrc = (char *) lpSrc + info.stride;
	}
}

void drawZebraCrossingFrame(AndroidBitmapInfo &info, void* pixels) {
	void* lpSrc = pixels;
	rgba * line;
	int count = 0;
	int mean = 0;
	int startingMean = 0;
	int zebraMean = 0;
	int lastLineMean = 0;
	bottomY = -1;
	topY = -1;

	int lineCount = 0;
	lpSrc = (char *) lpSrc + info.stride * (info.height-5);

	for (int y = info.height - 5; y >= region_top; y--) {
		line = (rgba *) lpSrc;
		count = 0;
		for (int x = 0; x < info.width; x++) {
			count += line[x].red;
		}
		if (lineCount > 3) {
			lastLineMean = mean;
			lineCount = 0;
		} else {
			lineCount++;
		}

		mean = count / info.width;
		if (startingMean == 0) {
			//this should happen on the first line
			startingMean = mean;
			lastLineMean = mean;
			LOGI("First Line");
		} else if (bottomY < 0 ) {
			if (abs(mean - lastLineMean) > 30) {
				bottomY = y;
				zebraMean = mean;
				lastLineMean = mean;
			}
			LOGI("Other");
		} else if (abs(mean - lastLineMean) > 20 ) {
			LOGI("Done");
			topY = y;
			break;
		}
		LOGI("Mean on line %d is %d", y, mean);

		lpSrc = (char *) lpSrc - info.stride;
	}


	LOGI("%d %d",topY ,bottomY);
	drawLineBressenham(info, pixels,0, topY - region_top, info.width - 1, topY- region_top, Color::Red());
	drawLineBressenham(info, pixels,0, bottomY- region_top, info.width - 1, bottomY- region_top, Color::Red());

}

void cleanPixels(AndroidBitmapInfo &info, void* pixels) {
	void* lpSrc = pixels;
	rgba * line;
	int count = 0;
	int lineCount = 0;
	int distance = 20;

	lpSrc = (char *) lpSrc + info.stride * (info.height-5);

	Color c = Color::Red();

	for (int y = bottomY; y >= topY; y--) {
		line = (rgba *) lpSrc;
		count = 0;

		distance = 0;
		for (int x = 0; x < info.width; x++) {
			if (line[x].blue == 255 ) {
				line[x].red = 255;
				line[x].red = 0;
				line[x].red = 0;
			}
		}

		lpSrc = (char *) lpSrc - info.stride;
	}
}

void copyBufferToImage(AndroidBitmapInfo &info, void* pixels, uint8_t* buffer) {
	int index;
	void* lpSrc = pixels;
	for (int y = 0; y < info.height; y++) {
		rgba * line = (rgba *) lpSrc;
		for (int x = 0; x < info.width; x++) {
			index = y*info.width+x;

			line[x].red = buffer[index];
			line[x].green = buffer[index];
			line[x].blue = buffer[index];
		}
		lpSrc = (char *) lpSrc + info.stride;
	}
}

/*
 * The intersection of two lines defined by (ro1, teta1), (ro2, teta2) is the
 * solution of system of equations:
 *
 * ro1 = x * cos(teta1) + y * sin(teta1)
 * ro2 = x * cos(teta2) + y * cos(teta2)
 *
 * Multiplication by PI/180 necesary because sin and cos work with radians.
 *
 * Hopefully casting to int will be ok.
 */
Point2D intersectionOfLines(float ro1, float teta1, float ro2, float teta2) {
	Point2D point;

	point.x = (1.0 * ro1 * sin (teta2 * PI/180) - 1.0 * ro2 * sin(teta1* PI/180))/sin((teta2-teta1) * PI/180);
	point.y = (1.0 * ro1 * cos (teta2 * PI/180) - 1.0 * ro2 * cos(teta1* PI/180))/sin((teta1-teta2) * PI/180);

	return point;
}

/*
 * Return true if the point is on straight line defined by the parameters ro and teta.
 *
 * Multiplication by PI/180 necesary because sin and cos work with radians.
 */

bool isOnLine(int x, int y, float ro, float teta) {
	float buffer = x * cos(teta * PI/180) + y * sin (teta * PI/180);
	return (abs((long) (buffer - ro)) >= 0 && abs((long)(buffer - ro)) <= 5);
	//return (ro == buffer)
}


void drawTestLines(AndroidBitmapInfo &info, void* pixels) {
	hough_lines.clear();
	houghLM current;
	current.ro = 100;
	current.teta = 135;
	hough_lines.push_back(current);

	current.ro = 100;
	current.teta = 0;
	hough_lines.push_back(current);

	/*
	current.ro = 100;
	current.teta = 60;
	hough_lines.push_back(current);

	current.ro = 100;
	current.teta = 135;
	hough_lines.push_back(current);
	*/
	drawLines(info,pixels,hough_lines);
	hough_lines.clear();
}


