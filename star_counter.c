// Aluno: José Olavo 
// Matrícula: 2023.1.08.009
// Disciplina: Computação Paralela e Distribuída
// Atividade 04: Contador de estrelas em uma imagem PGM
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define LIMIAR 200 

int* lerPGM(const char* filename, int* largura, int* altura) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("Erro abrindo imagem");
        exit(1);
    }

    char formato[3];
    fscanf(f, "%2s", formato); // lê P2
    if (formato[0] != 'P' || formato[1] != '2') {
        printf("Formato não suportado (use P2 ASCII)\n");
        exit(1);
    }

    int maxval;
    fscanf(f, "%d %d %d", largura, altura, &maxval);

    int tamanho = (*largura) * (*altura);
    int* dados = (int*) malloc(sizeof(int) * tamanho);
    for (int i = 0; i < tamanho; i++) {
        fscanf(f, "%d", &dados[i]);
    }

    fclose(f);
    return dados;
}

int contaEstrelas(int* img, int largura, int altura) {
    int* visitado = (int*) calloc(largura * altura, sizeof(int));
    int estrelas = 0;

    int dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

    for (int y = 0; y < altura; y++) {
        for (int x = 0; x < largura; x++) {
            int idx = y * largura + x;

            if (img[idx] > LIMIAR && !visitado[idx]) {
                estrelas++;

                int* fila = (int*) malloc(sizeof(int) * largura * altura);
                int frente = 0, tras = 0;

                fila[tras++] = idx;
                visitado[idx] = 1;

                while (frente < tras) {
                    int atual = fila[frente++];
                    int ay = atual / largura;
                    int ax = atual % largura;

                    for (int k = 0; k < 8; k++) {
                        int nx = ax + dx[k];
                        int ny = ay + dy[k];

                        if (nx >= 0 && nx < largura && ny >= 0 && ny < altura) {
                            int nidx = ny * largura + nx;
                            if (img[nidx] > LIMIAR && !visitado[nidx]) {
                                visitado[nidx] = 1;
                                fila[tras++] = nidx;
                            }
                        }
                    }
                }
                free(fila);
            }
        }
    }

    free(visitado);
    return estrelas;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int largura, altura, tamanho;
    int* imagem = NULL;

    if (world_rank == 0) {
        if (argc < 2) {
            printf("Uso: mpirun -np N ./star_counter imagem.pgm\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        imagem = lerPGM(argv[1], &largura, &altura);
        tamanho = largura * altura;

        int bloco = tamanho / (world_size - 1);

        // envia blocos
        for (int dest = 1; dest < world_size; dest++) {
            int inicio = (dest - 1) * bloco;
            int fim = (dest == world_size - 1) ? tamanho : inicio + bloco;
            int tamBloco = fim - inicio;

            MPI_Send(&tamBloco, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
            MPI_Send(&largura, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
            MPI_Send(imagem + inicio, tamBloco, MPI_INT, dest, 0, MPI_COMM_WORLD);
        }

        int totalEstrelas = 0;
        for (int src = 1; src < world_size; src++) {
            int parcial;
            MPI_Recv(&parcial, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            totalEstrelas += parcial;
        }

        printf("Total de estrelas detectadas: %d\n", totalEstrelas);
        free(imagem);
    }
    else {
        int tamBloco, largura_local;
        MPI_Recv(&tamBloco, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&largura_local, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        int* bloco = (int*) malloc(sizeof(int) * tamBloco);
        MPI_Recv(bloco, tamBloco, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        int altura_local = tamBloco / largura_local;
        int estrelas = contaEstrelas(bloco, largura_local, altura_local);

        MPI_Send(&estrelas, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        free(bloco);
    }

    MPI_Finalize();
    return 0;
}
