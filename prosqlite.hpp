#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND,
} MetaCommandResult;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR,
} PrepareResult;

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
    EXECUTE_UNKNOWN_ERROR,
} ExecuteResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT,
} StatementType;




#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
} Row;

typedef struct {
    StatementType type;
    Row row_to_insert;
} Statement;

typedef struct {
    char* buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;





// A brilliant way to calcuate attribute size
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

// This is the offset in the serialized struct rather than memory
const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;


/**
 * serialize_row - serialize a roww
*/
void serialize_row(Row* src, void* dst) {
    memcpy((char*)dst + ID_OFFSET, &(src->id), ID_SIZE);
    memcpy((char*)dst + USERNAME_OFFSET, src->username, USERNAME_SIZE); // difference
    memcpy((char*)dst + EMAIL_OFFSET, src->email, EMAIL_SIZE);
}

/**
 * deserialize_row - deseialize a row
*/
void deserialize_row(void* src, Row* dst) {
    memcpy(&(dst->id), (char*)src + ID_OFFSET, ID_SIZE);
    memcpy(dst->username, (char*)src + USERNAME_OFFSET, USERNAME_SIZE); // difference
    memcpy(dst->email, (char*)src + EMAIL_OFFSET, EMAIL_SIZE);
}



const uint32_t PAGE_SIZE = 4096;
const uint32_t TABLE_MAX_PAGES = 100;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / TABLE_MAX_PAGES;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct {
    void* pages[TABLE_MAX_PAGES]; // array of pointers
    uint32_t num_rows;
} Table;

/**
 * row_slot - get the address of the row_num-th row in the table
*/
void* row_slot(Table* table, uint32_t row_num) {
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    void* page = table->pages[page_num];
    if (!page) {
        page = table->pages[page_num] = malloc(PAGE_SIZE);
    }
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return (char*)page + byte_offset;
}

/**
 * print_row - print the row
*/
void print_row(Row* row) {
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}



/**
 * new_table - create a table
*/
Table* new_table() {
    Table* table = (Table*)malloc(sizeof(Table));
    memset(table->pages, 0, sizeof table->pages);
    table->num_rows = 0;
    return table;
}

/**
 * new_input_buffer - create an input buffer
*/
InputBuffer* new_input_buffer() {
    InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;
    return input_buffer;
}

/**
 * print_prompt - print prompt to the console
*/
void print_prompt() {
    printf("prosqlite > ");
}


/**
 * read_input - read from console
*/
void read_input(InputBuffer* input_buffer) {
    ssize_t bytes_read = getline(&(input_buffer->buffer), 
                                &(input_buffer->buffer_length), 
                                stdin);
    
    if (bytes_read <= 0) {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }

    // replace '\n' with '\0
    input_buffer->buffer_length = bytes_read - 1;
    input_buffer->buffer[bytes_read - 1] = '\0';
}