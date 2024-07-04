/*
June 14, 2024
COP 3402 Systems Software Assignment 3
This program is written by: Devon Villalona and Izaac Plambeck
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_IDENT_LEN 11    // Maximum length of an identifier
#define MAX_NUM_LEN 5       // Maximum length of a number
#define MAX_LEXEMES 1000    // Maximum number of lexemes
#define MAX_SYMBOL_TABLE_SIZE 500  // Maximum size of the symbol table
#define MAX_CODE_LENGTH 500  // Maximum length of the code

// Enum for token types
typedef enum {
    oddsym = 1, identsym, numbersym, plussym, minussym,
    multsym, slashsym, fisym, eqlsym, neqsym, lessym, leqsym,
    gtrsym, geqsym, lparentsym, rparentsym, commasym, semicolonsym,
    periodsym, becomessym, beginsym, endsym, ifsym, thensym,
    whilesym, dosym, callsym, constsym, varsym, procsym, writesym,
    readsym, elsesym, errorsym
} token_type;

// Structure for a lexeme
typedef struct {
    char lexeme[MAX_IDENT_LEN + 1];  // The lexeme string
    token_type token;                // Token type
    char error[50];                  // Error message if any
} Lexeme;

// Structure for a symbol
typedef struct {
    int kind; // const = 1, var = 2, proc = 3
    char name[MAX_IDENT_LEN + 1]; // name up to 11 chars
    int val; // number (ASCII value)
    int level; // L level
    int addr; // M address
    int mark; // to indicate unavailable or deleted
} symbol;

// Structure for code instructions
typedef struct {
    char op[4];  // Operation code
    int l;       // Lexicographical level
    int m;       // Modifier
} instruction;

Lexeme lexemes[MAX_LEXEMES];
int lexeme_count = 0;    // Count of lexemes
int current_token = 0;   // Current token index

symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];
int symbol_count = 0;

instruction code[MAX_CODE_LENGTH];
int code_index = 0;

// Array of reserved words
char *reservedWords[] = {
    "const", "var", "procedure", "call", "begin", "end", "if", "fi",
    "then", "else", "while", "do", "read", "write"
};

// Array of corresponding token types for reserved words
token_type reservedTokens[] = {
    constsym, varsym, procsym, callsym, beginsym, endsym, ifsym, fisym,
    thensym, elsesym, whilesym, dosym, readsym, writesym
};

// Function prototypes
void addLexeme(char *lex, token_type token, char *error);
token_type identifyReservedWord(char *word);
void processInput(const char *input);
void readInputFile(const char *filename, char *buffer, int size);
void writeOutputFile(const char *filename);
void writeSymbolTable();
void writeCode();

// Parser and code generation function prototypes
void program();
void block();
void const_declaration();
int var_declaration();
void statement();
void condition();
void expression();
void term();
void factor();
int symbol_table_check(const char *name);
void add_symbol(int kind, const char *name, int val, int level, int addr);
void emit(const char *op, int l, int m);
void error(const char *message, const char *detail);

// Function to add a lexeme to the lexeme array
void addLexeme(char *lex, token_type token, char *error) {
    if (lexeme_count < MAX_LEXEMES) {
        strcpy(lexemes[lexeme_count].lexeme, lex);
        lexemes[lexeme_count].token = token;
        if (error) {
            strcpy(lexemes[lexeme_count].error, error);
        } else {
            lexemes[lexeme_count].error[0] = '\0';
        }
        lexeme_count++;
    }
}

// Function to identify if a word is a reserved word
token_type identifyReservedWord(char *word) {
    for (int i = 0; i < sizeof(reservedWords) / sizeof(reservedWords[0]); i++) {
        if (strcmp(word, reservedWords[i]) == 0) {
            return reservedTokens[i];
        }
    }
    return identsym;
}

// Function to process the input string and generate lexemes
void processInput(const char *input) {
    int length = strlen(input);
    char buffer[MAX_IDENT_LEN + 1];
    int buffer_index = 0;

    for (int i = 0; i < length; i++) {
        char c = input[i];

        if (isspace(c)) continue;  // Ignore whitespace

        if (isalpha(c)) {  // Identifiers or reserved words
            buffer_index = 0;
            while (isalnum(c) && buffer_index < MAX_IDENT_LEN) {
                buffer[buffer_index++] = c;
                c = input[++i];
            }
            buffer[buffer_index] = '\0';
            if (isalnum(c)) {  // Name too long
                while (isalnum(c)) c = input[++i];
                addLexeme(buffer, errorsym, "Name too long");
            } else {
                token_type token = identifyReservedWord(buffer);
                addLexeme(buffer, token, NULL);
            }
            i--;  // Adjust for extra increment
        } else if (isdigit(c)) {  // Numbers
            buffer_index = 0;
            while (isdigit(c) && buffer_index < MAX_NUM_LEN) {
                buffer[buffer_index++] = c;
                c = input[++i];
            }
            buffer[buffer_index] = '\0';
            if (isdigit(c)) {  // Number too long
                while (isdigit(c)) c = input[++i];
                addLexeme(buffer, errorsym, "Number too long");
            } else {
                addLexeme(buffer, numbersym, NULL);
            }
            i--;  // Adjust for extra increment
        } else {  // Special symbols and errors
            buffer[0] = c;
            buffer[1] = '\0';
            switch (c) {
                case '+': addLexeme(buffer, plussym, NULL); break;
                case '-': addLexeme(buffer, minussym, NULL); break;
                case '*': addLexeme(buffer, multsym, NULL); break;
                case '/':
                    if (input[i + 1] == '*') {  // Start of comment
                        i += 2;  // Skip '/*'
                        while (i < length - 1 && !(input[i] == '*' && input[i + 1] == '/')) {
                            i++;
                        }
                        if (i < length - 1) {
                            i++;  // Skip '*/'
                        } else {
                            addLexeme("/*", errorsym, "Unterminated comment");
                        }
                    } else {
                        addLexeme(buffer, slashsym, NULL);
                    }
                    break;
                case '=': addLexeme(buffer, eqlsym, NULL); break;
                case '<':
                    if (input[i + 1] == '>') {
                        buffer[1] = '>';
                        buffer[2] = '\0';
                        addLexeme(buffer, neqsym, NULL);
                        i++;
                    } else if (input[i + 1] == '=') {
                        buffer[1] = '=';
                        buffer[2] = '\0';
                        addLexeme(buffer, leqsym, NULL);
                        i++;
                    } else {
                        addLexeme(buffer, lessym, NULL);
                    }
                    break;
                case '>':
                    if (input[i + 1] == '=') {
                        buffer[1] = '=';
                        buffer[2] = '\0';
                        addLexeme(buffer, geqsym, NULL);
                        i++;
                    } else {
                        addLexeme(buffer, gtrsym, NULL);
                    }
                    break;
                case ':':
                    if (input[i + 1] == '=') {
                        buffer[1] = '=';
                        buffer[2] = '\0';
                        addLexeme(buffer, becomessym, NULL);
                        i++;
                    } else {
                        addLexeme(buffer, errorsym, "Invalid character");
                    }
                    break;
                case ';': addLexeme(buffer, semicolonsym, NULL); break;
                case ',': addLexeme(buffer, commasym, NULL); break;
                case '.': addLexeme(buffer, periodsym, NULL); break;
                case '(': addLexeme(buffer, lparentsym, NULL); break;
                case ')': addLexeme(buffer, rparentsym, NULL); break;
                default:
                    addLexeme(buffer, errorsym, "Invalid character");
                    break;
            }
        }
    }
}

