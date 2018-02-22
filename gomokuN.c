/*
   Program:     Gomoku-N using POSIX threads
   Author:      Joshua David
   Date:        October 26, 2017
   File name:   gomokuN.c
   Compile:     cc -lpthread gomokuN.c
   Run:         ./a.out
   Note:        Designed on a FreeBSD OS.
   Decription:
   This program is a Gomoku-N game. Where N is the size of the board. It is
   assumed N > 2. The baord is dynamically allocated as a 2-D array.
   This is a turn based game for 2 players. Player 1 makes one move then
   it is Player 2's turn. You can not place a piece on an square that is
   already occupied. This program also checks if the coordinates are valid.
   Threads are used for each of the 3 board scans. Diagonal scans are
   implemented by creating a new board where each row is shifted,
   then the vertical scan can be applied to the newMatrix.
   The struct 'thread_args' is used for functions to grab arguments from. This
   is neccesary because we cannot pass multiple parameters to a function when
   we create a new pthread.
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

int winnerT = 0;     // shared by the 3 threads
char B = 'X'; // Player 1's piece
char W = 'O'; // Player 2's piece
char E = '.'; // Empty space

void print2DArray(char **array, int numOfRows, int numOfCols);
void insertPiece(char **array, int x, int y, int turn);
void *scanDiagonal (void *ptr);
void *scanVertical(void *ptr);
void *scanHorizontal (void *ptr);
void assignDefaults(char **array,int r, int c, char d);

typedef struct {
    char **array;
    char **newMatrix;
    int size;
    int numOfRows; // size * 2
    int numOfCols; // size * 2
    int numOfRowsD; // used for diagonal scans
    int numOfColsD; // used for diagonal scans
    int useDiagArray; // 0 or 1.
} thread_args;

int main() {
   int i, j;
   int size;
   pthread_t thread1, thread2, thread3;
   int iret1, iret2, iret3;
   thread_args buffer;

   printf("Please enter the size of the board :\n");
   scanf("%d", &size);
   buffer.size = size;
   buffer.numOfRows = buffer.size * 2;
   buffer.numOfCols = buffer.numOfRows;
   buffer.numOfRowsD = buffer.numOfRows;
   buffer.numOfColsD = buffer.numOfRows * 2 - 1; // r + c - 1

   // allocate an array of int pointers of the size of # of rows.
   //char **array2D = (char **) malloc(sizeof(char *) * numOfRows);
   buffer.array = (char **) malloc(sizeof(char *) * buffer.numOfRows);

   // for each int pointer, allocate space for numOfCols ints.
   for (i = 0; i < buffer.numOfRows; i++) {
      buffer.array[i] = (char*) malloc(sizeof(char) * buffer.numOfCols);
   }
   // assign default values to the array
   assignDefaults(buffer.array,buffer.numOfRows, buffer.numOfCols, E);

   printf("\nprint from print2DArray()\n");
   print2DArray(buffer.array, buffer.numOfRows, buffer.numOfCols);

   int turn = 0; // 0 = P1 . 1 = P2
   int x, y;
   buffer.useDiagArray = 0;
   int piecesPlaced = 0;
   // Game Loop
   while(!winnerT){
       printf("It is now Player %d's turn \n", turn + 1);
       printf("Please enter the coordinate of your next move (2 space delimited integers): \n");
       scanf("%d %d", &x, &y);
       // Check parameters
       if(piecesPlaced==size*4){
          printf("The board is full, draw!");
          exit(1);
       }
       if((x>buffer.size*2)||(y>buffer.size*2)||(x<0)||(y<0)){
          printf("Out of bounds!\n");
       }
       else if(buffer.array[x][y]!=E){
          printf("Sorry, that location is occupied.\n" );
       }
       else{
         insertPiece(buffer.array,x ,y, turn);
         piecesPlaced++;
         print2DArray(buffer.array, buffer.numOfRows, buffer.numOfCols);
         iret1 = pthread_create(&thread1, NULL, scanVertical, (void*) &buffer);
         iret2 = pthread_create(&thread2, NULL, scanHorizontal, (void*) &buffer);
         iret3 = pthread_create(&thread3, NULL, scanDiagonal, (void*) &buffer);

         pthread_join(thread1, NULL);
         pthread_join(thread2, NULL);
         pthread_join(thread3, NULL);
         turn = 1 - turn; // toggle the turn variable.
         }
   }
   printf ("Player %d is the winner! \n", 1-turn+1);
   // free resources
   for (i = 0; i < buffer.numOfRows; i++) {
      free(buffer.array[i]);
   }
   free(buffer.array);
}


void *scanVertical (void *ptr){
    int r = 0; // row
    int c = 0; // column
    thread_args *buffer = (thread_args*)ptr;
    char cur; // current char we are looking at
    int counter;
    int rows, cols;
    char **array;

   if(buffer->useDiagArray){
      array = buffer->newMatrix ;
      buffer->useDiagArray=0;
      rows = buffer->numOfRowsD;
      cols = buffer->numOfColsD;
   }
   else{
      array = buffer->array;
      buffer->useDiagArray=0;
      rows = buffer->numOfRows;
      cols = buffer->numOfCols;
   }
    while( (c < cols) && !winnerT){
        counter=1; // reset the counter for the next column.
        cur=array[0][c]; // current char we are looking at.
        // check each column
        for (r=0; r<rows; r++){
            // check each row of each column
            if (cur==array[r][c]&&(cur!=E)){
               if(r==0){;}
               else{
                  counter++;
               }
            }
            else{
                counter=1;
            }
            cur=array[r][c];
            if (counter == buffer->size){
                winnerT = 1;
            }
        }
        c++;
    }
    return NULL;
}

void *scanHorizontal (void *ptr){
    int r = 0; // row
    int c = 0; // column
    char cur; // current char we are looking at
    int counter;
    thread_args *buffer = (thread_args*)ptr;

    while( (r < buffer->numOfRows) && !winnerT){
        counter=1; // reset the counter for the next row.
        cur=buffer->array[r][0]; // current char we are looking at.
        // check each row
        for (c=0; c<buffer->numOfCols; c++){
            // check each row of each column
            if (cur==buffer->array[r][c]&&cur!=E){
               if(c==0){;}
               else{
                  counter++;
               }
            }
            else{
                counter=1;
            }
            cur=buffer->array[r][c];
            if (counter == buffer->size){
                winnerT = 1;
            }
        }
        r++;
    }
    return NULL;
}

void *scanDiagonal (void *ptr){
   thread_args *buffer = (thread_args*)ptr;
   // build a new matrix of size [r]x[r+c-1]
   // allocate an array of int pointers of the size of # of rows.
   buffer->newMatrix = (char **) malloc(sizeof(char *) * buffer->numOfRowsD);
   // for each int pointer, allocate space for numOfCols ints.
   int i, j;
   for (i = 0; i < buffer->numOfRowsD; i++) {
      buffer->newMatrix[i] = (char*) malloc(sizeof(char) * buffer->numOfColsD);
   }
   // assign default values to the array
   assignDefaults(buffer->newMatrix,buffer->numOfRowsD, buffer->numOfColsD, E);

   int s, c;
   int numOfColsOld=buffer->numOfRows;
      for (i= 0; i < buffer->numOfRowsD ; i ++){
         s= buffer->numOfRowsD-1-i;
         for(c=0;c<numOfColsOld;c++){ // iterate through the columns
            buffer->newMatrix[i][c+s]=buffer->array[i][c];
         }
      }
   buffer->useDiagArray=1;
   scanVertical(buffer);

   assignDefaults(buffer->newMatrix,buffer->numOfRowsD, buffer->numOfColsD, E);
   // now let's scan from the other end ( right end to left)
   for (s=0;s<buffer->numOfRowsD;s++){
      for(c=0;c<numOfColsOld;c++){ // iterate through the columns
         buffer->newMatrix[s][c+s]=buffer->array[s][c];
      }
   }
   buffer->useDiagArray = 1;
   scanVertical(buffer);
   buffer->useDiagArray = 0;
   return NULL;
}

void print2DArray(char **array, int numOfRows, int numOfCols) {
   int i, j;
   for (i = 0; i < numOfRows; i++) {
      for (j = 0; j < numOfCols; j++) {
         printf(" %c ", array[i][j]);
      }
      printf("\n");
   }
}

void assignDefaults(char **array,int r, int c, char d) {
   // assign default values to the array
   int i, j;
   for (i = 0; i < r; i++) {
      for (j = 0; j < c; j++) {
         array[i][j] = d; // default value
      }
   }
 }

void insertPiece(char **array, int x, int y, int turn) {
    char piece;
    if(turn){ // if turn is 1 ( Player 2)
        piece = W;
    }
    else{
        piece = B; // Player 1 ( turn = 0 )
    }
    array[x][y] = piece;
 }
 