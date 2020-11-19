ALL:
	mpicc -Wall -o main main.c sensor_nodes.c base_station.c

run:
	mpirun -oversubscribe -np 13 main 4 3 1000

clean:
	/bin/rm -f main *.o