// Function to read input from a file
void readInputFile(const char *filename, char *buffer, int size) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int index = 0;
    char c;
    while ((c = fgetc(file)) != EOF && index < size - 1) {
        buffer[index++] = c;
    }
    buffer[index] = '\0';
    fclose(file);
}

// Function to write output to a file
void writeOutputFile(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "Line OP L M\n");
    for (int i = 0; i < code_index; i++) {
        fprintf(file, "%d %s %d %d\n", i, code[i].op, code[i].l, code[i].m);
    }

    fprintf(file, "Symbol Table:\n");
    fprintf(file, "Kind | Name | Value | Level | Address | Mark\n");
    fprintf(file, "---------------------------------------------------\n");
    for (int i = 0; i < symbol_count; i++) {
        fprintf(file, "%d | %s | %d | %d | %d | %d\n", symbol_table[i].kind,
                symbol_table[i].name, symbol_table[i].val, symbol_table[i].level,
                symbol_table[i].addr, symbol_table[i].mark);
    }

    fclose(file);
}

// Function to write the symbol table to the output
void writeSymbolTable() {
    printf("Symbol Table:\n");
    printf("Kind | Name | Value | Level | Address | Mark\n");
    printf("---------------------------------------------------\n");
    for (int i = 0; i < symbol_count; i++) {
        printf("%d | %s | %d | %d | %d | %d\n", symbol_table[i].kind,
               symbol_table[i].name, symbol_table[i].val, symbol_table[i].level,
               symbol_table[i].addr, symbol_table[i].mark);
    }
}

