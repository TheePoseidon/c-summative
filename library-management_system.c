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

# Main menu

int main(void)
 {
    Library lib;
    int choice;
    int unsaved = 0;

    if (!libraryInit(&lib)) {
        fprintf(stderr, "Fatal: could not allocate initial memory.\n");
        return EXIT_FAILURE;
    }

    loadFromFile(&lib);
    printf("\n");
    printf("   LIBRARY BOOK INVENTORY MANAGEMEBNT SYSTEM\n");
    printf("\n");

    do {
        printMenu();
        choice = readMenuChoice(0, 8);

        switch (choice) {
            case 1: addBook(&lib);          unsaved = 1; break;
            case 2: displayAllBooks(&lib);               break;
            case 3: updateBook(&lib);       unsaved = 1; break;
            case 4: deleteBook(&lib);       unsaved = 1; break;
            case 5: searchMenu(&lib);                    break;
            case 6: sortMenu(&lib);         unsaved = 1; break;
            case 7: generateReport(&lib);                break;
            case 8:
                if (saveToFile(&lib)) {
                    printf("Records saved successfully to '%s'.\n", DATA_FILE);
                    unsaved = 0;
                }
                break;
            case 0:
                if (unsaved && lib.count > 0) {
                    char ans[8];
                    readString("You have unsaved changes. Save before exit? (y/n): ",
                               ans, sizeof ans);
                    if (tolower((unsigned char)ans[0]) == 'y') {
                        if (saveToFile(&lib))
                            printf("Records saved to '%s'.\n", DATA_FILE);
                    }
                }
                printf("Goodbye!\n");
                break;
        }
    } while (choice != 0);

    libraryFree(&lib);
    return EXIT_SUCCESS;
}

# Manage memroy


