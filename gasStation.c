#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define STATION_CAPACITY 7
#define ATTENDERS 3
#define MAX 100
#define LOCK pthread_mutex_lock
#define UNLOCK pthread_mutex_unlock
#define SIGNAL pthread_cond_signal
#define WAIT pthread_cond_wait
#define BLUE_TEXT "\033[34m"
#define GREEN_TEXT "\033[32m"
#define BOLD_TEXT "\033[1m"
#define RESET_TEXT "\033[0m"
#define RED_TEXT "\033[31m"
#define YELLOW_TEXT "\033[33m"

typedef struct Attender{
	pthread_t tid;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	int isWorking;
	int idx;
} Attender;

typedef struct Car{
	pthread_t tid;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	int attenderId;
	int idx;
	int jobId;
} Car;

typedef struct node{
	struct node *next;
	int val;
} node;

struct node *head;
pthread_mutex_t accessQueue;
pthread_mutex_t accessCars;
pthread_mutex_t accessATM;
int totalCars;
Car car[MAX];
Attender attender[MAX];

void addToQueue(int val) {
	LOCK(&accessQueue);
		struct node *newnode = (struct node*) malloc(sizeof(node));
		newnode -> val = val;
		newnode -> next = NULL;
		if (head == NULL)
			head = newnode;
		else {
			struct node* curr = head;
			while (curr -> next != NULL)
				curr = curr -> next;
			curr -> next = newnode;

		}
	UNLOCK(&accessQueue);
}

int popfromQueue() {
	LOCK(&accessQueue);
		if (head == NULL) {
			UNLOCK(&accessQueue);
			return -1;
		}
		node *ret = head;
		head = head -> next;
	UNLOCK(&accessQueue);
	return ret -> val;
}

int enterStation() {
	int entered = 0;
	LOCK(&accessCars);
		if (totalCars < STATION_CAPACITY) {
			++totalCars;
			entered = 1;
		}
	UNLOCK(&accessCars);

	return entered;
}

void waitInLine(int idx) {
	LOCK(&(car[idx].lock));
		addToQueue(idx);
		fprintf(stderr, "%sCar %d is waiting%s\n", YELLOW_TEXT, idx + 1, RESET_TEXT);

		WAIT(&(car[idx].cond), &(car[idx].lock));
	UNLOCK(&(car[idx].lock));
}

void goToPump(int idx) {
	car[idx].jobId = 1;

	LOCK(&(attender[car[idx].attenderId].lock));
	LOCK(&(car[idx].lock));
	SIGNAL(&(attender[car[idx].attenderId].cond));
	UNLOCK(&(attender[car[idx].attenderId].lock));
	WAIT(&(car[idx].cond), &(car[idx].lock));
	UNLOCK(&(car[idx].lock));
}

void serveCar(int attenderId, int carId) {
	fprintf(stderr, "%sAttender %d is serving Car %d%s\n", BLUE_TEXT, attenderId + 1, carId + 1, RESET_TEXT);
	sleep(1);
	fprintf(stderr, "%sAttender %d served Car %d%s\n", GREEN_TEXT, attenderId + 1, carId + 1, RESET_TEXT);
}

void pay(int idx) {
	car[idx].jobId = 2;

	LOCK(&(attender[car[idx].attenderId].lock));
	LOCK(&(car[idx].lock));
	SIGNAL(&(attender[car[idx].attenderId].cond));
	UNLOCK(&(attender[car[idx].attenderId].lock));

	WAIT(&(car[idx].cond), &(car[idx].lock));
	UNLOCK(&(car[idx].lock));
}

void acceptPayment(int attenderId, int carId) {
	LOCK(&(accessATM));	
	fprintf(stderr, "%sAttender %d is accepting payment of Car %d%s\n", BLUE_TEXT, attenderId + 1, carId + 1, RESET_TEXT);
	sleep(1);
	fprintf(stderr, "%sAttender %d accepted payment of Car %d%s\n", GREEN_TEXT, attenderId + 1, carId + 1, RESET_TEXT);
	UNLOCK(&(accessATM));	
}