// Function to write the generated code to the output
void writeCode() {
    printf("Line OP L M\n");
    for (int i = 0; i < code_index; i++) {
        printf("%d %s %d %d\n", i, code[i].op, code[i].l, code[i].m);
    }
}

// Function to display an error message and halt the program
void error(const char *message, const char *detail) {
    if (detail) {
        fprintf(stderr, "Error: %s %s\n", message, detail);
    } else {
        fprintf(stderr, "Error: %s\n", message);
    }
    exit(EXIT_FAILURE);
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const int bufferSize = 10000;  // Buffer size for input
    char input[bufferSize];

    readInputFile(argv[1], input, bufferSize);  // Read input from file

    processInput(input);  // Process the input to generate lexemes

    program(); // Call the parser function to start parsing

    writeCode(); // Print the generated code

    writeSymbolTable(); // Print the symbol table

    writeOutputFile("output.txt"); // Write the output to the file

    return 0;
}

// Symbol table related functions
int symbol_table_check(const char *name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0 && symbol_table[i].mark == 0) {
            return i;
        }
    }
    return -1;
}

void add_symbol(int kind, const char *name, int val, int level, int addr) {
    if (symbol_count >= MAX_SYMBOL_TABLE_SIZE) {
        error("Symbol table overflow", NULL);
    }
    symbol_table[symbol_count].kind = kind;
    strncpy(symbol_table[symbol_count].name, name, MAX_IDENT_LEN);
    symbol_table[symbol_count].val = val;
    symbol_table[symbol_count].level = level;
    symbol_table[symbol_count].addr = addr;
    symbol_table[symbol_count].mark = 0;
    symbol_count++;
}

// Function to emit code
void emit(const char *op, int l, int m) {
    if (code_index >= MAX_CODE_LENGTH) {
        error("Code size exceeded", NULL);
    }
    strncpy(code[code_index].op, op, 4);
    code[code_index].l = l;
    code[code_index].m = m;
    code_index++;
}

// Parser and code generation functions
void program() {
    emit("JMP", 0, 3);  // Placeholder for JMP
    block();
    if (lexemes[current_token].token != periodsym) {
        error("program must end with period", NULL);
    }
    emit("SYS", 0, 3);  // HALT instruction
}

void block() {
    const_declaration();
    int numVars = var_declaration();
    emit("INC", 0, 3 + numVars);
    statement();
}

void const_declaration() {
    if (lexemes[current_token].token == constsym) {
        do {
            current_token++;
            if (lexemes[current_token].token != identsym) {
                error("const must be followed by identifier", NULL);
            }
            if (symbol_table_check(lexemes[current_token].lexeme) != -1) {
                error("symbol name has already been declared", lexemes[current_token].lexeme);
            }
            char name[MAX_IDENT_LEN + 1];
            strcpy(name, lexemes[current_token].lexeme);
            current_token++;
            if (lexemes[current_token].token != eqlsym) {
                error("constants must be assigned with =", NULL);
            }
            current_token++;
            if (lexemes[current_token].token != numbersym) {
                error("constants must be assigned an integer value", NULL);
            }
            int value = atoi(lexemes[current_token].lexeme);
            add_symbol(1, name, value, 0, 0);
            current_token++;
        } while (lexemes[current_token].token == commasym);
        if (lexemes[current_token].token != semicolonsym) {
            error("constant declarations must be followed by a semicolon", NULL);
        }
        current_token++;
    }
}

int var_declaration() {
    int numVars = 0;
    if (lexemes[current_token].token == varsym) {
        do {
            current_token++;
            if (lexemes[current_token].token != identsym) {
                error("var must be followed by identifier", NULL);
            }
            if (symbol_table_check(lexemes[current_token].lexeme) != -1) {
                error("symbol name has already been declared", lexemes[current_token].lexeme);
            }
            add_symbol(2, lexemes[current_token].lexeme, 0, 0, 3 + numVars);
            numVars++;
            current_token++;
        } while (lexemes[current_token].token == commasym);
        if (lexemes[current_token].token != semicolonsym) {
            error("variable declarations must be followed by a semicolon", NULL);
        }
        current_token++;
    }
    return numVars;
}

