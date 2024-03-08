/*************************\
|  AUTHOR = Dobes Zdenek  |
|  FILE   = parkmeans.c   |
|  DATE   = 08.04.2023    |
\*************************/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdbool.h>

int main(int argc, char** argv){

    char* fileNumbers;
    double centers[4];
    int rank, size;
    int* numbersCount = (int*) malloc(sizeof(int));

    MPI_Init(&argc, &argv); 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // roots reads input file "numbers", calculates length of input and selects first centers
    if(rank == 0){
        FILE* file;
        char curChar;
        *numbersCount = 0;
        fileNumbers = (char*) malloc(size * sizeof(char));

        file = fopen("numbers", "r");
        do{
            curChar = fgetc(file);
            fileNumbers[*numbersCount] = curChar;
            if(*numbersCount < 4){
                centers[*numbersCount] = curChar;
            }
            *numbersCount = *numbersCount + 1;
        }while(*numbersCount < size && curChar != EOF && *numbersCount < 32);
        if(curChar == EOF){
            *numbersCount = *numbersCount - 1;
        }

        if(*numbersCount < size){
            *numbersCount = -1;
        }
        else if(*numbersCount > size){
            *numbersCount = size;
        }
        else if(*numbersCount < 4){
            *numbersCount = -2;
        }
        
        fclose(file);
    }

    // root sends length of input to others as check if input is valid 
    MPI_Bcast(numbersCount, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if(*numbersCount < 0){
        if(rank == 0 && *numbersCount == -1){
            fprintf(stderr, "Pocet procesoru musi byt stejny nebo mensi nez pocet vstupnich hodnot.\n");
            free(fileNumbers);
        }
        else if(rank == 0 && *numbersCount == -2){
            fprintf(stderr, "Pocet vstupnich hodnot musi byt vetsi jak 4.\n");
            free(fileNumbers);
        }
        MPI_Finalize();
        free(numbersCount);
        return 0;
    }

    // mask example - [0, 0, 1, 0] - given number is closests to the 2nd center
    // numbers example - [0, 0, 7, 0] - given number is 7 and is closests to the 2nd center
    int mask[4], numbers[4];
    int* rootMasks, *rootNumbers;
    double* prevCenters;
    char* curNum = (char*) malloc(sizeof(char));
    int* closestIndex = (int*) malloc(sizeof(int));
    int* isFinished = (int*) malloc(sizeof(int));

    if(rank == 0){
        rootMasks = (int*) malloc(4 * sizeof(int));
        rootNumbers = (int*) malloc(4 * sizeof(int));
        prevCenters = (double*) malloc(4 * sizeof(double));
    }

    // root sends each process 1 number
    MPI_Scatter(fileNumbers, 1, MPI_CHAR, curNum, 1, MPI_CHAR, 0, MPI_COMM_WORLD);

    // k means 4, until all 4 centers are same as in previous interation
    while(*isFinished != 4){
        MPI_Bcast(centers, 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // finding closest center
        *closestIndex = 0;
        for(int i = 1; i < 4; i++){
            if(abs(centers[i] - *curNum) < abs(centers[*closestIndex] - *curNum)){
                *closestIndex = i;
            }
        }
        for(int i = 0; i < 4; i++){
            if(i == *closestIndex){
                mask[i] = 1;
                numbers[i] = *curNum;
            }
            else{
                mask[i] = 0;
                numbers[i] = 0;
            }
        }

        // sending root sum of masks and  sum of numbers 
        MPI_Reduce(numbers, rootNumbers, 4, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(mask, rootMasks, 4, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        // root makes new centers and sends isFinished to check if work is done
        if(rank == 0){
            *isFinished = 0;
            for(int i = 0; i < 4; i++){
                prevCenters[i] = centers[i];
                centers[i] = (double) rootNumbers[i] / rootMasks[i];
                if(centers[i] == prevCenters[i]){
                    *isFinished = *isFinished + 1;
                }
            }
        }
        MPI_Bcast(isFinished, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
    // processes send root index of closest center for print purposes
    int* closestIndexces;
    if(rank == 0){
        closestIndexces = (int*) malloc(size * sizeof(int));
    }
    MPI_Gather(closestIndex, 1, MPI_INT, closestIndexces, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // root prints results
    if(rank == 0){
        for(int y = 0; y < 4; y++){
            printf("[%f]", centers[y]);
            for(int i = 0; i < size; i++){
                if(closestIndexces[i] == y){
                    printf(" %i", (int) fileNumbers[i]);
                }
            }
            printf("\n");
        }
        free(closestIndexces);
        free(fileNumbers);
        free(rootMasks);
        free(rootNumbers);
        free(prevCenters);
    }

    MPI_Finalize();
    free(numbersCount);
    free(curNum);
    free(closestIndex);
    free(isFinished);

    return 0;
}