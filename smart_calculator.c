#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define FILE_NAME "conversion_history.txt"
#define EPS 1e-6

typedef double (*ConvFn)(double);
typedef struct { int type; double in, out; } Rec;
typedef struct { Rec *a; size_t n, cap; } Hist;
typedef void (*RecCb)(Rec *, void *);
typedef int  (*FiltCb)(const Rec *, void *);
typedef int  (*CmpCb)(const Rec *, const Rec *);

static double c2f(double x){ return x * 9 / 5 + 32; }
static double f2c(double x){ return (x - 32) * 5 / 9; }
static double km2mi(double x){ return x * 0.621371; }
static double mi2km(double x){ return x / 0.621371; }
static double kg2lb(double x){ return x * 2.2046226218; }
static double lb2kg(double x){ return x / 2.2046226218; }
static double cm2in(double x){ return x / 2.54; }
static double in2cm(double x){ return x * 2.54; }

static const struct { const char *name, *fu, *tu; ConvFn fn; } T[] = {
    {"Celsius -> Fahrenheit","C","F",c2f},   {"Fahrenheit -> Celsius","F","C",f2c},
    {"Kilometres -> Miles","km","mi",km2mi}, {"Miles -> Kilometres","mi","km",mi2km},
    {"Kilograms -> Pounds","kg","lb",kg2lb}, {"Pounds -> Kilograms","lb","kg",lb2kg},
    {"Centimetres -> Inches","cm","in",cm2in},{"Inches -> Centimetres","in","cm",in2cm},
};
#define NT ((int)(sizeof T / sizeof *T))

static int readNum(const char *p, double lo, double hi, int isInt, double *out){
    char b[128], *e;
    for(;;){
        printf("%s", p);
        if(!fgets(b, sizeof b, stdin)) return 0;
        if(!strchr(b,'\n')){ int c; while((c=getchar())!='\n'&&c!=EOF); }
        double v = isInt ? (double)strtol(b,&e,10) : strtod(b,&e);
        while(isspace((unsigned char)*e)) e++;
        if(e==b || *e)            puts("  Invalid input: enter a number.");
        else if(isnan(v)||isinf(v)||v<lo||v>hi)
                                  printf("  Enter a value in [%g, %g].\n",lo,hi);
        else { *out=v; return 1; }
    }
}
static int readInt(const char *p,int lo,int hi,int *o){
    double v; if(!readNum(p,lo,hi,1,&v)) return 0; *o=(int)v; return 1;
}
static int add(Hist *h, Rec r){
    if(h->n == h->cap){
        size_t c = h->cap ? h->cap*2 : 8;
        Rec *t = realloc(h->a, c * sizeof *t);
        if(!t){ fprintf(stderr,"Error: allocation failed.\n"); return 0; }
        h->a = t; h->cap = c;
    }
    h->a[h->n++] = r; return 1;
}
static void show1(size_t i, const Rec *r){
    printf("  %3zu. %-24s %12.4f %-3s = %12.4f %-3s\n", i+1,
           T[r->type].name, r->in, T[r->type].fu, r->out, T[r->type].tu);
}
static void showAll(const Hist *h){
    if(!h->n){ puts("\nHistory is empty."); return; }
    puts("\n--- Conversion History ---");
    for(size_t i=0;i<h->n;i++) show1(i,&h->a[i]);
}

static void roundCb(Rec *r, void *ctx){
    double f = pow(10, *(int*)ctx);
    r->in = round(r->in*f)/f; r->out = round(r->out*f)/f;
}
typedef struct { int type; double lo, hi, val; } Crit;
static int byTypeCb (const Rec *r, void *c){ return r->type == ((Crit*)c)->type; }
static int byValCb  (const Rec *r, void *c){ return fabs(r->out-((Crit*)c)->val)<EPS; }
static int byRangeCb(const Rec *r, void *c){ Crit *k=c; return r->out>=k->lo && r->out<=k->hi; }
static int cmpType(const Rec *a,const Rec *b){ return a->type - b->type; }
static int cmpVal (const Rec *a,const Rec *b){ return (a->out>b->out)-(a->out<b->out); }

static size_t match(const Hist *h, FiltCb f, void *c){
    size_t n=0;
    for(size_t i=0;i<h->n;i++) if(f(&h->a[i],c)){ show1(i,&h->a[i]); n++; }
    if(!n) puts("  (no matches)"); else printf("Found %zu match(es).\n", n);
    return n;
}
static void sortH(Hist *h, CmpCb cmp){
    for(size_t i=1;i<h->n;i++){
        Rec k=h->a[i]; size_t j=i;
        while(j>0 && cmp(&h->a[j-1],&k)>0){ h->a[j]=h->a[j-1]; j--; }
        h->a[j]=k;
    }
}

