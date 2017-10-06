#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define BLUE_TEXT "\033[34m"
#define GREEN_TEXT "\033[32m"
#define BOLD_TEXT "\033[1m"
#define RESET_TEXT "\033[0m"
#define RED_TEXT "\033[31m"
#define YELLOW_TEXT "\033[33m"

int* sarr;


int insertionSort(int lo, int hi) {
	for (int i=lo ; i<=hi ; ++i) {
		for (int j=i-1 ; j>=lo ; --j) {
			if (sarr[j+1] < sarr[j]) {
				int temp = sarr[j];
				sarr[j] = sarr[j+1];
				sarr[j+1] = temp;
			}
		}
	}
}

void merge(int arr[], int lo, int mid, int hi) {
	int left = lo, right = mid + 1;
	int position = 0, k = hi - lo + 1;
	while(position < k) {
		if (left > mid)
			arr[position++] = sarr[right++];
		else if (right > hi)
			arr[position++] = sarr[left++];
		else if (sarr[left] > sarr[right])
			arr[position++] = sarr[right++];
		else
			arr[position++] = sarr[left++];
	}
}

void mergeSort(int lo, int hi) {
	if (hi - lo + 1 <= 5) {
		insertionSort(lo, hi);
		return;
	}

	int MID = (lo + hi) >> 1;
	pid_t pidLeft, pidRight;
	pidLeft = fork();

	if (pidLeft < 0) {
		fprintf(stderr, "%sfork() error: %s%s\n", RED_TEXT, strerror(errno), RESET_TEXT);
		exit(-1);
	}
	else if (pidLeft > 0) {
		pidRight = fork();
		if (pidRight < 0) {
			fprintf(stderr, "%sfork() error: %s%s\n", RED_TEXT, strerror(errno), RESET_TEXT);
			exit(-1);
		}
	}

	if (pidLeft == 0) {
		mergeSort(lo, MID);
		exit(0);
	}
	else if (pidRight == 0) {
		mergeSort(MID + 1, hi);
		exit(0);
	}
	else {
		int status_left;
		int status_right;
		waitpid(pidLeft, &status_left, 0);
		waitpid(pidRight, &status_right, 0);
		int *tempArray = calloc((hi - lo + 1), (sizeof(int)));
		
		merge(tempArray, lo, MID, hi);
		for (int i=lo ; i<=hi ; ++i)
			sarr[i] = tempArray[i-lo];
		free(tempArray);
	}
}

int main(int argc, char *argv[]) {
	clock_t startTime, endTime;

	int shmid, n, status = 0;

	scanf("%d", &n);

	if ((shmid = shmget(IPC_PRIVATE, sizeof(int) * (n + 1), IPC_CREAT | IPC_EXCL | 0644)) == -1) {
		fprintf(stderr, "%sshmget error: %s%s\n", RED_TEXT, strerror(errno), RESET_TEXT);
		exit(-1);
	}

	if ((sarr = shmat(shmid, 0, 0)) == NULL) {
		fprintf(stderr, "%sshmat error: %s%s\n", RED_TEXT, strerror(errno), RESET_TEXT);
		exit(-1);
	}

	for (int i=0 ; i<n ; ++i)
		scanf(" %d", &sarr[i]);

	pid_t pid = fork();

	if (pid < 0) {
		fprintf(stderr, "%sfork error: %s%s\n", RED_TEXT, strerror(errno), RESET_TEXT);
		exit(-1);
	}
	else if (pid == 0) {
		startTime = clock();
		mergeSort(0, n-1);
		endTime = clock();

		double timeSpent = (double)(endTime - startTime) / CLOCKS_PER_SEC;

		FILE* fp = fopen("mergeSort-fork_data.txt", "a");
		fprintf(fp, "%lf\n", timeSpent);
		fclose(fp);

		exit(0);
	}
	else {
		waitpid(pid, &status, 0);
	
		for (int i=0 ; i<n ; ++i)
			printf("%d ", sarr[i]);
		printf("\n");
	}

	return 0;
}
