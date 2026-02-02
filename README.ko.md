!!! 학부 시절에 작성한 코드입니다 (완전 엉망)… 도대체 언제 리팩토링할 수 있을까요?

# DB Engine의 Data-Level 설계 및 구현
+ 데이터 I/O 흐름이 Index layer → Buffer layer → Disk layer 순서로 진행되도록 설계
+ Index layer와 Buffer layer는 메모리 상에서 동작
+ Disk space layer는 system call을 이용하여 디스크 I/O 수행

# Project Diagram
![db_engine](https://github.com/yeschan119/Database_Build_Project/assets/83147205/2b062ec0-91de-4392-8f68-4844a1aca92f)

# DB_Build_Project
동시성 제어를 지원하며 데이터 저장과 insert, delete, find 연산이 가능한 데이터베이스 시스템을 구현한 프로젝트입니다.

최종 디스크 기반 B+ Tree 구현은 크게 두 부분으로 구성되어 있습니다.

- 첫 번째 부분은 B+ Tree의 개요 및 핵심 기능에 대한 설명입니다.
  > - Index layer 소개 (open, insert, find, delete)
  > - File API 소개 (write, read, allocate 등)
  > - File API를 지원하는 하위 함수들에 대한 설명

- 두 번째 부분은 각 함수의 역할과 디스크 기반 B+ Tree의 전체 설계를 설명하는 다이어그램입니다.

# 1. B+ Tree 개요

## Index Layer

### - open_table
  > - `open_table`은 다음과 같은 system call을 사용합니다.  
    `if (0 < (fd = open(pathname, O_RDWR | O_CREAT | O_EXCL, 0644)))`  
    이는 기존 테이블이 존재하면 열고, 존재하지 않으면 새로 생성하는 동작을 의미합니다.
  > - 생성 이후 `page_setting` 함수를 호출하여 page size를 정의하고 header page를 첫 번째 위치로 설정합니다.
  > - 최초 생성 시에는 header page만 생성됩니다.

### - db_insert
  > - 파일을 열고 key를 이용해 대상 page number를 가져옵니다.
  > - 초기 상태에서는 header page 하나만 존재합니다.
  > - header page를 통해 free page 또는 mapping page를 얻어 insertion에 사용합니다.
  > - 중복 key 여부를 검사하고, page split이 필요한지 확인합니다.
  > - 적절한 위치에 key를 기록하고 필요 시 root로 설정합니다.

### - db_find
  > - 파일을 열고 key를 이용해 page number를 조회합니다.
  > - `file_read` API를 이용해 key를 탐색합니다.
  > - key를 찾으면 0을 반환하고, 존재하지 않으면 -1을 반환합니다.

### - db_delete
  > - 파일을 열고 key를 이용해 page number를 조회합니다.
  > - search 함수를 이용해 정확한 위치를 찾습니다.
  > - 기존 값을 덮어쓰는 방식으로 삭제를 수행합니다.
  > - page의 key 개수를 감소시키고 재조정이 필요한지 확인합니다.
  > - `fsync()`를 호출하여 데이터 영속성을 보장합니다.

### - db_print
  > - header page를 이용해 root page를 가져옵니다.
  > - internal page(루트 포함)는 key와 page_id를 저장하며, leaf page는 key와 value를 저장합니다.
  > - page가 leaf인지 여부를 판단합니다.
  > - leaf page인 경우 모든 key와 value를 출력합니다.
  > - internal page인 경우 key만 출력합니다.
  > - root부터 leaf까지 level-order 방식으로 출력합니다.
  > - sibling 출력만을 담당하는 `sub_print` 함수가 존재하며 `db_print`를 보조합니다.

## File API (write, read, alloc, free)

### - file_alloc_page()
  > - header page를 통해 root를 확인합니다.
  > - header에 free page가 존재하면 해당 page를 반환하고 다음 free page를 갱신합니다.
  > - free page가 없을 경우 새 free page를 생성하며, 첫 free page는 header 바로 다음 page가 됩니다.
  > - 이후 free page 탐색은 `find_free_page()` 함수를 통해 수행합니다.

### - file_write_page()
  > - 주어진 page의 key 개수와 `is_leaf` 값을 확인합니다.
  > - `is_leaf` 플래그를 설정합니다 (1: leaf, 0: internal).
  > - 기존 page인 경우 바로 write를 수행합니다.
  > - write 시 linear sorting을 함께 수행합니다.
  > - 신규 page인 경우 page의 첫 위치에 write합니다.

### - write_internal_page()
  > - page split 이후 internal page 작성을 지원하기 위한 전용 API입니다.
  > - split 발생 시 left child의 key와 page number를 parent에 기록합니다.
  > - left child의 key와 page number를 복사합니다.
  > - parent의 올바른 위치에 이를 기록합니다.
  > - leftmost page 여부에 따른 예외 처리를 포함합니다.

### - file_read_page()
  > - page parameter를 이용해 파일을 열고 탐색을 수행합니다.
  > - binary search를 사용합니다.
  > - key를 찾으면 0, 찾지 못하면 -1을 반환합니다.
  > - search 결과를 확인합니다.

## Supporting Sub-Functions

### - insert_parent()
  > - page split이 필요할 때 호출됩니다.
  > - `file_alloc_page()`를 이용해 새로운 child 및 parent page를 생성합니다.
  > - 기존 parent가 없는 경우 `insert_new_parent()`를 호출합니다.
  > - new page와 old page 모두에 parent page number를 설정합니다.
  > - `write_internal_page()`를 호출합니다.
  > - key 개수를 증가시키고 sibling 정보를 설정합니다.

### - search_key()
  > - leaf page와 internal page 모두에서 사용 가능한 search 함수입니다.
  > - binary search 알고리즘 기반입니다.
  > - leaf 여부와 leftmost 여부를 먼저 확인합니다.
  > - leaf, 일반 internal, leftmost internal page 탐색을 모두 지원합니다.
  > - key를 찾으면 위치를 반환하고, 없으면 -1을 반환합니다.

### - get_page_number()
  > - 최초 호출 시 root page를 가져옵니다.
  > - root 존재 여부를 확인합니다.
  > - root가 없으면 새로운 page를 생성하여 root로 설정합니다.
  > - root가 존재하면 `find_leaf_page()`를 호출합니다.
  > - 최종 page number를 반환합니다.

### - find_leaf_page()
  > - `num_keys`와 `is_leaf`를 확인합니다.
  > - leaf page이면 즉시 반환합니다.
  > - internal page인 경우 leftmost 여부를 판단합니다.
  > - root부터 탐색을 진행하여 leaf page를 찾습니다.

### - sort_leaf_page()
  > - key/value 삽입 시 항상 정렬을 수행합니다.
  > - 첫 위치부터 비교하여 삽입 위치를 찾습니다.
  > - 이후 key들을 한 칸씩 뒤로 이동시킵니다.
  > - 해당 위치에 key와 value를 기록합니다.

### - split_page()
  > - `file_alloc_page()`를 이용해 새로운 child page를 생성합니다.
  > - 기존 page가 leaf인지 확인합니다.
  > - leaf page 또는 internal page에 맞는 split을 수행합니다.
  > - split 기준은 `num_keys`의 절반입니다.
  > - 새로 생성된 child page number를 반환합니다.

### - merge_page()
  > - 지연 병합 방식으로, page에 key가 하나도 없을 때만 병합을 수행합니다.
  > - right sibling을 확인하고 데이터를 이동합니다.
  > - sibling이 없으면 병합을 수행하지 않습니다.
  > - leaf page 데이터 제거 후 parent 존재 여부를 확인합니다.
  > - parent가 존재하면 leftmost 여부를 판단합니다.
  > - `search_key()`를 이용해 parent에서 해당 key를 제거합니다.
  > - 상위 parent에 대해 재귀적으로 동일 작업을 수행합니다.

### 기타 유틸리티 함수
  > - mapping table (header, free, internal/leaf)
  > - 에러 체크를 포함한 파일 open wrapper 함수
  > - `fsync()` 수행 및 에러 처리를 위한 동기화 함수
  > - `lltoa`, `itoa` (int, uint64_t → string 변환)
  > - page의 key 개수를 증가시키는 `keys_plus` 함수

# 2. 디스크 기반 B+ Tree 다이어그램 및 설명
- 코드는 `main`, `file.c`, `file.h` 세 부분으로 구성됩니다.
- main 함수는 open, find, delete, insert, print 기능을 포함합니다.
- `file.h`는 header 파일입니다.
- `file.c`는 다음 세 종류의 함수로 구성됩니다.
  > - Index layer 함수
  > - File operation 함수
  > - Supporting utility 함수
- insert 연산은 최소 4개의 함수 호출을 포함합니다.  
  (`db_insert → get_page_number → get_mapping_table → write → sort`)
- find 연산 역시 최소 4개의 함수 호출을 포함합니다.  
  (`db_find → get_page_number → get_mapping_table → search → read`)
- delete 연산은 최소 3개의 함수 호출을 포함합니다.  
  (`db_delete → get_page_number → search → delete`)
- 필요에 따라 split 및 merge가 수행됩니다.
  > - split 조건: `num_keys < leaf_order - fill_factor`
  > - merge 조건: `num_keys == 0` 이고 sibling이 존재할 경우

# Disk-Based B+ Tree Diagram
- [B+ Tree 개요](outline)
- [Insert 연산](insert)
- [Find 연산](find)
- [Delete 연산](delete)