void exitStation(int idx) {
	LOCK(&accessCars);
		fprintf(stderr, "%s%sCar %d is exiting the station%s\n", BOLD_TEXT, GREEN_TEXT, idx + 1, RESET_TEXT);
		--totalCars;
	UNLOCK(&accessCars);
}

void *newCar(void *arg) {
	int idx = *((int *)arg);
	if (enterStation()) {
		fprintf(stderr, "%s%sCar %d entered the station%s\n", BOLD_TEXT, BLUE_TEXT, idx + 1, RESET_TEXT);
		waitInLine(idx);
		goToPump(idx);
		waitInLine(idx);
		pay(idx);
		exitStation(idx);
	}
	else
		printf("%s%sCar %d could not enter the station as it is full.%s\n", BOLD_TEXT, RED_TEXT, idx + 1, RESET_TEXT);

	return 0;
}

void *newAttender(void *arg) {
	int idx = *((int *)arg);
	attender[idx].isWorking = 1;

	while (attender[idx].isWorking) {
		int carId = popfromQueue();
		if (carId == -1)
			continue;
		// fprintf(stderr, "Car %d waited for attender %d\n", carId + 1, idx + 1);	
		car[carId].attenderId = idx;
		LOCK(&(car[carId].lock));
		LOCK(&(attender[idx].lock));
		SIGNAL(&(car[carId].cond));
		UNLOCK(&(car[carId].lock));
		WAIT(&(attender[idx].cond), &(attender[idx].lock));
		UNLOCK(&(attender[idx].lock));

		if (attender[idx].isWorking) {
			switch(car[carId].jobId) {
				case 1: serveCar(idx, carId);
								break;
				case 2: acceptPayment(idx, carId);
								break;
				default: fprintf(stderr, "%sJob for Job ID %d does not exist.%s\n", RED_TEXT, car[carId].jobId, RESET_TEXT);
			}
		}
		LOCK(&(car[carId].lock));
			SIGNAL(&(car[carId].cond));
		UNLOCK(&(car[carId].lock));
	}

	return 0;
}

int main() {
	srand(time(NULL));
	int n;

	scanf("%d", &n);

	if (pthread_mutex_init(&accessQueue, NULL) != 0)
		fprintf(stderr, "mutex init has failed\n");
	if (pthread_mutex_init(&accessCars, NULL) != 0)
		fprintf(stderr, "mutex init has failed\n");
	if (pthread_mutex_init(&accessATM, NULL) != 0)
		fprintf(stderr, "mutex init has failed\n");

	for (int i=0 ; i<ATTENDERS ; ++i) {
		attender[i].idx = i;
		if (pthread_mutex_init(&attender[i].lock, NULL) != 0)
			fprintf(stderr, "mutex init has failed\n");
		pthread_create(&(attender[i].tid), NULL, &newAttender, (void *)(&attender[i].idx));
	}

	for (int i=0 ; i<n ; ++i) {
		car[i].idx = i;
		sleep(rand() % 3);
		fprintf(stderr, "%sCar %d arrived%s\n", BLUE_TEXT, i+1, RESET_TEXT);
		if (pthread_mutex_init(&car[i].lock, NULL) != 0)
			fprintf(stderr, "mutex init has failed\n");
		pthread_create(&(car[i].tid), NULL, &newCar, (void *)(&car[i].idx));
	}

	for (int i=0 ; i<n ; ++i)
		pthread_join(car[i].tid, NULL);
	
	for (int i=0 ; i<ATTENDERS ; ++i) {
		attender[i].isWorking = 0;
		SIGNAL(&(attender[i].cond));
		pthread_join(attender[i].tid, NULL);
		fprintf(stderr, "%sAttender %d stopped working%s\n", YELLOW_TEXT, i+1, RESET_TEXT);
	}

	return 0;
}