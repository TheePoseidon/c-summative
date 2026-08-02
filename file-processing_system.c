#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>

#define OUTPUT_SUFFIX "_analysis.txt"
typedef struct {
    long lines;
    long words;
    long characters;
} FileStats;
typedef struct {
    int         thread_id;
    const char *input_path;
    char        output_path[4096];
    FileStats   stats;
    int         success;
    char        error_msg[256];
} ThreadTask;

static int analyze_file(const char *path, FileStats *stats)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }

    stats->lines = 0;
    stats->words = 0;
    stats->characters = 0;

    int c;
    int in_word = 0;
    int last_char = '\n';

    while ((c = fgetc(fp)) != EOF) {
        stats->characters++;

        if (c == '\n') {
            stats->lines++;
        }

        if (isspace(c)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            stats->words++;
        }

        last_char = c;
    }
    if (last_char != '\n' && stats->characters > 0) {
        stats->lines++;
    }

    fclose(fp);
    return 0;
}
static int write_report(const char *output_path,
                        const char *input_path,
                        const FileStats *stats)
{
    FILE *out = fopen(output_path, "w");
    if (out == NULL) {
        return -1;
    }

    fprintf(out, "-- File Analysis Report--\n");
    fprintf(out, "Input file : %s\n", input_path);
    fprintf(out, "--\n");
    fprintf(out, "Lines      : %ld\n", stats->lines);
    fprintf(out, "Words      : %ld\n", stats->words);
    fprintf(out, "Characters : %ld\n", stats->characters);
    fprintf(out, "--\n");

    fclose(out);
    return 0;
}
 */
static void build_output_path(const char *input_path,
                              char *buffer, size_t buffer_size)
{
    snprintf(buffer, buffer_size, "%s%s", input_path, OUTPUT_SUFFIX);
}
static void *process_file_thread(void *arg)
{
    ThreadTask *task = (ThreadTask *)arg;

    printf("[Thread %d] Started processing '%s'\n",
           task->thread_id, task->input_path);
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
static void print_summary(const ThreadTask *tasks, int count)
{
    int ok = 0, failed = 0;

    printf("\n-- Processing Summary --\n");
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
    printf("--\n");
    printf("  %d file(s) processed successfully, %d failed.\n", ok, failed);
    printf("--\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1> <file2> ... <fileN>\n", argv[0]);
        fprintf(stderr, "Example: %s notes.txt report.txt data.txt\n", argv[0]);
        return EXIT_FAILURE;
    }

    int file_count = argc - 1;
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
    for (int i = 0; i < file_count; i++) {
        tasks[i].thread_id = i + 1;
        tasks[i].input_path = argv[i + 1];
        tasks[i].success = 0;
        build_output_path(tasks[i].input_path,
                          tasks[i].output_path, sizeof(tasks[i].output_path));

        int rc = pthread_create(&threads[i], NULL,
                                process_file_thread, &tasks[i]);
        if (rc != 0) {
            snprintf(tasks[i].error_msg, sizeof(tasks[i].error_msg),
                     "pthread_create failed (%s)", strerror(rc));
            fprintf(stderr, "[Main] ERROR: could not create thread for "
                            "'%s': %s\n", tasks[i].input_path, strerror(rc));
            created[i] = 0;
        } else {
            created[i] = 1;
        }
    }
    for (int i = 0; i < file_count; i++) {
        if (created[i]) {
            pthread_join(threads[i], NULL);
        }
    }
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