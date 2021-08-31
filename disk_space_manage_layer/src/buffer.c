//
//  buffer.c
//  Test_project
//
//  Created by Eungchan on 2020/10/20.
//
#include "file.h"
#include <stdio.h>

buffer_ctr *frame;
buffer_ctr *queue = NULL;
char data_buffer[PAGE_SIZE];
int init_db(int num_buf){
    frame = malloc(sizeof(struct buffer_ctr) * 10);
    printf("frame %ld\n",sizeof(frame));
    for(int i = 0; i < num_buf; i++){
        frame[i].frame_id = i;
        frame[i].is_dirty = false;
        frame[i].is_pinned = 0;
        frame[i].pagenum = 1;    // header is 0  , to avoid this
        frame[i].table_id = 0;
        frame[i].data = malloc(sizeof(char) * PAGE_SIZE);
        for(int j =0; j < PAGE_SIZE; j++)
        {frame[i].data[j] = '\0';}
        frame[i].next = NULL;
    }
    return 0;
}

page_t format_page(pagenum_t key){
    page_t page;
    page.index = key;
    page.record = malloc(VALUE);
    page.page_id = malloc(PAGE_ID);
    for(int i = 0; i < VALUE; i++)
    {
        if(i < PAGE_ID)
           page.page_id[i] = '\0';
        page.record[i] = '\0';
    }
    return page;
}
int buffer_find(pagenum_t key){
    char root[PAGE_ID];
    //int fd = open_file(file_name);
    page_t b_find = format_page(key);  // initialize the key, value, page_id
    header_page header = mapping_header(file_name, page_ID);
    int i_header = find_frame(header.pagenum);  // get frame number
    frame_read_disk(header.pagenum, i_header);
    
    // get root
    for(int i = 0; i < PAGE_ID; i++)
    {
        root[i] = frame[i_header].data[8 + i];
    }
    
    //printf("root %lld\n", atoll(root));
    //printf("\n\n");
    int index = find_frame(atoll(root));    //
    frame_read_disk(atoll(root), index);
    //printf("return_page %lld\n",return_page(key, frame[index].data));
    
    while(!leaf_check(frame[index].data)){
        pagenum_t page = return_page(key, frame[index].data);
        //printf("page :: %lld\n", page);
        index = find_frame(page);
        frame_read_disk(page, index);
    }
     
    //printf("i_root %d\n",i_root);
    buffer_read(frame[index].data, b_find);
    return 0;
}

int buffer_insert(pagenum_t key, char * value){
    char root[PAGE_ID];
    //int fd = open_file(file_name);
    page_t b_insert = format_page(key);  // initialize the key, value, page_id
    b_insert.record = value;
    header_page header = mapping_header(file_name, page_ID);
    int i_header = find_frame(header.pagenum);  // get frame number
    frame_read_disk(header.pagenum, i_header);
    
    // get root
    for(int i = 0; i < PAGE_ID; i++)
    {
        root[i] = frame[i_header].data[8 + i];
    }
    //printf("root %lld\n", atoll(root));
    //printf("\n\n");
    int index = find_frame(atoll(root));    //
    frame_read_disk(atoll(root), index);
    //printf("return_page %lld\n",return_page(key, frame[index].data));
    
    while(!leaf_check(frame[index].data)){
        pagenum_t page = return_page(key, frame[index].data);
        //printf("page :: %lld\n", page);
        index = find_frame(page);
        frame_read_disk(page, index);
    }
    buffer_write(frame[index].data, b_insert);
    frame_write_disk(frame[index].pagenum, index);
    return 0;
}
// read data from pages in frame
void frame_read_disk(pagenum_t pagenum, int index){
    int fd = open_file(file_name);
    pread(fd, data_buffer, sizeof(data_buffer), pagenum);
    frame[index].data = data_buffer;
    frame[index].pagenum = pagenum;
    //printf("read done\n");
    close(fd);
}
// write data to pages in frame
void frame_write_disk(pagenum_t pagenum, int index){
    int fd = open_file(file_name);
    //printf("write page %lld\n", pagenum);
    pwrite(fd, frame[index].data, PAGE_SIZE, pagenum);
    synchro(fd);
    printf("write done\n");
    char test[VALUE];
    page_t write_test = mapping_page(pagenum);
    pread(fd, test, sizeof(test), write_test.pagenum + write_test.DATA + (2 * leaf_data) + PAGE_ID);
    if(key_plus(write_test) > leaf_ORDER - fin_factor)
        insert_parent(write_test);
    //printf("test %s\n",test);
    close(fd);
}

//  return  page number to find leaf
pagenum_t return_page (pagenum_t key, char * data){
    char pagenum[PAGE_ID];
    int offset = search_in_frame(key, data);
    //printf("offset %d\n",offset);
    int start = leaf_data;

    if(offset == 0){
        for(int i = 0; i < PAGE_ID; i++){
            pagenum[i] = data[start + i];
        }
        return atoll(pagenum);
    }
    else{
        if(!leftmost_check(data))
            start += PAGE_ID;
        int k = 0;
        --offset;   // to correct position
        for(int i = KEY; i < internal_data; i++, k++){
            pagenum[k] = data[start + (offset * internal_data) + i];
        }
           
        return atoll(pagenum);
    }
}
// first step(open, insert, find, print)
// return frame when asking buffer to same page

