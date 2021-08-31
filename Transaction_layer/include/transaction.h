//
//  transaction.h
//  Project2
//
//  Created by Eungchan on 2020/11/19.
//

#ifndef transaction_h
#define transaction_h

#include <stdio.h>

#endif /* transaction_h */
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
#define THREAD_NUM 10
#define TABLE_NUMBER             (3)
#define RECORD_NUMBER            (1000)
typedef uint64_t pagenum_t;
extern int trx_id;

typedef struct lock_t{
    short ex_mode;
    int table_id;
    int trx_id;
    int64_t key;
    struct lock_t * sentinel;
    struct lock_t * next;
    struct lock_t * prev;
    struct lock_t *record_id;
    struct lock_t * head;
    struct lock_t * tail;
}lock_t;
typedef struct trx{
    int thread_id;
    pthread_t thread;
    lock_t * next;
    lock_t * prev;
}trx;
extern trx * transaction;
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
lock_t* Lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode);
int Lock_release(lock_t* lock_obj);
void mutex_cond(void);
int lock_hash(int table_id);
#endif /* lock_table_h */

int trx_begin(void);
void * trx_db_find(int table_id, pagenum_t key, char* value, int trx_id);
int multi_thread(int table_id, pagenum_t key, char* value);
lock_t * share_mode(int table_id, pagenum_t key, int trx_id, lock_t* hash, lock_t *new);
lock_t * exclusive_mode(int table_id, pagenum_t key, int trx_id, lock_t * hash, lock_t *new);
