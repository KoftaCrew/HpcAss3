// Kareem Mohamed Morsy		         , ID: 20190386, Group: CS-S3, Program: CS
// Mohamed Ashraf Mohamed Ali        , ID: 20190424, Group: CS-S3, Program: CS
// Mostafa Mahmoud Anwar Morsy Sadek , ID: 20190544, Group: CS-S3, Program: CS

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <omp.h>
#include "mpi.h"

int main(int argc, char *argv[]) {
    int numprocs, rank;
    MPI_Status status; /* return status for recieve*/
    int num_bars, num_threads;
    int num_points = 0;
    int my_count;
    int size = 2;
	int *dataset = malloc(sizeof(int)*size);
    int max_input = INT_MIN;
    int rem, sum_disp = 0;
    int *send_counts, *displancements;
    int *sub_recv_buff, *main_recv_buff;
    int step;
    int i, dest, j, idx;
    int* histogram;
    FILE *file;


    //MPI Start
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0){
        //User Input
        printf("Enter number of Threads : ");
        scanf("%d", &num_threads);
        printf("Enter number of Bars : ");
        scanf("%d", &num_bars);

        file = fopen("./dataset.txt", "r");

		while (fscanf(file, "%d", dataset + num_points) == 1)
		{
			num_points++;

			// Expand array
			if (num_points > size)
			{
				size *= 2;
				dataset = realloc(dataset, size * sizeof(int));
			}
		}

		fclose(file);
        send_counts = malloc(sizeof(int)*numprocs);
        displancements = malloc(sizeof(int)*numprocs);
        rem = num_points % numprocs;

        for (i = 0; i < numprocs; i++){
            send_counts[i] =  num_points / numprocs;
            if (rem > 0){
                send_counts[i]++;
                rem--;
            }
            displancements[i] = sum_disp;
            sum_disp += send_counts[i];
        }
        my_count = send_counts[0];   
        for (dest = 1; dest < numprocs; dest++){
			MPI_Send(&send_counts[dest], 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
		}
        main_recv_buff = malloc(sizeof(int)*numprocs*2);
    }
    else {
        MPI_Recv(&my_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    }


    sub_recv_buff = malloc(sizeof(int)*my_count); 

    MPI_Scatterv(dataset, send_counts, displancements, MPI_INT, sub_recv_buff, my_count, MPI_INT, 0, MPI_COMM_WORLD);

    for (i = 0; i < my_count; i++) {
        if (sub_recv_buff[i] > max_input) max_input = sub_recv_buff[i];
    }

    MPI_Gather(&max_input, 1, MPI_INT, rank == 0 ? main_recv_buff : NULL, 1, MPI_INT, 0, MPI_COMM_WORLD);



    if (rank == 0){
        for (i = 0; i < numprocs; i++) {
            if (main_recv_buff[i] > max_input) max_input = main_recv_buff[i];
        }

        step = max_input / num_bars;
        if (max_input % num_bars != 0) step++;

        histogram = malloc(sizeof(int)*num_bars);
        omp_set_dynamic(0); //Disabling dynamic teams to garentee the required number of threads to be created
        omp_set_num_threads(num_threads); // Setting number of threads according to user input
        #pragma omp parallel shared(histogram, num_bars) private(i)
        {
            #pragma omp for schedule(static)
            for (i = 0; i < num_bars; i++){
                histogram[i] = 0;
            }
        }
        #pragma omp parallel shared(dataset, histogram, step, num_bars, num_points) private(i, j, idx)
        {
            #pragma omp for schedule(static)
            for (i = 0; i < num_points; i++){
                j = step;
                for (idx = 0; idx < num_bars; idx++){
                    if (dataset[i] < j){
                        #pragma omp critical
                        histogram[idx]++;
                        break;
                    }
                    j += step;
                }
                if (idx == num_bars){
                    #pragma omp critical
                    histogram[num_bars-1]++;
                }
            }
        }

        j = step;
        idx = 0;
        for (i = 0; idx < num_bars; i+=step, j+=step, idx++){
            printf("The range start with %d, end with %d with count %d\n", idx != 0 ? i + 1 : i, i+step, histogram[idx]);
        }
    }

    free(sub_recv_buff);

    MPI_Finalize();


}