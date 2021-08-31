//
//  lock_table.c
//  unittest
//
//  Created by Eungchan on 2020/11/05.
//

#include "lock_table.h"
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
               }
    }
    return 0;
}

int lock_hash(int table_id){
    return table_id % NUM_HASH;
}

lock_t* lock_acquire(int table_id, int64_t key)
{
    pthread_mutex_lock(&lock_table_latch[table_id][key]);
    lock_t *hash = &lock_table[table_id].record_id[key];   // hashing with table_id & key
    lock_t * new = malloc(sizeof(struct lock_t));
    
    if (hash->head == NULL)
    {
        hash->head = new;      // append new thread to the head
        hash->tail = new;      // point end of the thread list
        new->sentinel = &lock_table[table_id].record_id[key];
        new->next = NULL;
        new->prev = NULL;
        new->table_id = table_id;
        new->key = key;
    }
    else{
        if(hash->tail == NULL)
            lock_acquire(table_id, key);
        lock_t *tail = hash->tail;
        tail->next = new;    // append new to the lock table
        new->prev = tail;    // set prev for new
        new->next = NULL;    // set next for new
        hash->tail = new;     // update tail
        new->table_id = table_id;
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

int lock_release(lock_t* lock_obj)
{
    pthread_mutex_lock(&lock_table_latch[lock_obj->table_id][lock_obj->key]);
    lock_t * header = lock_obj->sentinel;
        if(lock_obj->next != NULL){
            header->head = lock_obj->next;
            lock_obj->next->prev = NULL;
            pthread_cond_signal(&cond[lock_obj->table_id][lock_obj->key]);
        }
        else{
            header->head = NULL;
            header->tail = NULL;
            pthread_cond_signal(&cond[lock_obj->table_id][lock_obj->key]);
        }
   
    if(0 > (pthread_mutex_unlock(&lock_table_latch[lock_obj->table_id][lock_obj->key]))){
        perror("lock_release error\n");
        return -1;
    }
    free(lock_obj);
    return 0;
}
