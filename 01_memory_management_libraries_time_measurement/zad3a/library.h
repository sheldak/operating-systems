#ifndef library_h
#define library_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct ArrayOfBlocks{
    int size;
    struct Block *blocks;
};

struct Block{
    int index;
    int size;
    struct Operation *operations;
};

struct Operation{
    int index;
    char *contents;
};

struct Sequence{
    struct FilesPair *filesPair;
};

struct FilesPair{
    char *file1;
    char *file2;
    struct FilesPair *next;
};

struct ArrayOfBlocks *createArrayOfBlocks(int size);
int validPair(char *potentialPair);
struct Sequence *makeSequence(char *pairs);
char **compareFiles(struct Sequence *sequence);
int makeBlock(struct ArrayOfBlocks *arrayOfBlocks, char *filename);
int numberOfOperations(struct Block block);
void removeOperation(struct ArrayOfBlocks *arrayOfBlocks, int blockIndex, int operationIndex);
void removeBlock(struct ArrayOfBlocks *arrayOfBlocks, int blockIndex);

#endif //library_h
