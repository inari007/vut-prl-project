/*************************\
|  AUTHOR = Dobes Zdenek  |
|  FILE   = parsplit.c    |
|  DATE   = 29.03.2023    |
\*************************/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char** argv){

    // fileNumbers = numbers from file read by root
    // processNumbers = numbers evenly divided for each process 
    char fileNumbers[64], processNumbers[64]; 
    int rank, size;
    char* median = (char*) malloc(sizeof(char));
    int* sizePerProcess = (int*) malloc(sizeof(int));
    int numbersCount;

    MPI_Init(&argc, &argv); 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // roots reads input file "numbers", selects median and calculates length of divided arrays
    if(rank == 0){
        FILE* file;
        char curChar;
        numbersCount = 0;

        file = fopen("numbers", "r");
        do{
            curChar = fgetc(file);
            fileNumbers[numbersCount] = curChar;
            numbersCount++;
        }while(numbersCount < 64 && curChar != EOF);
        numbersCount--;

        // specific loop for merlin (adds automatically end of line)
        while((fileNumbers[numbersCount - 1] == '\n' || fileNumbers[numbersCount - 1] == '\r') && numbersCount > 0){
            numbersCount--;
        }

        *median = fileNumbers[numbersCount / 2];
        *sizePerProcess = numbersCount / size;
        if(numbersCount % size > 0){
            *sizePerProcess = -1;
        }
        
        fclose(file);
    }

    // root sends length of divided arrays, median and divided arrays
    MPI_Bcast(sizePerProcess, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if(*sizePerProcess == -1){
        if(rank == 0){
            fprintf(stderr, "Pocet procesoru musi byt delitelny poctem vstupnich znaku.\n");
        }
        MPI_Finalize();
        free(median);
        free(sizePerProcess);
        return 0;
    }
    MPI_Bcast(median, 1, MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Scatter(fileNumbers, *sizePerProcess, MPI_CHAR, processNumbers, *sizePerProcess, MPI_CHAR, 0, MPI_COMM_WORLD);

    // each process compares each element of an obtained array with median and creates sets L, E and G
    char L[64], E[64], G[64];
    int* LIndex = (int*) malloc(sizeof(int));
    int* EIndex = (int*) malloc(sizeof(int));
    int* GIndex = (int*) malloc(sizeof(int));
    *LIndex = 0;
    *EIndex = 0;
    *GIndex = 0;
    for(int i = 0; i < *sizePerProcess; i++){
        if(processNumbers[i] < *median){
            L[*LIndex] = processNumbers[i];
            *LIndex = *LIndex + 1;
        }
        else if(processNumbers[i] == *median){
            E[*EIndex] = processNumbers[i];
            *EIndex = *EIndex + 1;
        }
        else{
            G[*GIndex] = processNumbers[i];
            *GIndex = *GIndex + 1;
        }
    }

    // each process sends root the length of his sets (L, E, G) 
    int NumberOfL[64], NumberOfE[64], NumberOfG[64];
    MPI_Gather(LIndex, 1, MPI_INT, NumberOfL, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(EIndex, 1, MPI_INT, NumberOfE, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(GIndex, 1, MPI_INT, NumberOfG, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // root calculates displacements
    int displacementL[64], displacementE[64], displacementG[64];
    if(rank == 0){
        displacementL[0] = 0;
        displacementE[0] = 0;
        displacementG[0] = 0;

        for(int i = 1; i < size; i++){
            displacementL[i] = displacementL[i-1] + NumberOfL[i-1];
            displacementE[i] = displacementE[i-1] + NumberOfE[i-1];
            displacementG[i] = displacementG[i-1] + NumberOfG[i-1];
        }
    }

    // each process sends root the sets
    char FinalL[64], FinalE[64], FinalG[64];
    MPI_Gatherv(L, *LIndex, MPI_CHAR, FinalL, NumberOfL, displacementL, MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Gatherv(E, *EIndex, MPI_CHAR, FinalE, NumberOfE, displacementE, MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Gatherv(G, *GIndex, MPI_CHAR, FinalG, NumberOfG, displacementG, MPI_CHAR, 0, MPI_COMM_WORLD);

    // each process sends the length of each set (for easier printing purposes)
    int* LLength = (int*) malloc(sizeof(int));
    int* ELength = (int*) malloc(sizeof(int));
    int* GLength = (int*) malloc(sizeof(int));
    MPI_Reduce(LIndex, LLength, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(EIndex, ELength, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(GIndex, GLength, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // root prints sets and a median
    if(rank == 0){
        printf("median: %i", *median);
        printf("\nL: ");
        for(int i = 0; i < *LLength; i++){
            printf("%i ", FinalL[i]);
        }
        printf("\nE: ");
        for(int i = 0; i < *ELength; i++){
            printf("%i ", FinalE[i]);
        }
        printf("\nG: ");
        for(int i = 0; i < *GLength; i++){
            printf("%i ", FinalG[i]);
        }
        printf("\n");
    }

    MPI_Finalize();
    free(median);
    free(sizePerProcess);
    free(LIndex);
    free(EIndex);
    free(GIndex);
    free(LLength);
    free(ELength);
    free(GLength);

    return 0;
}