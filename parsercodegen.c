#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define MAX_IDENTIFIER_LENGTH 11
#define MAX_NUMBER_LENGTH 5
#define MAX_BUFFER_LENGTH 1000
#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_CODE_LENGTH MAX_SYMBOL_TABLE_SIZE
#define MEMORY_OFFSET 10

typedef enum {
    ODDSYM = 1, IDENTSYM, NUMBERSYM, PLUSSYM, MINUSSYM, MULTSYM, SLASHSYM,
    EQSYM, NEQSYM, LESSYM, LEQSYM, GTRSYM, GEQSYM, LPARENTSYM, RPARENTSYM,
    COMMASYM, SEMICOLONSYM, PERIODSYM, BECOMESSYM, BEGINSYM, ENDSYM, IFSYM,
    THENSYM, WHILESYM, DOSYM, CONSTSYM, VARSYM, WRITESYM, READSYM, FISYM
} TokenType;

typedef struct {
    char lexeme[MAX_BUFFER_LENGTH + 1];
    char value[MAX_BUFFER_LENGTH + 1];
} Token;

typedef struct {
    Token *tokens;
    int size;
    int capacity;
} TokenList;

typedef struct {
    int kind; // const = 1, var = 2
    char name[MAX_IDENTIFIER_LENGTH + 1];
    int value;
    int level;
    int address;
    int mark;
} Symbol;

typedef struct {
    int opcode;
    int level;
    int modifier;
} Instruction;

FILE *inputFile;
FILE *outputFile;
Symbol symbolTable[MAX_SYMBOL_TABLE_SIZE];
Instruction code[MAX_CODE_LENGTH];
TokenList *tokenList;
int codeIndex = 0, symbolIndex = 0, currentLevel = 0;
Token currentToken;

