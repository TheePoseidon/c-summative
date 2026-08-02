/*
 * ============================================================================
 *  file_processor.c
 *
 *  Multi-threaded File Processing System (POSIX threads)
 *  -----------------------------------------------------
 *  Processes multiple text files concurrently. One thread is created per
 *  input file. Each thread independently:
 *      1. Opens and validates its assigned file.
 *      2. Counts lines, words, and characters.
 *      3. Writes the results to a separate output file
 *         (e.g. "notes.txt"  ->  "notes.txt_analysis.txt").
 *      4. Reports its processing status on the terminal.
 *
 *  Because every thread works on a distinct file and writes to a distinct
 *  output file, no mutexes or other synchronization primitives are needed
 *  for the file work itself.
 *
 *  Build:   gcc -Wall -Wextra -pthread -o file_processor file_processor.c
 *  Usage:   ./file_processor file1.txt file2.txt file3.txt ...
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>

#define OUTPUT_SUFFIX "_analysis.txt"

/* ----------------------------------------------------------------------------
 * Data structures
 * ------------------------------------------------------------------------- */

/* Holds the statistics gathered for a single file. */
typedef struct {
    long lines;      /* number of newline-terminated lines (incl. last line) */
    long words;      /* number of whitespace-separated words                 */
    long characters; /* total number of characters (bytes) read              */
} FileStats;

/* Per-thread task descriptor: everything a worker thread needs, plus the
 * fields it fills in so main() can print a final summary. */
typedef struct {
    int         thread_id;   /* human-friendly thread number (1-based)   */
    const char *input_path;  /* file this thread must analyze            */
    char        output_path[4096]; /* where the results are written      */
    FileStats   stats;       /* results of the analysis                  */
    int         success;     /* 1 = processed OK, 0 = failed             */
    char        error_msg[256]; /* description of the failure, if any    */
} ThreadTask;

/* ----------------------------------------------------------------------------
 * Analysis functions
 * ------------------------------------------------------------------------- */

/*
 * analyze_file
 * ------------
 * Reads the file character by character and fills in `stats`.
 *
 * Counting rules:
 *   - characters: every byte read counts (including spaces and newlines).
 *   - words:      maximal runs of non-whitespace characters.
 *   - lines:      each '\n' ends a line; a final line without a trailing
 *                 newline is still counted as a line.
 *
 * Returns 0 on success, -1 if the file cannot be opened (errno is set).
 */
static int analyze_file(const char *path, FileStats *stats)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return -1; /* caller inspects errno for the reason */
    }

    stats->lines = 0;
    stats->words = 0;
    stats->characters = 0;

    int c;
    int in_word = 0;          /* are we currently inside a word?          */
    int last_char = '\n';     /* tracks whether file ends with a newline  */

    while ((c = fgetc(fp)) != EOF) {
        stats->characters++;

        if (c == '\n') {
            stats->lines++;
        }

        if (isspace(c)) {
            in_word = 0;      /* whitespace ends any current word */
        } else if (!in_word) {
            in_word = 1;      /* first character of a new word    */
            stats->words++;
        }

        last_char = c;
    }

    /* Count a trailing line that has content but no final newline. */
    if (last_char != '\n' && stats->characters > 0) {
        stats->lines++;
    }

    fclose(fp);
    return 0;
}

/*
 * write_report
 * ------------
 * Writes the analysis results for one file to its own output file.
 * Returns 0 on success, -1 on failure (errno is set).
 */
static int write_report(const char *output_path,
                        const char *input_path,
                        const FileStats *stats)
{
    FILE *out = fopen(output_path, "w");
    if (out == NULL) {
        return -1;
    }

    fprintf(out, "===== File Analysis Report =====\n");
    fprintf(out, "Input file : %s\n", input_path);
    fprintf(out, "--------------------------------\n");
    fprintf(out, "Lines      : %ld\n", stats->lines);
    fprintf(out, "Words      : %ld\n", stats->words);
    fprintf(out, "Characters : %ld\n", stats->characters);
    fprintf(out, "================================\n");

    fclose(out);
    return 0;
}

/*
 * build_output_path
 * -----------------
 * Derives the output file name from the input file name by appending
 * OUTPUT_SUFFIX, e.g. "data.txt" -> "data.txt_analysis.txt".
 */
static void build_output_path(const char *input_path,
                              char *buffer, size_t buffer_size)
{
    snprintf(buffer, buffer_size, "%s%s", input_path, OUTPUT_SUFFIX);
}

/* ----------------------------------------------------------------------------
 * Thread worker
 * ------------------------------------------------------------------------- */

/*
 * process_file_thread
 * -------------------
 * Entry point for each worker thread. Analyzes the assigned file, writes
 * the report, and prints status messages to the terminal.
 *
 * Note on terminal output: individual printf() calls are thread-safe on
 * POSIX systems (stdio streams are internally locked), so status lines from
 * different threads will not be interleaved mid-line.
 */
