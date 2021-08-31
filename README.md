# DBMS_project
Build DBMS that has functions like insert, delete, find with concurrency control.

There are 2 parts for Disk_based_B+tree(final)

- The first part is to show outlines and brief explains of my B+tree
  > - introduction of index layer(open, insert, find, delete)
  > - introduction of file API(write, read, alloc..)
  > - introduction of sub functions to support file API

- The second is the diagram to explain roles of each function and my design for Disk_Based_B+tree.

# 1. outlines of my B+tree
## index layer
### - open_table
  > - open_table uses system call like this `if ( 0 < (fd = open( pathname, O_RDWR|O_CREAT|O_EXCL, 0644)))` this means open existed table and create if not,
  > - After creation, call the page_setting function(define the page_size and header page as the first position)
  > - make only header page at the first time.
### - db_insert
  > - open file and get page number with key.
  > - In the first step, there is only one page, 'header'.
  > - Using the header, get free and mapping page and use it to insert. 
  > - check the duplicated key and check the number of keys for the page to split or not
  > - write the key to the write position and make this be a root.
### - db_find
  > - open file and get page number with key.
  > - find the key from file_read API
  > - check the return value( 0 or -1), 0 is OK, -1 is no key you want find.
### - db_delete
  > - open file and get page number with key
  > - find the right position using search function.
  > - start to delete overwriting other values.
  > - reduce number of keys of the page and check the num_keys because of splitting.
  > execute fsync()
### - db_print
  > - get the root using header page.
  > - internal pages including root have key and page_id and leaf page has key and value.
  > - check the leaf or not.
  > - if a page is a leaf, this will print all keys and values
  > - if a page is not a leaf, this will print only keys, not page_id
  > - The printing starts from root to leaf by level order.
  > - There is one more function(sub_print) to print only siblings. it supports db_print.

## File API(write, read, alloc, free)
### - file_alloc_page()
  > - first, check the root from header page.
  > - If the root in the header page(0) exits, just return and update the next free page to the header page
  > - if not, make the free page. the first free page will be the right next page of the header.
  > - To find next free page, looking for the right next page of the old free page using find_free_page func()
### - file_write_page()
  > - 1, check the number of keys and is_leaf of given page
  > - 2. if not is_leaf, put the 1 into the is_leaf of the page. is_leaf has 1 or 0
  > - 3. if it is not new, write
  > - 4. when writing, sorting also follows.(linear sorting)
  > - 5. it it is new, just write at the first position of the page.
### - write_internal_page()
  > - Added API is to support only for internal page writing after splitting.
  > - When splitting pages, we need to write in the internal page(parent)
  > - After splitting, this writes the left child key and page number to the parent
  > - copy the keys and page number from left child
  > - write them to the right position in the parent
  > - define the leftmost or not(leftmost page is different to read and write)
### - file_read_page()
  > - open file and search(no need to get page because it has it as a parameter)
  > - search is a binary search
  > - search function will return 0 if find, return -1 if not find.
  > - check the return value from the search.
## sub_functions to support main functions
### - insert_parent functions()
  > - when needed split, get new child, and new parent 'from file_alloc_page API'
  > - if no parent for old page, call the another func(insert_new_parent)
  > - if parent exists, set the parent page number for both new, old
  > - call the func, write_internal_page()
  > - increase num_keys and set the sibling
### - search_key()
  > - we can us this when search both leaf page and internal page
  > - It is based on binary search algorithm
  > - first, it will check is_leaf and check leftmost or not.
  > - as a result, it can give three types of page searching(leaf, normal internal and leftmost internal page)
  > - return right position if OK and return -1 if not find.
### get_page_number()
  > - get root at the first time.
  > - check root is NULL or exists
  > - get new free page and make it root if NULL
  > - call the find_leaf_page() if root exists.
  > - return pages after that.
### find_leaf_page()
  > - 1. get num_keys and is_leaf
  > - 2. if it is leaf, just return given page.
  > - 3. if not, check leftmost or not for internal page
  > - 4, After all the above check, search the page from the root
### sort_leaf_page()
  > - it sorts the keys and values whenever they are inserted.
  > - comparing keys from the first position to find right position
  > - At the right position, other keys are moving back by one position
  > - And writ the key and value into the right position.
### split_page()
  > - make the new child page using file_alloc_page()
  > - check is_leaf of the old page
  > - if it's leaf, invoke the leaf page splitting 
  > - if not, invoke the internal page splitting
  > - split number is the half of the num_keys.
  > - return new child page number.
### merge_page()
  > - for delayed merge, the merge operation is executed only when no keys in a page.
  > - check the right sibling and move the data from right sibling
  > - if no sibling, no splitting
  > - after removing data from leaf page, check whether parent exists or not
  > - if exists, check leftmost or not for the parent.
  > - find the key in the parent page using search_key func().
  > - remove the key and also check the parent of the parent recursively.
### Other small functions
  > - mapping table(header, free, internal/leaf)
  > - open file function(use this whenever open file) because of error check
  > - synchro func(for fsync() operation and check error)
  > - lltoa, itoa functions(support to convert int to string, uint64_t to string)
  > - keys_plus func(only for increasing number of keys for pages)
# 1. The diagram and brief explain of my disk_based_b+tree
- The code has three parts(main, file.c, file.h)
- main functions has open, find, delete, insert, print
- file.h is header fiile
- file.c has 3 kinds of functions
  > - index layer, file operation, support functions
- insert operation invokes at least 4 funcions(db_insert -> get_page_num -> get mapping table -> write -> sorting)
- find operation invokes at least 4 functions (db_find -> get_page_num -> get mapping table -> search -> read)
- delete operation invokes at least (db_delete -> get_page_num -> search -> delete)
- Additionally split, merge functions can be needed
  > - split conditions : num_keys < leaf_order - fin_factor
  > - merge conditions : num_keys == 0 & has sibling.  
# Diagram of DiskBased_b+tree
 - [outline of b+tree](outline)
 - [insert operation](insert)
 - [find operation](find)
 - [delete operation](delete)
