//
//  file.c
//  Disk_Based_B+tree
//
//  Created by Eungchan on 2020/10/05.
//

#include "file.h"
char * file_name;
char num_keys[num_key];
char is_leaf[leaf_size];
char * path;
char table_name[Table_ID][path_length];
int fd;
int file_descripter[Table_ID];
pagenum_t page_ID[FILE_SIZE] = {0,};
char page_buffer[KEY];
int table_id = 0;
buffer_ctr *frame;
int buffer_num;
int * pin_queue;
//int fd;
//================================ initialize the buffer(init_db)===================================//


void usage_2( void ) {
    printf("================================================================\n");
    printf("Enter any of the following commands after the prompt > :\n"
    "\ti <k>  -- Insert (key, value) keyrange(1 ~ 100000).\n"
    "\tf <k>  -- Find the value under key <k>.\n"
    "\td <k>  -- Delete key <k> and its associated value.\n"
    "\tc      -- close table\n"
    "\to      -- open new table or existing table\n"
    "\ts      -- shutdown_db\n"
    "\tp      -- Print the B+ tree.\n"
    "\tq      -- Quit. (Or use Ctl-D.)\n"
  );
    printf("\n================================================================\n");
}

// initialize the buffer
int init_db(int num_buf){
    buffer_num = num_buf;
    frame = malloc(sizeof(struct buffer_ctr) * num_buf);
    pin_queue = malloc(sizeof(int) * num_buf);    // initialize the pin_count manager
    for(int i = 0; i < buffer_num; i++){
        frame[i].frame_id = i;
        frame[i].is_dirty = false;
        frame[i].pin_count = 0;
        frame[i].pagenum = 1;    // header is 0  , to avoid this
        frame[i].table_id = 0;
        frame[i].data = malloc(sizeof(char) * PAGE_SIZE);
        for(int j =0; j < PAGE_SIZE; j++)
        {frame[i].data[j] = '\0';}
        frame[i].next = NULL;
        pin_queue[i] = -1;   // frame id is from 0,
    }
    return 0;
}

// open file and return 1(unique id)
int open_table(char *pathname){
    int fd;
    //check the table count
    if(table_id >= 10){
        printf("The number of tables are full, no allowed any more\n");
        return -1;
    }
    else if(0 <(fd = open(pathname, O_RDWR|O_CREAT|O_EXCL, 0644)))
    {
        printf("%s is new table\n", pathname);
        printf("Creat done\n");
        strcpy(table_name[table_id], pathname);
        file_descripter[table_id] = fd;
        table_id++;
        init_table(fd);
        return table_id;
    }
    else{
        printf("%s table exists\n", pathname);
        return get_table_id(pathname);   // if table has already opend, find it.
    }
}

// find table id using path name.
// And return table id
int get_table_id (char * pathname){
    int i = 0;
    printf(" fd[tmp] %s\n", table_name[i]);
    while(strcmp(table_name[i], pathname) != 0 && i < Table_ID){
    //printf(" fd[tmp] %s\n", table_name[i]);
        i++;
    }
    if (i == Table_ID)
        return -1;
    return ++i;    // if table id is zero, return 1
}


// initialize table at the first time. (make haeder,  and set the root)
void init_table(int fd){
    char root_buffer[PAGE_ID];
    char * leaf = "111";
    header_page header = mapping_header(page_ID);    // call the mapping table for the header page
    pread(fd, root_buffer, sizeof(root_buffer), header.pagenum + header.root_page);  // check the root
    pagenum_t root =  file_alloc_page();  // return the root
    pagenum_t free = root + PAGE_SIZE;
    lltoa(root, root_buffer);
    pwrite(fd, root_buffer, sizeof(root_buffer), header.pagenum + header.root_page);
    page_t init_root = mapping_page(root);
    pwrite(fd, leaf, sizeof(leaf), init_root.pagenum + init_root.is_leaf);
    lltoa(free, page_buffer);
    pwrite(fd, page_buffer, sizeof(page_buffer), header.pagenum + header.free_page);

}

//============================== index layer(insert, find, delete) ==============================//

int db_insert(int table_id, pagenum_t key, char *value){
    fd = file_descripter[table_id-1];   // the real index is id - 1
    page_t b_insert = format_page(key);  // initialize the key, value, page_id
    b_insert.record = value;
    header_page header = mapping_header(page_ID);
    int i_header = get_frame_id(header.pagenum, table_id);
    // get root
    pagenum_t root = find_root(i_header);
    int index = get_frame_id(root, table_id);  // find root and updata root in buffer
    index = find_leaf_in_buffer(key, index);   // find leaf page using root
    
    // duplicate check
    if(search_key_frame(key, index) >= 0)
    {
        perror("No allowed for duplicated key\n");
        return -1;
    }
    pin_plus(index);     //increase pin_count
    buffer_write(index, b_insert);   // write data to the page in a buffer
    check_dirty(index);        // mark dirty
    increase_key(index);   // increase num_keys of page in a buffer
    pin_minus();   // decrease pin_count
    return 0;
}

int db_find(int table_id, pagenum_t key, char * ret_val){
    fd = file_descripter[table_id-1];
    page_t b_find = format_page(key);  // initialize the key, value, page_id
    
    header_page header = mapping_header(page_ID);
    int i_header = get_frame_id(header.pagenum, table_id);
    int index = get_frame_id(find_root(i_header), table_id);
    index = find_leaf_in_buffer(key, index);
    
    if(search_key_frame(key, index) < 0)
    {
        perror("No key to find\n");
        return -1;
    }
    pin_plus(index);
    buffer_read(index, b_find);
    pin_minus();
    strcpy(ret_val, b_find.record);
    return 0;
}

