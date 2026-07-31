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