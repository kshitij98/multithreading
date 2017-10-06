#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

typedef struct Params {
	int param1;
	int param2;
} Params;

int n;
int *arr;


int insertionSort(int lo, int hi) {
	for (int i=lo ; i<=hi ; ++i) {
		for (int j=i-1 ; j>=lo ; --j) {
			if (arr[j+1] < arr[j]) {
				int temp = arr[j];
				arr[j] = arr[j+1];
				arr[j+1] = temp;
			}
		}
	}
}

void merge(int tempArray[], int lo, int mid, int hi) {
	int left = lo, right = mid + 1;
	int position = 0, k = hi - lo + 1;
	while(position < k) {
		if (left > mid)
			tempArray[position++] = arr[right++];
		else if (right > hi)
			tempArray[position++] = arr[left++];
		else if (arr[left] > arr[right])
			tempArray[position++] = arr[right++];
		else
			tempArray[position++] = arr[left++];
	}
}

void *mergeSort(void *params) {
	int lo = ((Params *)params) -> param1;
	int hi = ((Params *)params) -> param2;
	free(params);

	if (hi - lo + 1 <= 5)
		insertionSort(lo, hi);
	else {
		int mid = (lo + hi) >> 1;
		pthread_t leftChild, rightChild;

		Params *leftParams = (Params*) malloc(sizeof(Params));
		leftParams -> param1 = lo;
		leftParams -> param2 = mid;

		Params *rightParams = (Params*) malloc(sizeof(Params));
		rightParams -> param1 = mid + 1;
		rightParams -> param2 = hi;

		pthread_create(&leftChild, NULL, mergeSort, leftParams);
		pthread_create(&rightChild, NULL, mergeSort, rightParams);
		pthread_join(leftChild, NULL);
		pthread_join(rightChild, NULL);

		int *tempArray = calloc((hi - lo + 1), sizeof(int));
		merge(tempArray, lo, mid, hi);
		for (int i=lo ; i<=hi ; ++i)
			arr[i] = tempArray[i-lo];
		free(tempArray);
	}

	return NULL;
}

int main(int argc, char *argv[]) {
	clock_t startTime, endTime;

	scanf("%d", &n);

	arr = calloc((n + 1), sizeof(int));

	for (int i=0 ; i<n ; ++i)
		scanf(" %d", &arr[i]);

	Params* params = (Params*) malloc(sizeof(Params));
	params -> param1 = 0;
	params -> param2 = n - 1;

	startTime = clock();
	
	pthread_t thread;
	pthread_create(&thread, NULL, mergeSort, params);
	pthread_join(thread, NULL);
	
	endTime = clock();
	
	double timeSpent = (double)(endTime - startTime) / CLOCKS_PER_SEC;

	FILE* fp = fopen("mergeSort-thread_data.txt", "a");
	fprintf(fp, "%lf\n", timeSpent);
	fclose(fp);

	for (int i=0 ; i<n ; ++i)
		printf("%d ", arr[i]);
	printf("\n");

	return 0;
}
