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

int main( int argc, char ** argv ) {
    char * pathname = malloc(sizeof(path_length));
    pagenum_t input;
    int table_id;
    char* value = malloc(sizeof(VALUE));
    char instruction;
    int num_buf;
    
    printf("How many buffers do you want : ");
    scanf("%d", &num_buf);
    init_db(num_buf);
    printf("Input the pathname : ");
    scanf("%s", pathname);
    printf("%s open\n", pathname);
    table_id = open_table(pathname);
    if(table_id < 0)
    {
        perror("Table exists but can not use. Try again\n");
        exit(1);
    }
    printf("table id %d\n", table_id);
    usage_2();
    
    printf("> ");
    getchar();
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'd':
            printf("Insert the key : ");
            scanf("%lld", &input);
            //db_delete(input);
            db_delete(table_id, input);
            break;
        case 'i':
            printf("Insert the key, value(using comma) : ");
            scanf("%lld, %s", &input ,value);
            db_insert(table_id, input, value);
            break;
        case 'b':
            printf("Insert the key, value(using comma) : ");
            scanf("%lld, %s", &input ,value);
            //disk_insert(id, input, value);
            break;
        case 'f':
            printf("Insert the key : ");
            scanf("%lld", &input);
            db_find(table_id, input, value);
            break;
        case 'p':
            printf("Print start >> \n\n");
            db_print(table_id);
            break;
        case 'o':
            printf("Input the pathname : ");
            scanf("%s", pathname);
            printf("%s open\n", pathname);
            table_id = open_table(pathname);
            printf("table id %d\n",table_id);
            break;
        case 'c':
            if(0 <= (close_table(table_id)))
                printf("close succes\n");
            else
                printf("close error\n");
            break;
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;
        case 's':
                if(0 <= (shutdown_db()))
                    printf("sutdown_db succes\n");
                else
                    printf("shutdown_db error\n");
                break;
        default:
            usage_2();
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");
    return EXIT_SUCCESS;
}
//
