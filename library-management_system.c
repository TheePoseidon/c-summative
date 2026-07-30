#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype>

#define TITLE_LEN     100
#define AUTHOR_LEN    100
#define CATEGORY_LEN  50
#define INITIAL_CAP   4
#define DATA_FILE     "library.dat"

typedef struct {
    int  id;
    char title[TITLE_LEN];
    char author[AUTHOR_LEN];
    char category[CATEGORY_LEN];
    int  copies;
} Book;

typedef struct {
    Book *data;
    int   count;
    int   capacity;
} Library;