int db_delete(int table_id, pagenum_t key){
    fd = file_descripter[table_id-1];
    header_page header = mapping_header(page_ID);
    int i_header = get_frame_id(header.pagenum, table_id);  // get page in a buffer
    page_t p_del = format_page(key);
    pagenum_t root = find_root(i_header);
    int index = get_frame_id(root, table_id);
    index = find_leaf_in_buffer(key, index);
    pin_plus(index);
    delete_in_frame(index, p_del);
    check_dirty(index);     // check the dirty mark
    decrease_key(index);  // decrease the number of key
    pin_minus();
    
    return 0;
}

// close table using dequeue
// extract a page from buffer, and check dirty or not
// write a data to the disk if it is dirty.
int close_table(int table_id){
    fd = file_descripter[table_id - 1];
    int index = dequeue(table_id);
    
    while(index >= 0)
    {
        if(frame[index].is_dirty == true)
            frame_write_disk(index, table_id);
        index = dequeue(table_id);
    }
    close(fd);
    return 0;
}

int shutdown_db(){
    while(table_id > 0)
    {
        close_table(table_id);
        table_id--;
    }
    frame = NULL;
    return 0;
}
//===================================== Buffer layer =====================================//

//====================== primary support functions(read, write, remove, search...)========================//


// initialize the page
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

void check_dirty(int index){
    frame[index].is_dirty = true;
}
pagenum_t find_root(int index){
    char root[PAGE_ID];
    for(int i = 0; i < PAGE_ID; i++)
    {
        root[i] = frame[index].data[8 + i];
    }
    return atoll(root);
}
int find_leaf_in_buffer(pagenum_t key, int index){
    while(!leaf_check(frame[index].data)){
        pagenum_t page = return_page(key, index);
        index = get_frame_id(page, fd);
    }
    return index;
}
// find the value using key in a buffer
// delete key & value in a frame
void delete_in_frame(int index, page_t del){
    //char *data = frame[index].data;
    int i = 0;
    int start = leaf_data;
    int offset = search_key_frame(del.index, index);
    if(offset < 0)
        perror("No key to delete\n");
    else{
    int keys = key_check(frame[index].data);
    offset += 1;
    for(i = offset; i <= keys; i++)
            {// make the position moving back
            for(int j = 0; j < leaf_data; j++){
                frame[index].data[(start * i) + j] = frame[index].data[(start * (i + 1)) + j];  // move forward one by oneb to replace
            }
        }
    printf("delete done\n");
    }
}

// find same page in buffer
// return frame id if exists,
// find empty page if not same exists
// find page using LRU replacement policy
int get_frame_id(pagenum_t page, int table_id){
    int frame_id = find_frame(page, table_id);
    if(frame_id >= 0){
        return frame_id;}
    else if( 0 <= (frame_id = get_empty_frame(table_id))){
        frame_read_disk(page, frame_id, table_id);  // read page from disk, if no page in buffer
    }
    else{
        frame_id = get_used_frame(table_id);
        if(frame_id < 0)
            exit(1);     // no more table to use.
    }
    return frame_id;
}


// return frame when asking buffer to find same page
int find_frame(pagenum_t page, int table_id){
    int i = 0;
    for(i = 0; i < buffer_num; i++){
        if( page == frame[i].pagenum && table_id == frame[i].table_id)
            break;
    }
    if(i < buffer_num){
        if(frame[i].pin_count == 0)   // in use or not?
            return i;
        else{
            perror("The page is in use, try again later\n");
            exit(1);   // if in use, stop!
        }
    }
    else
        return -1;
}

// find empty frame and return frame id
int get_empty_frame(int table_id){
    
    int i = 0;
    int frame_size = buffer_num;
    while(i < frame_size && frame[i].pagenum != 1 && frame[i].table_id != 0){i++;}
    if(i == frame_size)
    {
        printf("No empty frame\n");
        return -1;
    }
    else{
       
        return i;
    }
}

// find frame in LRU queue
int get_used_frame(table_id){
    int index = dequeue(table_id);
    if(index >= 0 && frame[index].is_dirty == true)
    {
        frame_write_disk(index, table_id);
        frame[index].is_dirty = false;
        frame[index].table_id = table_id;
    }
    return index;
}

//  return  page number to find leaf
pagenum_t return_page (pagenum_t key, int index){
    //char * data = frame[index].data;
    char pagenum[PAGE_ID];
    int offset = search_in_frame(key, index);
    if(offset < 0)
        return -1;
    int start = leaf_data;

    if(offset == 0){
        for(int i = 0; i < PAGE_ID; i++){
            pagenum[i] = frame[index].data[start + i];
        }
        return atoll(pagenum);
    }
    else{
        if(!leftmost_check(frame[index].data))
            start += PAGE_ID;
        int k = 0;
        --offset;   // to correct position
        for(int i = KEY; i < internal_data; i++, k++){
            pagenum[k] = frame[index].data[start + (offset * internal_data) + i];
        }
           
        return atoll(pagenum);
    }
}

// to read data from page in buffer
void buffer_read(int index, page_t src){
    //char *data = frame[index].data;
    int offset = search_key_frame(src.index, index);
    bool check_leaf = leaf_check(frame[index].data);
    bool not_leftmost = leftmost_check(frame[index].data);
    //int keys = key_check(data);
    pagenum_t start = leaf_data;
    
    if(index < 0){
        perror("No key to find\n");}
    else if(check_leaf){
            int i = KEY, j = 0;
            while(i < leaf_data){
                if(frame[index].data[(start * (offset + 1)) + i] == '\0')
                    break;
                src.record[j] = frame[index].data[(start * (offset + 1)) + i];
                printf("%c", frame[index].data[(start * (offset + 1)) + i]);
                i++; j++;
                }
           }
    else{
        if(!not_leftmost)
            start += PAGE_ID;
        int i = KEY, j = 0;
        while(i < internal_data){
            if(frame[index].data[(start * (offset + 1)) + i] == '\0')
                break;
            src.page_id[j] = frame[index].data[(start * (offset + 1)) + i];
            printf("%c ", frame[index].data[(start * (offset + 1)) + i]);
            i++; j++;
        }
    }
    printf("\n");
}

