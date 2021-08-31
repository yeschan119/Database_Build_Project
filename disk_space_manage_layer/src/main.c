//
//  main.c
//  Project2
//
//  Created by Eungchan on 2020/09/28.
//

#include "file.h"
#include <stdio.h>
// MAIN
//char table_id = '1';   // table_id
char **File_ID;

int main( int argc, char ** argv ) {
    table_ID();
    char * pathname = malloc(sizeof(path_length));
    pagenum_t input;
    int fd = 0;
    char* value = malloc(sizeof(120));
    char instruction;
    init_test_table();
    init_db(20);
    printf("Input the pathname : ");
    scanf("%s", pathname);
    printf("%s open\n", pathname);
    printf("table id %d\n", open_table(pathname));
    usage_2();
    
    printf("> ");
    getchar();
    while (scanf("%c", &instruction) != EOF) {
        
        switch (instruction) {
        case 'd':
            printf("Insert the key : ");
            scanf("%lld", &input);
            //db_delete(input);
            buffer_delete(input);
            break;
        case 'i':
            //printf("choose table id : ");
            //scanf("%d", &fd);
            //fd = get_fd(k);
            printf("Insert the key, value(using comma) : ");
            scanf("%lld, %s", &input ,value);
            db_insert(fd, input, value);
            break;
        case 'b':
            printf("Insert the key, value(using comma) : ");
            scanf("%lld, %s", &input ,value);
            buffer_insert(input, value);
            break;
        case 'f':
            printf("Insert the key : ");
            scanf("%lld", &input);
            //db_find(input, value);
            buffer_find(input);
            break;
        case 'p':
            printf("Print start >> \n\n");
            db_print();
            break;
        case 't':
            printf("Input the pathname : ");
            scanf("%s", pathname);
            printf("%s open\n", pathname);
            printf("table id %d\n", open_table(pathname));
            break;
        case 'c':
            printf("close table %d\n",close_table(0));
            break;
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;
        
        default:
            //usage_2();
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");
    return EXIT_SUCCESS;
}

// get pathname and return pathname with unique table id


void table_ID()
{
    printf("호출\\n");
    char buffer[20]= {0,};
    File_ID = (char**)malloc(FILE_ID);
    for (int i = 0; i < FILE_ID; i++)
    {
        File_ID[i] = (char*)malloc(path_length);
        strcpy(File_ID[i] ,buffer);
    }
}
test_table_id * sample;

void init_test_table(){
    sample = malloc(sizeof(struct test_table_id) * 10);
    for(int i = 0; i < sizeof(sample); i++){
        sample[i].id = 0;
        sample[i].path = malloc(sizeof(char) * 20);
    }
}
int my_table(char * pathname){
    int i = 0;
    int id = 0;
    bool loop = true;
    printf("sample[i] %s\n", sample[0].path);
    while(sample[i].id != 0 && i < sizeof(sample) && loop){
        printf("%s, %s", sample[i].path, pathname);
        if(strcmp(sample[i].path, pathname) == 0)
        {return sample[i].id;}
        id = sample[i].id;
        i++;
    }
    printf("table id %d\n", sample[i].id);
    if(id == sizeof(sample))
    {
        printf("no need more table\n");
        return -1;
    }
    else if(sample[i].id == 0){
        sample[i].path = pathname;
        sample[i].id = id + 1;
        return sample[i].id;
    }
    else
        return sample[i].id;
}
int get_table_id(char * path){
    int i = 0;
    char *buffer = malloc(sizeof(char) * 20);
    //int xx = strcmp(File_ID[0], path);
    printf("strcmp %s\n",File_ID[1]);
    strcpy(buffer, File_ID[i]);
    while(strcmp(path, buffer) != 0 && i < FILE_ID ){
        i++;
        strcpy(buffer, File_ID[i]);
    }
    printf("after strcmp %d\n", i);
    if(i == FILE_ID)
    {
        printf("No matched file\n");
        return -1;
    }
    else
        return ++i;
}

int put_table_id(char *path){
    int i = 0;
    printf("file_id %d\n",(*File_ID[0]));
    while(((*File_ID[i]) != 0) && i < FILE_ID){
        i++;
    }
    if(i == FILE_ID)
    {
        printf("No more space to get table\n");
        return -1;
    }
    else
    {
        printf("put %d\n", i);
        File_ID[i] = path;
        printf("\nnothing? %s\n", File_ID[i]);
        return ++i;
    }
}