char peekChar();
void printBoth(const char *format, ...);
void clearBuffer(char *str, int index);
int handleReservedWord(char *buffer);
int handleSpecialSymbol(char *buffer);
int isSpecialSymbol(char c);
TokenList *createTokenList();
void destroyTokenList(TokenList *list);
void appendToken(TokenList *list, Token token);
void getNextToken();
void emit(int opcode, int level, int modifier);
void error(int errorCode, const char *message);
int findSymbol(char *name);
void addSymbol(int kind, char *name, int value, int level, int address);
void parseProgram();
void parseBlock();
void parseConstDeclaration();
int parseVarDeclaration();
void parseStatement();
void parseCondition();
void parseExpression();
void parseTerm();
void parseFactor();
void printSymbolTable();
void printInstructions();
void getOpName(int opcode, char *name);
void generateElfFile();

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printBoth("Usage: %s <input file> <output file>\n", argv[0]);
        return 1;
    }

    inputFile = fopen(argv[1], "r");
    outputFile = fopen(argv[2], "w");

    if (inputFile == NULL) {
        printBoth("Error: Could not open input file %s\n", argv[1]);
        exit(1);
    }

    if (outputFile == NULL) {
        printBoth("Error: Could not open output file %s\n", argv[2]);
        exit(1);
    }

    tokenList = createTokenList();

    char c;
    char buffer[MAX_BUFFER_LENGTH + 1] = {0};
    int bufferIndex = 0;

    while ((c = fgetc(inputFile)) != EOF) {
        if (iscntrl(c) || isspace(c)) continue;
        if (isdigit(c)) {
            buffer[bufferIndex++] = c;
            while (1) {
                char nextChar = peekChar();
                if (isspace(nextChar) || isSpecialSymbol(nextChar)) {
                    Token token;
                    if (bufferIndex > MAX_NUMBER_LENGTH) {
                        error(1, "Number too long");
                    } else {
                        sprintf(token.value, "%d", NUMBERSYM);
                        strcpy(token.lexeme, buffer);
                        appendToken(tokenList, token);
                    }
                    clearBuffer(buffer, bufferIndex);
                    bufferIndex = 0;
                    break;
                } else if (isdigit(nextChar)) {
                    c = getc(inputFile);
                    buffer[bufferIndex++] = c;
                } else if (nextChar == EOF) break;
                else if (isalpha(nextChar)) {
                    Token token;
                    sprintf(token.value, "%d", NUMBERSYM);
                    strcpy(token.lexeme, buffer);
                    appendToken(tokenList, token);
                    clearBuffer(buffer, bufferIndex);
                    bufferIndex = 0;
                    break;
                }
            }
        } else if (isalpha(c)) {
            buffer[bufferIndex++] = c;
            while (1) {
                char nextChar = peekChar();
                if (isspace(nextChar) || isSpecialSymbol(nextChar) || nextChar == EOF) {
                    int tokenValue = handleReservedWord(buffer);
                    if (tokenValue) {
                        Token token;
                        sprintf(token.value, "%d", tokenValue);
                        strcpy(token.lexeme, buffer);
                        appendToken(tokenList, token);
                        clearBuffer(buffer, bufferIndex);
                        bufferIndex = 0;
                        break;
                    } else {
                        Token token;
                        if (bufferIndex > MAX_IDENTIFIER_LENGTH) {
                            error(2, "Identifier too long");
                        } else {
                            sprintf(token.value, "%d", IDENTSYM);
                            strcpy(token.lexeme, buffer);
                            appendToken(tokenList, token);
                        }
                        clearBuffer(buffer, bufferIndex);
                        bufferIndex = 0;
                        break;
                    }
                } else if (isalnum(nextChar)) {
                    c = getc(inputFile);
                    buffer[bufferIndex++] = c;
                }
            }
        } else if (isSpecialSymbol(c)) {
            buffer[bufferIndex++] = c;
            char nextChar = peekChar();

            if (isSpecialSymbol(nextChar)) {
                if (c == '/' && nextChar == '*') {
                    clearBuffer(buffer, bufferIndex);
                    bufferIndex = 0;
                    while (1) {
                        c = getc(inputFile);
                        nextChar = peekChar();
                        if (c == '*' && nextChar == '/') {
                            c = getc(inputFile);
                            break;
                        }
                    }
                    continue;
                }

                if (c == '/' && nextChar == '/') {
                    clearBuffer(buffer, bufferIndex);
                    bufferIndex = 0;
                    while (1) {
                        c = getc(inputFile);
                        nextChar = peekChar();
                        if (c == '\n') break;
                    }
                    continue;
                }

                if (nextChar == ';') {
                    Token token;
                    int tokenValue = handleSpecialSymbol(buffer);
                    if (!tokenValue) error(3, "Invalid symbol");

                    sprintf(token.value, "%d", tokenValue);
                    strcpy(token.lexeme, buffer);
                    appendToken(tokenList, token);

                    sprintf(token.value, "%d", SEMICOLONSYM);
                    strcpy(token.lexeme, ";");
                    appendToken(tokenList, token);

                    clearBuffer(buffer, bufferIndex);
                    bufferIndex = 0;

                    continue;
                }

                c = getc(inputFile);
                buffer[bufferIndex++] = c;

                Token token;
                int tokenValue = handleSpecialSymbol(buffer);
                if (!tokenValue) error(3, "Invalid symbol");
                else {
                    sprintf(token.value, "%d", tokenValue);
                    strcpy(token.lexeme, buffer);
                    appendToken(tokenList, token);
                }

                clearBuffer(buffer, bufferIndex);
                bufferIndex = 0;
            } else {
                Token token;
                int tokenValue = handleSpecialSymbol(buffer);
                if (!tokenValue) error(3, "Invalid symbol");
                else {
                    sprintf(token.value, "%d", tokenValue);
                    strcpy(token.lexeme, buffer);
                    appendToken(tokenList, token);
                }

                clearBuffer(buffer, bufferIndex);
                bufferIndex = 0;
            }
        }
    }

    code[0].opcode = 7;
    code[0].level = 0;
    code[0].modifier = 3 + MEMORY_OFFSET;
    codeIndex++;

    parseProgram();

    printSymbolTable();
    printInstructions();
    generateElfFile();

    destroyTokenList(tokenList);
    fclose(inputFile);
    fclose(outputFile);
    return 0;
}