// write key & value to the page in a buffer
void buffer_write(int index, page_t src){
    //char * data = frame[index].data;
    int i = 0;
    int start = leaf_data;
    char write_key[KEY];
    int offset = search_in_frame(src.index, index);
    int keys = key_check(frame[index].data);
    int pivot = keys - 1;   // real position of number of key
    
    if(leaf_check(frame[index].data)){  // if it is leaf,
        for(i = offset; i < keys; keys--, pivot--)
            {  // make the position moving back
                for(int j = 0; j < leaf_data; j++){
                    frame[index].data[(start * (keys + 1)) + j] = frame[index].data[(start * (pivot + 1)) + j];  // copy and paste one by one
                    frame[index].data[(start * (pivot + 1)) + j] = '\0';
                }
            }
        // write
        int j = 0, k = 0;
        lltoa(src.index, write_key);
        while(j < leaf_data){
            if(j < KEY)
            {frame[index].data[(start * (offset+1)) + j] = write_key[j];
                if(write_key[j] == '\0')
                    j = KEY;
            }// copy
            if(j >= KEY)
            {frame[index].data[(start * (offset+1)) + j] = src.record[k];   // paste one by one
                if(src.record[k] == '\0')
                   j = leaf_data;
                 k++;}
            j++;
        }
        printf("\n");
     
        }
     
    else{  // it is not leaf
        if(!(leftmost_check(frame[index].data)))
            start += PAGE_ID;
        for(i = offset; i < keys; keys--, pivot--)
        {
            for(int j = 0; j < internal_data; j++){
                frame[index].data[(start * keys) + j] = frame[index].data[(start * pivot) + j];
                frame[index].data[(start * pivot) + j] = '\0';
            }
        }
        int j = 0, k = 0;
        lltoa(src.index, write_key);
        while(j < internal_data){
            if(j < KEY)
                frame[index].data[(start * (offset + 1)) + j] = write_key[j];  // copy
            else
            {frame[index].data[(start * (offset+1)) + j] = src.page_id[k];
                k++;
            }   // paste one by one
            
            j++;
        }
    }
    
    //output result
  /*
    //print out leaf page after inserting
    keys = key_check(frame[index].data);
    int j = 0;
    for(int i = 0; i < keys + 1; i++){
    while(j < leaf_data){
        if(j < KEY)
        {if(frame[index].data[(start * (i + 1)) + j] == '\0')
                j = KEY;
            printf("%c ", frame[index].data[(start * (i + 1) + j)]);
        }// copy
        
        if(j >= KEY)
        {  // paste one by one
            printf("%c ", frame[index].data[(start * (i + 1) + j)]);
            if(frame[index].data[(start * (i + 1) + j)] == '\0')
                j = leaf_data;
        }
        j++;
        }
        printf("\n");
        j = 0;
    }
   */
}

// find a position to insert or path to find a leaf
int search_in_frame(pagenum_t key, int index){
    //char *data = frame[index].data;
    char check_key[KEY];
    int i = 0, start = leaf_data, j = 0;
    int keys = key_check(frame[index].data);
    if(leaf_check(frame[index].data))  // is leaf?
    {
        for(i = 0; i < keys; i++){
            for (j = 0; j < sizeof(check_key); j++){  // j is for giver, k is for receiver
                check_key[j] = frame[index].data[j + start];   // read key from page
            }
            if(key <= atoll(check_key))
                break;
            start += leaf_data;
            //k = 0;
        }
    }
    else{  // is not leaf?
        if(!(leftmost_check(frame[index].data))){   // is not leftmost?
            start += PAGE_ID;  // first position is page number not key
            }
        for(i = 0; i < keys; i++){
            for(int j = 0; j < sizeof(check_key); j++){
                check_key[j] = frame[index].data[j + start];
                }
            if(key < atoll(check_key))
                break;
            start += internal_data;
           // k = 0;
        }
    }
    if(i > keys)
    {
        return -1;
    }
    else
        return i;   // return index of key in frame
}

// find a key to delete or find
int search_key_frame(pagenum_t key, int index){
    //char *data = frame[index].data;
    char check_key[KEY];
    int i = 0, start = leaf_data, j = 0;
    int keys = key_check(frame[index].data);
    if(leaf_check(frame[index].data))  // is leaf?
    {
        for(i = 0; i < keys; i++){
            for (j = 0; j < sizeof(check_key); j++){  // j is for giver, k is for receiver
                check_key[j] = frame[index].data[j + start];   // read key from page
            }
            if(key == atoll(check_key))
                break;
            start += leaf_data;
            //k = 0;
        }
    }
    else{  // is not leaf?
        if(!(leftmost_check(frame[index].data))){   // is not leftmost?
            start += PAGE_ID;  // first position is page number not key
            }
        for(i = 0; i < keys; i++){
            for(int j = 0; j < sizeof(check_key); j++){
                check_key[j] = frame[index].data[j + start];
                }
            if(key <= atoll(check_key))
                break;
            start += internal_data;
            //k = 0;
        }
    }
    if(i == keys)
    {
        return -1;
    }
    else
        return i;   // return index of key in frame
}

//============================ disk access functions(file write & read) ==============================//

// read data from pages in frame
void frame_read_disk(pagenum_t page, int index, int table_id){
    fd = file_descripter[table_id-1];
    pread(fd, frame[index].data, PAGE_SIZE, page);
    enqueue(index, table_id);
    frame[index].pagenum = page;  // assign pagenum
    frame[index].table_id = table_id;
 
}

// write data to pages in frame checking split or not
void frame_write_disk(int index, int table_id){
    fd = file_descripter[table_id-1];
    pwrite(fd, frame[index].data, PAGE_SIZE, frame[index].pagenum);
    synchro(fd);
    page_t check_split_merge = mapping_page(frame[index].pagenum);
    if(key_plus(check_split_merge) > leaf_ORDER - fill_factor)
        insert_parent(check_split_merge);
    else if(key_minus(check_split_merge) == 0)
        merge_page(check_split_merge);
}


