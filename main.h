#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1
#define NEIGHBOURS 4
#define MAX_TEMPERATURE 90
#define MIN_TEMPERATURE 50
#define TOLERANCE 5

/* Prototype functions. */
int base_station(MPI_Comm world_comm, MPI_Comm comm, int iter, int nrows, int ncols);
int sensor_nodes(MPI_Comm world_comm, MPI_Comm comm, int nrows, int ncols);
void *generate_temperature(void *pArg);
void convertToTimeStamp(char *buf, int size);
