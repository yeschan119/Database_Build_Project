!!! Code written during my undergraduate years (a complete mess)… when will I ever refactor this?

# Data-Level Design and Implementation in the DB Engine
+ Designed the data I/O flow to follow the order: Index layer → Buffer layer → Disk layer
+ The Index layer and Buffer layer operate entirely in memory
+ The Disk space layer performs disk I/O via system calls

## [한국어 🇰🇷](README.ko.md)

# Project Diagram
![db_engine](https://github.com/yeschan119/Database_Build_Project/assets/83147205/2b062ec0-91de-4392-8f68-4844a1aca92f)

# DB_Build_Project
Build a database system that supports data storage and basic operations such as insert, delete, and find, with concurrency control.

There are two main parts in the final disk-based B+ tree implementation.

- The first part provides an overview and brief explanation of the B+ tree:
  > - Introduction to the index layer (open, insert, find, delete)
  > - Introduction to the file API (write, read, allocate, etc.)
  > - Introduction to supporting sub-functions for the file API

- The second part consists of diagrams explaining the role of each function and the overall design of the disk-based B+ tree.

# 1. Outline of the B+ Tree

## Index Layer

### - open_table
  > - `open_table` uses a system call such as  
    `if (0 < (fd = open(pathname, O_RDWR | O_CREAT | O_EXCL, 0644)))`,  
    which opens an existing table or creates a new one if it does not exist
  > - After creation, it calls the `page_setting` function to define the page size and initialize the header page at the first position
  > - Only the header page is created initially

### - db_insert
  > - Opens the file and retrieves the target page number using the key
  > - Initially, only the header page exists
  > - Uses the header to obtain a free or mapped page for insertion
  > - Checks for duplicate keys and determines whether page splitting is required
  > - Writes the key to the appropriate position and initializes it as the root if needed

### - db_find
  > - Opens the file and retrieves the page number using the key
  > - Searches for the key using the `file_read` API
  > - Returns 0 if the key is found, or -1 if the key does not exist

### - db_delete
  > - Opens the file and retrieves the page number using the key
  > - Finds the correct position using the search function
  > - Deletes data by overwriting existing values
  > - Decreases the number of keys in the page and checks whether rebalancing is required
  > - Executes `fsync()` to ensure persistence

### - db_print
  > - Retrieves the root page using the header page
  > - Internal pages (including the root) store keys and page IDs, while leaf pages store keys and values
  > - Determines whether a page is a leaf
  > - If the page is a leaf, prints all keys and values
  > - If the page is an internal node, prints only keys (not page IDs)
  > - Printing proceeds from root to leaf in level-order traversal
  > - An additional function (`sub_print`) is used to print sibling nodes and supports `db_print`

## File API (write, read, alloc, free)

### - file_alloc_page()
  > - Checks the root from the header page
  > - If a free page exists in the header, it returns that page and updates the next free page
  > - Otherwise, creates a new free page; the first free page is placed immediately after the header
  > - Finds subsequent free pages using the `find_free_page()` function

### - file_write_page()
  > - Checks the number of keys and the `is_leaf` flag of the given page
  > - Sets the `is_leaf` flag appropriately (1 for leaf, 0 for internal)
  > - Writes data directly if the page already exists
  > - Performs linear sorting while writing
  > - If the page is new, writes data to the first position

### - write_internal_page()
  > - An additional API used exclusively for writing internal pages after splitting
  > - When a split occurs, writes the left child’s key and page number to the parent
  > - Copies keys and page numbers from the left child
  > - Writes them to the correct position in the parent page
  > - Handles special logic for leftmost pages

### - file_read_page()
  > - Opens the file and searches using the provided page parameter
  > - Uses binary search
  > - Returns 0 if the key is found, or -1 otherwise
  > - Checks the return value of the search result

## Supporting Sub-Functions

### - insert_parent()
  > - Invoked when a page split is required
  > - Allocates a new child and parent page using `file_alloc_page()`
  > - If no parent exists, calls `insert_new_parent()`
  > - Updates parent page numbers for both new and old pages
  > - Calls `write_internal_page()`
  > - Increments the key count and sets sibling pointers

### - search_key()
  > - Used for searching both leaf and internal pages
  > - Based on a binary search algorithm
  > - Checks whether the page is a leaf and whether it is leftmost
  > - Supports searching in leaf pages, normal internal pages, and leftmost internal pages
  > - Returns the correct position if found, or -1 otherwise

### - get_page_number()
  > - Retrieves the root page initially
  > - Checks whether the root exists
  > - Allocates a new page and sets it as the root if none exists
  > - Calls `find_leaf_page()` if the root exists
  > - Returns the resulting page number

### - find_leaf_page()
  > - Retrieves `num_keys` and `is_leaf`
  > - Returns the page immediately if it is a leaf
  > - Otherwise, determines whether the page is leftmost
  > - Continues searching from the root until a leaf page is found

### - sort_leaf_page()
  > - Sorts keys and values upon insertion
  > - Compares keys sequentially to find the correct position
  > - Shifts existing keys backward to make space
  > - Writes the key and value at the correct position

### - split_page()
  > - Allocates a new child page using `file_alloc_page()`
  > - Checks whether the old page is a leaf
  > - Splits leaf pages or internal pages accordingly
  > - Splits at half of `num_keys`
  > - Returns the new child page number

### - merge_page()
  > - Delayed merge is executed only when a page has no keys
  > - Checks the right sibling and moves data from it
  > - If no sibling exists, no merge is performed
  > - After removing data from a leaf page, checks whether a parent exists
  > - If a parent exists, determines whether it is leftmost
  > - Finds and removes the corresponding key in the parent using `search_key()`
  > - Recursively checks and updates ancestor pages

### Other Utility Functions
  > - Mapping tables (header, free, internal/leaf)
  > - File open wrapper with error checking
  > - Synchronization functions for `fsync()` and error handling
  > - `lltoa`, `itoa` functions for integer and `uint64_t` to string conversion
  > - `keys_plus` function for incrementing key counts

# 2. Diagram and Brief Explanation of the Disk-Based B+ Tree
- The code consists of three components: `main`, `file.c`, and `file.h`
- The main module includes open, find, delete, insert, and print operations
- `file.h` serves as the header file
- `file.c` contains three categories of functions:
  > - Index layer functions
  > - File operation functions
  > - Supporting utility functions
- Insert operations invoke at least four functions  
  (`db_insert → get_page_number → get_mapping_table → write → sort`)
- Find operations invoke at least four functions  
  (`db_find → get_page_number → get_mapping_table → search → read`)
- Delete operations invoke at least three functions  
  (`db_delete → get_page_number → search → delete`)
- Split and merge operations may also be triggered
  > - Split condition: `num_keys < leaf_order - fill_factor`
  > - Merge condition: `num_keys == 0` and a sibling exists

# Diagram of Disk-Based B+ Tree
- [Outline of B+ Tree](outline)
- [Insert Operation](insert)
- [Find Operation](find)
- [Delete Operation](delete)
