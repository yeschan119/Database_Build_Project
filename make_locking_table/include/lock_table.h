//
//  lock_table.h
//  unittest
//
//  Created by Eungchan on 2020/11/05.
//
#ifndef lock_table_h
#define lock_table_h

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define NUM_HASH 3
#define TRANSFER_THREAD_NUMBER    (8)
#define SCAN_THREAD_NUMBER        (1)

#define TRANSFER_COUNT            (1000)
#define SCAN_COUNT                (1000)
#define TABLE_NUMBER             (3)
#define RECORD_NUMBER            (10)
#define INITIAL_MONEY            (100000)
#define MAX_MONEY_TRANSFERRED    (100)
#define SUM_MONEY                (TABLE_NUMBER * RECORD_NUMBER * INITIAL_MONEY)


typedef struct lock_t{
    int table_id;
    int hash_id;
    int64_t key;
    struct lock_t * sentinel;
    struct lock_t * next;
    struct lock_t * prev;
    struct lock_t *record_id;
    struct lock_t * head;
    struct lock_t * tail;
}lock_t;

// hash table to lock
typedef struct hashing {
    struct hashing *record_id;
    int table_id;
    lock_t * head;
    lock_t * tail;
}hashing;

/* This is shared data pretected by your lock table. */
//int accounts[TABLE_NUMBER][RECORD_NUMBER];
//pthread_mutex_t *lock_table_latch;
/*
 * This thread repeatedly transfers some money between accounts randomly.
 */
void* transfer_thread_func(void* arg);
void* scan_thread_func(void* arg);

/* APIs for lock table */
int init_lock_table(void);
lock_t* lock_acquire(int table_id, int64_t key);
int lock_release(lock_t* lock_obj);
void mutex_cond(void);
int lock_hash(int table_id);
#endif /* lock_table_h */
