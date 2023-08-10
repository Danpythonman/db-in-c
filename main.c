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
 * Result of preparing a statement.
 */
typedef enum {
    PREPARE_STATEMENT_SUCCESS,
    PREPARE_STATEMENT_UNRECOGNIZED_STATEMENT
} PrepareStatementResult;

/**
 * Represents the type of a statement.
 */
typedef enum {
    INSERT,
    SELECT
} StatementType;

/**
 * Internal representation of a statement. This is what is used to retrieve and
 * manipulate data.
 */
typedef struct {
    StatementType type;
} Statement;

/**
 * Result of executing a meta command.
 */
typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND,
    META_COMMAND_EXIT
} MetaCommandResult;

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

/**
 * Executes a meta command.
 *
 * Currently, the only available meta command is ".exit", which exits from the
 * database CLI.
 *
 * @param inputBuffer The input buffer containing the meta command.
 *
 * @return MetaCommandResult The result of the meta command.
 */
MetaCommandResult executeMetaCommand(InputBuffer *inputBuffer) {
    if (strcmp(inputBuffer->buffer, ".exit") == 0) {
        return META_COMMAND_EXIT;
    } else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

/**
 * Prepares a statement.
 *
 * Converts an input buffer to a Statement, which is our internal representation
 * of a statement.
 *
 * @param inputBuffer The input buffer to be converted to a statement.
 * @param statement   The statement to be "filled in" with data from the input
 *                    buffer. Note that the memory for the statement should
 *                    already be allocated.
 *
 * @return The result of the statement preparation.
 */
PrepareStatementResult prepareStatement(
    InputBuffer *inputBuffer,
    Statement *statement
) {
    if (strncmp(inputBuffer->buffer, "insert", 6) == 0) {
        statement->type = INSERT;
        return PREPARE_STATEMENT_SUCCESS;
    } else if (strcmp(inputBuffer->buffer, "select") == 0) {
        statement->type = SELECT;
        return PREPARE_STATEMENT_SUCCESS;
    } else {
        return PREPARE_STATEMENT_UNRECOGNIZED_STATEMENT;
    }
}

/**
 * Executes a statement.
 *
 * The statement should be the internal representation of the statement, not a
 * string or InputBuffer.
 *
 * @param statement The statement to be executed.
 */
void executeStatement(Statement *statement) {
    switch (statement->type) {
        case INSERT:
            printf("Inserting into DB...\n");
            break;
        case SELECT:
            printf("Selecting from DB...\n");
            break;
    }
}

int main(int argc, char **argv) {
    InputBuffer *inputBuffer = allocateInputBuffer();
    Statement statement;

    // Start with failure, if program succeeds change to success
    int status = EXIT_FAILURE;

    while (1) {
        printf("db > ");

        readInput(inputBuffer);

        if (inputBuffer->buffer[0] == '.') {
            switch (executeMetaCommand(inputBuffer)) {
                case META_COMMAND_SUCCESS:
                    continue;
                case META_COMMAND_UNRECOGNIZED_COMMAND:
                    printf("Unrecognized command '%s'\n", inputBuffer->buffer);
                    continue;
                case META_COMMAND_EXIT:
                    status = EXIT_SUCCESS;
                    break;
            }
        }

        if (status == EXIT_SUCCESS) {
            break;
        }

        switch (prepareStatement(inputBuffer, &statement)) {
            case PREPARE_STATEMENT_SUCCESS:
                break;
            case PREPARE_STATEMENT_UNRECOGNIZED_STATEMENT:
                printf("Unrecognized statement '%s'\n", inputBuffer->buffer);
                continue;
        }

        executeStatement(&statement);
    }

    freeInputBuffer(inputBuffer);
    return status;
}