static void *process_file_thread(void *arg)
{
    ThreadTask *task = (ThreadTask *)arg;

    printf("[Thread %d] Started processing '%s'\n",
           task->thread_id, task->input_path);

    /* --- Step 1: analyze the input file ---------------------------------- */
    if (analyze_file(task->input_path, &task->stats) != 0) {
        snprintf(task->error_msg, sizeof(task->error_msg),
                 "cannot open input file (%s)", strerror(errno));
        task->success = 0;
        fprintf(stderr, "[Thread %d] ERROR: '%s': %s\n",
                task->thread_id, task->input_path, task->error_msg);
        return NULL;
    }

    printf("[Thread %d] Analyzed '%s': %ld lines, %ld words, %ld characters\n",
           task->thread_id, task->input_path,
           task->stats.lines, task->stats.words, task->stats.characters);

    /* --- Step 2: write the per-file report ------------------------------- */
    if (write_report(task->output_path, task->input_path, &task->stats) != 0) {
        snprintf(task->error_msg, sizeof(task->error_msg),
                 "cannot write output file (%s)", strerror(errno));
        task->success = 0;
        fprintf(stderr, "[Thread %d] ERROR: '%s': %s\n",
                task->thread_id, task->output_path, task->error_msg);
        return NULL;
    }

    task->success = 1;
    printf("[Thread %d] Finished. Results saved to '%s'\n",
           task->thread_id, task->output_path);
    return NULL;
}

/* ----------------------------------------------------------------------------
 * Summary
 * ------------------------------------------------------------------------- */

/*
 * print_summary
 * -------------
 * After all threads have joined, prints a consolidated overview of what
 * succeeded and what failed.
 */
static void print_summary(const ThreadTask *tasks, int count)
{
    int ok = 0, failed = 0;

    printf("\n================ Processing Summary ================\n");
    for (int i = 0; i < count; i++) {
        if (tasks[i].success) {
            printf("  [OK]   %-25s -> %s\n",
                   tasks[i].input_path, tasks[i].output_path);
            ok++;
        } else {
            printf("  [FAIL] %-25s (%s)\n",
                   tasks[i].input_path, tasks[i].error_msg);
            failed++;
        }
    }
    printf("----------------------------------------------------\n");
    printf("  %d file(s) processed successfully, %d failed.\n", ok, failed);
    printf("====================================================\n");
}

/* ----------------------------------------------------------------------------
 * Main
 * ------------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    /* --- Validate command-line arguments --------------------------------- */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1> <file2> ... <fileN>\n", argv[0]);
        fprintf(stderr, "Example: %s notes.txt report.txt data.txt\n", argv[0]);
        return EXIT_FAILURE;
    }

    int file_count = argc - 1;

    /* --- Allocate one task descriptor and one thread per file ------------ */
    ThreadTask *tasks = calloc((size_t)file_count, sizeof(ThreadTask));
    pthread_t  *threads = calloc((size_t)file_count, sizeof(pthread_t));
    int        *created = calloc((size_t)file_count, sizeof(int));

    if (tasks == NULL || threads == NULL || created == NULL) {
        fprintf(stderr, "ERROR: out of memory\n");
        free(tasks); free(threads); free(created);
        return EXIT_FAILURE;
    }

    printf("Starting multi-threaded processing of %d file(s)...\n\n",
           file_count);

    /* --- Launch one thread per input file -------------------------------- */
    for (int i = 0; i < file_count; i++) {
        tasks[i].thread_id = i + 1;
        tasks[i].input_path = argv[i + 1];
        tasks[i].success = 0;
        build_output_path(tasks[i].input_path,
                          tasks[i].output_path, sizeof(tasks[i].output_path));

        int rc = pthread_create(&threads[i], NULL,
                                process_file_thread, &tasks[i]);
        if (rc != 0) {
            /* Thread creation itself failed: record it and move on. */
            snprintf(tasks[i].error_msg, sizeof(tasks[i].error_msg),
                     "pthread_create failed (%s)", strerror(rc));
            fprintf(stderr, "[Main] ERROR: could not create thread for "
                            "'%s': %s\n", tasks[i].input_path, strerror(rc));
            created[i] = 0;
        } else {
            created[i] = 1;
        }
    }

    /* --- Wait for every successfully created thread to finish ------------ */
    for (int i = 0; i < file_count; i++) {
        if (created[i]) {
            pthread_join(threads[i], NULL);
        }
    }

    /* --- Final report ----------------------------------------------------- */
    print_summary(tasks, file_count);

    int all_ok = 1;
    for (int i = 0; i < file_count; i++) {
        if (!tasks[i].success) {
            all_ok = 0;
            break;
        }
    }

    free(tasks);
    free(threads);
    free(created);

    return all_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}