char peekChar() {
    int c = getc(inputFile);
    ungetc(c, inputFile);
    return (char)c;
}

void printBoth(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    va_start(args, format);
    vfprintf(outputFile, format, args);
    va_end(args);
}

void clearBuffer(char *str, int index) {
    for (int i = 0; i < index; i++) str[i] = '\0';
}

int handleReservedWord(char *buffer) {
    if (strcmp(buffer, "const") == 0) return CONSTSYM;
    else if (strcmp(buffer, "var") == 0) return VARSYM;
    else if (strcmp(buffer, "begin") == 0) return BEGINSYM;
    else if (strcmp(buffer, "end") == 0) return ENDSYM;
    else if (strcmp(buffer, "if") == 0) return IFSYM;
    else if (strcmp(buffer, "then") == 0) return THENSYM;
    else if (strcmp(buffer, "while") == 0) return WHILESYM;
    else if (strcmp(buffer, "do") == 0) return DOSYM;
    else if (strcmp(buffer, "read") == 0) return READSYM;
    else if (strcmp(buffer, "write") == 0) return WRITESYM;
    else if (strcmp(buffer, "fi") == 0) return FISYM;
    return 0;
}

int handleSpecialSymbol(char *buffer) {
    if (strcmp(buffer, "+") == 0) return PLUSSYM;
    else if (strcmp(buffer, "-") == 0) return MINUSSYM;
    else if (strcmp(buffer, "*") == 0) return MULTSYM;
    else if (strcmp(buffer, "/") == 0) return SLASHSYM;
    else if (strcmp(buffer, "(") == 0) return LPARENTSYM;
    else if (strcmp(buffer, ")") == 0) return RPARENTSYM;
    else if (strcmp(buffer, ",") == 0) return COMMASYM;
    else if (strcmp(buffer, ";") == 0) return SEMICOLONSYM;
    else if (strcmp(buffer, ".") == 0) return PERIODSYM;
    else if (strcmp(buffer, "=") == 0) return EQSYM;
    else if (strcmp(buffer, "<") == 0) return LESSYM;
    else if (strcmp(buffer, ">") == 0) return GTRSYM;
    else if (strcmp(buffer, ":=") == 0) return BECOMESSYM;
    else if (strcmp(buffer, "<=") == 0) return LEQSYM;
    else if (strcmp(buffer, ">=") == 0) return GEQSYM;
    else if (strcmp(buffer, "<>") == 0) return NEQSYM;
    return 0;
}

int isSpecialSymbol(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '(' || c == ')' ||
            c == '=' || c == ',' || c == '.' || c == '<' || c == '>' || c == ':' ||
            c == ';');
}

TokenList *createTokenList() {
    TokenList *list = malloc(sizeof(TokenList));
    list->size = 0;
    list->capacity = 10;
    list->tokens = malloc(sizeof(Token) * list->capacity);
    return list;
}

void destroyTokenList(TokenList *list) {
    free(list->tokens);
    free(list);
}

void appendToken(TokenList *list, Token token) {
    if (list->size == list->capacity) {
        list->capacity *= 2;
        list->tokens = realloc(list->tokens, sizeof(Token) * list->capacity);
    }
    list->tokens[list->size++] = token;
}

void getNextToken() {
    currentToken = tokenList->tokens[0];
    for (int i = 0; i < tokenList->size - 1; i++) {
        tokenList->tokens[i] = tokenList->tokens[i + 1];
    }
    tokenList->size--;
}

