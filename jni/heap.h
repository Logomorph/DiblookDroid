#ifndef HEAP_H_
#define HEAP_H_

#define PARENT(i) i/2
#define LEFT(i) 2*i
#define RIGHT(i) 2*i+1

struct houghLM {
	int lm; //localmaxim
	int ro;
	int teta;
};

struct Heap {
	houghLM* A;
	int size;
	int maxSize;
};

void swap(houghLM* a, houghLM* b);
void heapInsert(Heap* heap, houghLM value);
void maxHeapify(Heap* heap, int i);
houghLM popMaxFromHeap(Heap* heap);

#endif