int find_frame(pagenum_t page){
    int i = 0;
    //printf("buffer.pagenum %lld, %lld\n", page, frame[i].pagenum);
    printf("find_frame\n");
    while(page != frame[i].pagenum && i < sizeof(frame)){i++;}
    
    if(i < sizeof(frame)){
        return i;
    }
    else
    {printf("no matched frame\n");
        i = get_empty_frame();
        if (i < 0)
            return get_used_frame();
        return i;
    }
}
int get_empty_frame(void){
    int i = 0;
    int frame_size = sizeof(frame);
    while(i < frame_size && frame[i].pagenum != 1)
    {
        i++;
    }
    if(i == frame_size)
    {
        printf("No empty frame\n");
        return -1;
    }
    else
        return i;
}
int get_used_frame(void){
    int frame_id = dequeue();
    return frame_id;
}
pagenum_t put_page_buffer(pagenum_t page, char * data){
    int i = 0;
    page_t test = mapping_page(page);
    test.index = 20;
    test.record = malloc(VALUE);
    test.page_id = malloc(PAGE_ID);
    for(int i = 0; i < VALUE; i++)
    {
        if(i < PAGE_ID)
            test.page_id[i] = '\0';
        test.record[i] = '\0';
    }
    while(frame[i].pagenum){
        i++;
    }
    if(i < sizeof(frame[i])){
        frame[i].pagenum = page;
        frame[i].data = data;    // get data from disk
        buffer_read(frame[i].data, test);
        //buffer_write(buffer.frame[i].data, test);
        return i;
    }
    else
        return -1;
}

void test(){
    int i = 0;
   
    while(!(frame[i].pagenum)){
        i++;
    }
    for(int j = 0; j<8; j++){
    char tmp =  frame[i].data[8 + j];
        printf("%c", tmp);
        //if(tmp == '\n')
    }
    printf("\n");
}

// to read data from page in buffer
void buffer_read(char *data, page_t src){
    int offset = search_key_frame(src.index, data);
    bool check_leaf = leaf_check(data);
    bool not_leftmost = leftmost_check(data);
    int keys = key_check(data);
    pagenum_t start = leaf_data;
    
    if(index < 0)
        perror("No key to find\n");
    else if(check_leaf){
            int i = KEY, j = 0;
            printf("leaf offset to read %d\n", offset);
            while(i < leaf_data){
                if(data[(start * (offset + 1)) + i] == '\0')
                    break;
                src.record[j] = data[(start * (offset + 1)) + i];
                printf("%c", data[(start * (offset + 1)) + i]);
                i++; j++;
            }
           }
    else{
        if(!not_leftmost)
            start += PAGE_ID;
        int i = KEY, j = 0;
        //printf("internal offset to read %d\n", offset);
        while(i < internal_data){
            if(data[(start * (offset + 1)) + i] == '\0')
                break;
            src.page_id[j] = data[(start * (offset + 1)) + i];
            printf("%c ", data[(start * (offset + 1)) + i]);
            i++; j++;
        }
    }
    printf("\n");
}
void buffer_write(char *data, page_t src){
    //printf("src key %lld, %s\n", src.index, src.record);
    int i = 0;
    int start = leaf_data;
    char write_key[KEY];
    int offset = search_in_frame(src.index, data);
    //printf("offset, %d\n", offset);
    int keys = key_check(data);
    int pivot = keys - 1;   // real position of number of key
    
    if(leaf_check(data)){  // if it is leaf,
        for(i = offset; i < keys; keys--, pivot--)
            {  // make the position moving back
                for(int j = 0; j < leaf_data; j++){
                    data[(start * (keys + 1)) + j] = data[(start * (pivot + 1)) + j];  // copy and paste one by one
                    data[(start * (pivot + 1)) + j] = '\0';
                }
            }
        // write
     
        printf("write start\n");
        int j = 0, k = 0;
        lltoa(src.index, write_key);
        while(j < leaf_data){
            if(j < KEY)
            {data[(start * (offset+1)) + j] = write_key[j];
                if(write_key[j] == '\0')
                    j = KEY;
            }// copy
            if(j >= KEY)
            {data[(start * (offset+1)) + j] = src.record[k];   // paste one by one
                if(src.record[k] == '\0')
                   j = leaf_data;
                 k++;}
            j++;
        }
        printf("\n");
     
        }
     
    else{  // it is not leaf
        if(!(leftmost_check(data)))
            start += PAGE_ID;
        for(i = offset; i < keys; keys--, pivot--)
        {
            for(int j = 0; j < internal_data; j++){
                data[(start * keys) + j] = data[(start * pivot) + j];
                data[(start * pivot) + j] = '\0';
            }
        }
        int j = 0, k = 0;
        lltoa(src.index, write_key);
        //printf("print keys\n");
        while(j < internal_data){
            if(j < KEY)
                data[(start * (offset + 1)) + j] = write_key[j];  // copy
            else
            {data[(start * (offset+1)) + j] = src.page_id[k];
                k++;
            }   // paste one by one
            
            j++;
        }
    }
     
    //print out leaf page after inserting
    keys = key_check(data);
    //printf("inserted value in leaf %d\n", keys);
    int j = 0;
    for(int i = 0; i < keys + 1; i++){
    while(j < leaf_data){
        if(j < KEY)
        {if(data[(start * (i + 1)) + j] == '\0')
                j = KEY;
            printf("%c ", data[(start * (i + 1) + j)]);
        }// copy
        
        if(j >= KEY)
        {  // paste one by one
            printf("%c ", data[(start * (i + 1) + j)]);
            if(data[(start * (i + 1) + j)] == '\0')
                j = leaf_data;
        }
        j++;
        }
        printf("\n");
        j = 0;
    }
   
}

