#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define FILE_NAME "library.dat"

typedef struct {
    int  id;
    char title[100];
    char author[100];
    char category[50];
    int  copies;
} Book;

Book *books = NULL;
int count = 0, capacity = 0;

int readInt(const char *prompt, int min, int max) {
    char line[64]; int v; char extra;
    while (1) {
        printf("%s", prompt);
        if (!fgets(line, sizeof line, stdin)) exit(1);
        if (sscanf(line, "%d %c", &v, &extra) == 1 && v >= min && v <= max)
            return v;
        printf("  Invalid input. Enter a number between %d and %d.\n", min, max);
    }
}
void readString(const char *prompt, char *buf, int size) {
    while (1) {
        printf("%s", prompt);
        if (!fgets(buf, size, stdin)) exit(1);
        buf[strcspn(buf, "\n")] = '\0';
        if (buf[0] != '\0') return;
        printf("  Input cannot be empty.\n");
    }
}
int ensureCapacity(void) {
    if (count < capacity) return 1;
    int newCap = capacity ? capacity * 2 : 4;
    Book *tmp = realloc(books, newCap * sizeof(Book));
    if (!tmp) { printf("Error: memory allocation failed.\n"); return 0; }
    books = tmp; capacity = newCap;
    return 1;
}
int findById(int id) {
    for (int i = 0; i < count; i++)
        if (books[i].id == id) return i;
    return -1;
}

void printBook(Book *b) {
    printf("%-6d %-30.30s %-20.20s %-15.15s %6d\n",
           b->id, b->title, b->author, b->category, b->copies);
}

void printHeader(void) {
    printf("%-6s %-30s %-20s %-15s %6s\n", "ID", "Title", "Author", "Category", "Copies");
    printf("\n");
}

void displayAll(void) {
    if (count == 0) { printf("Inventory is empty.\n"); return; }
    printHeader();
    for (int i = 0; i < count; i++) printBook(&books[i]);
}

void addBook(void) {
    Book b;
    b.id = readInt("Book ID: ", 1, 999999);
    if (findById(b.id) != -1) { printf("Error: ID %d already exists.\n", b.id); return; }
    readString("Title: ", b.title, sizeof b.title);
    readString("Author: ", b.author, sizeof b.author);
    readString("Category: ", b.category, sizeof b.category);
    b.copies = readInt("Copies available: ", 0, 100000);
    if (!ensureCapacity()) return;
    books[count++] = b;
    printf("Book added.\n");
}

void updateBook(void) {
    int i = findById(readInt("ID of book to update: ", 1, 999999));
    if (i == -1) { printf("Book not found.\n"); return; }
    printHeader(); printBook(&books[i]);
    readString("New title: ", books[i].title, sizeof books[i].title);
    readString("New author: ", books[i].author, sizeof books[i].author);
    readString("New category: ", books[i].category, sizeof books[i].category);
    books[i].copies = readInt("New copies: ", 0, 100000);
    printf("Book updated.\n");
}

void deleteBook(void) {
    int i = findById(readInt("ID of book to delete: ", 1, 999999));
    if (i == -1) { printf("Book not found.\n"); return; }
    for (; i < count - 1; i++) books[i] = books[i + 1];
    count--;
    printf("Book deleted.\n");
}

void searchBooks(void) {
    int c = readInt("Search by 1) ID  2) Title: ", 1, 2);
    if (c == 1) {
        int i = findById(readInt("Book ID: ", 1, 999999));
        if (i == -1) printf("Book not found.\n");
        else { printHeader(); printBook(&books[i]); }
    } else {
        char q[100]; int found = 0;
        readString("Title contains: ", q, sizeof q);
        for (int i = 0; i < count; i++)
            if (strstr(books[i].title, q)) {
                if (!found) printHeader();
                printBook(&books[i]); found = 1;
            }
        if (!found) printf("No matching books.\n");
    }
}

void sortBooks(void) {
    int key = readInt("Sort by 1) ID  2) Title  3) Copies: ", 1, 3);
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - 1 - i; j++) {
            int swap = (key == 1) ? books[j].id > books[j + 1].id
                     : (key == 2) ? strcmp(books[j].title, books[j + 1].title) > 0
                                  : books[j].copies < books[j + 1].copies;
            if (swap) { Book t = books[j]; books[j] = books[j + 1]; books[j + 1] = t; }
        }
    printf("Sorted.\n");
    displayAll();
}

void report(void) {
    if (count == 0) { printf("Inventory is empty.\n"); return; }
    long total = 0; int max = 0;
    for (int i = 0; i < count; i++) {
        total += books[i].copies;
        if (books[i].copies > books[max].copies) max = i;
    }
    printf("-- INVENTORY REPORT --\n");
    printf("Total number of books : %d\n", count);
    printf("Total copies available: %ld\n", total);
    printf("Most copies           : \"%s\" (%d copies)\n",
           books[max].title, books[max].copies);
    printf("Books per category:\n");
    int *done = calloc(count, sizeof(int));
    if (!done) { printf("Error: memory allocation failed.\n"); return; }
    for (int i = 0; i < count; i++) {
        if (done[i]) continue;
        int c = 0;
        for (int j = i; j < count; j++)
            if (strcmp(books[i].category, books[j].category) == 0)
                { c++; done[j] = 1; }
        printf("  %-15s : %d\n", books[i].category, c);
    }
    free(done);
    printf("\n");
}

void saveFile(void) {
    FILE *f = fopen(FILE_NAME, "wb");
    if (!f) { printf("Error: cannot open file for writing.\n"); return; }
    fwrite(&count, sizeof count, 1, f);
    fwrite(books, sizeof(Book), count, f);
    fclose(f);
    printf("Saved %d record(s) to %s.\n", count, FILE_NAME);
}

void loadFile(void) {
    FILE *f = fopen(FILE_NAME, "rb");
    if (!f) return;
    if (fread(&count, sizeof count, 1, f) == 1 && count > 0) {
        books = malloc(count * sizeof(Book));
        if (!books || fread(books, sizeof(Book), count, f) != (size_t)count) {
            printf("Warning: could not load data. Starting empty.\n");
            free(books); books = NULL; count = 0;
        } else capacity = count;
    } else count = 0;
    fclose(f);
    if (count) printf("Loaded %d record(s) from %s.\n", count, FILE_NAME);
}

int main(void) {
    loadFile();
    while (1) {
        printf("\n-- LIBRARY INVENTORY --\n"
               "1. Add book\n2. Display all\n3. Update book\n4. Delete book\n"
               "5. Search\n6. Sort\n7. Report\n8. Save to file\n0. Exit\n");
        switch (readInt("Choice: ", 0, 8)) {
            case 1: addBook();    break;
            case 2: displayAll(); break;
            case 3: updateBook(); break;
            case 4: deleteBook(); break;
            case 5: searchBooks();break;
            case 6: sortBooks();  break;
            case 7: report();     break;
            case 8: saveFile();   break;
            case 0: saveFile(); free(books); return 0;
        }
    }
}