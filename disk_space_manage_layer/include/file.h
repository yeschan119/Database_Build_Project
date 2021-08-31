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
#define leaf_ORDER 5
#define internal_ORDER 4
#define leaf_data 128
#define internal_data 16
#define KEY 8
#define VALUE 120
#define PAGE_ID 8
#define fin_factor 1
#define FILE_ID 10
#define path_length 20
#define Frame_ID 10
#define leaf_size 4
#define num_key 4
// needed variables
typedef uint64_t pagenum_t;
extern pagenum_t page_ID[FILE_SIZE];
extern char * file_name;
void usage_2(void);

extern char num_keys[4];
extern char **File_ID;
extern char * path;
extern char **table_id;
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
//API_for_DB

int open_table(char* pathname);
int db_insert(int fd, pagenum_t key, char *value);
int db_find(pagenum_t key, char * ret_val);
int db_delete(pagenum_t key);
int open_file(char * path);
void db_print(void);
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
void sub_print(int fd, pagenum_t sibling);
void merge_page(page_t merge);


// buffer segment
int store_table_id(char * pathname);
int get_table_id(char * pathname);
int init_db(int buf_num);
int buffer_insert(pagenum_t key, char * value);
int buffer_find(pagenum_t key);


pagenum_t put_page_buffer(pagenum_t page, char * data);
char *  make_table_id(char *path);
void test(void);
void table_ID(void);
int get_table_id(char * path);
int put_table_id(char *path);
bool leaf_check (char * data);
int key_check (char *data);
int search_in_frame(pagenum_t key, char * data);
bool leftmost_check(char *data);
void buffer_write(char *data, page_t src);
void buffer_read(char *data, page_t src);
int search_key_frame(pagenum_t key, char * data);
int find_frame(pagenum_t page);

int buffer_delete(pagenum_t key);

void delete_in_frame(char * data, page_t del);
int get_frame_id(pagenum_t page);
void frame_write_disk(pagenum_t pagenum, int index);
void frame_write_disk_after_delete(pagenum_t pagenum, int index);
void frame_read_disk(pagenum_t pagenum, int index);
int get_used_frame(void);
int get_empty_frame(void);
page_t format_page(pagenum_t key);
pagenum_t return_page (pagenum_t key, char * data);

void pin_plus(int i);
void pin_minus(void);

typedef struct queue{
    int frame_id;
    struct queue * next;
}queue;
extern queue * LRU;
queue* queue_format(int id);
void enqueue(int id);
int dequeue(void);

typedef struct test_table_id{
    int id;
    char * path;
}test_table_id;

void init_test_table(void);
int my_table(char * pathname);


char* get_fd(int i);
void increase_key(char *data);
void decrease_key(char *data);
int close_table(int table_id);
void init_table(int fd);
