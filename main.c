/*
 * Single Server Queue simulation with expovariate arrivals.
 * 
 */

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include "rngs.h"

#define SEEDS        4
#define LAST         10000L                   /* number of jobs processed */
#define START        0.0                      /* initial time             */
#define EPS          0.001
#define SAMPLE       50
#define NUMEL(x)    (sizeof(x) / sizeof((x)[0]))
#define LIMIT       3.83

typedef struct { double i, r, a, s, b, d, c, w; } Job;
typedef struct { double d, w, s, r,idle, verify; } Record;
typedef struct { double l, q, x, verify; } TimeStats;
typedef struct {
    int x[SEEDS][LAST/SAMPLE];
    double y[SEEDS][LAST/SAMPLE];
    long seed[SEEDS];
} Test;


double Exponential(double m);
double Uniform(double a, double b);
double GetArrival(void);
double GetService(void);
void check(double verify, double n);

void plotter(Test *test) {
    FILE *gnuplot = popen("gnuplot -persistent", "w");
    fprintf(gnuplot,"set title \"Average Wait \"\n set yrange [2:5]\n plot ");

    char f[16];
    int i, j;

    for (i = 0; i < NUMEL(test->seed); i++) {
        sprintf(f, "%ld.tmp",test->seed[i]);
        printf("%s\n",f);
        FILE *temp = fopen(f, "w");
        for (j = 0; j < NUMEL(test->x[i]); j++)
            fprintf(temp, "%d %2.2f \n", test->x[i][j],test->y[i][j]); //Write the data to a temporary file

        fprintf(gnuplot, "'%s' with lines ",f);

        if(i < NUMEL(test->seed)-1)
            fprintf(gnuplot,",");
    }
    fprintf(gnuplot,"\n");
}

/* Functions for exponential/uniform distributions */
double Exponential(double m) { return (-m * log( 1.0 - Random())); }
double Uniform(double a, double b) { return (a + (b - a) * Random()); }


/* expovariate arrival time */
double GetArrival(void)
{
    static double arrival = START;
    SelectStream(0);
    arrival += Exponential(2.0);
    return (arrival);
}

/* Uniform service time */
double GetService(void)
{
    SelectStream(2);
    return (Uniform(1.0, 2.0));
}

/* verifies job/time stats */
void check(double verify, double n)
{
    printf(fabs(verify - n) < EPS ? "Pass.\n" : "Fail.\n");
}


int init(Test *test, int ind) {
    /* Some structs to keep track of data */
    Record sum = {0.0, 0.0, 0.0, 0.0, 0.0};
    Record avg = {0.0, 0.0, 0.0, 0.0, 0.0};
    TimeStats timeStats = {0.0, 0.0, 0.0, 0.0};
    Job job = {0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    int n = 0;
    long index = 0;                         /* job index            */
    double arrival = START;                     /* time of arrival      */
    double delay;                                 /* delay in queue       */
    double service;                               /* service time         */
    double wait;                                  /* delay + service      */
    double departure = START;                     /* time of departure    */
    double idle;                                  /* idle time b/w jobs   */
    double running = 0;
    PlantSeeds(test->seed[ind]);                           /* Set the RNG seed */
    printf("\n\nSeed = %ld\n",test->seed[ind]);
    while (index < LAST) {
        index++;
        job.r = arrival;
        arrival = GetArrival();
        job.r = arrival - job.r;
        if (arrival < departure) {
            delay = departure - arrival;         /* delay in queue    */
            idle = 0;
        } else {
            delay = 0.0;                         /* no delay          */
            idle = arrival - departure;
        }

        service = GetService();
        wait = delay + service;
        running += wait;
        departure = arrival + wait;              /* time of departure */
        sum.d += delay;
        sum.w += wait;
        sum.s += service;
        sum.idle += idle;

        job.a = arrival;
        job.i = index;
        job.s = service;
        job.d = delay;
        job.w = wait;
        job.b = job.a + job.d;
        job.c = job.s + job.d + job.a;
        /* print avg wait every 50 jobs */
        if (index % SAMPLE == 0) {
            test->x[ind][n] = (int) index;
            test->y[ind][n] = sum.w/index;
//            running = 0;
            n += 1;
        }
        /* Reset the job struct */
        job = (Job) {index, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    }

    sum.r = arrival - START;
    avg.s = sum.s / (double) index;
    avg.d = sum.d / (double) index;
    avg.r = sum.r / (double) index;
    avg.w = sum.w / (double) index;
    avg.verify = avg.d + avg.s;

    timeStats.l = sum.w / departure;
    timeStats.q = sum.d / departure;
    timeStats.x = sum.s / departure;
    timeStats.verify = timeStats.q + timeStats.x;

    printf("\nfor %ld jobs\n", index);
    printf("   average interarrival time = %6.2f\n", sum.r / index);
    printf("   average wait ............ = %6.2f\n", sum.w / index);
    printf("   average delay ........... = %6.2f\n", sum.d / index);
    printf("   average service time .... = %6.2f\n", sum.s / index);
    printf("   average # in the node ... = %6.2f\n", sum.w / departure);
    printf("   average # in the queue .. = %6.2f\n", sum.d / departure);
    printf("   utilization ............. = %6.2f\n", sum.s / departure);
    printf("   convergence to %2.2f...... = %6.2f\n", LIMIT, fabs(LIMIT-sum.w/ index));
    /* Do some sanity checks here */
    printf("\nVerification\n\n");
    printf("   job-averaged statistics .... ");
    check(avg.verify, avg.w);
    printf("   time-averaged statistics ... ");
    check(timeStats.verify, timeStats.l);
    printf("   random number generator .... ");
    printf(TestRandom() == 1 ? "Pass.\n" : "Fail.\n");
    printf("   ....................................\n");
    return (0);
}


int main(void) {
    /* Test seed values */
    Test test = {.seed ={12345, 123456789, 54321, 212121} };

    static_assert(NUMEL(test.seed) <= SEEDS, "");

    for(int i = 0; i < NUMEL(test.seed); i++)
        init(&test,i);
    /* plot average wait times for each seed with gnuplot */
    plotter(&test);
    return 0;
}

