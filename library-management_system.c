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

int   libraryInit(Library *lib);
int   libraryEnsureCapacity(Library *lib);
void  libraryFree(Library *lib);

void  flushInput(void);
int   readInt(const char *prompt, int min, int max);
void  readString(const char *prompt, char *buffer, int size);
int   readMenuChoice(int min, int max);
void  toLowerStr(const char *src, char *dst, int size);

void  addBook(Library *lib);
void  displayAllBooks(const Library *lib);
void  updateBook(Library *lib);
void  deleteBook(Library *lib);

int   findIndexById(const Library *lib, int id);
void  searchMenu(const Library *lib);
void  searchById(const Library *lib);
void  searchByTitle(const Library *lib);
void  sortMenu(Library *lib);
void  swapBooks(Book *a, Book *b);
void  sortById(Library *lib);
void  sortByTitle(Library *lib);
void  sortByCopies(Library *lib);
void  generateReport(const Library *lib);
int   saveToFile(const Library *lib);
int   loadFromFile(Library *lib);
void  printHeader(void);
void  printBook(const Book *b);
void  printMenu(void);