void emit(int opcode, int level, int modifier) {
    if (codeIndex > MAX_CODE_LENGTH) {
        error(16, "Program too long");
    } else {
        code[codeIndex].opcode = opcode;
        code[codeIndex].level = level;
        code[codeIndex].modifier = modifier;
        codeIndex++;
    }
}

void error(int errorCode, const char *message) {
    printBoth("Error: %s\n", message);
    exit(1);
}

int findSymbol(char *name) {
    for (int i = 0; i < symbolIndex; i++) {
        if (strcmp(name, symbolTable[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

void addSymbol(int kind, char *name, int value, int level, int address) {
    symbolTable[symbolIndex].kind = kind;
    strcpy(symbolTable[symbolIndex].name, name);
    symbolTable[symbolIndex].value = value;
    symbolTable[symbolIndex].level = level;
    symbolTable[symbolIndex].address = address;
    symbolTable[symbolIndex].mark = 0;
    symbolIndex++;
}

void parseProgram() {
    getNextToken();
    parseBlock();
    if (strtol(currentToken.value, NULL, 10) != PERIODSYM) {
        error(1, "Program must end with period");
    }
    emit(9, 0, 3);
}

void parseBlock() {
    parseConstDeclaration();
    int numVars = parseVarDeclaration();
    emit(6, 0, 3 + numVars);
    parseStatement();
}

void parseConstDeclaration() {
    char name[MAX_IDENTIFIER_LENGTH + 1];
    if (strtol(currentToken.value, NULL, 10) == CONSTSYM) {
        do {
            getNextToken();
            if (strtol(currentToken.value, NULL, 10) != IDENTSYM) {
                error(2, "const, var, and read keywords must be followed by identifier");
            }
            strcpy(name, currentToken.lexeme);
            if (findSymbol(currentToken.lexeme) != -1) {
                error(3, "Symbol name has already been declared");
            }
            getNextToken();
            if (strtol(currentToken.value, NULL, 10) != EQSYM) {
                error(4, "Constants must be assigned with =");
            }
            getNextToken();
            if (strtol(currentToken.value, NULL, 10) != NUMBERSYM) {
                error(5, "Constants must be assigned an integer value");
            }
            addSymbol(1, name, strtol(currentToken.lexeme, NULL, 10), currentLevel, 0);
            getNextToken();
        } while (strtol(currentToken.value, NULL, 10) == COMMASYM);
        if (strtol(currentToken.value, NULL, 10) != SEMICOLONSYM) {
            error(6, "Constant and variable declarations must be followed by a semicolon");
        }
        getNextToken();
    }
}

int parseVarDeclaration() {
    int numVars = 0;
    if (strtol(currentToken.value, NULL, 10) == VARSYM) {
        do {
            numVars++;
            getNextToken();
            if (strtol(currentToken.value, NULL, 10) != IDENTSYM) {
                error(2, "const, var, and read keywords must be followed by identifier");
            }
            if (findSymbol(currentToken.lexeme) != -1) {
                error(3, "Symbol name has already been declared");
            }
            addSymbol(2, currentToken.lexeme, 0, 0, numVars + 2);
            getNextToken();
        } while (strtol(currentToken.value, NULL, 10) == COMMASYM);
        if (strtol(currentToken.value, NULL, 10) != SEMICOLONSYM) {
            error(6, "Constant and variable declarations must be followed by a semicolon");
        }
        getNextToken();
    }
    return numVars;
}

void parseStatement() {
    if (strtol(currentToken.value, NULL, 10) == IDENTSYM) {
        int symbolIndex = findSymbol(currentToken.lexeme);
        if (symbolIndex == -1) {
            error(7, "Undeclared identifier");
        }
        if (symbolTable[symbolIndex].kind != 2) {
            error(8, "Only variable values may be altered");
        }
        getNextToken();
        if (strtol(currentToken.value, NULL, 10) != BECOMESSYM) {
            error(9, "Assignment statements must use :=");
        }
        getNextToken();
        parseExpression();
        emit(4, 0, symbolTable[symbolIndex].address);
    } else if (strtol(currentToken.value, NULL, 10) == BEGINSYM) {
        do {
            getNextToken();
            parseStatement();
        } while (strtol(currentToken.value, NULL, 10) == SEMICOLONSYM);
        if (strtol(currentToken.value, NULL, 10) != ENDSYM) {
            error(10, "begin must be followed by end");
        }
        getNextToken();
    } else if (strtol(currentToken.value, NULL, 10) == IFSYM) {
        getNextToken();
        parseCondition();
        int jumpIndex = codeIndex;
        emit(8, 0, 0);
        if (strtol(currentToken.value, NULL, 10) != THENSYM) {
            error(11, "if must be followed by then");
        }
        getNextToken();
        parseStatement();
        if (strtol(currentToken.value, NULL, 10) != FISYM) {
            error(15, "then must be followed by fi");
        }
        getNextToken();
        code[jumpIndex].modifier = codeIndex;
    } else if (strtol(currentToken.value, NULL, 10) == WHILESYM) {
        getNextToken();
        int loopIndex = codeIndex;
        parseCondition();
        if (strtol(currentToken.value, NULL, 10) != DOSYM) {
            error(12, "while must be followed by do");
        }
        getNextToken();
        int jumpIndex = codeIndex;
        emit(8, 0, 0);
        parseStatement();
        emit(7, 0, loopIndex);
        code[jumpIndex].modifier = codeIndex;
    } else if (strtol(currentToken.value, NULL, 10) == READSYM) {
        getNextToken();
        if (strtol(currentToken.value, NULL, 10) != IDENTSYM) {
            error(2, "const, var, and read keywords must be followed by identifier");
        }
        int symbolIndex = findSymbol(currentToken.lexeme);
        if (symbolIndex == -1) {
            error(7, "Undeclared identifier");
        }
        if (symbolTable[symbolIndex].kind != 2) {
            error(8, "Only variable values may be altered");
        }
        getNextToken();
        emit(9, 0, 2);
        emit(4, 0, symbolTable[symbolIndex].address);
    } else if (strtol(currentToken.value, NULL, 10) == WRITESYM) {
        getNextToken();
        parseExpression();
        emit(9, 0, 1);
    }
}

void parseCondition() {
    if (strtol(currentToken.value, NULL, 10) == ODDSYM) {
        getNextToken();
        parseExpression();
        emit(2, 0, 11);
    } else {
        parseExpression();
        switch (strtol(currentToken.value, NULL, 10)) {
            case EQSYM:
                getNextToken();
                parseExpression();
                emit(2, 0, 5);
                break;
            case NEQSYM:
                getNextToken();
                parseExpression();
                emit(2, 0, 6);
                break;
            case LESSYM:
                getNextToken();
                parseExpression();
                emit(2, 0, 7);
                break;
            case LEQSYM:
                getNextToken();
                parseExpression();
                emit(2, 0, 8);
                break;
            case GTRSYM:
                getNextToken();
                parseExpression();
                emit(2, 0, 9);
                break;
            case GEQSYM:
                getNextToken();
                parseExpression();
                emit(2, 0, 10);
                break;
            default:
                error(13, "Condition must contain comparison operator");
                break;
        }
    }
}

void parseExpression() {
    if (strtol(currentToken.value, NULL, 10) == MINUSSYM) {
        getNextToken();
        parseTerm();
        emit(2, 0, 1);
    } else {
        parseTerm();
    }

    while (strtol(currentToken.value, NULL, 10) == PLUSSYM || strtol(currentToken.value, NULL, 10) == MINUSSYM) {
        if (strtol(currentToken.value, NULL, 10) == PLUSSYM) {
            getNextToken();
            parseTerm();
            emit(2, 0, 1);
        } else {
            getNextToken();
            parseTerm();
            emit(2, 0, 2);
        }
    }
}

void parseTerm() {
    parseFactor();
    while (strtol(currentToken.value, NULL, 10) == MULTSYM || strtol(currentToken.value, NULL, 10) == SLASHSYM) {
        if (strtol(currentToken.value, NULL, 10) == MULTSYM) {
            getNextToken();
            parseFactor();
            emit(2, 0, 3);
        } else {
            getNextToken();
            parseFactor();
            emit(2, 0, 4);
        }
    }
}

void parseFactor() {
    if (strtol(currentToken.value, NULL, 10) == IDENTSYM) {
        int symbolIndex = findSymbol(currentToken.lexeme);
        if (symbolIndex == -1) {
            error(7, "Undeclared identifier");
        }
        if (symbolTable[symbolIndex].kind == 1) {
            emit(1, 0, symbolTable[symbolIndex].value);
        } else {
            emit(3, currentLevel - symbolTable[symbolIndex].level, symbolTable[symbolIndex].address);
        }
        getNextToken();
    } else if (strtol(currentToken.value, NULL, 10) == NUMBERSYM) {
        emit(1, 0, strtol(currentToken.lexeme, NULL, 10));
        getNextToken();
    } else if (strtol(currentToken.value, NULL, 10) == LPARENTSYM) {
        getNextToken();
        parseExpression();
        if (strtol(currentToken.value, NULL, 10) != RPARENTSYM) {
            error(14, "Right parenthesis must follow left parenthesis");
        }
        getNextToken();
    } else {
        error(15, "Arithmetic expressions must contain operands, parentheses, numbers, or symbols");
    }
}

void printSymbolTable() {
    printBoth("\nSymbol Table:\n");
    printBoth("%10s | %10s | %10s | %10s | %10s | %10s\n", "Kind", "Name", "Value", "Level", "Address", "Mark");
    printBoth("    -----------------------------------------------------------------------\n");

    for (int i = 0; i < symbolIndex; i++) {
        symbolTable[i].mark = 1;
        if (symbolTable[i].kind == 1)
            printBoth("%10d | %10s | %10d | %10s | %10s | %10d\n", symbolTable[i].kind, symbolTable[i].name, symbolTable[i].value, "-", "-", symbolTable[i].mark);
        else
            printBoth("%10d | %10s | %10d | %10d | %10d | %10d\n", symbolTable[i].kind, symbolTable[i].name, symbolTable[i].value, symbolTable[i].level, symbolTable[i].address, symbolTable[i].mark);
    }
}

void printInstructions() {
    printBoth("Assembly Code:\n");
    printBoth("%10s %10s %10s %10s\n", "Line", "OP", "L", "M");
    for (int i = 0; i < codeIndex; i++) {
        char name[4];
        getOpName(code[i].opcode, name);
        printBoth("%10d %10s %10d %10d\n", i, name, code[i].level, code[i].modifier);
    }
}

void getOpName(int opcode, char *name) {
    switch (opcode) {
        case 1: strcpy(name, "LIT"); break;
        case 2: strcpy(name, "OPR"); break;
        case 3: strcpy(name, "LOD"); break;
        case 4: strcpy(name, "STO"); break;
        case 6: strcpy(name, "INC"); break;
        case 7: strcpy(name, "JMP"); break;
        case 8: strcpy(name, "JPC"); break;
        case 9: strcpy(name, "SYS"); break;
    }
}

void generateElfFile() {
    FILE *elfFile = fopen("output.elf", "w");
    if (elfFile == NULL) {
        printBoth("Error: Could not create ELF file\n");
        return;
    }

    for (int i = 0; i < codeIndex; i++) {
        fprintf(elfFile, "%d %d %d\n", code[i].opcode, code[i].level, code[i].modifier);
    }

    fclose(elfFile);
}
