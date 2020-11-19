#include "main.h"

int sensor_nodes(MPI_Comm world_comm, MPI_Comm comm, int nrows, int ncols) {
	int i, rand_temperature, count, exit = 0;
	int ndims = 2, size, my_rank, reorder, my_cart_rank, ierr, world_size;
	int nbr_i_lo, nbr_i_hi;
	int nbr_j_lo, nbr_j_hi;
	int dims[ndims], coord[ndims];
	int wrap_around[ndims];
	double time_taken;
	
	MPI_Comm comm2D;
	
	MPI_Comm_size(world_comm, &world_size);		/* Size of world communicator. */
  	MPI_Comm_size(comm, &size);					/* Size of sensor node communicator. */
	MPI_Comm_rank(comm, &my_rank);				/* Rank of sensor node communicator. */
	
	dims[0] = nrows;							/* Number of rows. */
	dims[1] = ncols;							/* Number of columns. */
	
	/* Create cartesian topology for processes. */
	MPI_Dims_create(size, ndims, dims);
	if (my_rank == 0)
		printf("Root Rank: %d. Comm Size: %d: Grid Dimension = [%d x %d] \n", my_rank, size, dims[0], dims[1]);
	
	/* Create cartesian mapping. */
	wrap_around[0] = 0;
	wrap_around[1] = 0;							/* Periodic shift is false. */
	reorder = 1;
	ierr = 0;
	ierr = MPI_Cart_create(comm, ndims, dims, wrap_around, reorder, &comm2D);
	if (ierr != 0)
		printf("ERROR[%d] creating CART\n", ierr);
	
	/* Find my coordinates in the cartesian communicator group. */
	MPI_Cart_coords(comm2D, my_rank, ndims, coord);
	
	/* Use my cartesian coordinates to find my rank in cartesian group. */
	MPI_Cart_rank(comm2D, coord, &my_cart_rank);
	
	/* Get my neighbors; axis is coordinate dimension of shift.
	 * axis = 0 ==> shift along the rows: P[my_row-1] : P[me] : P[my_row+1]
	 * axis = 1 ==> shift along the columns: P[my_col-1] : P[me] : P[my_col+1] */
	MPI_Cart_shift(comm2D, SHIFT_ROW, DISP, &nbr_i_lo, &nbr_i_hi);
	MPI_Cart_shift(comm2D, SHIFT_COL, DISP, &nbr_j_lo, &nbr_j_hi);
	
	/* Initialize request and status arrays. */
	MPI_Request send_request[NEIGHBOURS];
	MPI_Request receive_request[NEIGHBOURS];
	MPI_Status send_status[NEIGHBOURS];
	MPI_Status receive_status[NEIGHBOURS];
	MPI_Status status;
	
	/* Seed random value. */
	srand((unsigned) time(NULL) + my_rank);
	
	while (1) {
		/* Check if base station signals to terminate. */
		MPI_Recv(&exit, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, world_comm, &status);
		
		/* Send received signal to the next process. */
		if (my_rank != size - 1)
			MPI_Send(&exit, 1, MPI_INT, my_rank + 1, 0, world_comm);
		if (exit)
			break;
		
		/* Generate a random temperature. */
		rand_temperature = (rand() % (MAX_TEMPERATURE - MIN_TEMPERATURE + 1)) + MIN_TEMPERATURE;
		
		/* Local process asynchronously send its temperature to its neighbours and likewise receive its neighbours' temperatures. */
		int neighbours[] = {nbr_i_lo, nbr_i_hi, nbr_j_lo, nbr_j_hi};
		int recv_temperature[] = {-1, -1, -1, -1};
		for (i = 0; i < NEIGHBOURS; i++) {
			MPI_Isend(&rand_temperature, 1, MPI_INT, neighbours[i], 0, comm2D, &send_request[i]);
			MPI_Irecv(&recv_temperature[i], 1, MPI_INT, neighbours[i], 0, comm2D, &receive_request[i]);
		}
		
		MPI_Waitall(NEIGHBOURS, send_request, send_status);
		MPI_Waitall(NEIGHBOURS, receive_request, receive_status);
		
		if (rand_temperature >= 80) {
			/* Local process compares the received temperatures with its own temperature. */
			count = 0;
			for (i = 0; i < NEIGHBOURS; i++) {
				if (abs(rand_temperature - recv_temperature[i]) <= TOLERANCE)
					count++;
			}
			
			/* If at least 2 neighbours have approximately same temperatures as the local process, alert the base station. */
			if (count >= 2) {
			/* Print results of alert. */
				printf("[ALERT] Global rank: %d. Cart rank: %d. Coord: (%d, %d). Random Temp: %d. Recv Top: %d. Recv Bottom: %d. Recv Left: %d. Recv Right: %d.\n", 
						my_rank, my_cart_rank, coord[0], coord[1], rand_temperature, recv_temperature[0], recv_temperature[1], recv_temperature[2], recv_temperature[3]);
				
				time_taken = MPI_Wtime();
				double report[] = {(double)my_rank, (double)nbr_i_lo, (double)nbr_i_hi, (double)nbr_j_lo, (double)nbr_j_hi, (double)rand_temperature, 
									(double)recv_temperature[0], (double)recv_temperature[1], (double)recv_temperature[2], (double)recv_temperature[3], time_taken};
				
				/* Send alert report to base station. */
				MPI_Send(report, 11, MPI_DOUBLE, world_size - 1, 0, world_comm);
			}
		}
	}
	
	/* Clean up. */
	MPI_Comm_free(&comm2D);
	return 0;
}
