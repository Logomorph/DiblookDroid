#include "processing.h"
#include "heap.h"

/*
 *	Memcpy might work faster here.
 */
void swap(houghLM* a, houghLM* b) {
	houghLM aux;
	aux.lm = a->lm;
	aux.ro = a->ro;
	aux.teta = a->teta;

	a->lm = b->lm;
	a->ro = b->ro;
	a->teta = b->teta;

	b->lm = aux.lm;
	b->ro = aux.ro;
	b->teta = aux.teta;
}

void heapInsert(Heap* heap, houghLM value) {
	heap->size++;
	heap->A[heap->size] = value;
	int i = heap->size;

	//heap comparison condition on the local maxima value
	while ( i > 1 &&  heap->A[PARENT(i)].lm < heap->A[i].lm) {
		swap(&heap->A[i], &heap->A[PARENT(i)]);
		i = PARENT(i);
	}
}

void maxHeapify(Heap* heap, int i) {
	int l = LEFT(i);
	int r = RIGHT(i);
	int largest = -1;

	if (l <= heap->size && heap->A[l].lm > heap->A[i].lm) {
		largest = l;
	} else {
		largest = i;
	}
	if (r <= heap->size && heap->A[r].lm > heap->A[largest].lm) {
		largest = r;
	}
	if (largest != i) {
		swap(&heap->A[i],&heap->A[largest]);
		maxHeapify(heap, largest);
	}
}

houghLM popMaxFromHeap(Heap* heap) {
	houghLM	max;
	max.lm = heap->A[1].lm;
	max.ro = heap->A[1].ro;
	max.teta = heap->A[1].teta;

	swap(&heap->A[1],&heap->A[heap->size]);
	heap->size--;

	maxHeapify(heap,1);
	return max;
}
