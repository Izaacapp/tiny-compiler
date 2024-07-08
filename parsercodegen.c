#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

// Constants and Data Structures
#define MAX_ID_LENGTH 11
#define MAX_NUM_LENGTH 5
#define MAX_BUFFER_SIZE 1000
#define SYMBOL_TABLE_SIZE 500
#define CODE_ARRAY_SIZE SYMBOL_TABLE_SIZE
#define INIT_MEMORY_OFFSET 10

typedef enum {
    ODD_SYM = 1, IDENT_SYM, NUM_SYM, PLUS_SYM, MINUS_SYM, MULT_SYM, DIV_SYM,
    EQUAL_SYM, NOT_EQUAL_SYM, LESS_SYM, LESS_EQ_SYM, GREATER_SYM, GREATER_EQ_SYM,
    LEFT_PAREN_SYM, RIGHT_PAREN_SYM, COMMA_SYM, SEMICOLON_SYM, PERIOD_SYM,
    ASSIGN_SYM, BEGIN_SYM, END_SYM, IF_SYM, THEN_SYM, WHILE_SYM, DO_SYM,
    CONST_SYM, VAR_SYM, WRITE_SYM, READ_SYM, FI_SYM
} TokenType;

typedef struct {
    char lexeme[MAX_BUFFER_SIZE + 1];
    char value[MAX_BUFFER_SIZE + 1];
} Token;

typedef struct {
    Token *tokens;
    int count;
    int capacity;
} TokenList;

