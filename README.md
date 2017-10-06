# Multithreading

## INTRODUCTION

Simple shell implemented in C.

## AUTHORS

* Kshitij Gupta (20161234)

## PROBLEMS

### The Gas Station Problem

#### Problem

Simulate a gas station with implementing the cars and attenders as threads. There is a fixed capacity of the gas station, pumps and the size of the waiting queue.

#### Design

Cars push theirs ids into a global queue which is accessed by the attenders concurrenty. The attender pops the car id from the queue and starts executing the job set by the cars. Once all the car threads are joined, The attenders threads are also joined.

#### HOW TO RUN

```
gcc gasStation.c -pthread -o gasStation.out
./gasStation.out
```

### The Queue at the Polling Booth

#### Problem

Simulate an election in which there are multiple voters at a polling booth with multiple EVMs to handle voting in slots. Only one EVM of the polling booth can move to the voting phase at once.

#### Design

A queue of the free EVMs at a polling booth is maintained. All the voters at each booth are assigned to the EVM at the front of the queue and once all the slots of the EVM are full, They wait to move into the voting phase.

#### HOW TO RUN

```
gcc pollingBooth.c -pthread -o pollingBooth.out
./pollingBooth.out
```

### Concurrent Merge Sort

#### Problem

Implement concurrent merge sort using forking and multithreading.

#### OUTPUT

The time of the execution of merge sort is appended into files mergeSort-fork_data.txt and mergeSort-thread_data.txt respectively.

#### HOW TO RUN

```
gcc mergeSort-fork.c -o mergeSort-fork.out
./mergeSort-fork.out
gcc mergeSort-thread.c -pthread -o mergeSort-thread.out
./mergeSort-thread.out
```