bool leaf_check (char * data){
    char leaf[leaf_size];
    for(int i = 0; i < sizeof(leaf); i++)
    {
        leaf[i] = data[8 + i];  // 8 is is_leaf segment
    }
    //printf("leaf_check %d\n", atoi(leaf));
    if(atoi(leaf))
        return true;
    else
        return false;
}

int key_check (char *data){
    char keys[num_key];
    for(int i = 0; i < sizeof(keys); i++)
    {
        keys[i] = data[12 + i];   // 12 is num_key region
    }
    //printf("num_key check %d\n", atoi(keys));
    if(atoi(keys))
        return atoi(keys);
    else
        return 0;
}

bool leftmost_check(char *data){    // no leftmost?
    char leftNOTmost[num_key];
    for(int i = 0; i < sizeof(leftNOTmost); i++)
    {
        leftNOTmost[i] = data[16 + i];  // from 16, reserved region
    }
    //printf("leftmost check %d\n", atoi(leftNOTmost));
    if(atoi(leftNOTmost))
        return true;
    else
        return false;
}

int search_in_frame(pagenum_t key, char * data){
    char check_key[KEY];
    int i = 0, k = 0, start = leaf_data, j = 0;
    int keys = key_check(data);
    if(leaf_check(data))  // is leaf?
    {
        for(i = 0; i < keys; i++){
            for (j = 0; j < sizeof(check_key); j++){  // j is for giver, k is for receiver
                check_key[j] = data[j + start];   // read key from page
            }
            if(key <= atoll(check_key))
                break;
            start += leaf_data;
            //k = 0;
        }
    }
    else{  // is not leaf?
        if(!(leftmost_check(data))){   // is not leftmost?
            start += PAGE_ID;  // first position is page number not key
            }
        for(i = 0; i < keys; i++){
            for(int j = 0; j < sizeof(check_key); j++){
                check_key[j] = data[j + start];
                //printf("%c, ", data[j + start]);
                }
            if(key < atoll(check_key))
                break;
            start += internal_data;
           // k = 0;
        }
    }
    if(i > keys)
    {
        perror("No key to find\n");
        return -1;
    }
    else
        return i;   // return index of key in frame
}

int search_key_frame(pagenum_t key, char * data){
    char check_key[KEY];
    int i = 0, k = 0, start = leaf_data, j = 0;
    int keys = key_check(data);
    if(leaf_check(data))  // is leaf?
    {
        for(i = 0; i < keys; i++){
            for (j = 0; j < sizeof(check_key); j++){  // j is for giver, k is for receiver
                check_key[j] = data[j + start];   // read key from page
            }
            if(key == atoll(check_key))
                break;
            start += leaf_data;
            //k = 0;
        }
    }
    else{  // is not leaf?
        if(!(leftmost_check(data))){   // is not leftmost?
            start += PAGE_ID;  // first position is page number not key
            }
        for(i = 0; i < keys; i++){
            for(int j = 0; j < sizeof(check_key); j++){
                check_key[j] = data[j + start];
                printf("%c, ", data[j + start]);
                }
            if(key <= atoll(check_key))
                break;
            start += internal_data;
            //k = 0;
        }
    }
    if(i == keys)
    {
        perror("No key to find\n");
        return -1;
    }
    else
        return i;   // return index of key in frame
}


void enqueue(buffer_ctr * new_frame) {
    buffer_ctr * b;
    if (queue == NULL) {
        queue = new_frame;
        queue->next = NULL;
    }
    else {
        b = queue;
        while(b->next != NULL) {
            b = b->next;
        }
        b->next = new_frame;
        new_frame->next = NULL;
    }
}

int dequeue(void) {
    buffer_ctr * n = queue;
    queue = queue->next;
    n->next = NULL;
    return n->frame_id;
}
