//  file.h
//  Disk_Based_B+tree
//
//  Created by Eungchan on 2020/10/05.
#ifndef file_h
#define file_h
// Uncomment the line below if you are compiling on Windows.
//#define WINDOWS
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#define _CRT_SECURE_NO_WARNINGS
#define false 0
#define true 1

#endif /* file_h */
#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define FILE_SIZE 1024
#define PAGE_SIZE 4096
#define leaf_ORDER 32
#define internal_ORDER 248
#define leaf_data 128
#define internal_data 16
#define KEY 8
#define VALUE 120
#define PAGE_ID 8
#define fill_factor 0
#define FILE_ID 10
#define path_length 20
#define Table_ID 10
#define leaf_size 4
#define num_key 4
// needed variables
typedef uint64_t pagenum_t;
extern pagenum_t page_ID[FILE_SIZE];
extern char * file_name;
void usage_2(void);

extern int file_descripter[Table_ID];
extern char num_keys[4];
extern char **File_ID;
extern char * path;
extern char table_name[Table_ID][path_length];
extern int fd;
extern int table_id;
// initiate each page
typedef struct header_page {
    pagenum_t pagenum;
    pagenum_t free_page;
    pagenum_t root_page;
    pagenum_t num_pages;
    pagenum_t reserved;
    
}header_page;

typedef struct free_page{
    pagenum_t pagenum;
    pagenum_t next;
}free_page;


typedef struct page_t {
    char * record;           //record ID
    char * page_id;
    pagenum_t index;         // key
    int offset;
    pagenum_t pagenum;
    pagenum_t parent_page;
    pagenum_t is_leaf;
    pagenum_t num_keys;
    pagenum_t reserved;
    pagenum_t right_sibling;
    pagenum_t DATA;
}page_t;


//*************************** disk space managing layer ***************************//

//API_for_DB

int open_table(char* pathname);
int disk_insert(int id, pagenum_t key, char *value);
int disk_find(pagenum_t key, char * ret_val);
int disk_delete(pagenum_t key);
int open_file(char * path);
void db_print(int table_id);
// File API

pagenum_t get_page_number(pagenum_t key);
int synchro(int fd);
pagenum_t file_alloc_page(void);

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest)
int file_read_page(pagenum_t pagenum, page_t dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t src);
void write_internal_page(page_t parent, page_t child);
// Allocate an on-disk page from the free page list

//paging

pagenum_t internal_search(page_t page, pagenum_t key);
void page_setting(pagenum_t *page_ID);
header_page mapping_header(pagenum_t *page_ID);
free_page mapping_free(pagenum_t page_ID);
page_t mapping_page(pagenum_t page_ID);


//pagenum_t search_key(pagenum_t pagenum, pagenum_t key);
int search_key(page_t page, pagenum_t key);

pagenum_t find_leaf_page(pagenum_t root, pagenum_t key);
pagenum_t split_page (pagenum_t pagenum);
void lltoa(pagenum_t num, char *str);
void itoa(int num, char *str);
//pagenum_t cut( pagenum_t length);
pagenum_t find_free_page(pagenum_t free);
page_t insert_parent(page_t old);
page_t insert_new_parent(page_t old, page_t new);
int cut_page(int length);

int key_plus(page_t page);
int key_minus(page_t page);
void sort_leaf_page(page_t src);
void sub_print(pagenum_t sibling);
void merge_page(page_t merge);


typedef struct buffer_ctr{
    //struct buffer_ctr *frame;
    int frame_id;
    int table_id;
    pagenum_t pagenum;
    bool is_dirty;
    int pin_count;
    char * data;
    struct buffer_ctr * next;
    
}buffer_ctr;

extern int * pin_queue;

//*************************** index layer ***************************//
int store_table_id(char * pathname);
int get_table_id(char * pathname);
int init_db(int buf_num);
int db_insert(int table_id, pagenum_t key, char * value);
int db_find(int table_id, pagenum_t key, char * ret_val);
int db_delete(int table_id, pagenum_t key);
int close_table(int table_id);
int shutdown_db(void);

//*************************** buffer managing layer ***************************//

page_t format_page(pagenum_t key);

//=================== primary support functions ===================//
// return frame id
int get_frame_id(pagenum_t page, int table_id);
int find_frame(pagenum_t page, int table_id);
int get_used_frame(int table_id);
int get_empty_frame(int table_id);
pagenum_t return_page (pagenum_t key, int index);   // find leaf page

// search in buffer
int search_in_frame(pagenum_t key, int index);
int search_key_frame(pagenum_t key, int index);

// read, write, delete
void buffer_write(int index, page_t src);
void buffer_read(int index, page_t src);
void delete_in_frame(int index, page_t del);

// disk access functions in buffer layer
void frame_read_disk(pagenum_t page, int index, int table_id);
void frame_write_disk(int index, int table_id);
int find_leaf_in_buffer(pagenum_t key, int index);
pagenum_t find_root(int index);
//============== secondary support functions in buffer layer ================//

bool leftmost_check(char *data);
bool leaf_check (char * data);
int key_check (char *data);
void increase_key(int index);
void decrease_key(int index);
void check_dirty(int index);
// replacement policy part functions
//================ structure to support LRU policy ===================//
typedef struct queue{
    int table_id;
    int frame_id;
    struct queue * next;
}queue;
extern queue * LRU;
queue* queue_format(int index, int table_id);
void enqueue(int index, int table_id);
int dequeue(int table_id);

void pin_plus(int i);
void pin_minus(void);
void init_table(int id);
