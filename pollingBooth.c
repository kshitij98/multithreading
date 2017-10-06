#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

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

// waitToGetFull -> polling_ready_evm;


typedef struct node {
	struct node *next;
	int val;
} node;

typedef struct EVM {
	pthread_t tid;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	pthread_mutex_t accessSlot;
	int idx;
	int boothId;
	int count;
	int slot;
} EVM;

typedef struct Voter {
	pthread_t tid;
	int idx;
	int boothId;
} Voter;

typedef struct Booth {
	pthread_t tid;
	int idx;
	pthread_mutex_t accessVoting;
	pthread_mutex_t accessVoters;
	pthread_mutex_t accessQueue;
	int voters;
	int evms;
	int votersLeft;
	struct node *freeEVMs;
	EVM evm[MAX];
	Voter voter[MAX];
} Booth;

typedef struct Params {
	int id1, id2;
} Params;

Booth booth[MAX];

int start_voting_phase(int boothId, int voters, int evmId) {
	LOCK(&(booth[boothId].accessVoting));
	printf("%s%sEVM %d at Booth %d is moving for voting phase%s\n", BOLD_TEXT, BLUE_TEXT, evmId + 1, boothId + 1, RESET_TEXT);
		sleep(0.5 * voters);
	printf("%s%sEVM %d at Booth %d finished voting phase%s\n", BOLD_TEXT, GREEN_TEXT, evmId + 1, boothId + 1, RESET_TEXT);
	UNLOCK(&(booth[boothId].accessVoting));
}

int popFromQueue(int boothId) {
	int evmId = -1;
	LOCK(&(booth[boothId].accessQueue));
		node *head = booth[boothId].freeEVMs;
		if (head == NULL);
		else {
			evmId = head -> val;
			if (booth[boothId].evm[evmId].count <= 1) {
				node *temp = head;
				booth[boothId].freeEVMs = head -> next;
				free(head);
			}
			else
				booth[boothId].evm[evmId].count -= 1;
		}
	UNLOCK(&(booth[boothId].accessQueue));
	return evmId;
}

int addToEVMQueue(int idx, int evmId, int count) {
	LOCK(&(booth[idx].accessQueue));
		booth[idx].evm[evmId].count = count;
		node *head = booth[idx].freeEVMs;
		node *newnode = (struct node*) malloc(sizeof(node));
	
		newnode -> val = evmId;
		newnode -> next = NULL;

		if (head == NULL)
			booth[idx].freeEVMs = newnode;
		else {
			node *curr = head;
			while (curr -> next != NULL)
				curr = curr -> next;
			curr -> next = newnode;
		}

	UNLOCK(&(booth[idx].accessQueue));
}

int assignEVM(int boothId) {
	int assignedEVM = -1;
	while (assignedEVM == -1)
		assignedEVM = popFromQueue(boothId);

	return assignedEVM;
}

int voter_in_slot(int boothId, int voterId, int evmId) {
	int ret = 0;
	LOCK(&(booth[boothId].accessVoters));
		booth[boothId].votersLeft--;
		ret = booth[boothId].votersLeft == 0;
	UNLOCK(&(booth[boothId].accessVoters));

	LOCK(&(booth[boothId].evm[evmId].accessSlot));
		booth[boothId].evm[evmId].slot--;
		ret = ret | (booth[boothId].evm[evmId].slot == 0);
	UNLOCK(&(booth[boothId].evm[evmId].accessSlot));
	printf("%sVoter %d at Booth %d got allocated EVM %d.%s\n", GREEN_TEXT, voterId + 1, boothId + 1, evmId + 1, RESET_TEXT);
	return ret;
}

void *newVoter(void *params) {
	int boothId = ((Params *)params) -> id1;
	int voterId = ((Params *)params) -> id2;

	int evmId = assignEVM(boothId);

	int slotFull = voter_in_slot(boothId, voterId, evmId);

	if (slotFull) {
		LOCK(&(booth[boothId].evm[evmId].lock));
		SIGNAL(&(booth[boothId].evm[evmId].cond));
		UNLOCK(&(booth[boothId].evm[evmId].lock));
	}
}

