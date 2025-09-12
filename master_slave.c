#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

int buscaEscravo(int *vector, int size)
{
    for (int i = 0; i < size; i++)
        if (vector[i] == 0)
            return i;
    return -1;
}

float calcMedia(int *vector, int size)
{
    int sum = 0;
    for (int i = 0; i < size; i++)
        sum += vector[i];
    return sum / size;
}

void filtroPassaBaixa(int *vector, int size)
{
    for (int i = 1; i < size - 1; i++)
    {
        vector[i] = (int)vector[i - 1] + vector[i] + vector[i + 1] / 3;
    }
}

void bubbleSort(int *vector, int size)
{
    int i, j;
    for (i = 0; i < size - 1; i++)
        for (j = 0; j < size - i - 1; j++)
            if (vector[j] > vector[j + 1])
            {
                int temp = vector[j];
                vector[j] = vector[j + 1];
                vector[j + 1] = temp;
            }
}

float calcDesvioPadrao(int *vector, int size)
{
    float sum = 0.0;
    float avg = calcMedia(vector, size);
    for (int i = 0; i < size; i++)
    {
        sum += pow(vector[i] - avg, 2);
    }
    return sqrt(sum / size);
}

int main(int argc, char **argv)
{
    MPI_Init(NULL, NULL);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_rank == 0)
    {
        srand(time(NULL));
        int tarefa, tam;
        int *escravos = (int *)calloc(world_size, sizeof(int));
        int requisicoes = 20000;
        MPI_Status status;
        while (requisicoes > 0)
        {
            int resposta = 0;
            int escravo = buscaEscravo(escravos, world_size);
            while (escravo + 1)
            {
                tarefa = (rand() % 4) + 1;
                tam = (rand() % 300) + 1;
                int *vetor = (int *)malloc(sizeof(int) * tam);
                for (int i = 0; i < tam; i++)
                {
                    vetor[i] = (rand() % 1000);
                }
                escravos[escravo] = 1;
                printf("enviando tarefa para escravo %d\n", escravo);
                MPI_Send(vetor, tam, MPI_INT, escravo, tarefa, MPI_COMM_WORLD);
                requisicoes--;
                escravo = buscaEscravo(escravos, world_size);
                free(vetor);
            }
            MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&resposta, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("escravo %d fez a tarefa\n", status.MPI_SOURCE);
            escravos[status.MPI_SOURCE] = resposta;
        }
        for (int i = 0; i < world_size; i++)
        {
            MPI_Send(&tam, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
        free(escravos);
    }
    else
    {
        MPI_Status status;
        do
        {
            MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int tam;
            MPI_Get_count(&status, MPI_INT, &tam);
            int *vetor = (int *)malloc(sizeof(int) * tam);
            MPI_Recv(vetor, tam, MPI_INT, 0, status.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("escravo %d recebeu tarefa %d\n", world_rank, status.MPI_TAG);

            if (status.MPI_TAG == 1)
            {
                float avg = calcMedia(vetor, tam);
            }
            else if (status.MPI_TAG == 2)
            {
                filtroPassaBaixa(vetor, tam);
            }
            else if (status.MPI_TAG == 3)
            {
                bubbleSort(vetor, tam);
            }
            else if (status.MPI_TAG == 4)
            {
                float sd = calcDesvioPadrao(vetor, tam);
            }
            tam = 0;
            MPI_Send(&tam, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        } while (status.MPI_TAG);
    }

    MPI_Finalize();
}
