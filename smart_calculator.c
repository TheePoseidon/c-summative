#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define INITIAL_CAPACITY   8 
#define MAX_LINE           256
#define HISTORY_FILE       "conversion_history.txt"
#define VALUE_EPSILON      1e-6

typedef double (*ConversionFunc)(double input);
typedef struct {
    const char     *name;
    const char     *fromUnit;
    const char     *toUnit;
    ConversionFunc  convert;
} ConversionType;

typedef struct {
    int    typeIndex;
    double inputValue;
    double outputValue;
} ConversionRecord;

typedef struct {
    ConversionRecord *records;
    size_t            count;
    size_t            capacity;
} History;

typedef void (*RecordCallback)(ConversisonRecord *rec, void *context);
typedef int (*FilterCallback)(const ConversionRecord *rec, void *criteria);
typedef int (*CompareCallback)(const ConversionRecord *a,
                               const ConversionRecord *b);

static double celsiusToFahrenheit(double c)  { return c * 9.0 / 5.0 + 32.0; }
static double fahrenheitToCelsius(double f)  { return (f - 32.0) * 5.0 / 9.0; }
static double kilometresToMiles(double km)   { return km * 0.621371; }
static double milesToKilometres(double mi)   { return mi / 0.621371; }
static double kilogramsToPounds(double kg)   { return kg * 2.2046226218; }
static double poundsToKilograms(double lb)   { return lb / 2.2046226218; }
static double centimetresToInches(double cm) { return cm / 2.54; }
static double inchesToCentimetres(double in) { return in * 2.54; }

static const ConversionType CONVERSIONS[] = {
    { "Celsius -> Fahrenheit",     "C",  "F",  celsiusToFahrenheit  },
    { "Fahrenheit -> Celsius",     "F",  "C",  fahrenheitToCelsius  },
    { "Kilometres -> Miles",       "km", "mi", kilometresToMiles    },
    { "Miles -> Kilometres",       "mi", "km", milesToKilometres    },
    { "Kilograms -> Pounds",       "kg", "lb", kilogramsToPounds    },
    { "Pounds -> Kilograms",       "lb", "kg", poundsToKilograms    },
    { "Centimetres -> Inches",     "cm", "in", centimetresToInches  },
    { "Inches -> Centimetres",     "in", "cm", inchesToCentimetres  },
};
#define NUM_CONVERSIONS ((int)(sizeof(CONVERSIONS) / sizeof(CONVERSIONS[0])))

static int readLine(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL) {
        return 0;
    }
    char *nl = strchr(buf, '\n');
    if (nl != NULL) {
        *nl = '\0';
    } else {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) { /* skip */ }
    }
    return 1;
}

static int readInt(const char *prompt, int min, int max, int *out)
{
    char line[MAX_LINE];

    for (;;) {
        printf("%s", prompt);
        if (!readLine(line, sizeof line)) {
            return 0;
        }
        char *end = NULL;
        long v = strtol(line, &end, 10);

        while (end != NULL && isspace((unsigned char)*end)) end++;

        if (end == line || (end != NULL && *end != '\0')) {
            printf("  Invalid input: please enter a whole number.\n");
        } else if (v < min || v > max) {
            printf("  Out of range: enter a value between %d and %d.\n",
                   min, max);
        } else {
            *out = (int)v;
            return 1;
        }
    }
}

static int readDouble(const char *prompt, double *out)
{
    char line[MAX_LINE];

    for (;;) {
        printf("%s", prompt);
        if (!readLine(line, sizeof line)) {
            return 0;
        }
        char *end = NULL;
        double v = strtod(line, &end);

        while (end != NULL && isspace((unsigned char)*end)) end++;

        if (end == line || (end != NULL && *end != '\0')) {
            printf("  Invalid input: please enter a number.\n");
        } else if (isnan(v) || isinf(v)) {
            printf("  Invalid input: value is not finite.\n");
        } else {
            *out = v;
            return 1;
        }
    }
}

static int historyInit(History *h)
{
    h->records  = malloc(INITIAL_CAPACITY * sizeof *h->records);
    h->count    = 0;
    h->capacity = INITIAL_CAPACITY;
    if (h->records == NULL) {
        fprintf(stderr, "Fatal: could not allocate history storage.\n");
        return 0;
    }
    return 1;
}

static int historyAdd(History *h, ConversionRecord rec)
{
    if (h->count == h->capacity) {
        size_t newCap = h->capacity * 2;
        ConversionRecord *tmp =
            realloc(h->records, newCap * sizeof *tmp);
        if (tmp == NULL) {
            fprintf(stderr,
                    "Error: memory allocation failed; record not stored.\n");
            return 0;
        }
        h->records  = tmp;
        h->capacity = newCap;
    }
    h->records[h->count++] = rec;
    return 1;
}

static void historyFree(History *h)
{
    free(h->records);
    h->records  = NULL;
    h->count    = 0;
    h->capacity = 0;
}

static void printRecord(size_t index, const ConversionRecord *r)
{
    const ConversionType *t = &CONVERSIONS[r->typeIndex];
    printf("  %3zu. %-26s %12.4f %-3s = %12.4f %-3s\n",
           index + 1, t->name,
           r->inputValue, t->fromUnit,
           r->outputValue, t->toUnit);
}

static void historyPrint(const History *h)
{
    if (h->count == 0) {
        printf("\nHistory is empty.\n");
        return;
    }
    printf("\n-- Conversion History --\n");
    for (size_t i = 0; i < h->count; i++) {
        printRecord(i, &h->records[i]);
    }
    printf("\n");
}
static void forEachRecord(History *h, RecordCallback cb, void *context)
{
    for (size_t i = 0; i < h->count; i++) {
        cb(&h->records[i], context);
    }
}