//
//  transaction.c
//  Project2
//
//  Created by Eungchan on 2020/11/19.
//

#include "transaction.h"
#include "file.h"
//
//  lock_table.c
//  unittest
//
//  Created by Eungchan on 2020/11/05.
//
pthread_mutex_t **lock_table_latch;
pthread_cond_t **cond;

lock_t * lock_table;

void mutex_cond(){
    
    lock_table_latch = (pthread_mutex_t**)malloc(sizeof(pthread_mutex_t*) * TABLE_NUMBER);
    cond = (pthread_cond_t**)malloc(sizeof(pthread_cond_t*) * TABLE_NUMBER);
    for(int i = 0; i< TABLE_NUMBER; i++)
    {
        lock_table_latch[i] = malloc(sizeof(pthread_mutex_t) * RECORD_NUMBER);
        cond[i] = malloc(sizeof(pthread_cond_t) * RECORD_NUMBER);
        for(int j = 0; j < RECORD_NUMBER; j++)
        {
            pthread_mutex_init(&lock_table_latch[i][j], NULL);
            pthread_cond_init(&cond[i][j], NULL);
        }
    }
}

int init_lock_table()
{
    mutex_cond();    // initialize mutex and cond
    lock_table = malloc(sizeof(struct lock_t) * TABLE_NUMBER);  // build the hash
    
    for(int i = 0; i < TABLE_NUMBER; i++){
        lock_table[i].record_id = malloc(sizeof(struct lock_t) * RECORD_NUMBER);
               for(int j = 0; j < RECORD_NUMBER; j++)
               {
                   lock_table[i].record_id[j].head = NULL;
                   lock_table[i].record_id[j].tail = NULL;
                   lock_table[i].record_id[j].next = NULL;
                   lock_table[i].record_id[j].prev = NULL;
                   lock_table[i].record_id[j].ex_mode = 0;
               }
    }
    return 0;
}

int lock_hash(int table_id){
    return table_id % NUM_HASH;
}

lock_t* Lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode)
{
    lock_t *hash = &lock_table[table_id].record_id[key];   // hashing with table_id & key
    lock_t * new = malloc(sizeof(struct lock_t));
    if(lock_mode == 1){        // check exclusive mode or not
        hash->ex_mode = 1;  // turn on exclusive_mode for this record
        return exclusive_mode(table_id, key, trx_id, hash, new);
    }
    else if(hash->ex_mode == 1)   // if ex mode is on
        return exclusive_mode(table_id, key, trx_id, hash, new);
    else
        return share_mode(table_id, key, trx_id, hash, new);

}

lock_t * share_mode(int table_id, pagenum_t key, int trx_id, lock_t * hash, lock_t * new){
    
    if (hash->head == NULL)
    {
        hash->head = new;      // append new thread to the head
        hash->tail = new;      // point end of the thread list
        new->sentinel = &lock_table[table_id].record_id[key];
        new->next = NULL;
        new->prev = hash->head;
        new->table_id = table_id;
        new->trx_id = trx_id;
        new->key = key;
    }
    else{
        if(hash->tail == NULL)
            share_mode(table_id, key, trx_id, hash, new);
        lock_t *tail = hash->tail;
        tail->next = new;    // append new to the lock table
        new->prev = tail;    // set prev for new
        new->next = NULL;    // set next for new
        hash->tail = new;     // update tail
        new->table_id = table_id;
        new->trx_id = trx_id;
        new->key = key;
        new->sentinel = &lock_table[table_id].record_id[key];   // point header
        }
    return new;
}
lock_t * exclusive_mode(int table_id, pagenum_t key, int trx_id, lock_t* hash, lock_t * new){
    pthread_mutex_lock(&lock_table_latch[table_id][key]);
    
    if (hash->head == NULL)
    {
        hash->head = new;      // append new thread to the head
        hash->tail = new;      // point end of the thread list
        new->sentinel = &lock_table[table_id].record_id[key];
        new->next = NULL;
        new->prev = hash->head;
        new->table_id = table_id;
        new->trx_id = trx_id;
        new->key = key;
    }
    else{
        if(hash->tail == NULL)
            exclusive_mode(table_id, key, trx_id, hash, new);
        lock_t *tail = hash->tail;
        tail->next = new;    // append new to the lock table
        new->prev = tail;    // set prev for new
        new->next = NULL;    // set next for new
        hash->tail = new;     // update tail
        new->table_id = table_id;
        new->trx_id = trx_id;
        new->key = key;
        new->sentinel = &lock_table[table_id].record_id[key];   // point header
        pthread_cond_wait(&cond[table_id][key], &lock_table_latch[table_id][key]);
        }
    if(0 > (pthread_mutex_unlock(&lock_table_latch[table_id][key]))){
        perror("lock_acquire error\n");
        return NULL;
    }
    return new;
}
int Lock_release(lock_t* lock_obj)
{
    lock_t * header = lock_obj->sentinel;
    if(header->ex_mode == 1){
        pthread_mutex_lock(&lock_table_latch[lock_obj->table_id][lock_obj->key]);
        if(lock_obj->next != NULL){
            lock_obj->prev = lock_obj->next;
            lock_obj->next->prev = lock_obj->prev;
            header->ex_mode = 0;
            pthread_cond_signal(&cond[lock_obj->table_id][lock_obj->key]);
        }
        else{
            header->head = NULL;
            header->tail = NULL;
            header->ex_mode = 0;
            pthread_cond_signal(&cond[lock_obj->table_id][lock_obj->key]);
        }
   
    if(0 > (pthread_mutex_unlock(&lock_table_latch[lock_obj->table_id][lock_obj->key]))){
        perror("lock_release error\n");
        return -1;}
        }
    else{
        if(lock_obj->next != NULL){
            lock_obj->prev = lock_obj->next;
            lock_obj->next->prev = lock_obj->prev;
            }
        else{
            header->head = NULL;
            header->tail = NULL;
        }
    }
    free(lock_obj);
    return 0;
}