void statement() {
    if (lexemes[current_token].token == identsym) {
        int symIdx = symbol_table_check(lexemes[current_token].lexeme);
        if (symIdx == -1) {
            error("undeclared identifier", lexemes[current_token].lexeme);
        }
        if (symbol_table[symIdx].kind != 2) {
            error("only variable values may be altered", NULL);
        }
        current_token++;
        if (lexemes[current_token].token != becomessym) {
            error("assignment statements must use :=", NULL);
        }
        current_token++;
        expression();
        emit("STO", 0, symbol_table[symIdx].addr);
    } else if (lexemes[current_token].token == beginsym) {
        current_token++;
        do {
            statement();
            if (lexemes[current_token].token == semicolonsym) {
                current_token++;
            }
        } while (lexemes[current_token].token == semicolonsym);
        if (lexemes[current_token].token != endsym) {
            error("begin must be followed by end", NULL);
        }
        current_token++;
    } else if (lexemes[current_token].token == ifsym) {
        current_token++;
        condition();
        int jpcIdx = code_index;
        emit("JPC", 0, 0);  // Placeholder for JPC
        if (lexemes[current_token].token != thensym) {
            error("if must be followed by then", NULL);
        }
        current_token++;
        statement();
        code[jpcIdx].m = code_index;  // Update JPC instruction with current code index
    } else if (lexemes[current_token].token == whilesym) {
        current_token++;
        int loopIdx = code_index;
        condition();
        if (lexemes[current_token].token != dosym) {
            error("while must be followed by do", NULL);
        }
        current_token++;
        int jpcIdx = code_index;
        emit("JPC", 0, 0);  // Placeholder for JPC
        statement();
        emit("JMP", 0, loopIdx);  // Jump back to the start of the loop
        code[jpcIdx].m = code_index;  // Update JPC instruction with current code index
    } else if (lexemes[current_token].token == readsym) {
        current_token++;
        if (lexemes[current_token].token != identsym) {
            error("read must be followed by identifier", NULL);
        }
        int symIdx = symbol_table_check(lexemes[current_token].lexeme);
        if (symIdx == -1) {
            error("undeclared identifier", lexemes[current_token].lexeme);
        }
        if (symbol_table[symIdx].kind != 2) {
            error("only variable values may be altered", NULL);
        }
        current_token++;
        emit("READ", 0, 0);
        emit("STO", 0, symbol_table[symIdx].addr);
    } else if (lexemes[current_token].token == writesym) {
        current_token++;
        expression();
        emit("WRITE", 0, 0);
    } else {
        error("Invalid statement", NULL);
    }
}

void condition() {
    if (lexemes[current_token].token == oddsym) {
        current_token++;
        expression();
        emit("ODD", 0, 0);
    } else {
        expression();
        token_type relOp = lexemes[current_token].token;
        current_token++;
        expression();
        switch (relOp) {
            case eqlsym: emit("EQL", 0, 0); break;
            case neqsym: emit("NEQ", 0, 0); break;
            case lessym: emit("LSS", 0, 0); break;
            case leqsym: emit("LEQ", 0, 0); break;
            case gtrsym: emit("GTR", 0, 0); break;
            case geqsym: emit("GEQ", 0, 0); break;
            default:
                error("condition must contain comparison operator", NULL);
        }
    }
}

void expression() {
    if (lexemes[current_token].token == plussym || lexemes[current_token].token == minussym) {
        token_type addOp = lexemes[current_token].token;
        current_token++;
        term();
        if (addOp == minussym) {
            emit("NEG", 0, 0);
        }
    } else {
        term();
    }
    while (lexemes[current_token].token == plussym || lexemes[current_token].token == minussym) {
        token_type addOp = lexemes[current_token].token;
        current_token++;
        term();
        if (addOp == plussym) {
            emit("ADD", 0, 0);
        } else {
            emit("SUB", 0, 0);
        }
    }
}

void term() {
    factor();
    while (lexemes[current_token].token == multsym || lexemes[current_token].token == slashsym) {
        token_type mulOp = lexemes[current_token].token;
        current_token++;
        factor();
        if (mulOp == multsym) {
            emit("MUL", 0, 0);
        } else {
            emit("DIV", 0, 0);
        }
    }
}

void factor() {
    if (lexemes[current_token].token == identsym) {
        int symIdx = symbol_table_check(lexemes[current_token].lexeme);
        if (symIdx == -1) {
            error("undeclared identifier", lexemes[current_token].lexeme);
        }
        if (symbol_table[symIdx].kind == 1) {
            emit("LIT", 0, symbol_table[symIdx].val);
        } else {
            emit("LOD", 0, symbol_table[symIdx].addr);
        }
        current_token++;
    } else if (lexemes[current_token].token == numbersym) {
        emit("LIT", 0, atoi(lexemes[current_token].lexeme));
        current_token++;
    } else if (lexemes[current_token].token == lparentsym) {
        current_token++;
        expression();
        if (lexemes[current_token].token != rparentsym) {
            error("right parenthesis must follow left parenthesis", NULL);
        }
        current_token++;
    } else {
        error("arithmetic equations must contain operands, parentheses, numbers, or symbols", NULL);
    }
}