//================== secondary support functions(key, leaf check & inc, dec key) =====================//

bool leaf_check (char * data){
    char leaf[leaf_size];
    for(int i = 0; i < sizeof(leaf); i++)
    {
        leaf[i] = data[8 + i];  // 8 is is_leaf segment
    }
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

// increase num_keys of page in a buffer
void increase_key(int index){
    char * data = frame[index].data;
    int key = key_check(data);
    int num_of_key = key + 1;
    char keys[num_key];
    itoa(num_of_key, keys);
    for(int i = 0; i < sizeof(keys); i++)
    {
        data[12 + i] = keys[i];   // 12 is num_key region
    }
    
}
// decrease num_keys of page in a buffer
void decrease_key(int index){
    int key = key_check(frame[index].data);
    int num_of_key = key - 1;
    char keys[num_key];
    itoa(num_of_key, keys);
    for(int i = 0; i < sizeof(keys); i++)
    {
        frame[index].data[12 + i] = keys[i];   // 12 is num_key region
    }
}


//============================== replacement policy(pin count & LRU) ==============================//

// queue to control LRU
queue * LRU = NULL;
queue * queue_format(int index, int table_id){
    queue * f = malloc(sizeof(struct queue));
    f->table_id = table_id;
    f->frame_id = index;
    f->next = NULL;
    return f;
}

// get frame name, not structure
// And put it into the queue, LRU
void enqueue(int index, int table_id){
    queue * new;
    if(LRU == NULL){
        LRU = queue_format(index, table_id);    // assign the frame id and set null to next
    }
    else{
        new = LRU;
        while(new->next != NULL){
            new = new ->next;
        }
        new -> next = queue_format(index, table_id);
    }
}

//take out the frame id from the queue, LRU
int dequeue(int table_id){
    queue * d = LRU;
    queue * dd = d;
    int i = 0;
    while(d != NULL && d->table_id != table_id){
    i++;
    dd = d;
    d = d->next;
    }
    if(d == NULL)
        return -1;
    else if(i == 0)
        LRU = LRU->next;
    else
        dd->next = d->next;
    d->next = NULL;
    return d->frame_id;
}

// pin_plus is to increase the pin_count
void pin_plus(int frame_id){
    frame[frame_id].pin_count++;
    int i = 0;
    while(pin_queue[i] >= 0 && i < sizeof(pin_queue))   // pin_queue is initialized by -1
        i++;
    pin_queue[i] = frame_id;
}

// decrease the pin_count
void pin_minus(){
    int i = 0;
    while(pin_queue[i] >= 0 && i < sizeof(pin_queue))
    {
        int frame_id = pin_queue[i];
        frame[frame_id].pin_count--;
        pin_queue[i] = -1;
        i++;
    }
}

//============================= disk space management layer ===================================//
pagenum_t file_alloc_page(void){
    pagenum_t free;
    char next[PAGE_ID];
    header_page h1;
    pagenum_t next_free;
    h1 = mapping_header(page_ID);  // get the mapping table for header
    //fd = open_file(file_name);
    pread(fd, page_buffer, sizeof(page_buffer), h1.pagenum + h1.free_page);
    pagenum_t free_in_header = atoll(page_buffer);      //convert string to int
    if(free_in_header){      // exits free page in header
        free = free_in_header;
        next_free = find_free_page(free + PAGE_SIZE);
        lltoa(next_free, next);
        pwrite(fd, next, sizeof(next), h1.pagenum + h1.free_page);  // update the free pageID in the header page

    }
    else{                   // no free page in header
        free = find_free_page(h1.pagenum + PAGE_SIZE);
        next_free = find_free_page(free + PAGE_SIZE);
        lltoa(next_free, page_buffer);
        pwrite(fd, page_buffer, sizeof(page_buffer), h1.pagenum + h1.free_page);  // update the free pageID in the header page
    }
    synchro(fd);
    return free;
    }
pagenum_t find_free_page(pagenum_t free){
    char free_buffer[KEY];
    pagenum_t i = 0;
    pagenum_t next;
    //int fd = open_file(file_name);
    int fd = file_descripter[0];
    while(i < FILE_SIZE){
        pread(fd, free_buffer, sizeof(free_buffer), free + (i * PAGE_SIZE));
        next = atoll(free_buffer);
        if(!next)
            break;       // empty page if 0
        i++;
    }
    //printf("find free page\n");
    next = free+(PAGE_SIZE * i);    // free page set up
    
    //printf("find free page %lld\n", f1.pagenum);
    return next;
}


void file_write_page(pagenum_t pagenum, const page_t src){
    char insert_key[KEY];
    //int fd = open_file(file_name);
    char *leaf = "1111";
    pread(fd, is_leaf, sizeof(is_leaf), src.pagenum + src.is_leaf);
    pread(fd, num_keys, sizeof(num_keys), src.pagenum + src.num_keys);
    if(!(atoi(is_leaf)))
        pwrite(fd, leaf, sizeof(is_leaf), src.pagenum + src.is_leaf);      // set the is_leaf true
    if(atoi(num_keys) > 0){
        sort_leaf_page(src);}
    else{
    lltoa(src.index, insert_key);
    pwrite(fd, insert_key, sizeof(insert_key), src.pagenum + src.DATA);
    pwrite(fd, src.record, VALUE, src.pagenum + src.DATA + KEY);
    }
    int key_check = key_plus(src);
    if(key_check >= leaf_ORDER-fill_factor){   // bulk loading
        insert_parent(src);  // set a parent
    }
    printf("Write the keys in page\n");
    synchro(fd);
   
}


int file_read_page(pagenum_t pagenum, page_t dest){
    //int fd = open_file(file_name);
    printf("call search\n");
    dest.offset = search_key(dest, dest.index);
    if(dest.offset < 0){
        return -1;
    }
    else{
        pread(fd, dest.record,VALUE, (dest.pagenum + dest.DATA + ((leaf_data * dest.offset) + KEY)));
    return 0;
    }
}
void write_internal_page(page_t parent, page_t child){
    //int fd = open_file(file_name);
    char temp_key[KEY];
    char temp_page[PAGE_ID];
    char temp_record[internal_data];
    char check_key[KEY];
    pagenum_t start = parent.pagenum + parent.DATA + PAGE_ID;
    pread(fd, check_key, sizeof(check_key), child.pagenum + child.DATA);  // key to check equality
    pagenum_t child_key = atoll(check_key);
    pread(fd, num_keys, sizeof(num_keys), parent.pagenum + parent.num_keys);  // get num_keys of parent
    int pivot = atoi(num_keys) - 1;  // to enlarge a list
    int i = 0;
    
    while(i < atoi(num_keys)){
        pread(fd, temp_key, sizeof(temp_key), start + (i * internal_data));  // from first record
        if(child_key < atoll(temp_key))    // check equal
            break;
        i++;
    }

    for(int j = atoi(num_keys); j > i; j--, pivot--){ // to move back
        pread(fd, temp_record, sizeof(temp_record), start + (internal_data * pivot)); // if need to interverne
        pwrite(fd, temp_record, sizeof(temp_record), start + (internal_data * j));  // move backward
    }
 
    lltoa(child.pagenum, temp_page);  // child page number to input into parent
    lseek(fd, start + (internal_data * i), SEEK_SET);  // find right position using i
    write(fd, check_key, sizeof(check_key));
    write(fd, temp_page, PAGE_ID);
    synchro(fd);
    
}

void file_free_page(pagenum_t pagenum){
    char not_leftmost[4];
    char parent_numkeys[4];
    char parent_buffer[PAGE_ID];
    char remove_key[KEY];
    char del_buffer[internal_data] = {0,};
    char del_free[leaf_data] = {0,};
    char sibling[PAGE_ID];
    //int fd = open_file(file_name);
    int pivot;
    page_t p_free = mapping_page(pagenum);  // call the mapping table
    pread(fd, parent_buffer, sizeof(parent_buffer), p_free.pagenum + p_free.parent_page);  // get parent
    page_t parent = mapping_page(atoll(parent_buffer));
    pagenum_t start_point = parent.pagenum + parent.DATA;
    
    pread(fd, sibling, sizeof(sibling), p_free.pagenum + p_free.right_sibling);  // child
    
    if(atoll(parent_buffer)){
        pread(fd, parent_numkeys, sizeof(parent_numkeys), parent.pagenum + parent.num_keys);  // parent
    // if child has a key
    if(!(atoll(sibling))){    // if child is empty
        pivot = atoi(parent_numkeys) - 1;
    }
    else{
        pread(fd, remove_key, sizeof(remove_key), p_free.pagenum + p_free.DATA);  // get first key to delete
        pivot = search_key(parent, atoll(remove_key));  // find the right position to be deleted
    }
    if(pivot >= 0){  // greater than the first key of parent
        pread(fd, not_leftmost, sizeof(not_leftmost), parent.pagenum + parent.reserved);
        if(!atoi(not_leftmost)){
            start_point += PAGE_ID; // first position is not key(page id)
            }
            // correct the sibling pagenum
            // get pagenum from left of the being removed key
            pread(fd, sibling, sizeof(sibling), start_point + (pivot * internal_data) - PAGE_ID);
            page_t leftchild = mapping_page(atoll(sibling));
            // get sibling from being removed page
            pread(fd, sibling, sizeof(sibling), p_free.pagenum + p_free.right_sibling);
            pwrite(fd, sibling, sizeof(sibling), leftchild.pagenum + leftchild.right_sibling);

            for(int i = pivot; i < atoi(parent_numkeys); i++, pivot++){  // pivot + 1 is right next key
                pread(fd, del_buffer, sizeof(del_buffer), start_point + (i+1 * internal_data)); //i = pivot + 1
                pwrite(fd, del_buffer, sizeof(del_buffer), start_point + (pivot * internal_data)); // replace data
            }
    
    int order = atoi(parent_numkeys);
    order -= 1;
    lltoa(order, parent_numkeys);
    if(atoi(parent_numkeys) == 0)
        file_free_page(parent.pagenum);
    else
        pwrite(fd, parent_numkeys, sizeof(parent_numkeys), parent.pagenum + parent.num_keys);
    }
    else{  // if pivot < 0 : less than the first key of parent
        pread(fd, not_leftmost, sizeof(not_leftmost), p_free.pagenum + p_free.reserved);
        start_point = p_free.pagenum + p_free.DATA;
        if(!atoi(not_leftmost))
            start_point += PAGE_ID;
        pwrite(fd, del_free, sizeof(del_free), start_point);
    }
        
    }
}
// third step
//support functions to support file API(insert_parent, search, get_page_num, find_leaf, split, merge...)

page_t insert_parent(page_t old)

{   char not_leftmost[4] = "111";
    char sibling[PAGE_ID];
    char parent_buffer[PAGE_ID];
    //int fd = open_file(file_name);
    page_t new;
    page_t parent;
    pread(fd, page_buffer, sizeof(page_buffer), old.pagenum + old.DATA);
    new = mapping_page(split_page(old.pagenum));   //new page after spliting
    pwrite(fd, not_leftmost, sizeof(not_leftmost), new.pagenum + new.reserved);  // set not leftmost
    pread(fd, parent_buffer, sizeof(parent_buffer), old.pagenum + old.parent_page);  //get parent from old
    printf("parent creation\n");
    pread(fd, page_buffer, sizeof(page_buffer), old.pagenum + old.DATA);
    if(!(atoll(parent_buffer))){    // no parent
        parent = insert_new_parent(old, new);
    }
    else{
        parent = mapping_page(atoll(parent_buffer)); // mapping parent page
        lltoa(parent.pagenum, parent_buffer);  // page number(right child for parent)
        pwrite(fd, parent_buffer, sizeof(parent_buffer), new.pagenum + new.parent_page); //set parent for new
        pread(fd, num_keys, sizeof(num_keys), parent.pagenum + parent.num_keys);
        if(atoi(num_keys ))
        write_internal_page(parent, new);
        key_plus(parent);
    }
     
    pread(fd, sibling, sizeof(sibling), old.pagenum + old.right_sibling);
    if(atoll(sibling)){
        pwrite(fd, sibling, sizeof(sibling), new.pagenum + new.right_sibling);
    }
    lltoa(new.pagenum, sibling);
    pwrite(fd, sibling, sizeof(sibling), old.pagenum + old.right_sibling); // update sibling for old

    if(atoi(num_keys) > internal_ORDER - fill_factor)  // check the parent node
        return insert_parent(parent);
    return parent;
}
page_t insert_new_parent(page_t old, page_t new){
    char leftmost[PAGE_ID];
    //int fd = open_file(file_name);
    page_t new_parent;
    pagenum_t parent_ID = file_alloc_page();
    new_parent = mapping_page(parent_ID);   // get new page
    
    lltoa(new_parent.pagenum, page_buffer);
    pwrite(fd, page_buffer, sizeof(page_buffer), old.pagenum + old.parent_page); //set new parent for old
    pwrite(fd, page_buffer, sizeof(page_buffer), new.pagenum + new.parent_page); // set new parent for new
    // set children for parent
    lltoa(old.pagenum, leftmost);
    pwrite(fd, leftmost, sizeof(leftmost), new_parent.pagenum + new_parent.DATA);  // leftmost child
    pread(fd, page_buffer, sizeof(page_buffer), old.pagenum + old.DATA);
    write_internal_page(new_parent, new);
    key_plus(new_parent);
   
    lltoa(new_parent.pagenum, page_buffer);
    header_page h1;
    h1 = mapping_header(page_ID);
    pwrite(fd, page_buffer, sizeof(page_buffer), h1.pagenum + h1.root_page);  // set new root

    return new_parent;
}


int search_key(page_t page, pagenum_t key){
    //int fd = open_file(file_name);
    int keys;
    char check_key[KEY];
    char not_leftmost[4];
    pagenum_t here = page.pagenum + page.DATA;  // start point to lookup
    pread(fd, num_keys, sizeof(num_keys), page.pagenum + page.num_keys);
    pread(fd, is_leaf, sizeof(is_leaf), page.pagenum + page.is_leaf);
    keys = atoi(num_keys);
    int start = 0;
    int end = keys;
    int mid = (start + end)/2;
    if(atoi(is_leaf)){ // leaf page search
    while(start != mid && mid != end){
        pread(fd, check_key, sizeof(check_key), here + (mid * leaf_data));
        if(key < atoll(check_key))
        {
            end = mid;
            mid = (start + end)/2;
        }
        else if(key > atoll(check_key))
        {
            start = mid;
            mid = (start + end)/2;
        }
        else
            break;
    }
    pread(fd, check_key, sizeof(check_key), here + (mid*leaf_data));  // need one more
    }
    else{ // internal page search
        pread(fd, not_leftmost, sizeof(not_leftmost), page.pagenum + page.reserved);  // check leftmost
        if(!(atoi(not_leftmost))){
            here = page.pagenum + page.DATA + PAGE_ID;  // leftmost internal page
        }
            while(start != mid && mid != end){
                pread(fd, check_key, sizeof(check_key), here + (mid * internal_data));
                if(key < atoll(check_key))
                {
                    end = mid;
                    mid = (start + end)/2;
                }
                else if(key > atoll(check_key))
                {
                    start = mid;
                    mid = (start + end)/2;
                }
                else
                    break;
            }
            pread(fd, check_key, sizeof(check_key), here + (mid*internal_data));  // need one more
        }
        
    if(key == atoll(check_key))
    {
        return mid;
    }
    else{
        return -1;
    }
    
}

pagenum_t get_page_number(pagenum_t key){
    //int fd = open_file(file_name);
    char root_buffer[PAGE_ID];
    //int fd = get_fd(id);
    header_page header;
    header = mapping_header(page_ID);    // call the mapping table for the header page
    pread(fd, root_buffer, sizeof(root_buffer), header.pagenum + header.root_page);  // check the root
    if(!(atoll(root_buffer))){    // root exists??
        pagenum_t root =  file_alloc_page();  // return the root
        lltoa(root, root_buffer);
        pwrite(fd, root_buffer, sizeof(root_buffer), header.pagenum + header.root_page);
        synchro(fd);
        return root;
        }
    
    else{
        pagenum_t pagenum = find_leaf_page(atoll(root_buffer), key);

        return pagenum;
    }
}


pagenum_t find_leaf_page(pagenum_t root , pagenum_t key){
    int i = 0;
    char not_leftmost[4];
    //int fd = open_file(file_name);
    char key_buffer[KEY];
    page_t p1;
    p1 = mapping_page(root);   // call the mapping table for internal page
    pread(fd, is_leaf, sizeof(is_leaf), p1.pagenum + p1.is_leaf);  // get the is_leaf
    pagenum_t offset; // find the key location, KEY is right most

    if(!(atoi(is_leaf))){
    while(!(atoi(is_leaf))){
        pread(fd, not_leftmost, sizeof(not_leftmost), p1.pagenum + p1.reserved);  // to check leftmost page
        i = 0;
        offset =  p1.pagenum + p1.DATA;
        if(!(atoi(not_leftmost))){
            offset += PAGE_ID;
        }
            //internal pages except rightmost
            pread(fd, num_keys, sizeof(num_keys), p1.pagenum + p1.num_keys);  // get the number of keys in page
            pread(fd, key_buffer, sizeof(key_buffer), offset + (i * internal_data));
              while(i < atoi(num_keys) && key >= atoll(key_buffer))
              {
                  i++;
                  pread(fd, key_buffer, sizeof(key_buffer), offset + (i * internal_data));
              }
             if(i > 0)
             {
                 pread(fd, page_buffer, sizeof(page_buffer), (offset + (i * internal_data) - PAGE_ID) );
                 pread(fd, key_buffer, sizeof(key_buffer), offset + (i * internal_data));
             }
            else
            { pread(fd, page_buffer, sizeof(page_buffer), p1.pagenum + p1.DATA);}
        
        p1 = mapping_page(atoll(page_buffer));
        pread(fd, is_leaf, sizeof(is_leaf), p1.pagenum + p1.is_leaf);
        pread(fd, num_keys, sizeof(num_keys), p1.pagenum + p1.num_keys);  // get the number of keys in page
            }
        
        return(atoll(page_buffer));
        }
    
    return root;   // find leaf_page
}


void sort_leaf_page(page_t src){
    char temp_key[KEY];
    char temp_record[leaf_data];
    int pivot = atoi(num_keys) - 1;  // real position of record
    int i = 0;
   // int fd = open_file(file_name);
    while(i < atoi(num_keys)){
        pread(fd, temp_key, sizeof(temp_key), src.pagenum + src.DATA + (i * leaf_data));  // from first record
        if(src.index < atoll(temp_key))    // check equal
            break;
        i++;
    }

    for(int j = atoi(num_keys); j > i; j--, pivot--){ // to move back
        pread(fd, temp_record, sizeof(temp_record), src.pagenum + src.DATA + (leaf_data * pivot));
        pwrite(fd, temp_record, sizeof(temp_record), src.pagenum + src.DATA + (leaf_data * j));  // move backward
    }
    lltoa(src.index ,temp_key);
    lseek(fd, src.pagenum + src.DATA + (leaf_data * i), SEEK_SET);  // find right position using i
    write(fd, temp_key, sizeof(temp_key));
    write(fd, src.record, VALUE);
    synchro(fd);
  
}


pagenum_t split_page(pagenum_t pagenum){
    page_t p_old;
    p_old = mapping_page(pagenum);
    int split, k, j;
    char *leaf = "1111";
    char delete_leaf[leaf_data] = {0,};
    char copy_leaf[leaf_data];
    char delete_int[internal_data] = {0,};
    char copy_int[internal_data];
    //int fd = open_file(file_name);
    pagenum_t new = file_alloc_page();
    page_t p_new;
    p_new = mapping_page(new);
    pread(fd, is_leaf, sizeof(is_leaf), p_old.pagenum + p_old.is_leaf);  // read leaf
    if(atoi(is_leaf)){    // is leaf?
        split = cut_page(leaf_ORDER);
        j = split;       // for old
        k = 0;      // for new
            while(j < leaf_ORDER){
            lseek(fd, p_old.pagenum + p_old.DATA + (j * leaf_data), SEEK_SET);  // move the cursor to the middle of page
            
            read(fd, copy_leaf, sizeof(copy_leaf));   // read old page to input new page
            pwrite(fd, delete_leaf, sizeof(delete_leaf), p_old.pagenum + p_old.DATA + (j * leaf_data));  // remove the data from old
            lseek(fd, p_new.pagenum + p_new.DATA+(k * leaf_data), SEEK_SET); // move the cursor to the new
            write(fd, copy_leaf, sizeof(copy_leaf));
            j++;
            k++;
        }
        pwrite(fd, leaf, sizeof(is_leaf), p_new.pagenum + p_new.is_leaf);  // set leaf for new
    }
    else{
            split = cut_page(internal_ORDER);
            j = split;
            k = 0;
            pagenum_t start = p_old.pagenum + p_old.DATA + PAGE_ID;
            while(j < internal_ORDER){
            pread(fd, copy_int, sizeof(copy_int), start + (j * internal_data));
            pwrite(fd, delete_int, sizeof(delete_int), start + (j * internal_data));  // remove the data from old
        
            pwrite(fd, copy_int, sizeof(copy_int), p_new.pagenum + p_new.DATA + ( k * internal_data));
            
            j++;
            k++;
        }
    }
    itoa(split, num_keys);
    pwrite(fd, num_keys, sizeof(num_keys), p_old.pagenum + p_old.num_keys);  // change num_keys for old
    itoa(k, num_keys);
    pwrite(fd, num_keys, sizeof(num_keys), p_new.pagenum + p_new.num_keys);  // set num_keys for new
    
    synchro(fd);
  
    return p_new.pagenum;
}

void merge_page(page_t merge){
    char sibling[PAGE_ID];
    char data_buffer[PAGE_SIZE - leaf_data]; // from data segment
    char parent[PAGE_ID];
    //int fd = open_file(file_name);
    
    // remove parent page
    pread(fd, parent, sizeof(parent), merge.pagenum + merge.parent_page);
    if(atoll(parent))  // no sibling, but parent is
        {file_free_page(merge.pagenum);}
    else{
    // move data from sibling page to removed page
    pread(fd, sibling, sizeof(sibling),merge.pagenum + merge.right_sibling);
    while(atoi(sibling)){  // has sibling and has also parent
        page_t sib_page = mapping_page(atoll(sibling));
        pread(fd, data_buffer, sizeof(data_buffer), sib_page.pagenum + sib_page.DATA);
        pwrite(fd, data_buffer, sizeof(data_buffer), merge.pagenum + merge.DATA);
        pread(fd, sibling, sizeof(sibling), (sib_page.pagenum + sib_page.right_sibling));
    }
    }

    synchro(fd);
}



// mapping step

char paging_buffer[8];
header_page mapping_header(pagenum_t *page_ID){
    header_page header;
    header.pagenum = page_ID[0];
    header.free_page = 0;
    header.root_page = 8;
    header.num_pages = 16;    // the point of start in the page
    header.reserved = 24;
    return header;
}

page_t mapping_page(pagenum_t page_ID){
    page_t mapping;
    mapping.pagenum = page_ID;      // the point of start in the page.
    mapping.parent_page = 0;
    mapping.is_leaf = 8;
    mapping.num_keys = 12;
    mapping.reserved = 16;
    mapping.right_sibling = 120;
    mapping.DATA = 128;
    return mapping;
}

free_page mapping_free(pagenum_t pagenum){
    free_page free;
    free.pagenum = pagenum;    // first free page is at the end of the file
    free.next = 0;
    return free;
}


// last function is for small missions(open, sync, int to string..)

int open_file(char * path){       // only for open file
    int fd;
if(0 > (fd = open(path, O_RDWR, 0644)))
    {
        perror("Table open error");
        EXIT_FAILURE;
    }
    return fd;
}

int synchro(int fd){  // only for fsync()
    if(fsync(fd) == -1)
    {
        perror("sync error\n");
        EXIT_FAILURE;
    }
    return fd;
}

int cut_page(int length) {
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}

void lltoa(pagenum_t num, char *str){
    pagenum_t i=0;
    pagenum_t radix = 10;
    pagenum_t deg=1;
    pagenum_t cnt = 0;
    
    while(1){
        if( (num/deg) > 0)
            cnt++;
        else
            break;
        deg *= radix;
    }
    deg /=radix;
    for(i=0; i<cnt; i++)    {
        *(str+i) = num/deg + '0';
        num -= ((num/deg) * deg);
        deg /=radix;
    }
    *(str+i) = '\0';
}

void itoa(int num, char *str){
    int i=0;
    int radix = 10;
    int deg=1;
    int cnt = 0;

    while(1){
        if( (num/deg) > 0)
            cnt++;
        else
            break;
        deg *= radix;
    }
    deg /=radix;

    for(i=0; i<cnt; i++)    {
        *(str+i) = num/deg + '0';
        num -= ((num/deg) * deg);
        deg /=radix;
    }
    *(str+i) = '\0';
}

int key_plus(page_t page){
    //int fd = open_file(file_name);
    pread(fd, num_keys, sizeof(num_keys), page.pagenum + page.num_keys);
    int order = atoi(num_keys);
    order += 1;
    itoa(order, num_keys);
    pwrite(fd, num_keys, sizeof(num_keys), page.pagenum + page.num_keys);  //update num_keys for parent
    return order;
}

int key_minus(page_t page){
    //int fd = open_file(file_name);
    pread(fd, num_keys, sizeof(num_keys), page.pagenum + page.num_keys);
    int order = atoi(num_keys);
    order -= 1;
    itoa(order, num_keys);
    pwrite(fd, num_keys, sizeof(num_keys), page.pagenum + page.num_keys);  //update num_keys for parent
    return order;
}


//************** print tree for debugging ******************//
void db_print(int id){
    //file_name = get_fd(id);
    //page_t p_print;
    char root[PAGE_ID];
    char print_key[KEY];
    char sibling[PAGE_ID];
    char print_value[VALUE];
    //printf("print %s\n", file_name);
   // int fd = open_file(file_name);
    fd = file_descripter[id-1];
    header_page header;
    header = mapping_header(page_ID);
    pread(fd, root, sizeof(root), header.pagenum + header.root_page);  // get root from header
    page_t p_print = mapping_page(atoll(root));
    pread(fd, is_leaf, sizeof(is_leaf), p_print.pagenum + p_print.is_leaf);
    pread(fd, num_keys, sizeof(num_keys), p_print.pagenum + p_print.num_keys);
    int keys;
    if(atoll(root)){
        while(!atoi(is_leaf)){
            keys = atoi(num_keys);
            lseek(fd, p_print.pagenum + p_print.DATA + PAGE_ID, SEEK_SET);  // first position to get key
            read(fd, print_key, sizeof(print_key));
            while(atoll(print_key)&& keys > 0){
                printf("%s  ", print_key);
                lseek(fd, PAGE_ID, SEEK_CUR);
                read(fd, print_key, sizeof(print_key));
                keys--;
            }
           
            pread(fd, sibling, sizeof(sibling), p_print.pagenum + p_print.right_sibling);
            if(atoll(sibling)){
            printf("|");
            sub_print(atoll(sibling));   // call the sibling printer
            }
            printf("\n");
            //printf("\n-------\n");
            pread(fd, root, sizeof(root), p_print.pagenum + p_print.DATA);
            p_print = mapping_page(atoll(root));
            pread(fd, is_leaf, sizeof(is_leaf), p_print.pagenum + p_print.is_leaf);
            pread(fd, num_keys, sizeof(num_keys), p_print.pagenum + p_print.num_keys);
        }
    do{
        pread(fd, num_keys, sizeof(num_keys), p_print.pagenum + p_print.num_keys);
        keys = atoi(num_keys);
        lseek(fd, p_print.pagenum + p_print.DATA, SEEK_SET);
        read(fd, print_key, sizeof(print_key));
        
        while(atoll(print_key) && keys > 0){
            printf("<");
            printf("%s, ",print_key);
            read(fd, print_value, sizeof(print_value));
            printf("%s",print_value);
            printf("> ");
            read(fd, print_key, sizeof(print_key));
            keys--;
        }
            pread(fd, sibling, sizeof(sibling), p_print.pagenum + p_print.right_sibling);
        if(atoll(sibling)){
            p_print = mapping_page(atoll(sibling));
            printf("|");
        }
    }while(atoll(sibling));}
    else{
        printf("Empty File\n");
        EXIT_FAILURE;
    }

    printf("\n");
}
void sub_print(pagenum_t sibling){
    page_t sub = mapping_page(sibling);
    char next[PAGE_ID];   // next sibling
    char sub_keys[4];
    char print_key[KEY];
    pread(fd, sub_keys, sizeof(sub_keys), sub.pagenum + sub.num_keys);

    for(int i = 0; i < atoi(sub_keys); i++){
        pread(fd, print_key, KEY, sub.pagenum + sub.DATA + (i * internal_data));
        printf(" %s", print_key);
    }
    pread(fd, next, sizeof(next), sub.pagenum + sub.right_sibling);
    if(atoll(next))
        sub_print(atoll(next));
    
}
