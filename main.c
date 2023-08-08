#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
#include <sys/types.h>
#endif

/**
 * Represents user input.
 */
typedef struct {
    char *buffer;
    int bufferLength;
    int inputLength;
} InputBuffer;

/**
 * Allocates memory for an InputBuffer and returns the InputBuffer as a pointer.
 *
 * Remember to free the InputBuffer by calling .
 *
 * @return The InputBuffer allocated.
 */
InputBuffer* allocateInputBuffer() {
    InputBuffer *inputBuffer = malloc(sizeof(InputBuffer));

    inputBuffer->buffer = NULL;
    inputBuffer->bufferLength = 0;
    inputBuffer->inputLength = 0;

    return inputBuffer;
}

/**
 * Free the memory allocated to the InputBuffer (and the memory allocated to the
 * buffer member of the InputBuffer).
 *
 * @param inputBuffer The InputBuffer to be freed.
 */
void freeInputBuffer(InputBuffer *inputBuffer) {
    free(inputBuffer->buffer);
    free(inputBuffer);
}

/**
 * Reads user input from stdin into an InputBuffer.
 *
 * @param inputBuffer The InputBuffer to hold the user input.
 */
void readInput(InputBuffer *inputBuffer) {
    ssize_t bytesRead = getline(
        &(inputBuffer->buffer),
        &(inputBuffer->bufferLength),
        stdin
    );

    // Remove newline
    inputBuffer->buffer[bytesRead - 1] = 0;
    // Input length is 1 less than buffer length because newline is removed
    inputBuffer->inputLength = bytesRead - 1;
}

int main(int argc, char **argv) {
    InputBuffer *inputBuffer = allocateInputBuffer();

    // Start with failure, if program succeeds change to success
    int status = EXIT_FAILURE;

    while (1) {
        printf("db > ");

        readInput(inputBuffer);

        printf("%s\n", inputBuffer->buffer);

        if (strcmp(inputBuffer->buffer, ".exit") == 0) {
            status = EXIT_SUCCESS;
            break;
        } else {
            printf("Unknown command [%s]\n", inputBuffer->buffer);
        }
    }

    freeInputBuffer(inputBuffer);
    return status;
}