static void save(const Hist *h){
    FILE *fp = fopen(FILE_NAME,"w");
    if(!fp){ perror("Error opening file"); return; }
    for(size_t i=0;i<h->n;i++)
        fprintf(fp,"%d %.10g %.10g\n",h->a[i].type,h->a[i].in,h->a[i].out);
    if(fclose(fp)) perror("Error closing file");
    else printf("Saved %zu record(s) to '%s'.\n", h->n, FILE_NAME);
}
static void load(Hist *h){
    FILE *fp = fopen(FILE_NAME,"r");
    if(!fp){ perror("Error opening file"); return; }
    h->n = 0;
    char b[128]; Rec r; size_t ok=0, bad=0;
    while(fgets(b,sizeof b,fp)){
        if(sscanf(b,"%d %lf %lf",&r.type,&r.in,&r.out)==3
           && r.type>=0 && r.type<NT && add(h,r)) ok++;
        else bad++;
    }
    fclose(fp);
    printf("Loaded %zu record(s)", ok);
    if(bad) printf(" (%zu bad line(s) skipped)", bad);
    puts(".");
}

static void convert(Hist *h){
    puts("");
    for(int i=0;i<NT;i++) printf("  %d. %s\n", i+1, T[i].name);
    int c; double v; char p[48];
    if(!readInt("Select conversion (1-8): ",1,NT,&c)) return;
    snprintf(p,sizeof p,"Enter value in %s: ",T[c-1].fu);
    if(!readNum(p,-1e12,1e12,0,&v)) return;
    double out = T[c-1].fn(v);
    printf("Result: %.4f %s = %.4f %s\n", v, T[c-1].fu, out, T[c-1].tu);
    if(add(h,(Rec){c-1,v,out})) puts("Record stored.");
}
static void search(const Hist *h){
    if(!h->n){ puts("\nHistory is empty."); return; }
    int o; Crit k={0};
    if(!readInt("\n1. By type  2. By converted value\nSelect (1-2): ",1,2,&o)) return;
    if(o==1){
        for(int i=0;i<NT;i++) printf("  %d. %s\n", i+1, T[i].name);
        if(!readInt("Type (1-8): ",1,NT,&o)) return;
        k.type=o-1; match(h,byTypeCb,&k);
    } else {
        if(!readNum("Value to find: ",-1e18,1e18,0,&k.val)) return;
        match(h,byValCb,&k);
    }
}
static void sortMenu(Hist *h){
    if(h->n<2){ puts("\nNeed at least two records."); return; }
    int o;
    if(!readInt("\n1. Sort by type  2. Sort by converted value\nSelect (1-2): ",1,2,&o)) return;
    sortH(h, o==1 ? cmpType : cmpVal);
    puts("Sorted."); showAll(h);
}
static void callbacks(Hist *h){
    if(!h->n){ puts("\nHistory is empty."); return; }
    int o;
    if(!readInt("\n1. Round all to precision  2. Filter by value range\nSelect (1-2): ",1,2,&o)) return;
    if(o==1){
        int d; if(!readInt("Decimal places (0-9): ",0,9,&d)) return;
        for(size_t i=0;i<h->n;i++) roundCb(&h->a[i], &d);
        printf("Rounded %zu record(s) to %d dp.\n", h->n, d); showAll(h);
    } else {
        Crit k={0};
        if(!readNum("Min value: ",-1e18,1e18,0,&k.lo) ||
           !readNum("Max value: ",-1e18,1e18,0,&k.hi)) return;
        if(k.lo>k.hi){ double t=k.lo; k.lo=k.hi; k.hi=t; }
        match(h,byRangeCb,&k);
    }
}

int main(void){
    Hist h = {0};
    for(int run=1; run;){
        puts("\n-- UNIT CONVERSION TOOLKIT --\n"
             " 1. Perform a conversion\n 2. View history\n 3. Search records\n"
             " 4. Sort records\n 5. Apply callback operations\n"
             " 6. Save history\n 7. Load history\n 8. Exit");
        int c;
        if(!readInt("Choice (1-8): ",1,8,&c)) break;
        switch(c){
            case 1: convert(&h);   break;
            case 2: showAll(&h);   break;
            case 3: search(&h);    break;
            case 4: sortMenu(&h);  break;
            case 5: callbacks(&h); break;
            case 6: save(&h);      break;
            case 7: load(&h);      break;
            case 8: puts("Goodbye!"); run=0; break;
        }
    }
    free(h.a);
    return 0;
}