/*
 * COP 3402: Systems Software
 * 
 * Authors: Devon Villalona, Izaac Plambeck
 * 
 * Description:
 * This program implements a PL/0 compiler which performs lexical analysis,
 * parsing, and code generation to produce executable code for a Virtual Machine (VM).
 * 
 * University of Central Florida
 * Department of Computer Science
 * Summer 2024
 * Homework #4 (PL/0 Compiler)
 * Due: July 21st, 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Define the max values
#define MAX_LEXEMES 1000
#define MAX_TOKENS 1000
#define MAX_SOURCE_SIZE 10000
#define MAX_IDENTIFIER_LENGTH 11
#define MAX_NUMBER_LENGTH 5
#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_CODE_LENGTH 500

// Enum for token types
typedef enum {
    oddsym = 1, identsym, numbersym, plussym, minussym,
    multsym, slashsym, nulsym, eqsym, neqsym, lessym, leqsym,
    gtrsym, geqsym, lparentsym, rparentsym, commasym, semicolonsym,
    periodsym, becomessym, beginsym, endsym, ifsym, thensym,
    whilesym, dosym, callsym, constsym, varsym, procsym, writesym,
    readsym, elsesym, fisym, error_token
} token_type;

// Lexeme structure
typedef struct {
    char lexeme[12];
    token_type token;
    char errorMessage[50];
} Lexeme;

// Token structure
typedef struct {
    token_type token;
    char value[12];
} Token;

// Symbol structure
typedef struct {
    int kind; // const = 1, var = 2, proc = 3
    char name[10]; // name up to 11 chars
    int val; // number (ASCII value)
    int level; // L level
    int addr; // M address
    int mark; // to indicate unavailable or deleted
} symbol;

// Instruction structure
typedef struct {
    int op;
    int l;
    int m;
} instruction;

// Compiler state structure
typedef struct {
    Lexeme lexemes[MAX_LEXEMES];
    Token tokens[MAX_TOKENS];
    int lexemeCount;
    int tokenCount;
    int current_token;
    symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];
    int symbol_table_index;
    instruction code[MAX_CODE_LENGTH];
    int code_index;
} CompilerState;

// Function declarations
void initializeCompilerState(CompilerState *state);
void lexicalAnalyzer(const char *source, CompilerState *state);
void addLexeme(CompilerState *state, const char *lexeme, token_type token, const char *errorMessage);
void addToken(CompilerState *state, token_type token, const char *value);
token_type getReservedWordToken(const char *word);
token_type getSpecialSymbolToken(const char *symbol);
void printLexicalOutput(const CompilerState *state, FILE *outputFile);
void print_error(int error_number, const char *message, const char *identifier, FILE *errorFile);
int parseProgram(CompilerState *state, FILE *errorFile);
void block(CompilerState *state, FILE *errorFile, int level);
void const_declaration(CompilerState *state, FILE *errorFile, int level);
int var_declaration(CompilerState *state, FILE *errorFile, int level);
void statement(CompilerState *state, FILE *errorFile, int level);
void condition(CompilerState *state, FILE *errorFile, int level);
void expression(CompilerState *state, FILE *errorFile, int level);
void term(CompilerState *state, FILE *errorFile, int level);
void factor(CompilerState *state, FILE *errorFile, int level);
int symbol_table_check(CompilerState *state, char *name, int level);
int symbol_table_check_declaration(CompilerState *state, char *name, int level);
void add_to_symbol_table(CompilerState *state, int kind, char *name, int val, int level, int addr);
void mark_symbols(CompilerState *state, int level);
void emit(CompilerState *state, int op, int l, int m);
void printCode(const CompilerState *state, FILE *outputFile);
void outputElfFile(const CompilerState *state);
void printsymbols(const CompilerState *state);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <source file>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    char sourceProgram[MAX_SOURCE_SIZE];
    size_t sourceSize = fread(sourceProgram, 1, MAX_SOURCE_SIZE - 1, file);
    fclose(file);
    sourceProgram[sourceSize] = '\0';

    CompilerState state;
    initializeCompilerState(&state);

    lexicalAnalyzer(sourceProgram, &state);

    FILE *lexOutputFile = fopen("lexOutput.txt", "w");
    if (!lexOutputFile) {
        perror("Error opening lexOutput.txt file");
        return 1;
    }

    printLexicalOutput(&state, lexOutputFile);
    fclose(lexOutputFile);

    FILE *errorOutputFile = fopen("errorOutput.txt", "w");
    if (!errorOutputFile) {
        perror("Error opening errorOutput.txt file");
        return 1;
    }

    if (!parseProgram(&state, errorOutputFile)) {
        fprintf(stderr, "Compilation failed. See errorOutput.txt for details.\n");
        fclose(errorOutputFile);
        return 1;
    }

    fclose(errorOutputFile);

    FILE *outputFile = fopen("output.txt", "w");
    if (!outputFile) {
        perror("Error opening output.txt file");
        return 1;
    }

    printCode(&state, outputFile);
    fclose(outputFile);

    outputElfFile(&state);

    printsymbols(&state);

    return 0;
}

// Initialize the compiler state
void initializeCompilerState(CompilerState *state) {
    state->lexemeCount = 0;
    state->tokenCount = 0;
    state->current_token = 0;
    state->symbol_table_index = 0;
    state->code_index = 0;
}

// Add a lexeme to the state
void addLexeme(CompilerState *state, const char *lexeme, token_type token, const char *errorMessage) {
    strncpy(state->lexemes[state->lexemeCount].lexeme, lexeme, sizeof(state->lexemes[state->lexemeCount].lexeme) - 1);
    state->lexemes[state->lexemeCount].lexeme[sizeof(state->lexemes[state->lexemeCount].lexeme) - 1] = '\0';
    state->lexemes[state->lexemeCount].token = token;
    if (errorMessage != NULL) {
        strncpy(state->lexemes[state->lexemeCount].errorMessage, errorMessage, sizeof(state->lexemes[state->lexemeCount].errorMessage) - 1);
        state->lexemes[state->lexemeCount].errorMessage[sizeof(state->lexemes[state->lexemeCount].errorMessage) - 1] = '\0';
    } else {
        state->lexemes[state->lexemeCount].errorMessage[0] = '\0';
    }
    state->lexemeCount++;
}

// Add a token to the state
void addToken(CompilerState *state, token_type token, const char *value) {
    state->tokens[state->tokenCount].token = token;
    strncpy(state->tokens[state->tokenCount].value, value, sizeof(state->tokens[state->tokenCount].value) - 1);
    state->tokens[state->tokenCount].value[sizeof(state->tokens[state->tokenCount].value) - 1] = '\0';
    state->tokenCount++;
}

// Get token for reserved words
token_type getReservedWordToken(const char *word) {
    static const char *reservedWords[] = {
        "const", "var", "procedure", "call", "begin", "end", "if", "fi", "then",
        "else", "while", "do", "read", "write"
    };
    static const token_type reservedTokens[] = {
        constsym, varsym, procsym, callsym, beginsym, endsym, ifsym, fisym, thensym, elsesym, whilesym, dosym, readsym, writesym
    };

    for (int i = 0; i < sizeof(reservedWords) / sizeof(reservedWords[0]); i++) {
        if (strcmp(word, reservedWords[i]) == 0) {
            return reservedTokens[i];
        }
    }
    return identsym;
}

// Get token for special symbols
token_type getSpecialSymbolToken(const char *symbol) {
    static const char *specialSymbols[] = {
        "+", "-", "*", "/", "(", ")", "=", ",", ".", "<", ">", ";", ":=", "<>"
    };
    static const token_type specialTokens[] = {
        plussym, minussym, multsym, slashsym, lparentsym, rparentsym, eqsym, commasym, periodsym, lessym, gtrsym, semicolonsym, becomessym
    };

    for (int i = 0; i < sizeof(specialSymbols) / sizeof(specialSymbols[0]); i++) {
        if (strcmp(symbol, specialSymbols[i]) == 0) {
            return specialTokens[i];
        }
    }
    return nulsym;
}

// Lexical analyzer
void lexicalAnalyzer(const char *source, CompilerState *state) {
    int i = 0;
    while (source[i] != '\0') {
        if (isspace(source[i])) {
            i++;
            continue;
        }

        // Handle comments
        if (source[i] == '/' && source[i + 1] == '*') {
            i += 2;
            int comment_closed = 0;
            while (source[i] != '\0') {
                if (source[i] == '*' && source[i + 1] == '/') {
                    i += 2;
                    comment_closed = 1;
                    break;
                }
                i++;
            }
            if (!comment_closed) {
                addLexeme(state, "/*", error_token, "Error: unclosed comment");
                continue;
            }
            continue;
        }

        // Handle identifiers and reserved words
        if (isalpha(source[i])) {
            char buffer[12] = {0};
            int j = 0;
            while (isalnum(source[i]) && j < MAX_IDENTIFIER_LENGTH) {
                buffer[j++] = source[i++];
            }
            buffer[j] = '\0'; // Ensure null-terminated
            if (isalnum(source[i])) {
                addLexeme(state, buffer, error_token, "Error: Identifier too long");
                while (isalnum(source[i])) i++; // Skip the rest of the identifier
                continue;
            }
            token_type token = getReservedWordToken(buffer);
            addLexeme(state, buffer, token, NULL);
            addToken(state, token, buffer);
            continue;
        }

        // Handle numbers
        if (isdigit(source[i])) {
            char buffer[6] = {0};
            int j = 0;
            while (isdigit(source[i]) && j < MAX_NUMBER_LENGTH) {
                buffer[j++] = source[i++];
            }
            buffer[j] = '\0';
            if (isdigit(source[i])) {
                addLexeme(state, buffer, error_token, "Error: Number too long");
                while (isdigit(source[i])) i++;
                continue;
            }
            addLexeme(state, buffer, numbersym, NULL);
            addToken(state, numbersym, buffer);
            continue;
        }

        // Handle complex token sequences and special symbols
        char buffer[3] = {0};
        buffer[0] = source[i++];
        if ((buffer[0] == ':' && source[i] == '=') || (buffer[0] == '<' && source[i] == '>') ||
            (buffer[0] == '<' && source[i] == '=') || (buffer[0] == '>' && source[i] == '=')) {
            buffer[1] = source[i++];
        }
        token_type token = getSpecialSymbolToken(buffer);
        if (token == nulsym) {
            // Handle specific cases for two-character symbols
            if (buffer[0] == '<' && buffer[1] == '>') {
                token = neqsym;
            } else if (buffer[0] == '<' && buffer[1] == '=') {
                token = leqsym;
            } else if (buffer[0] == '>' && buffer[1] == '=') {
                token = geqsym;
            } else if (buffer[0] == ':' && buffer[1] == '=') {
                token = becomessym;
            } else {
                addLexeme(state, buffer, error_token, "Error: Invalid symbol");
                continue;
            }
        }
        addLexeme(state, buffer, token, NULL);
        addToken(state, token, buffer);
    }
}

// Print lexical analysis output
void printLexicalOutput(const CompilerState *state, FILE *outputFile) {
    fprintf(outputFile, "Lexeme Table:\n");
    fprintf(outputFile, "\nlexeme          token type\n");
    for (int i = 0; i < state->lexemeCount; i++) {
        fprintf(outputFile, "%-15s %-5d\n", state->lexemes[i].lexeme, state->lexemes[i].token);
    }

    fprintf(outputFile, "\nToken List:\n");
    for (int i = 0; i < state->tokenCount; i++) {
        fprintf(outputFile, "%d", state->tokens[i].token);
        if (state->tokens[i].token == identsym || state->tokens[i].token == numbersym) {
            fprintf(outputFile, " %s", state->tokens[i].value);
        }
        if (i < state->tokenCount - 1) {
            fprintf(outputFile, " ");
        }
    }
    fprintf(outputFile, "\n");
}

// Print error messages
void print_error(int error_number, const char *message, const char *identifier, FILE *errorFile) {
    if (identifier != NULL) {
        fprintf(errorFile, "***** Error number %d: %s %s\n", error_number, message, identifier);
    } else {
        fprintf(errorFile, "***** Error number %d: %s\n", error_number, message);
    }
    exit(1);
}

// Recursive Descent Parser and Intermediate Code Generator

int parseProgram(CompilerState *state, FILE *errorFile) {
    emit(state, 7, 0, 3); // JMP to main block
    block(state, errorFile, 0);
    if (state->tokens[state->current_token].token != periodsym) {
        print_error(9, "Period expected", NULL, errorFile);
    }
    emit(state, 9, 0, 3); // HALT

    return 1; // No error
}

void block(CompilerState *state, FILE *errorFile, int level) {
    int tx0 = state->symbol_table_index; // Save current table index
    int dx = 3; // Data allocation index

    int jmp_placeholder = state->code_index;
    emit(state, 7, 0, 0); // JMP to main block, address to be filled later

    do {
        if (state->tokens[state->current_token].token == constsym) {
            const_declaration(state, errorFile, level);
        }
        if (state->tokens[state->current_token].token == varsym) {
            dx += var_declaration(state, errorFile, level);
        }
        while (state->tokens[state->current_token].token == procsym) {
            state->current_token++;
            if (state->tokens[state->current_token].token == identsym) {
                add_to_symbol_table(state, 3, state->tokens[state->current_token].value, 0, level, state->code_index);
                state->current_token++;
            } else {
                print_error(4, "const, var, procedure must be followed by identifier", NULL, errorFile);
            }
            if (state->tokens[state->current_token].token != semicolonsym) {
                print_error(5, "Semicolon or comma missing", NULL, errorFile);
            }
            state->current_token++;
            block(state, errorFile, level + 1);
            if (state->tokens[state->current_token].token != semicolonsym) {
                print_error(17, "Semicolon or end expected after block", NULL, errorFile);
            }
            state->current_token++;
        }
    } while (state->tokens[state->current_token].token == constsym || state->tokens[state->current_token].token == varsym || state->tokens[state->current_token].token == procsym);

    int inc_index = state->code_index;  // Store the index of the INC instruction
    state->code[jmp_placeholder].m = inc_index;  // Update JMP to main block with the correct address

    emit(state, 6, 0, dx); // INC

    statement(state, errorFile, level);
    emit(state, 2, 0, 0); // OPR, return

    mark_symbols(state, level);
}

void const_declaration(CompilerState *state, FILE *errorFile, int level) {
    if (state->tokens[state->current_token].token == constsym) {
        do {
            state->current_token++;
            if (state->tokens[state->current_token].token != identsym) {
                print_error(4, "const, var, procedure must be followed by identifier", NULL, errorFile);
            }
            char const_name[12];
            strcpy(const_name, state->tokens[state->current_token].value);

            if (symbol_table_check_declaration(state, const_name, level) != -1) {
                print_error(4, "symbol name has already been declared", const_name, errorFile);
            }

            state->current_token++;
            if (state->tokens[state->current_token].token != eqsym) {
                print_error(3, "Identifier must be followed by =", NULL, errorFile);
            }

            state->current_token++;
            if (state->tokens[state->current_token].token != numbersym) {
                print_error(2, "= must be followed by a number", NULL, errorFile);
            }
            int const_value = atoi(state->tokens[state->current_token].value);

            add_to_symbol_table(state, 1, const_name, const_value, level, 0);

            state->current_token++;
        } while (state->tokens[state->current_token].token == commasym);

        if (state->tokens[state->current_token].token != semicolonsym) {
            print_error(5, "Semicolon or comma missing", NULL, errorFile);
        }
        state->current_token++;
    }
}

int var_declaration(CompilerState *state, FILE *errorFile, int level) {
    int numVars = 0;
    if (state->tokens[state->current_token].token == varsym) {
        do {
            numVars++;
            state->current_token++;
            if (state->tokens[state->current_token].token != identsym) {
                print_error(4, "const, var, procedure must be followed by identifier", NULL, errorFile);
            }
            if (symbol_table_check_declaration(state, state->tokens[state->current_token].value, level) != -1) {
                print_error(4, "symbol name has already been declared", state->tokens[state->current_token].value, errorFile);
            }
            add_to_symbol_table(state, 2, state->tokens[state->current_token].value, 0, level, numVars + 2);
            state->current_token++;
        } while (state->tokens[state->current_token].token == commasym);

        if (state->tokens[state->current_token].token != semicolonsym) {
            print_error(5, "Semicolon or comma missing", NULL, errorFile);
        }
        state->current_token++;
    }
    return numVars;
}

void statement(CompilerState *state, FILE *errorFile, int level) {
    if (state->tokens[state->current_token].token == identsym) {
        int symIdx = symbol_table_check(state, state->tokens[state->current_token].value, level);
        if (symIdx == -1) {
            print_error(11, "Undeclared identifier", state->tokens[state->current_token].value, errorFile);
        }
        if (state->symbol_table[symIdx].kind != 2) {
            print_error(12, "Assignment to constant or procedure is not allowed", NULL, errorFile);
        }
        state->current_token++;
        if (state->tokens[state->current_token].token != becomessym) {
            print_error(13, "Assignment operator expected", NULL, errorFile);
        }
        state->current_token++;
        expression(state, errorFile, level);
        emit(state, 4, level - state->symbol_table[symIdx].level, state->symbol_table[symIdx].addr); // STO
    } else if (state->tokens[state->current_token].token == beginsym) {
        do {
            state->current_token++;
            statement(state, errorFile, level);
        } while (state->tokens[state->current_token].token == semicolonsym);
        if (state->tokens[state->current_token].token != endsym) {
            print_error(17, "Semicolon or end expected", NULL, errorFile);
        }
        state->current_token++;
    } else if (state->tokens[state->current_token].token == ifsym) {
        state->current_token++;
        condition(state, errorFile, level);
        int jpcIdx = state->code_index;
        emit(state, 8, 0, 0); // JPC
        if (state->tokens[state->current_token].token != thensym) {
            print_error(16, "then expected", NULL, errorFile);
        }
        state->current_token++;
        statement(state, errorFile, level);
        state->code[jpcIdx].m = state->code_index;
        if (state->tokens[state->current_token].token == elsesym) {
            int jmpIdx = state->code_index;
            emit(state, 7, 0, 0); // JMP
            state->current_token++;
            statement(state, errorFile, level);
            state->code[jmpIdx].m = state->code_index;
        }
        if (state->tokens[state->current_token].token != fisym) {
            print_error(19, "Incorrect symbol following statement", NULL, errorFile);
        }
        state->current_token++;
    } else if (state->tokens[state->current_token].token == whilesym) {
        state->current_token++;
        int loopIdx = state->code_index;
        condition(state, errorFile, level);
        int jpcIdx = state->code_index;
        emit(state, 8, 0, 0); // JPC
        if (state->tokens[state->current_token].token != dosym) {
            print_error(18, "do expected", NULL, errorFile);
        }
        state->current_token++;
        statement(state, errorFile, level);
        emit(state, 7, 0, loopIdx); // JMP
        state->code[jpcIdx].m = state->code_index;
    } else if (state->tokens[state->current_token].token == readsym) {
        state->current_token++;
        if (state->tokens[state->current_token].token != identsym) {
            print_error(14, "call must be followed by an identifier", NULL, errorFile);
        }
        int symIdx = symbol_table_check(state, state->tokens[state->current_token].value, level);
        if (symIdx == -1) {
            print_error(11, "Undeclared identifier", state->tokens[state->current_token].value, errorFile);
        }
        if (state->symbol_table[symIdx].kind != 2) {
            print_error(15, "Call of a constant or variable is meaningless", NULL, errorFile);
        }
        state->current_token++;
        emit(state, 9, 0, 2); // READ input
        emit(state, 4, level - state->symbol_table[symIdx].level, state->symbol_table[symIdx].addr); // STO
    } else if (state->tokens[state->current_token].token == writesym) {
        state->current_token++;
        expression(state, errorFile, level);
        emit(state, 9, 0, 1); // WRITE output
    } else if (state->tokens[state->current_token].token == callsym) {
        state->current_token++;
        if (state->tokens[state->current_token].token != identsym) {
            print_error(14, "call must be followed by an identifier", NULL, errorFile);
        }
        int symIdx = symbol_table_check(state, state->tokens[state->current_token].value, level);
        if (symIdx == -1) {
            print_error(11, "Undeclared identifier", state->tokens[state->current_token].value, errorFile);
        }
        if (state->symbol_table[symIdx].kind != 3) {
            print_error(15, "Call of a constant or variable is meaningless", state->tokens[state->current_token].value, errorFile);
        }
        state->current_token++;
        emit(state, 5, level - state->symbol_table[symIdx].level, state->symbol_table[symIdx].addr); // CAL
    }
}

void condition(CompilerState *state, FILE *errorFile, int level) {
    if (state->tokens[state->current_token].token == oddsym) {
        state->current_token++;
        expression(state, errorFile, level);
        emit(state, 2, 0, 11); // ODD
    } else {
        expression(state, errorFile, level);
        if (state->tokens[state->current_token].token == eqsym) {
            state->current_token++;
            expression(state, errorFile, level);
            emit(state, 2, 0, 5); // EQL
        } else if (state->tokens[state->current_token].token == neqsym) {
            state->current_token++;
            expression(state, errorFile, level);
            emit(state, 2, 0, 6); // NEQ
        } else if (state->tokens[state->current_token].token == lessym) {
            state->current_token++;
            expression(state, errorFile, level);
            emit(state, 2, 0, 7); // LSS
        } else if (state->tokens[state->current_token].token == leqsym) {
            state->current_token++;
            expression(state, errorFile, level);
            emit(state, 2, 0, 8); // LEQ
        } else if (state->tokens[state->current_token].token == gtrsym) {
            state->current_token++;
            expression(state, errorFile, level);
            emit(state, 2, 0, 9); // GTR
        } else if (state->tokens[state->current_token].token == geqsym) {
            state->current_token++;
            expression(state, errorFile, level);
            emit(state, 2, 0, 10); // GEQ
        } else {
            print_error(20, "Relational operator expected", NULL, errorFile);
        }
    }
}

void expression(CompilerState *state, FILE *errorFile, int level) {
    int addop;
    if (state->tokens[state->current_token].token == plussym || state->tokens[state->current_token].token == minussym) {
        addop = state->tokens[state->current_token].token;
        state->current_token++;
        term(state, errorFile, level);
        if (addop == minussym) {
            emit(state, 2, 0, 2); // SUB
        }
    } else {
        term(state, errorFile, level);
    }
    while (state->tokens[state->current_token].token == plussym || state->tokens[state->current_token].token == minussym) {
        addop = state->tokens[state->current_token].token;
        state->current_token++;
        term(state, errorFile, level);
        if (addop == plussym) {
            emit(state, 2, 0, 1); // ADD
        } else {
            emit(state, 2, 0, 2); // SUB
        }
    }
}

void term(CompilerState *state, FILE *errorFile, int level) {
    factor(state, errorFile, level);
    while (state->tokens[state->current_token].token == multsym || state->tokens[state->current_token].token == slashsym) {
        int mulop = state->tokens[state->current_token].token;
        state->current_token++;
        factor(state, errorFile, level);
        if (mulop == multsym) {
            emit(state, 2, 0, 3); // MUL
        } else {
            emit(state, 2, 0, 4); // DIV
        }
    }
}

void factor(CompilerState *state, FILE *errorFile, int level) {
    if (state->tokens[state->current_token].token == identsym) {
        int symIdx = symbol_table_check(state, state->tokens[state->current_token].value, level);
        if (symIdx == -1) {
            print_error(11, "Undeclared identifier", state->tokens[state->current_token].value, errorFile);
        }
        if (state->symbol_table[symIdx].kind == 3) {
            print_error(21, "Expression must not contain a procedure identifier", state->tokens[state->current_token].value, errorFile);
        }
        state->current_token++;
        if (state->symbol_table[symIdx].kind == 1) { // const
            emit(state, 1, 0, state->symbol_table[symIdx].val); // LIT
        } else { // var
            emit(state, 3, level - state->symbol_table[symIdx].level, state->symbol_table[symIdx].addr); // LOD
        }
    } else if (state->tokens[state->current_token].token == numbersym) {
        emit(state, 1, 0, atoi(state->tokens[state->current_token].value)); // LIT
        state->current_token++;
    } else if (state->tokens[state->current_token].token == lparentsym) {
        state->current_token++;
        expression(state, errorFile, level);
        if (state->tokens[state->current_token].token != rparentsym) {
            print_error(22, "Right parenthesis missing", NULL, errorFile);
        }
        state->current_token++;
    } else {
        print_error(23, "The preceding factor cannot begin with this symbol", NULL, errorFile);
    }
}

// Check symbol table for existing symbol
int symbol_table_check(CompilerState *state, char *name, int level) {
    for (int i = state->symbol_table_index - 1; i >= 0; i--) {
        if (strcmp(state->symbol_table[i].name, name) == 0 && state->symbol_table[i].level <= level && state->symbol_table[i].mark == 0) {
            return i;
        }
    }
    return -1;
}

// Check if symbol is already declared in the table
int symbol_table_check_declaration(CompilerState *state, char *name, int level) {
    for (int i = state->symbol_table_index - 1; i >= 0; i--) {
        if (strcmp(state->symbol_table[i].name, name) == 0 && state->symbol_table[i].level == level) {
            return i;
        }
    }
    return -1;
}

// Add a symbol to the table
void add_to_symbol_table(CompilerState *state, int kind, char *name, int val, int level, int addr) {
    state->symbol_table[state->symbol_table_index].kind = kind;
    strncpy(state->symbol_table[state->symbol_table_index].name, name, sizeof(state->symbol_table[state->symbol_table_index].name) - 1);
    state->symbol_table[state->symbol_table_index].name[sizeof(state->symbol_table[state->symbol_table_index].name) - 1] = '\0';
    state->symbol_table[state->symbol_table_index].val = val;
    state->symbol_table[state->symbol_table_index].level = level;
    state->symbol_table[state->symbol_table_index].addr = addr;
    state->symbol_table[state->symbol_table_index].mark = 0;
    state->symbol_table_index++;
}

// Mark symbols in the table
void mark_symbols(CompilerState *state, int level) {
    for (int i = state->symbol_table_index - 1; i >= 0; i--) {
        if (state->symbol_table[i].level == level && state->symbol_table[i].mark == 0) {
            state->symbol_table[i].mark = 1;
        }
    }
}

// Emit a code instruction
void emit(CompilerState *state, int op, int l, int m) {
    if (state->code_index >= MAX_CODE_LENGTH) {
        fprintf(stderr, "Error: Code segment overflow.\n");
        exit(1);
    }
    state->code[state->code_index].op = op;
    state->code[state->code_index].l = l;
    state->code[state->code_index].m = m;
    state->code_index++;
}

// Print generated code
void printCode(const CompilerState *state, FILE *outputFile) {
    fprintf(outputFile, "Generated Code:\n");
    fprintf(outputFile, "Line    OP    L    M\n");
    for (int i = 0; i < state->code_index; i++) {
        char *op_name;
        switch (state->code[i].op) {
            case 1: op_name = "LIT"; break;
            case 2: op_name = "OPR"; break;
            case 3: op_name = "LOD"; break;
            case 4: op_name = "STO"; break;
            case 5: op_name = "CAL"; break;
            case 6: op_name = "INC"; break;
            case 7: op_name = "JMP"; break;
            case 8: op_name = "JPC"; break;
            case 9: op_name = "SYS"; break;
            default: op_name = "???"; break;
        }
        fprintf(outputFile, "%3d %6s %4d %4d\n", i, op_name, state->code[i].l, state->code[i].m);
    }
}

// Output ELF file
void outputElfFile(const CompilerState *state) {
    FILE *elfFile = fopen("elf.txt", "w");
    if (!elfFile) {
        perror("Error opening elf.txt file");
        exit(1);
    }

    for (int i = 0; i < state->code_index; i++) {
        fprintf(elfFile, "%d %d %d\n", state->code[i].op, state->code[i].l, state->code[i].m);
    }

    fclose(elfFile);
}

// Print symbol table
void printsymbols(const CompilerState *state) {
    printf("\nSymbol Table:\n\n");
    printf("Kind | Name        | Value | Level | Address | Mark\n");
    printf("---------------------------------------------------\n");

    for (int h = 0; h < state->symbol_table_index; h++) {
        printf("%4d | %11s | %5d | %5d | %7d | %5d\n",
            state->symbol_table[h].kind,
            state->symbol_table[h].name,
            state->symbol_table[h].val,
            state->symbol_table[h].level,
            state->symbol_table[h].addr,
            state->symbol_table[h].mark);
    }
}