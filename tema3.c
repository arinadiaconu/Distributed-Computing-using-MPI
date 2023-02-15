#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char *argv[])
{
    int number_tasks, rank, len;
    char hostname[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &number_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(hostname, &len);

    // the two arguments from the command line
    int vector_dimension = atoi(argv[1]);
    int error = atoi(argv[2]);

    FILE *input_file;
    char file_name[12] = "cluster";

    // each cluster will keep its workers in the workers vector
    int number_of_workers, workers[number_tasks];
    int coordinator = -1; // the coordinating cluster for each worker

    int topology[4][number_tasks]; // the matrix for the topology
    // the vector required at the end of the program
    int vector[vector_dimension];

    // initializes the topology matrix with zeros
    if (rank == 0) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < number_tasks; j++) {
                topology[i][j] = 0;
            }
        }
    }

    // the four clusters read the information from their input
    // file and announce their workers that they are their coordinator
    if (rank < 4) {
        // builds up the name of the file for each cluster
        char num_to_str[5];
        sprintf(num_to_str, "%d", rank);
        strcat(file_name, num_to_str);
        strcat(file_name, ".txt");

        input_file = fopen(file_name, "r");
        if (input_file == NULL) {
            printf("Error opening the file from %d\n", rank);
        }
        // reads the number of workers
        fscanf(input_file, "%d", &number_of_workers);
        // reads each worker and sends them their rank
        for (int i = 0; i < number_of_workers; i++) {
            fscanf(input_file, "%d", &workers[i]);
            MPI_Send(&rank, 1, MPI_INT, workers[i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, workers[i]);
        }
        fclose(input_file);
        // each cluster builds their line in the topology matrix 
        for (int i = 0; i < number_of_workers; i++) {
            topology[rank][i] = workers[i];
        }
    }

    if (rank > 3) { // each worker receives their coordinator
        MPI_Status status;
        MPI_Recv(&coordinator, 1, MPI_INT, MPI_ANY_SOURCE, 
                                            0, MPI_COMM_WORLD, &status);
    }

    // the first 2 error cases - normal topology and without a channel
    if (error != 2) {
        // PART 1 - the determination of the topology
        if (rank == 0) { // sends its line from the topology to cluster 3
            MPI_Send(&number_of_workers, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            MPI_Send(topology[rank], number_of_workers, MPI_INT, 
                                                    3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
        }

        if (rank == 3) {
            int workers0;
            MPI_Status status; // receives the line from cluster 0
            MPI_Recv(&workers0, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[0], workers0, MPI_INT, 0, 
                                                0, MPI_COMM_WORLD, &status);
            // sends both the line from cluster 0 and its
            // own from the topology matrix to cluster 2
            MPI_Send(&workers0, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
            MPI_Send(topology[0], workers0, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);

            MPI_Send(&number_of_workers, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
            MPI_Send(topology[rank], number_of_workers, MPI_INT, 
                                                    2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
        }

        if (rank == 2) {
            int workers0, workers3;
            MPI_Status status; // receives the lines from clusters 0 and 3
            MPI_Recv(&workers0, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[0], workers0, MPI_INT, 3, 
                                                0, MPI_COMM_WORLD, &status);
            MPI_Recv(&workers3, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[3], workers3, MPI_INT, 3, 
                                                0, MPI_COMM_WORLD, &status);

            // sends the lines from clusters 0 and 3 and
            // its own from the topology matrix to cluster 1         
            MPI_Send(&workers0, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 1);
            MPI_Send(topology[0], workers0, MPI_INT, 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 1);

            MPI_Send(&number_of_workers, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 1);
            MPI_Send(topology[rank], number_of_workers, MPI_INT, 
                                                     1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 1);

            MPI_Send(&workers3, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 1);
            MPI_Send(topology[3], workers3, MPI_INT, 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 1);
        }

        if (rank == 1) {
            int workers0, workers2, workers3;
            MPI_Status status; // receives the lines from clusters 0, 3 and 2
            MPI_Recv(&workers0, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[0], workers0, MPI_INT, 2, 
                                                0, MPI_COMM_WORLD, &status);
            MPI_Recv(&workers2, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[2], workers2, MPI_INT, 2, 
                                                0, MPI_COMM_WORLD, &status);
            MPI_Recv(&workers3, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[3], workers3, MPI_INT, 2, 
                                                0, MPI_COMM_WORLD, &status);
            
            // sends its line from the topology matrix to cluster 2
            MPI_Send(&number_of_workers, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
            MPI_Send(topology[rank], number_of_workers, MPI_INT, 
                                                    2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
        }

        if (rank == 2) {
            int workers1;
            MPI_Status status; // receives the line from cluster 1
            MPI_Recv(&workers1, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[1], workers1, MPI_INT, 1, 
                                                0, MPI_COMM_WORLD, &status);

            // sends both the line from cluster 1 and its
            // own from the topology matrix to cluster 3
            MPI_Send(&workers1, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            MPI_Send(topology[1], workers1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);

            MPI_Send(&number_of_workers, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            MPI_Send(topology[rank], number_of_workers, MPI_INT, 
                                                    3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
        }

        if (rank == 3) {
            int workers1, workers2;
            MPI_Status status; // receives the lines from clusters 1 and 2
            MPI_Recv(&workers1, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[1], workers1, MPI_INT, 2, 
                                                0, MPI_COMM_WORLD, &status);

            MPI_Recv(&workers2, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[2], workers2, MPI_INT, 2, 
                                                0, MPI_COMM_WORLD, &status);
            
            // sends the lines from clusters 1 and 2 and
            // its own from the topology matrix to cluster 0
            MPI_Send(&workers1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
            MPI_Send(topology[1], workers1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
            
            MPI_Send(&workers2, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
            MPI_Send(topology[2], workers2, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);

            MPI_Send(&number_of_workers, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
            MPI_Send(topology[rank], number_of_workers, MPI_INT, 
                                                     0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
        }

        if (rank == 0) {
            int workers1, workers2, workers3;
            MPI_Status status;  // receives the lines from clusters 1, 2 and 3
            MPI_Recv(&workers1, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[1], workers1, MPI_INT, 3, 
                                                0, MPI_COMM_WORLD, &status);

            MPI_Recv(&workers2, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[2], workers2, MPI_INT, 3, 
                                                0, MPI_COMM_WORLD, &status);

            MPI_Recv(&workers3, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[3], workers3, MPI_INT, 3, 
                                                0, MPI_COMM_WORLD, &status);
        }

        if (rank < 4) {
            printf("%d ->", rank); // the topolgy is complete - it prints it
            for (int i = 0; i < 4; i++) {
                printf(" %d:", i);
                for (int j = 0; j < number_tasks - 1; j++) {
                    if (topology[i][j] > 3 && topology[i][j] <= number_tasks) {
                        if (topology[i][j+1] < 3 || topology[i][j+1] 
                                                            > number_tasks) {
                            printf("%d", topology[i][j]);
                            break;
                        }
                        printf("%d,", topology[i][j]);
                    }
                }
            }
            printf("\n");

            // each cluster sends the topology to their workers
            for (int i = 0; i < number_of_workers; i++) {
                MPI_Send(topology, 4 * number_tasks, MPI_INT, 
                                            workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
            }
        }

        if (rank > 3) { // each worker receives the topology and prints it
            MPI_Status status;
            MPI_Recv(topology, 4 * number_tasks, MPI_INT, 
                                    coordinator, 0, MPI_COMM_WORLD, &status);
        
            printf("%d ->", rank);
            for (int i = 0; i < 4; i++) {
                printf(" %d:", i);
                for (int j = 0; j < number_tasks - 1; j++) {
                    if (topology[i][j] > 3 && topology[i][j] <= number_tasks) {
                        if (topology[i][j+1] < 3 || topology[i][j+1] 
                                                            > number_tasks) {
                            printf("%d", topology[i][j]);
                            break;
                        }
                        printf("%d,", topology[i][j]);
                    }
                }
            }
            printf("\n");
        }

        // PART 2 - the resulting vector
        if (rank == 0) { // cluster 0 generates the vector
            for (int k = 0; k < vector_dimension; k++) {
                vector[k] = vector_dimension - k - 1;
            }
            int n_workers = number_tasks - 4; // the number of workers
            // this is the integer number of tasks each worker will get
            int integer_number = vector_dimension / (n_workers);
            // these are the rest of the tasks for the workers in case
            // the workers are not perfectly divided by the vector dimension
            int left_numbers = vector_dimension % (n_workers);
            // 'each_workers_number' is a vector that keeps track of all the
            // tasks each worker must do and 'indices' keeps track of the
            // index in the final vector for each worker to know where to
            // start calculating their numbers or tasks
            int each_workers_number[n_workers], indices[n_workers];
            for (int i = 0; i < n_workers; i++) {
                each_workers_number[i] = integer_number;
            }
            for (int i = 0; i < left_numbers; i++) {
                each_workers_number[i] += 1;
            }
            indices[n_workers - 1] = vector_dimension - 1;
            for (int i = n_workers - 2; i >= 0; i--) {
                indices[i] = indices[i+1] - each_workers_number[i+1];
            }
            
            // calculates the number of workers for the other 3 clusters
            int count_workers_1 = 0, count_workers_2 = 0, count_workers_3 = 0;
            for (int i = 0; i < number_tasks; i++) {
                if (topology[1][i] > 3 && topology[1][i] <= number_tasks)
                    count_workers_1++;
                else
                    break;
            }
            for (int i = 0; i < number_tasks; i++) {
                if (topology[2][i] > 3 && topology[2][i] <= number_tasks)
                    count_workers_2++;
                else
                    break;
            }
            for (int i = 0; i < number_tasks; i++) {
                if (topology[3][i] > 3 && topology[3][i] <= number_tasks)
                    count_workers_3++;
                else
                    break;
            }

            // this keeps the index that must be sent to the other clusters
            // in order to know where their workers must do the culculus
            int keep_idex = indices[n_workers - 1];
            // this keeps the index of the tasks in 'each_workers_number'
            int index_for_all_workers = n_workers - 1;
            // the number of tasks for each of the four clusters
            int tasks_for_1 = 0, tasks_for_2 = 0;
            int tasks_for_3 = 0, tasks_for_0 = 0;
            // calculates for each cluster their number of tasks
            for (int i = 0; i < count_workers_1; i++) {
                tasks_for_1 += each_workers_number[index_for_all_workers];
                index_for_all_workers--;
            }
            for (int i = 0; i < count_workers_2; i++) {
                tasks_for_2 += each_workers_number[index_for_all_workers];
                index_for_all_workers--;
            }
            for (int i = 0; i < count_workers_3; i++) {
                tasks_for_3 += each_workers_number[index_for_all_workers];
                index_for_all_workers--;
            }
            for (int i = 0; i < number_of_workers; i++) {
                tasks_for_0 += each_workers_number[index_for_all_workers];
                index_for_all_workers--;
            }

            // sends the vector to cluster 3
            MPI_Send(vector, vector_dimension, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            // sends the index and the number of tasks for cluster 1 via 3
            MPI_Send(&keep_idex, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            MPI_Send(&tasks_for_1, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);

            // sends the index and the number of tasks for cluster 2 via 3
            keep_idex = indices[n_workers - 1 - count_workers_1];
            MPI_Send(&keep_idex, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            MPI_Send(&tasks_for_2, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);

            // sends the index and the number of tasks for cluster 3
            keep_idex = indices[n_workers - 1 - 
                                count_workers_1 - count_workers_2];
            MPI_Send(&keep_idex, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            MPI_Send(&tasks_for_3, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);

            // the tasks for 0
            keep_idex = indices[n_workers - 1 - 
                    count_workers_1 - count_workers_2 - count_workers_3];
            // the vector with the tasks for each worker
            int w[number_of_workers];
            // the tasks are split equally and the vector is built up as before
            int t = tasks_for_0 / number_of_workers;
            int r = tasks_for_0 % number_of_workers;
            for (int i = 0; i < number_of_workers; i++) {
                w[i] = t;
            }
            for (int i = 0; i < r; i++) {
                w[i] += 1;
            }

            // each worker gets the vector, the index at which it must
            // start to calculate and the numer of tasks it has to do
            for (int i = 0; i < number_of_workers; i++) {
                MPI_Send(vector, vector_dimension, MPI_INT, 
                                            workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                MPI_Send(&keep_idex, 1, MPI_INT, workers[i], 
                                                        0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                MPI_Send(&w[i], 1, MPI_INT, workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                keep_idex -= w[i];
            }
        }

        if (rank == 3) {
            MPI_Status status;
            int index_for_1, tasks_for_1;
            // receives the vector and data for cluster 1 and sends it via 2
            MPI_Recv(vector, vector_dimension, MPI_INT, 0, 0, 
                                                    MPI_COMM_WORLD, &status);
            MPI_Recv(&index_for_1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&tasks_for_1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

            MPI_Send(vector, vector_dimension, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
            MPI_Send(&index_for_1, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
            MPI_Send(&tasks_for_1, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);

            // receives the data for cluster 2 and sends it
            int index_for_2, tasks_for_2;
            MPI_Recv(&index_for_2, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&tasks_for_2, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

            MPI_Send(&index_for_2, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
            MPI_Send(&tasks_for_2, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);

            // receives its own data
            int index_for_3, tasks_for_3;
            MPI_Recv(&index_for_3, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&tasks_for_3, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

            // builds up the vector with the tasks for each worker
            int w[number_of_workers];
            int t = tasks_for_3 / number_of_workers;
            int r = tasks_for_3 % number_of_workers;
            for (int i = 0; i < number_of_workers; i++) {
                w[i] = t;
            }
            for (int i = 0; i < r; i++) {
                w[i] += 1;
            }
            // sends the data to each worker
            for (int i = 0; i < number_of_workers; i++) {
                MPI_Send(vector, vector_dimension, MPI_INT, 
                                            workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                MPI_Send(&index_for_3, 1, MPI_INT, workers[i], 
                                                        0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                MPI_Send(&w[i], 1, MPI_INT, workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                index_for_3 -= w[i];
            }
        }

        if (rank == 2) {
            MPI_Status status;
            int index_for_1, tasks_for_1;
            // receives the data for cluster 1 and sends it
            MPI_Recv(vector, vector_dimension, MPI_INT, 3, 
                                                0, MPI_COMM_WORLD, &status);
            MPI_Recv(&index_for_1, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&tasks_for_1, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);

            MPI_Send(vector, vector_dimension, MPI_INT, 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 1);
            MPI_Send(&index_for_1, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 1);
            MPI_Send(&tasks_for_1, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 1);

            // receives its own data
            int index_for_2, tasks_for_2;
            MPI_Recv(&index_for_2, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&tasks_for_2, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);

            // builds up the vector with the tasks for each worker
            int w[number_of_workers];
            int t = tasks_for_2 / number_of_workers;
            int r = tasks_for_2 % number_of_workers;
            for (int i = 0; i < number_of_workers; i++) {
                w[i] = t;
            }
            for (int i = 0; i < r; i++) {
                w[i] += 1;
            }
            // sends the data to each worker
            for (int i = 0; i < number_of_workers; i++) {
                MPI_Send(vector, vector_dimension, MPI_INT, 
                                            workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                MPI_Send(&index_for_2, 1, MPI_INT, workers[i], 
                                                        0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                MPI_Send(&w[i], 1, MPI_INT, workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                index_for_2 -= w[i];
            }
        }

        if (rank == 1) {
            MPI_Status status;
            int index_for_1, tasks_for_1; // receives it own data
            MPI_Recv(vector, vector_dimension, MPI_INT, 2, 
                                                0, MPI_COMM_WORLD, &status);
            MPI_Recv(&index_for_1, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&tasks_for_1, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);

            // builds up the vector with the tasks for each worker
            int w[number_of_workers];
            int t = tasks_for_1 / number_of_workers;
            int r = tasks_for_1 % number_of_workers;
            for (int i = 0; i < number_of_workers; i++) {
                w[i] = t;
            }
            for (int i = 0; i < r; i++) {
                w[i] += 1;
            }
            // sends the data to each worker
            for (int i = 0; i < number_of_workers; i++) {
                MPI_Send(vector, vector_dimension, MPI_INT, 
                                            workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                MPI_Send(&index_for_1, 1, MPI_INT, workers[i], 
                                                        0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                MPI_Send(&w[i], 1, MPI_INT, workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                index_for_1 -= w[i];
            }
        }

        // each worker receives the vector, the index where to start their task
        // and the number of numbers to calculate, they multiply the required
        // elements by 5 and send back the vector to their coordinating cluster
        if (rank > 3) {
            int index_in_vector, number_of_tasks;
            MPI_Status status;
            MPI_Recv(vector, vector_dimension, MPI_INT, coordinator, 
                                                0, MPI_COMM_WORLD, &status);
            MPI_Recv(&index_in_vector, 1, MPI_INT, coordinator, 
                                                0, MPI_COMM_WORLD, &status);
            MPI_Recv(&number_of_tasks, 1, MPI_INT, coordinator, 
                                                0, MPI_COMM_WORLD, &status);
            for (int i = 0; i < number_of_tasks; i++) {
                vector[index_in_vector] *= 5;
                index_in_vector--;
            }
            
            MPI_Send(vector, vector_dimension, MPI_INT, 
                                            coordinator, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, coordinator);
        }

        // each cluster receives the partial
        // vectors and build up their merged vector
        if (rank < 4) {
            MPI_Status status;
            int partial_vector[vector_dimension];
            for (int i = 0; i < number_of_workers; i++) {
                MPI_Recv(partial_vector, vector_dimension, MPI_INT, 
                                    workers[i], 0, MPI_COMM_WORLD, &status);
                for (int j = 0; j < vector_dimension; j++) {
                    if (partial_vector[j] > vector[j]) {
                        vector[j] = partial_vector[j];
                    }
                }
            }
        }

        // each cluster starting with 1 sends their vector back to cluster 0
        if (rank == 1) {
            MPI_Send(vector, vector_dimension, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
        }

        if (rank == 2) {
            MPI_Status status;
            int partial_vector[vector_dimension];
            MPI_Recv(partial_vector, vector_dimension, MPI_INT, 1, 
                                                0, MPI_COMM_WORLD, &status);
            for (int j = 0; j < vector_dimension; j++) {
                if (partial_vector[j] > vector[j]) {
                    vector[j] = partial_vector[j];
                }
            }
            MPI_Send(vector, vector_dimension, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
        }

        if (rank == 3) {
            MPI_Status status;
            int partial_vector[vector_dimension];
            MPI_Recv(partial_vector, vector_dimension, MPI_INT, 2, 0, 
                                                    MPI_COMM_WORLD, &status);
            for (int j = 0; j < vector_dimension; j++) {
                if (partial_vector[j] > vector[j]) {
                    vector[j] = partial_vector[j];
                }
            }
            MPI_Send(vector, vector_dimension, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
        }

        if (rank == 0) {
            MPI_Status status;
            int partial_vector[vector_dimension];
            MPI_Recv(partial_vector, vector_dimension, MPI_INT, 3, 
                                                0, MPI_COMM_WORLD, &status);
            for (int j = 0; j < vector_dimension; j++) {
                if (partial_vector[j] > vector[j]) {
                    vector[j] = partial_vector[j];
                }
            }
            printf("Rezultat: ");
            for (int i = 0; i < vector_dimension; i++) {
                printf("%d ", vector[i]);
            }
            printf("\n");
        }
    } else { // BONUS
        // the process is the same: cluster 0 sends its line to cluster 3
        // cluster 3 sends both lines from 0 and 3 to 2, cluster 2 now has
        // the full topology and sends back its line and finally, cluster
        // 3 sends the lines from 2 and 3 back to cluster 0 and everybody
        // knows the full linked topology, leaving cluster 1 on its own
        if (rank == 0) {
            MPI_Send(&number_of_workers, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            MPI_Send(topology[rank], number_of_workers, MPI_INT, 
                                                    3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
        }

        if (rank == 3) {
            int workers0;
            MPI_Status status;
            MPI_Recv(&workers0, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[0], workers0, MPI_INT, 0, 
                                            0, MPI_COMM_WORLD, &status);
                    
            MPI_Send(&workers0, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
            MPI_Send(topology[0], workers0, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);

            MPI_Send(&number_of_workers, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
            MPI_Send(topology[rank], number_of_workers, MPI_INT, 
                                                    2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
        }

        if (rank == 2) {
            int workers0, workers3;
            MPI_Status status;
            MPI_Recv(&workers0, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[0], workers0, MPI_INT, 3, 
                                            0, MPI_COMM_WORLD, &status);

            MPI_Recv(&workers3, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[3], workers3, MPI_INT, 3, 
                                            0, MPI_COMM_WORLD, &status);

            printf("%d ->", rank); // the topology is complete - it prints it
            for (int i = 0; i < 4; i++) {
                if (topology[i][0] > 3 && topology[i][0] <= number_tasks) {
                    printf(" %d:", i);
                    for (int j = 0; j < number_tasks - 1; j++) {
                        if (topology[i][j] > 3 && topology[i][j] 
                                                            <= number_tasks) {
                            if (topology[i][j+1] < 3 || topology[i][j+1] 
                                                            > number_tasks) {
                                printf("%d", topology[i][j]);
                                break;
                            }
                            printf("%d,", topology[i][j]);
                        }
                    }
                }
            }
            printf("\n");

            MPI_Send(&number_of_workers, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            MPI_Send(topology[rank], number_of_workers, MPI_INT, 
                                                    3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
        }

        if (rank == 1) { // this cluster just prints its topology
            printf("%d -> %d:", rank, rank);
            for (int j = 0; j < number_tasks - 1; j++) {
                if (topology[1][j] > 3 && topology[1][j] <= number_tasks) {
                    if (topology[1][j+1] < 3 || topology[1][j+1] 
                                                    > number_tasks) {
                        printf("%d", topology[1][j]);
                        break;
                    }
                    printf("%d,", topology[1][j]);
                }
            }
            printf("\n");
        }

        if (rank == 3) {
            int workers2;
            MPI_Status status;
            MPI_Recv(&workers2, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[2], workers2, MPI_INT, 2, 
                                                0, MPI_COMM_WORLD, &status);

            printf("%d ->", rank); // the topology is complete - it prints it
            for (int i = 0; i < 4; i++) {
                if (topology[i][0] > 3 && topology[i][0] <= number_tasks) {
                    printf(" %d:", i);
                    for (int j = 0; j < number_tasks - 1; j++) {
                        if (topology[i][j] > 3 && topology[i][j] 
                                                        <= number_tasks) {
                            if (topology[i][j+1] < 3 || topology[i][j+1] 
                                                        > number_tasks) {
                                printf("%d", topology[i][j]);
                                break;
                            }
                            printf("%d,", topology[i][j]);
                        }
                    }
                }
            }
            printf("\n");
            
            MPI_Send(&workers2, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
            MPI_Send(topology[2], workers2, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);

            MPI_Send(&number_of_workers, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
            MPI_Send(topology[rank], number_of_workers, MPI_INT, 
                                                    0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
        }

        if (rank == 0) {
            int workers2, workers3;
            MPI_Status status;

            MPI_Recv(&workers2, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[2], workers2, MPI_INT, 3, 
                                                0, MPI_COMM_WORLD, &status);

            MPI_Recv(&workers3, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(topology[3], workers3, MPI_INT, 3, 
                                                0, MPI_COMM_WORLD, &status);

            printf("%d ->", rank); // the topology is complete - it prints it
            for (int i = 0; i < 4; i++) {
                if (topology[i][0] > 3 && topology[i][0] <= number_tasks) {
                    printf(" %d:", i);
                    for (int j = 0; j < number_tasks - 1; j++) {
                        if (topology[i][j] > 3 && topology[i][j] 
                                                        <= number_tasks) {
                            if (topology[i][j+1] < 3 || topology[i][j+1] 
                                                        > number_tasks) {
                                printf("%d", topology[i][j]);
                                break;
                            }
                            printf("%d,", topology[i][j]);
                        }
                    }
                }
            }
            printf("\n");
        }

        if (rank < 4) { // each cluster sends the topology to their workers
            for (int i = 0; i < number_of_workers; i++) {
                MPI_Send(topology, 4 * number_tasks, MPI_INT, 
                                            workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
            }
        }

        if (rank > 3) { // each worker prints the topology
            MPI_Status status;
            MPI_Recv(topology, 4 * number_tasks, MPI_INT, coordinator, 
                                                0, MPI_COMM_WORLD, &status);
        
            printf("%d ->", rank);
            for (int i = 0; i < 4; i++) {
                if (topology[i][0] > 3 && topology[i][0] <= number_tasks) {
                    printf(" %d:", i);
                    for (int j = 0; j < number_tasks - 1; j++) {
                        if (topology[i][j] > 3 && topology[i][j] 
                                                        <= number_tasks) {
                            if (topology[i][j+1] < 3 || topology[i][j+1] 
                                                        > number_tasks) {
                                printf("%d", topology[i][j]);
                                break;
                            }
                            printf("%d,", topology[i][j]);
                        }
                    }
                }
            }
            printf("\n");
        }

        // the vector is calculated the same way, excluding
        // cluster 1 and its workers from the division of the tasks
        if (rank == 0) {
            for (int k = 0; k < vector_dimension; k++) {
                vector[k] = vector_dimension - k - 1;
            }
            
            int count_workers_1 = 0, count_workers_2 = 0, count_workers_3 = 0;
            for (int i = 0; i < number_tasks; i++) {
                if (topology[2][i] > 3 && topology[2][i] <= number_tasks)
                    count_workers_2++;
                else
                    break;
            }
            for (int i = 0; i < number_tasks; i++) {
                if (topology[3][i] > 3 && topology[3][i] <= number_tasks)
                    count_workers_3++;
                else
                    break;
            }
            count_workers_1 = number_tasks - 4 - number_of_workers -
                                    count_workers_2 - count_workers_3;
            // this represents the number of accesible workers
            // from cluster 0 in the partitioned topology
            int accesible_workers = number_tasks - 4 - count_workers_1;
            int integer_number = vector_dimension / accesible_workers;
            int left_numbers = vector_dimension % (accesible_workers);
            int each_workers_number[accesible_workers];
            int indices[accesible_workers];
            for (int i = 0; i < accesible_workers; i++) {
                each_workers_number[i] = integer_number;
            }
            for (int i = 0; i < left_numbers; i++) {
                each_workers_number[i] += 1;
            }

            indices[accesible_workers - 1] = vector_dimension - 1;
            for (int i = accesible_workers - 2; i >= 0; i--) {
                indices[i] = indices[i+1] - each_workers_number[i+1];
            }

            int keep_idex = indices[accesible_workers - 1];
            int index_for_all_workers = accesible_workers - 1;
            int tasks_for_2 = 0, tasks_for_3 = 0, tasks_for_0 = 0;
            for (int i = 0; i < count_workers_2; i++) {
                tasks_for_2 += each_workers_number[index_for_all_workers];
                index_for_all_workers--;
            }
            for (int i = 0; i < count_workers_3; i++) {
                tasks_for_3 += each_workers_number[index_for_all_workers];
                index_for_all_workers--;
            }
            for (int i = 0; i < number_of_workers; i++) {
                tasks_for_0 += each_workers_number[index_for_all_workers];
                index_for_all_workers--;
            }

            // the tasks for cluster 2 via cluster 3
            MPI_Send(vector, vector_dimension, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            MPI_Send(&keep_idex, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            MPI_Send(&tasks_for_2, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);

            // the tasks for cluster 3
            keep_idex = indices[accesible_workers - 1 - count_workers_2];
            MPI_Send(&keep_idex, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
            MPI_Send(&tasks_for_3, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);

            // the tasks for cluster 0
            keep_idex = indices[accesible_workers - 1 - count_workers_2
                                                        - count_workers_3];
            int w[number_of_workers];
            int t = tasks_for_0 / number_of_workers;
            int r = tasks_for_0 % number_of_workers;
            for (int i = 0; i < number_of_workers; i++) {
                w[i] = t;
            }
            for (int i = 0; i < r; i++) {
                w[i] += 1;
            }
            // sends the vector, the index and the num of tasks to each worker
            for (int i = 0; i < number_of_workers; i++) {
                MPI_Send(vector, vector_dimension, MPI_INT, 
                                            workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                MPI_Send(&keep_idex, 1, MPI_INT, workers[i], 
                                                        0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                MPI_Send(&w[i], 1, MPI_INT, workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                keep_idex -= w[i];
            }
        }

        if (rank == 3) {
            MPI_Status status;
            int index_for_2, tasks_for_2; // receives the data for cluster 2
            MPI_Recv(vector,vector_dimension, MPI_INT, 0, 
                                                0, MPI_COMM_WORLD, &status);
            MPI_Recv(&index_for_2, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&tasks_for_2, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

            MPI_Send(vector, vector_dimension, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
            MPI_Send(&index_for_2, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
            MPI_Send(&tasks_for_2, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
            // its own data
            int index_for_3, tasks_for_3;
            MPI_Recv(&index_for_3, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&tasks_for_3, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

            int w[number_of_workers];
            int t = tasks_for_3 / number_of_workers;
            int r = tasks_for_3 % number_of_workers;
            for (int i = 0; i < number_of_workers; i++) {
                w[i] = t;
            }
            for (int i = 0; i < r; i++) {
                w[i] += 1;
            }
            // sends the vector, the index and the num of tasks to each worker
            for (int i = 0; i < number_of_workers; i++) {
                MPI_Send(vector, vector_dimension, MPI_INT, 
                                            workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                MPI_Send(&index_for_3, 1, MPI_INT, workers[i], 
                                                        0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                MPI_Send(&w[i], 1, MPI_INT, workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                index_for_3 -= w[i];
            }
        }

        if (rank == 2) {
            MPI_Status status;
            int index_for_2, tasks_for_2; // receives its own tasks
            MPI_Recv(vector, vector_dimension, MPI_INT, 3, 
                                                0, MPI_COMM_WORLD, &status);
            MPI_Recv(&index_for_2, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&tasks_for_2, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &status);

            int w[number_of_workers];
            int t = tasks_for_2 / number_of_workers;
            int r = tasks_for_2 % number_of_workers;
            for (int i = 0; i < number_of_workers; i++) {
                w[i] = t;
            }
            for (int i = 0; i < r; i++) {
                w[i] += 1;
            }
            // sends the vector, the index and the num of tasks to each worker
            for (int i = 0; i < number_of_workers; i++) {
                MPI_Send(vector, vector_dimension, MPI_INT, 
                                            workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                MPI_Send(&index_for_2, 1, MPI_INT, workers[i], 
                                                        0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                MPI_Send(&w[i], 1, MPI_INT, workers[i], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, workers[i]);
                index_for_2 -= w[i];
            }
        }

        // each worker calculates its part of the vector and sends it back
        if ((rank > 3) && (coordinator != 1)) {
            int index_in_vector, number_of_tasks;
            MPI_Status status;
            MPI_Recv(vector, vector_dimension, MPI_INT, coordinator, 
                                                0, MPI_COMM_WORLD, &status);
            MPI_Recv(&index_in_vector, 1, MPI_INT, coordinator, 
                                                0, MPI_COMM_WORLD, &status);
            MPI_Recv(&number_of_tasks, 1, MPI_INT, coordinator, 
                                                0, MPI_COMM_WORLD, &status);
            for (int i = 0; i < number_of_tasks; i++) {
                vector[index_in_vector] *= 5;
                index_in_vector--;
            }
            
            MPI_Send(vector, vector_dimension, MPI_INT, 
                                        coordinator, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, coordinator);
        }

        // the clusters receive the partial vectors and merge them
        if ((rank < 4) && (rank != 1)) {
            MPI_Status status;
            int partial_vector[vector_dimension];
            for (int i = 0; i < number_of_workers; i++) {
                MPI_Recv(partial_vector, vector_dimension, MPI_INT, 
                                    workers[i], 0, MPI_COMM_WORLD, &status);
                for (int j = 0; j < vector_dimension; j++) {
                    if (partial_vector[j] > vector[j]) {
                        vector[j] = partial_vector[j];
                    }
                }
            }
        }

        // the process is the same as above, cluster 2 sends the vector to
        // cluster 3, which merges it with its own and sends it to cluster
        // 0, which does the same and prints the final result
        if (rank == 2) {
            MPI_Send(vector, vector_dimension, MPI_INT, 3, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 3);
        }

        if (rank == 3) {
            MPI_Status status;
            int partial_vector[vector_dimension];
            MPI_Recv(partial_vector, vector_dimension, MPI_INT, 2, 
                                                0, MPI_COMM_WORLD, &status);
            for (int j = 0; j < vector_dimension; j++) {
                if (partial_vector[j] > vector[j]) {
                    vector[j] = partial_vector[j];
                }
            }
            MPI_Send(vector, vector_dimension, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
        }

        if (rank == 0) {
            MPI_Status status;
            int partial_vector[vector_dimension];
            MPI_Recv(partial_vector, vector_dimension, MPI_INT, 3, 
                                                0, MPI_COMM_WORLD, &status);
            for (int j = 0; j < vector_dimension; j++) {
                if (partial_vector[j] > vector[j]) {
                    vector[j] = partial_vector[j];
                }
            }
            printf("Rezultat: ");
            for (int i = 0; i < vector_dimension; i++) {
                printf("%d ", vector[i]);
            }
            printf("\n");
        }
    }

    MPI_Finalize();
}

