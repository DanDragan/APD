all: serial_optimizat serial_neoptimizat paralel_optimizat paralel_neoptimizat

serial_optimizat:
	gcc serial_optimizat.c -o serial_optimizat -Wall
	
serial_neoptimizat:
	gcc serial_neoptimizat.c -o serial_neoptimizat -Wall

paralel_optimizat:
	gcc -fopenmp paralel_optimizat.c -o paralel_optimizat -Wall
	
paralel_neoptimizat:
	gcc -fopenmp paralel_neoptimizat.c -o paralel_neoptimizat -Wall
	
clean:
	rm -f serial_neoptimizat serial_optimizat paralel_neoptimizat paralel_optimizat
