// Aluno: José Olavo 
// Matrícula: 2023.1.08.009
// Disciplina: Computação Paralela e Distribuída
// Atividade 03: Multiplicação de Matrizes com MPI
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

int main(int argc, char *argv[]) {
    int rank, size;
    int n = 1024; 
    int m = 1024; 
    int p = 1024; 
    int *A = NULL;
    int *B = NULL;
    int *C = NULL;
    int *subA, *subC;
    int linhas_por_proc, resto, start_row;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    linhas_por_proc = n / size;
    resto = n % size;

    int local_n = linhas_por_proc + (rank < resto ? 1 : 0);
    start_row = rank * linhas_por_proc + (rank < resto ? rank : resto);

    subA = (int*) malloc(local_n * m * sizeof(int));
    subC = (int*) malloc(local_n * p * sizeof(int));

    if (rank == 0) {
        A = (int*) malloc(n * m * sizeof(int));
        B = (int*) malloc(m * p * sizeof(int));
        C = (int*) malloc(n * p * sizeof(int));
        srand(time(NULL));

        for (int i = 0; i < n*m; i++) A[i] = rand() % 10;
        for (int i = 0; i < m*p; i++) B[i] = rand() % 10;

        printf("Primeiros 100 valores de A:\n");
        for(int i=0;i<100;i++) printf("%d ", A[i]);
        printf("\n\n");

        printf("Primeiros 100 valores de B:\n");
        for(int i=0;i<100;i++) printf("%d ", B[i]);
        printf("\n\n");
    }

    if(rank != 0) B = (int*) malloc(m * p * sizeof(int));
    MPI_Bcast(B, m*p, MPI_INT, 0, MPI_COMM_WORLD);

    int *sendcounts = malloc(size * sizeof(int));
    int *displs = malloc(size * sizeof(int));
    for(int i=0;i<size;i++){
        sendcounts[i] = linhas_por_proc * m + (i < resto ? m : 0);
        displs[i] = i * linhas_por_proc * m + (i < resto ? i*m : resto*m);
    }
    MPI_Scatterv(A, sendcounts, displs, MPI_INT, subA, local_n*m, MPI_INT, 0, MPI_COMM_WORLD);

    for(int i=0;i<local_n;i++){
        for(int j=0;j<p;j++){
            subC[i*p + j] = 0;
            for(int k=0;k<m;k++){
                subC[i*p + j] += subA[i*m + k] * B[k*p + j];
            }
        }
    }

    int *recvcounts = malloc(size * sizeof(int));
    int *rdispls = malloc(size * sizeof(int));
    for(int i=0;i<size;i++){
        int ln = linhas_por_proc + (i < resto ? 1 : 0);
        recvcounts[i] = ln * p;
        rdispls[i] = (i*linhas_por_proc + (i<resto ? i : resto)) * p;
    }
    MPI_Gatherv(subC, local_n*p, MPI_INT, C, recvcounts, rdispls, MPI_INT, 0, MPI_COMM_WORLD);

    if(rank==0){
        printf("Primeiros 100 valores de C = A x B:\n");
        for(int i=0;i<100;i++) printf("%d ", C[i]);
        printf("\n");
    }

    free(subA);
    free(subC);
    free(sendcounts);
    free(displs);
    free(recvcounts);
    free(rdispls);
    if(rank==0){
        free(A); free(B); free(C);
    } else {
        free(B);
    }

    MPI_Finalize();
    return 0;
}