typedef struct {
    int type; // const = 1, var = 2
    char name[10];
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

// Global Variables
FILE *inputFile;
FILE *outputFile;
Symbol symbolTable[SYMBOL_TABLE_SIZE];
Instruction codeArray[CODE_ARRAY_SIZE];
TokenList *tokenList;
int currentCodeIndex = 0, currentSymbolIndex = 0, currentLevel = 0;
Token currentToken;

// Function Prototypes
char peekChar();
void formattedPrint(const char *format, ...);
void clearBuffer(char *str, int length);
int checkReservedWord(char *buffer);
int checkSpecialSymbol(char *buffer);
int isSpecialCharacter(char c);
TokenList *createTokenList();
void destroyTokenList(TokenList *list);
TokenList *appendToken(TokenList *list, Token token);
void fetchNextToken();
void generateCode(int opcode, int level, int modifier);
void throwError(int errorCode, const char *message);
int searchSymbolTable(char *name);
void insertSymbol(int type, char *name, int value, int level, int address);
void parseProgram();
void parseBlock();
void parseConstDeclaration();
int parseVarDeclaration();
void parseStatement();
void parseCondition();
void parseExpression();
void parseTerm();
void parseFactor();
void displaySymbolTable();
void displayCode();
void fetchOpcodeName(int opcode, char *name);
void generateELFFile();

// Main Function
int main(int argc, char *argv[]) {
    if (argc != 3) {
        formattedPrint("Usage: %s <input file> <output file>\n", argv[0]);
        return 1;
    }

    inputFile = fopen(argv[1], "r");
    outputFile = fopen(argv[2], "w");

    if (inputFile == NULL) {
        formattedPrint("Error: Could not open input file %s\n", argv[1]);
        exit(1);
    }

    if (outputFile == NULL) {
        formattedPrint("Error: Could not open output file %s\n", argv[2]);
        exit(1);
    }

    tokenList = createTokenList();

    char c;
    char buffer[MAX_BUFFER_SIZE + 1] = {0};
    int bufferIndex = 0;

    // Tokenize the input
    while ((c = fgetc(inputFile)) != EOF) {
        if (isspace(c) || iscntrl(c)) continue;
        if (isdigit(c)) {
            buffer[bufferIndex++] = c;
            while (1) {
                char nextChar = peekChar();
                if (isspace(nextChar) || isSpecialCharacter(nextChar)) {
                    Token token;
                    if (bufferIndex > MAX_NUM_LENGTH) {
                        throwError(1, "Number exceeds maximum length");
                    } else {
                        sprintf(token.value, "%d", NUM_SYM);
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
                    sprintf(token.value, "%d", NUM_SYM);
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
                if (isspace(nextChar) || isSpecialCharacter(nextChar) || nextChar == EOF) {
                    int tokenValue = checkReservedWord(buffer);
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
                        if (bufferIndex > MAX_ID_LENGTH) {
                            throwError(2, "Identifier exceeds maximum length");
                        } else {
                            sprintf(token.value, "%d", IDENT_SYM);
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
        } else if (isSpecialCharacter(c)) {
            buffer[bufferIndex++] = c;
            char nextChar = peekChar();

            if (isSpecialCharacter(nextChar)) {
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
                    int tokenValue = checkSpecialSymbol(buffer);
                    if (!tokenValue) throwError(3, "Invalid symbol");

                    sprintf(token.value, "%d", tokenValue);
                    strcpy(token.lexeme, buffer);
                    appendToken(tokenList, token);

                    sprintf(token.value, "%d", SEMICOLON_SYM);
                    strcpy(token.lexeme, ";");
                    appendToken(tokenList, token);

                    clearBuffer(buffer, bufferIndex);
                    bufferIndex = 0;

                    continue;
                }

                c = getc(inputFile);
                buffer[bufferIndex++] = c;

                Token token;
                int tokenValue = checkSpecialSymbol(buffer);
                if (!tokenValue) throwError(3, "Invalid symbol");
                else {
                    sprintf(token.value, "%d", tokenValue);
                    strcpy(token.lexeme, buffer);
                    appendToken(tokenList, token);
                }

                clearBuffer(buffer, bufferIndex);
                bufferIndex = 0;
            } else {
                Token token;
                int tokenValue = checkSpecialSymbol(buffer);
                if (!tokenValue) throwError(3, "Invalid symbol");
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

    codeArray[0].opcode = 7;
    codeArray[0].level = 0;
    codeArray[0].modifier = 3 + INIT_MEMORY_OFFSET;
    currentCodeIndex++;

    parseProgram();

    displayCode();
    displaySymbolTable();
    generateELFFile();

    destroyTokenList(tokenList);
    fclose(inputFile);
    fclose(outputFile);
    return 0;
}

// Function Implementations
char peekChar() {
    int c = getc(inputFile);
    ungetc(c, inputFile);
    return (char)c;
}

void formattedPrint(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    va_start(args, format);
    vfprintf(outputFile, format, args);
    va_end(args);
}

void clearBuffer(char *str, int length) {
    for (int i = 0; i < length; i++) str[i] = '\0';
}

int checkReservedWord(char *buffer) {
    if (strcmp(buffer, "const") == 0) return CONST_SYM;
    else if (strcmp(buffer, "var") == 0) return VAR_SYM;
    else if (strcmp(buffer, "begin") == 0) return BEGIN_SYM;
    else if (strcmp(buffer, "end") == 0) return END_SYM;
    else if (strcmp(buffer, "if") == 0) return IF_SYM;
    else if (strcmp(buffer, "then") == 0) return THEN_SYM;
    else if (strcmp(buffer, "while") == 0) return WHILE_SYM;
    else if (strcmp(buffer, "do") == 0) return DO_SYM;
    else if (strcmp(buffer, "read") == 0) return READ_SYM;
    else if (strcmp(buffer, "write") == 0) return WRITE_SYM;
    else if (strcmp(buffer, "fi") == 0) return FI_SYM;
    return 0; // Default case: identifier
}

int checkSpecialSymbol(char *buffer) {
    if (strcmp(buffer, "+") == 0) return PLUS_SYM;
    else if (strcmp(buffer, "-") == 0) return MINUS_SYM;
    else if (strcmp(buffer, "*") == 0) return MULT_SYM;
    else if (strcmp(buffer, "/") == 0) return DIV_SYM;
    else if (strcmp(buffer, "(") == 0) return LEFT_PAREN_SYM;
    else if (strcmp(buffer, ")") == 0) return RIGHT_PAREN_SYM;
    else if (strcmp(buffer, ",") == 0) return COMMA_SYM;
    else if (strcmp(buffer, ";") == 0) return SEMICOLON_SYM;
    else if (strcmp(buffer, ".") == 0) return PERIOD_SYM;
    else if (strcmp(buffer, "=") == 0) return EQUAL_SYM;
    else if (strcmp(buffer, "<") == 0) return LESS_SYM;
    else if (strcmp(buffer, ">") == 0) return GREATER_SYM;
    else if (strcmp(buffer, ":=") == 0) return ASSIGN_SYM;
    else if (strcmp(buffer, "<=") == 0) return LESS_EQ_SYM;
    else if (strcmp(buffer, ">=") == 0) return GREATER_EQ_SYM;
    else if (strcmp(buffer, "<>") == 0) return NOT_EQUAL_SYM;
    return 0; // Default case: invalid symbol
}

int isSpecialCharacter(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '(' || c == ')' ||
            c == '=' || c == ',' || c == '.' || c == '<' || c == '>' || c == ':' ||
            c == ';');
}

TokenList *createTokenList() {
    TokenList *list = malloc(sizeof(TokenList));
    list->count = 0;
    list->capacity = 10;
    list->tokens = malloc(sizeof(Token) * list->capacity);
    return list;
}

void destroyTokenList(TokenList *list) {
    free(list->tokens);
    free(list);
}

TokenList *appendToken(TokenList *list, Token token) {
    if (list->count == list->capacity) {
        list->capacity *= 2;
        list->tokens = realloc(list->tokens, sizeof(Token) * list->capacity);
    }
    list->tokens[list->count++] = token;
    return list;
}

void fetchNextToken() {
    currentToken = tokenList->tokens[0];
    for (int i = 0; i < tokenList->count - 1; i++) {
        tokenList->tokens[i] = tokenList->tokens[i + 1];
    }
    tokenList->count--;
}

void generateCode(int opcode, int level, int modifier) {
    if (currentCodeIndex > CODE_ARRAY_SIZE) {
        throwError(16, "Program exceeds maximum length");
    } else {
        codeArray[currentCodeIndex].opcode = opcode;
        codeArray[currentCodeIndex].level = level;
        codeArray[currentCodeIndex].modifier = modifier;
        currentCodeIndex++;
    }
}

void throwError(int errorCode, const char *message) {
    formattedPrint("Error: %s\n", message);
    exit(1);
}

int searchSymbolTable(char *name) {
    for (int i = 0; i < currentSymbolIndex; i++) {
        if (strcmp(name, symbolTable[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

void insertSymbol(int type, char *name, int value, int level, int address) {
    symbolTable[currentSymbolIndex].type = type;
    strcpy(symbolTable[currentSymbolIndex].name, name);
    symbolTable[currentSymbolIndex].value = value;
    symbolTable[currentSymbolIndex].level = level;
    symbolTable[currentSymbolIndex].address = address;
    symbolTable[currentSymbolIndex].mark = 0;
    currentSymbolIndex++;
}

void parseProgram() {
    fetchNextToken();
    parseBlock();
    if (atoi(currentToken.value) != PERIOD_SYM) {
        throwError(1, "Program must end with a period");
    }
    generateCode(9, 0, 3);
}

void parseBlock() {
    parseConstDeclaration();
    int varCount = parseVarDeclaration();
    generateCode(6, 0, 3 + varCount);
    parseStatement();
}

void parseConstDeclaration() {
    char name[MAX_ID_LENGTH + 1];
    if (atoi(currentToken.value) == CONST_SYM) {
        do {
            fetchNextToken();
            if (atoi(currentToken.value) != IDENT_SYM) {
                throwError(2, "Constant declaration must be followed by an identifier");
            }
            strcpy(name, currentToken.lexeme);
            if (searchSymbolTable(currentToken.lexeme) != -1) {
                throwError(3, "Identifier already declared");
            }
            fetchNextToken();
            if (atoi(currentToken.value) != EQUAL_SYM) {
                throwError(4, "Constant must be assigned with '='");
            }
            fetchNextToken();
            if (atoi(currentToken.value) != NUM_SYM) {
                throwError(5, "Constant must be assigned a numeric value");
            }
            insertSymbol(1, name, atoi(currentToken.lexeme), currentLevel, 0);
            fetchNextToken();
        } while (atoi(currentToken.value) == COMMA_SYM);
        if (atoi(currentToken.value) != SEMICOLON_SYM) {
            throwError(6, "Constant declaration must end with a semicolon");
        }
        fetchNextToken();
    }
}

int parseVarDeclaration() {
    int varCount = 0;
    if (atoi(currentToken.value) == VAR_SYM) {
        do {
            varCount++;
            fetchNextToken();
            if (atoi(currentToken.value) != IDENT_SYM) {
                throwError(2, "Variable declaration must be followed by an identifier");
            }
            if (searchSymbolTable(currentToken.lexeme) != -1) {
                throwError(3, "Identifier already declared");
            }
            insertSymbol(2, currentToken.lexeme, 0, currentLevel, varCount + 2);
            fetchNextToken();
        } while (atoi(currentToken.value) == COMMA_SYM);
        if (atoi(currentToken.value) != SEMICOLON_SYM) {
            throwError(6, "Variable declaration must end with a semicolon");
        }
        fetchNextToken();
    }
    return varCount;
}

void parseStatement() {
    if (atoi(currentToken.value) == IDENT_SYM) {
        int symbolIndex = searchSymbolTable(currentToken.lexeme);
        if (symbolIndex == -1) {
            throwError(7, "Undeclared identifier");
        }
        if (symbolTable[symbolIndex].type != 2) {
            throwError(8, "Only variable values can be altered");
        }
        fetchNextToken();
        if (atoi(currentToken.value) != ASSIGN_SYM) {
            throwError(9, "Assignment must use ':='");
        }
        fetchNextToken();
        parseExpression();
        generateCode(4, 0, symbolTable[symbolIndex].address);
    } else if (atoi(currentToken.value) == BEGIN_SYM) {
        do {
            fetchNextToken();
            parseStatement();
        } while (atoi(currentToken.value) == SEMICOLON_SYM);
        if (atoi(currentToken.value) != END_SYM) {
            throwError(10, "Begin must be followed by end");
        }
        fetchNextToken();
    } else if (atoi(currentToken.value) == IF_SYM) {
        fetchNextToken();
        parseCondition();
        int jumpIndex = currentCodeIndex;
        generateCode(8, 0, 0);
        if (atoi(currentToken.value) != THEN_SYM) {
            throwError(11, "If must be followed by then");
        }
        fetchNextToken();
        parseStatement();
        if (atoi(currentToken.value) != FI_SYM) {
            throwError(15, "Then must be followed by fi");
        }
        fetchNextToken();
        codeArray[jumpIndex].modifier = currentCodeIndex;
    } else if (atoi(currentToken.value) == WHILE_SYM) {
        fetchNextToken();
        int loopIndex = currentCodeIndex;
        parseCondition();
        if (atoi(currentToken.value) != DO_SYM) {
            throwError(12, "While must be followed by do");
        }
        fetchNextToken();
        int jumpIndex = currentCodeIndex;
        generateCode(8, 0, 0);
        parseStatement();
        generateCode(7, 0, loopIndex);
        codeArray[jumpIndex].modifier = currentCodeIndex;
    } else if (atoi(currentToken.value) == READ_SYM) {
        fetchNextToken();
        if (atoi(currentToken.value) != IDENT_SYM) {
            throwError(2, "Read must be followed by an identifier");
        }
        int symbolIndex = searchSymbolTable(currentToken.lexeme);
        if (symbolIndex == -1) {
            throwError(7, "Undeclared identifier");
        }
        if (symbolTable[symbolIndex].type != 2) {
            throwError(8, "Only variable values can be altered");
        }
        fetchNextToken();
        generateCode(9, 0, 2);
        generateCode(4, 0, symbolTable[symbolIndex].address);
    } else if (atoi(currentToken.value) == WRITE_SYM) {
        fetchNextToken();
        parseExpression();
        generateCode(9, 0, 1);
    }
}

void parseCondition() {
    if (atoi(currentToken.value) == ODD_SYM) {
        fetchNextToken();
        parseExpression();
        generateCode(2, 0, 11);
    } else {
        parseExpression();
        switch (atoi(currentToken.value)) {
            case EQUAL_SYM:
                fetchNextToken();
                parseExpression();
                generateCode(2, 0, 5);
                break;
            case NOT_EQUAL_SYM:
                fetchNextToken();
                parseExpression();
                generateCode(2, 0, 6);
                break;
            case LESS_SYM:
                fetchNextToken();
                parseExpression();
                generateCode(2, 0, 7);
                break;
            case LESS_EQ_SYM:
                fetchNextToken();
                parseExpression();
                generateCode(2, 0, 8);
                break;
            case GREATER_SYM:
                fetchNextToken();
                parseExpression();
                generateCode(2, 0, 9);
                break;
            case GREATER_EQ_SYM:
                fetchNextToken();
                parseExpression();
                generateCode(2, 0, 10);
                break;
            default:
                throwError(13, "Condition must have a comparison operator");
                break;
        }
    }
}

void parseExpression() {
    if (atoi(currentToken.value) == MINUS_SYM) {
        fetchNextToken();
        parseTerm();
        generateCode(2, 0, 1);
    } else {
        parseTerm();
    }

    while (atoi(currentToken.value) == PLUS_SYM || atoi(currentToken.value) == MINUS_SYM) {
        if (atoi(currentToken.value) == PLUS_SYM) {
            fetchNextToken();
            parseTerm();
            generateCode(2, 0, 1);
        } else {
            fetchNextToken();
            parseTerm();
            generateCode(2, 0, 2);
        }
    }
}

void parseTerm() {
    parseFactor();
    while (atoi(currentToken.value) == MULT_SYM || atoi(currentToken.value) == DIV_SYM) {
        if (atoi(currentToken.value) == MULT_SYM) {
            fetchNextToken();
            parseFactor();
            generateCode(2, 0, 3);
        } else {
            fetchNextToken();
            parseFactor();
            generateCode(2, 0, 4);
        }
    }
}

void parseFactor() {
    if (atoi(currentToken.value) == IDENT_SYM) {
        int symbolIndex = searchSymbolTable(currentToken.lexeme);
        if (symbolIndex == -1) {
            throwError(7, "Undeclared identifier");
        }
        if (symbolTable[symbolIndex].type == 1) {
            generateCode(1, 0, symbolTable[symbolIndex].value);
        } else {
            generateCode(3, currentLevel - symbolTable[symbolIndex].level, symbolTable[symbolIndex].address);
        }
        fetchNextToken();
    } else if (atoi(currentToken.value) == NUM_SYM) {
        generateCode(1, 0, atoi(currentToken.lexeme));
        fetchNextToken();
    } else if (atoi(currentToken.value) == LEFT_PAREN_SYM) {
        fetchNextToken();
        parseExpression();
        if (atoi(currentToken.value) != RIGHT_PAREN_SYM) {
            throwError(14, "Right parenthesis must follow left parenthesis");
        }
        fetchNextToken();
    } else {
        throwError(15, "Invalid expression");
    }
}

void displaySymbolTable() {
    formattedPrint("\nSymbol Table:\n");
    formattedPrint("%10s | %10s | %10s | %10s | %10s | %10s\n", "Type", "Name", "Value", "Level", "Address", "Mark");
    formattedPrint("-----------------------------------------------------------------------\n");

    for (int i = 0; i < currentSymbolIndex; i++) {
        symbolTable[i].mark = 1;
        if (symbolTable[i].type == 1)
            formattedPrint("%10d | %10s | %10d | %10s | %10s | %10d\n", symbolTable[i].type, symbolTable[i].name, symbolTable[i].value, "-", "-", symbolTable[i].mark);
        else
            formattedPrint("%10d | %10s | %10d | %10d | %10d | %10d\n", symbolTable[i].type, symbolTable[i].name, symbolTable[i].value, symbolTable[i].level, symbolTable[i].address, symbolTable[i].mark);
    }
}

void displayCode() {
    formattedPrint("Assembly Code:\n");
    formattedPrint("%10s %10s %10s %10s\n", "Line", "Op", "L", "M");
    for (int i = 0; i < currentCodeIndex; i++) {
        char opcodeName[4];
        fetchOpcodeName(codeArray[i].opcode, opcodeName);
        formattedPrint("%10d %10s %10d %10d\n", i, opcodeName, codeArray[i].level, codeArray[i].modifier);
    }
}

void fetchOpcodeName(int opcode, char *name) {
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

void generateELFFile() {
    FILE *elfFile = fopen("output.elf", "w");
    if (elfFile == NULL) {
        formattedPrint("Error: Unable to create ELF file\n");
        return;
    }

    for (int i = 0; i < currentCodeIndex; i++) {
        fprintf(elfFile, "%d %d %d\n", codeArray[i].opcode, codeArray[i].level, codeArray[i].modifier);
    }

    fclose(elfFile);
}