int waitToGetFull(int boothId, int evmId, int count) {
	WAIT(&(booth[boothId].evm[evmId].cond), &(booth[boothId].evm[evmId].lock));
	UNLOCK(&(booth[boothId].evm[evmId].lock));
}

void *newEVM(void *params) {
	int boothId = ((Params *)params) -> id1;
	int evmId = ((Params *)params) -> id2;

	if (pthread_mutex_init(&booth[boothId].evm[evmId].lock, NULL) != 0)
		printf("mutex init has failed\n");
	if (pthread_mutex_init(&booth[boothId].evm[evmId].accessSlot, NULL) != 0)
		printf("mutex init has failed\n");

	while (booth[boothId].votersLeft) {
		int count = (rand() % 10) + 1;
		LOCK(&(booth[boothId].evm[evmId].lock));
		LOCK(&(booth[boothId].evm[evmId].accessSlot));
			booth[boothId].evm[evmId].slot = count;			
		UNLOCK(&(booth[boothId].evm[evmId].accessSlot));

		printf("%sEVM %d at Booth %d is free with slots = %d.%s\n", YELLOW_TEXT, evmId + 1, boothId + 1, count, RESET_TEXT);
		addToEVMQueue(boothId, evmId, count);
		waitToGetFull(boothId, evmId, count);
		
		int noVoters = 0;
		LOCK(&(booth[boothId].evm[evmId].accessSlot));
			noVoters = booth[boothId].evm[evmId].slot == count;
		UNLOCK(&(booth[boothId].evm[evmId].accessSlot));
		if (!noVoters)
			start_voting_phase(boothId, count, evmId);
	}
}

void *newBooth(void *arg) {
	int idx = *((int *) arg);
	int voters = booth[idx].voters;
	int evms = booth[idx].evms;
	booth[idx].votersLeft = voters;
	booth[idx].freeEVMs = NULL;
	if (pthread_mutex_init(&booth[idx].accessVoters, NULL) != 0)
		printf("mutex init has failed\n");

	for (int i=0 ; i<evms ; ++i) {
		booth[idx].evm[i].idx = i;
		booth[idx].evm[i].boothId = idx;

		Params *params = (struct Params*) malloc(sizeof(Params));
		params -> id1 = idx;
		params -> id2 = i;

		pthread_create(&(booth[idx].evm[i].tid), NULL, &newEVM, (void *)params);
	}

	for (int i=0 ; i<voters ; ++i) {
		booth[idx].voter[i].boothId = idx;
		booth[idx].voter[i].idx = i;

		Params *params = (struct Params*) malloc(sizeof(Params));
		params -> id1 = idx;
		params -> id2 = i;

		pthread_create(&(booth[idx].voter[i].tid), NULL, &newVoter, (void *)params);
	}

	for (int i=0 ; i<voters ; ++i)
		pthread_join(booth[idx].voter[i].tid, NULL);

	for (int i=0 ; i<evms ; ++i) {
		LOCK(&(booth[idx].evm[i].lock));
		SIGNAL(&(booth[idx].evm[i].cond));
		UNLOCK(&(booth[idx].evm[i].lock));
		pthread_join(booth[idx].evm[i].tid, NULL);
	}

	return 0;
}

int main() {
	int n;

	scanf(" %d", &n);

	for (int i=0 ; i<n ; ++i) {
		booth[i].idx = i;
		if (pthread_mutex_init(&booth[i].accessVoting, NULL) != 0)
			printf("mutex init has failed\n");
		if (pthread_mutex_init(&booth[i].accessQueue, NULL) != 0)
			printf("mutex init has failed\n");
		scanf(" %d %d", &booth[i].voters, &booth[i].evms);
		pthread_create(&(booth[i].tid), NULL, &newBooth, (void *)(&booth[i].idx));
	}

	for (int i=0 ; i<n ; ++i)
		pthread_join(booth[i].tid, NULL);

	return 0;
}