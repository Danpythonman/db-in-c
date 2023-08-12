#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _MSC_VER
    #include <BaseTsd.h>
    typedef SSIZE_T ssize_t;
#else
    #include <sys/types.h>
#endif

#define sizeOfAttribute(STRUCT, ATTRIBUTE) sizeof(((STRUCT*)NULL)->ATTRIBUTE)

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

/**
 * Internal representation of a row.
 */
typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
} Row;

#define ID_SIZE sizeOfAttribute(Row, id)
#define USERNAME_SIZE sizeOfAttribute(Row, username)
#define EMAIL_SIZE sizeOfAttribute(Row, email)
#define ID_OFFSET 0
#define USERNAME_OFFSET (ID_OFFSET + ID_SIZE)
#define EMAIL_OFFSET (USERNAME_OFFSET + USERNAME_SIZE)
#define ROW_SIZE (ID_SIZE + USERNAME_SIZE + EMAIL_SIZE)

/**
 * Serializes a row to an area in memory.
 *
 * The area in memory should be in a page (allocated by findRowLocation).
 *
 * @param source      The row to be serialized to memory.
 * @param destination The area in memory to hold the serialized row.
 */
void serializeRow(Row *source, void *destination) {
    // Cast to char because pointer arithmetic is not allowed for void
    // pointers. Using char, we can do pointer arithmetic with bytes.
    memcpy((char*) destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(
        (char*) destination + USERNAME_OFFSET,
        &(source->username),
        USERNAME_SIZE
    );
    memcpy((char*) destination + EMAIL_OFFSET, &(source->email), EMAIL_OFFSET);
}

/**
 * Deserializes a row from an area in memory.
 *
 * @param source      The area in memory holding the serialized row.
 * @param destination The row for the memory to be deserialized into.
 */
void deserializeRow(void *source, Row *destination) {
    // Cast to char because pointer arithmetic is not allowed for void
    // pointers. Using char, we can do pointer arithmetic with bytes.
    memcpy(&(destination->id), (char*) source + ID_OFFSET, ID_SIZE);
    memcpy(
        &(destination->username),
        (char*) source + USERNAME_OFFSET,
        USERNAME_SIZE
    );
    memcpy(&(destination->email), (char*) source + EMAIL_OFFSET, EMAIL_SIZE);
}

#define TABLE_MAX_PAGES 100
#define PAGE_SIZE 4096
#define ROWS_PER_PAGE (PAGE_SIZE / ROW_SIZE)
#define TABLE_MAX_ROWS (ROWS_PER_PAGE * TABLE_MAX_PAGES)

/**
 * Internal representation of a table.
 */
typedef struct {
    uint32_t rowCount;
    uint32_t pageCount;
    void* pages[TABLE_MAX_PAGES];
} Table;

/**
 * Finds the location in memory of the given row of the given table.
 *
 * This function allocates a page for the row if one does not yet exist.
 *
 * @param table     The table to find the row location in.
 * @param rowNumber The number of the row within the table.
 *
 * @return A pointer to the row.
 */
void* findRowLocation(Table *table, uint32_t rowNumber) {
    uint32_t pageNumber = rowNumber / ROWS_PER_PAGE;
    void *page = table->pages[pageNumber];

    if (page == NULL) {
        // Allocate a new page
        page = malloc(PAGE_SIZE);
        table->pages[pageNumber] = page;
        table->pageCount++;
    }

    // Get the location of the row within the page
    uint32_t rowOffset = rowNumber % ROWS_PER_PAGE;
    uint32_t byteOffset = rowOffset * ROW_SIZE;

    return (char*) page + byteOffset;
}

/**
 * Result of preparing a statement.
 */
typedef enum {
    PREPARE_STATEMENT_SUCCESS,
    PREPARE_STATEMENT_UNRECOGNIZED_STATEMENT,
    PREPARE_STATEMENT_SYNTAX_ERROR
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
    Row *rowToInsert;
} Statement;

/**
 * Result of executing a statement.
 */
typedef enum {
    EXECUTE_INSERT_SUCCESS,
    EXECUTE_SELECT_SUCCESS,
    EXECUTE_STATEMENT_TABLE_FULL
} ExecuteStatementResult;

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
 * Allocates memory for an InputBuffer.
 *
 * Remember to free the InputBuffer by calling freeInputBuffer().
 *
 * @return A pointer to the allocated InputBuffer.
 */
InputBuffer* allocateInputBuffer() {
    InputBuffer *inputBuffer = malloc(sizeof(InputBuffer));

    inputBuffer->buffer = NULL;
    inputBuffer->bufferLength = 0;
    inputBuffer->inputLength = 0;

    return inputBuffer;
}

/**
 * ALlocates memory for a Table.
 *
 * Remember to free the Table by calling freeTable().
 *
 * @return A pointer to the allocated Table.
 */
Table* allocateTable() {
    Table *table = malloc(sizeof(Table));

    table->rowCount = 0;
    table-> pageCount = 0;
    for (int i = 0; i < TABLE_MAX_PAGES; i++) {
        table->pages[i] = NULL;
    }

    return table;
}

/**
 * @brief Allocates memory for a Statement and a row that will be part of the
 * Statement.
 *
 * Remember to free the Table by calling freeStatement().
 *
 * @return A pointer to the allocated Statement.
 */
Statement* allocateStatement() {
    Statement *statement = malloc(sizeof(Statement));
    Row *rowToInsert = malloc(sizeof(Row));

    statement->rowToInsert = rowToInsert;

    return statement;
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
 * Free the memory allocated to a table (and the memory allocated to its pages).
 *
 * @param table The table to be freed.
 */
void freeTable(Table *table) {
    for (int i = 0; i < table->pageCount; i++) {
        free(table->pages[i]);
    }
    free(table);
}

/**
 * Free the memory allocated to a Statement (and the memory allocated to its
 * row to insert).
 *
 * @param statement The statement to be freed.
 */
void freeStatement(Statement *statement) {
    free(statement->rowToInsert);
    free(statement);
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
        int argsAssigned = sscanf(
            inputBuffer->buffer,
            "insert %d %s %s",
            &(statement->rowToInsert->id),
            statement->rowToInsert->username,
            statement->rowToInsert->email
        );
        if (argsAssigned < 3) {
            return PREPARE_STATEMENT_SYNTAX_ERROR;
        }
        return PREPARE_STATEMENT_SUCCESS;
    } else if (strcmp(inputBuffer->buffer, "select") == 0) {
        statement->type = SELECT;
        return PREPARE_STATEMENT_SUCCESS;
    } else {
        return PREPARE_STATEMENT_UNRECOGNIZED_STATEMENT;
    }
}

/**
 * Inserts a row into the database.
 *
 * @param statement The statement to be executed. Should be an insert statement
 *                  and should include the row to be inserted.
 * @param table     The table that the row is being inserted to.
 *
 * @return The result of the insert statement execution.
 */
ExecuteStatementResult executeInsertStatement(
    Statement *statement,
    Table *table
) {
    if (table->rowCount >= TABLE_MAX_ROWS) {
        return EXECUTE_STATEMENT_TABLE_FULL;
    }

    Row rowToInsert = *(statement->rowToInsert);
    void* rowLocation = findRowLocation(table, table->rowCount);

    serializeRow(&rowToInsert, rowLocation);

    table->rowCount++;

    return EXECUTE_INSERT_SUCCESS;
}

/**
 * Prints all rows from a table.
 *
 * @param table The table to be printed.
 *
 * @return The result of executing this statement.
 */
ExecuteStatementResult printAllRows(Table *table) {
    Row row;
    for (uint32_t rowNumber = 0; rowNumber < table->rowCount; rowNumber++) {
        void* rowLocation = findRowLocation(table, rowNumber);
        deserializeRow(rowLocation, &row);
        printf("%d %s %s\n", row.id, row.username, row.email);
    }
    return EXECUTE_SELECT_SUCCESS;
}

/**
 * Executes a statement.
 *
 * The statement should be the internal representation of the statement, not a
 * string or InputBuffer.
 *
 * @param statement The statement to be executed.
 */
ExecuteStatementResult executeStatement(Statement *statement, Table *table) {
    switch (statement->type) {
        case INSERT:
            return executeInsertStatement(statement, table);
        case SELECT:
            return printAllRows(table);
    }
}

int main(int argc, char **argv) {
    InputBuffer *inputBuffer = allocateInputBuffer();
    Table *table = allocateTable();
    Statement *statement = allocateStatement();

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
                    printf("Goodbye\n");
                    status = EXIT_SUCCESS;
                    break;
            }
        }

        if (status == EXIT_SUCCESS) {
            break;
        }

        switch (prepareStatement(inputBuffer, statement)) {
            case PREPARE_STATEMENT_SUCCESS:
                break;
            case PREPARE_STATEMENT_SYNTAX_ERROR:
                printf("Syntax error\n");
                continue;
            case PREPARE_STATEMENT_UNRECOGNIZED_STATEMENT:
                printf("Unrecognized statement '%s'\n", inputBuffer->buffer);
                continue;
        }

        switch (executeStatement(statement, table)) {
            case EXECUTE_INSERT_SUCCESS:
                printf("Inserted row\n");
                break;
            case EXECUTE_SELECT_SUCCESS:
                break;
            case EXECUTE_STATEMENT_TABLE_FULL:
                printf("Table is full\n");
                break;
        }
    }

    freeInputBuffer(inputBuffer);
    freeTable(table);
    freeStatement(statement);

    return status;
}
