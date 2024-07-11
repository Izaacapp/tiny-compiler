// Authors: Josie Goreczky and Kyutza Lopez-Herrera

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_NUM 4
#define MAX_WORD 10
#define MAX_SYMBOL_TABLE_SIZE 500
#define MAX_ARRAY_LENGTH 1000



//struct for token types
typedef enum {
oddsym = 1, identsym, numbersym, plussym, minussym,
multsym, slashsym, fisym, eqsym, neqsym, lessym, leqsym,
gtrsym, geqsym, lparentsym, rparentsym, commasym, semicolonsym,
periodsym, becomessym, beginsym, endsym, ifsym, thensym,
whilesym, dosym, callsym, constsym, varsym, procsym, writesym,
readsym , elsesym, modsym} token_type;

// Global variables for PL/0 Instructions
int LIT = 1, OPR = 2, RTN = 0, ADD = 1, SUB = 2, MUL = 3,
    DIV = 4, EQL = 5, NEQ = 6, LSS = 7, LEQ = 8,
    GTR = 9, GEQ = 10, ODD = 11, MOD = 12, LOD = 3, STO = 4, CAL = 5, INC = 6,
    JMP = 7, JPC = 8, SYS = 9;

// struct for generating code
typedef struct {
    int op;
    int l;
    int m;
} instruction;

//token structure
typedef struct{
    token_type tokenType;
    int error;
    int number;
    char tokenName[MAX_WORD]; 
} tokenStruct;

// symbol table struct
typedef struct {
    int kind; // const = 1, var = 2, proc = 3
    char name[10]; // name up to 11 chars
    int val; // number (ASCII value)
    int level; // L level
    int addr; // M address
    int mark; // to indicate unavailable or deleted
} symbol;

// array for symbol table
symbol * symbolTable[MAX_SYMBOL_TABLE_SIZE];
int symIndex = 0; // keeps track of symbol table index

// array for reserved words names
char * reservedWords[] = {"const", "var", "procedure", "call", "begin", "end", "if", "fi", "then", "else", "while", "do", "read", "write"};

// array for special symbols
char specialSymbols[] = {'+', '-', '*', '/', '(', ')', '=', ',', '.', '<', '>', ';', ':'};

tokenStruct * lexemeList; //array of token structs
int lexSize = 0;   //keeps track of array location
int lexIndex = 0; // To iterate through the lexeme list to populate the symbol table

instruction * code[MAX_SYMBOL_TABLE_SIZE]; //array of instructions
int codeIndex = 0;

int numVars = 0; // number of variables

// struct for identifiers
typedef struct {
  char id[MAX_ARRAY_LENGTH];
} stringHolder;

// array for identifiers
stringHolder identArray[MAX_ARRAY_LENGTH];

// for calling procedures
typedef struct {
  int destination;
  char name[12];
} callProcs;

// array for calls
callProcs callArray[MAX_ARRAY_LENGTH];
int indexCall = 0;

int level = -1; // lexigraphical level

tokenStruct * assignResToken(int isResWord, char * word) {
    switch (isResWord)
    {
    case 0:
        lexemeList[lexSize].tokenType = constsym;
        break;
    case 1:
        //printf("TEST\n");
        lexemeList[lexSize].tokenType = varsym;
        break;
    case 2:
        lexemeList[lexSize].tokenType = procsym;
        break;
    case 3:
        lexemeList[lexSize].tokenType = callsym;
        break;
    case 4:
        lexemeList[lexSize].tokenType = beginsym;
        break;
    case 5:
        lexemeList[lexSize].tokenType = endsym;
        break;
    case 6:
        lexemeList[lexSize].tokenType = ifsym;
        break;
    case 7:
        lexemeList[lexSize].tokenType = fisym;
        break;
    case 8:
        lexemeList[lexSize].tokenType = thensym;
        break;
    case 9:
        lexemeList[lexSize].tokenType = elsesym;
        break;
    case 10:
        lexemeList[lexSize].tokenType = whilesym;
        break;
    case 11:
        lexemeList[lexSize].tokenType = dosym;
        break;
    case 12:
        lexemeList[lexSize].tokenType = readsym;
        break;
    case 13:
        lexemeList[lexSize].tokenType = writesym;
        break;
    default:
        lexemeList[lexSize].tokenType = identsym;
        strcpy(lexemeList[lexSize].tokenName, word);
        break;
    }
    return lexemeList;
}

tokenStruct * assignSymbolToken (int sym, char * invalidSymbol, FILE * input, int sameToken, int token) {
    switch (sym)
    {
    //+
    case 0:
        lexemeList[lexSize].tokenType = plussym;
        strcpy(lexemeList[lexSize].tokenName, "+");
        lexSize++;
        break;
    //-
    case 1:
        lexemeList[lexSize].tokenType = minussym;
        strcpy(lexemeList[lexSize].tokenName, "-");
        lexSize++;
        break;
    //*
    case 2:
        lexemeList[lexSize].tokenType = multsym;
        strcpy(lexemeList[lexSize].tokenName, "*");
        lexSize++;
        break;
    //comment
    case 3:
        token = fgetc(input);
        sameToken = 1;
        // commment handling
        if (token == '*') {
            token = fgetc(input);
            int flag = 1;
            sameToken = 0;
            while (flag) {
                if (token == '*') {
                    token = fgetc(input);
                    if (token == '/')
                        flag = 0;
                } else {
                    token = fgetc(input);
                }
            }
        }
        else { // just a slash. not a comment
            lexemeList[lexSize].tokenType = slashsym;
            strcpy(lexemeList[lexSize].tokenName, "/");
            lexSize++;
        }
        break;
    //)
    case 4:
        lexemeList[lexSize].tokenType = lparentsym;
        strcpy(lexemeList[lexSize].tokenName, ")");
        lexSize++;
        break;
    //(
    case 5:
        lexemeList[lexSize].tokenType = rparentsym;
        strcpy(lexemeList[lexSize].tokenName, "(");
        lexSize++;
        break;
    //=
    case 6:
        lexemeList[lexSize].tokenType = eqsym;
        strcpy(lexemeList[lexSize].tokenName, "=");
        lexSize++;
        break;
    //,
    case 7:
        lexemeList[lexSize].tokenType = commasym;
        strcpy(lexemeList[lexSize].tokenName, ",");
        lexSize++;
        break;
    //.
    case 8:
        lexemeList[lexSize].tokenType = periodsym;
        strcpy(lexemeList[lexSize].tokenName, ".");
        lexSize++;
        break;
    //<>, <=, <
    case 9:
        token = fgetc(input);
        sameToken = 1;
        if (token == '>') {
            lexemeList[lexSize].tokenType = neqsym;
            strcpy(lexemeList[lexSize].tokenName, "<>");
            sameToken = 0;
        }
        else if (token == '=') {
            lexemeList[lexSize].tokenType = leqsym;
            strcpy(lexemeList[lexSize].tokenName, "<=");
            sameToken = 0;
        } else {
            lexemeList[lexSize].tokenType = lessym;
            strcpy(lexemeList[lexSize].tokenName, "<");
        }
        lexSize++;
        break;
    //>=, ><, >
    case 10:
        token = fgetc(input);
        sameToken = 1;
        if (token == '=') {
            lexemeList[lexSize].tokenType = geqsym;
            strcpy(lexemeList[lexSize].tokenName, ">=");
            sameToken = 0;
        } else if (token == '<') {
            lexemeList[lexSize].error = 3;
            strcpy(lexemeList[lexSize].tokenName, "><");
            printf("Error: Invalid Symbol\n");
            exit(0);
        }
        else {
            lexemeList[lexSize].tokenType = gtrsym;
            strcpy(lexemeList[lexSize].tokenName, ">");
        }
        lexSize++;
        break;
    //;
    case 11:
        lexemeList[lexSize].tokenType = semicolonsym;
        strcpy(lexemeList[lexSize].tokenName, ";");
        lexSize++;
        break;
    //:=
    case 12:
        token = fgetc(input);
        if (token == '=') {
            lexemeList[lexSize].tokenType = becomessym;
            strcpy(lexemeList[lexSize].tokenName, ":=");
            lexSize++;
        }
        else {
            lexemeList[lexSize].error = 3;
            printf("Error: Invalid Symbol\n");
            exit(0);
        }
        break;
    default:
        lexemeList[lexSize].error = 3;
        strcpy(lexemeList[lexSize].tokenName, invalidSymbol);
        printf("Error: Invalid Symbol\n");
        exit(0);
        lexSize++;
        break;
    }
}

// generate code
void emit(int op, int L, int M) {
  if (codeIndex > 1000)
    printf("Error: program too long.\n");
  else {

    code[codeIndex]->op = op;
    code[codeIndex]->l = L;
    code[codeIndex]->m = M;
    codeIndex++;
  }
}

// add to symbol table
void enter (int k, char * name, int val, int level, int addr) {
    symbolTable[symIndex]->kind = k;
    strcpy(symbolTable[symIndex]->name, name);
    symbolTable[symIndex]->val = val;
    symbolTable[symIndex]->level = level;
    symbolTable[symIndex]->addr = addr;
    symbolTable[symIndex]->mark = 0;
    symIndex++;
}

int SYMBOLTABLECHECK() {
    for (int i = 0; i < MAX_SYMBOL_TABLE_SIZE; i++) {
        if (symbolTable[i] != NULL && strcmp(symbolTable[i]->name, lexemeList[lexIndex].tokenName) == 0) {
            
            return i;
        }
    }
    return -1;
} 

token_type getToken(int index) {
    return lexemeList[index].tokenType;
}

void printSourceProgram(FILE * fp) {
    char input;

    fseek(fp, 0, SEEK_SET);
    while((input = fgetc(fp)) != EOF) {
        putchar(input);
    }
    printf("\n\n");
}

void printError(int index) {
    if (lexemeList[index].tokenType == 3){ // IF NUMBER
        if(lexemeList[index].error == 1){
            printf("\n%d\t\t", lexemeList[index].number);
            printf("Error: Number too long");
        } else {
            printf("\n%d\t\t%d\t", lexemeList[index].number, lexemeList[index].tokenType);
        }
        
    }else{ // NOT NUMBER
        if(lexemeList[index].error == 2){
            printf("\n%s\t\t", lexemeList[index].tokenName);
            printf("Error: Name too long");
        }else if(lexemeList[index].error == 3){
            printf("\n%s\t\t", lexemeList[index].tokenName);
            printf("Error: Invalid Symbol");
        } else {
            printf("\n%s\t\t%d\t", lexemeList[index].tokenName, lexemeList[index].tokenType);
        }
    }
}

void printLexList(FILE * tokens) {
    printf("\nLexeme Table:\nLexeme\t\tToken Type\n");
    for (int i = 0; i < lexSize; i++) {
        printError(i);
    }
    printf("\n");

    //prints lexeme list
    fprintf(tokens, "Token List\n");
    for (int j = 0; j < lexSize; j++){
        if (lexemeList[j].error != -1) {
            continue;
        }
        fprintf(tokens, "%d ", lexemeList[j].tokenType);
        if(lexemeList[j].tokenType == 2){
            fprintf(tokens, "%s ", lexemeList[j].tokenName);
        } else if (lexemeList[j].tokenType == 3){
            fprintf(tokens, "%d ", lexemeList[j].number);
        }
    }
    fprintf(tokens, "\n");
}

void program();
int block();
void constDeclaration();
void varDeclaration();
void procDeclaration();
void readStatement();
void readCondition();
void readExpression();
void readFactor();
void readTerm();
void printInstructions();
void printSymbolTable();

void program() {
    int token = getToken(lexIndex);

    block();

    token = getToken(lexIndex);

    if(token != periodsym) {
        //  program must end with period
       printf("Error: program must end with period\n");
       exit(0);
    }

    // halt
    emit(SYS, 0, 3);


    for (int i = 0; i < codeIndex; i++) {
    if (code[i]->op == EQL) {
      for (int j = 0; j < indexCall; j++) {
        if (strcmp(identArray[code[i]->m].id, callArray[j].name) == 0)
          code[i]->m = callArray[j].destination * 3;
      }
    }
  }
}

int block() {
    level++;

    int prev_symIndex = symIndex;

    int space = 3;

    int jmpAddr = codeIndex;

    
    emit(JMP, 0, 666);

    constDeclaration();

    if (getToken(lexIndex) == varsym) {
        varDeclaration();
        space += numVars;
        numVars = 0;
    }

    procDeclaration();

    code[jmpAddr]->m = codeIndex * 3;

    int procAddr = jmpAddr;

    // inc
    emit(INC, 0, space);

    readStatement();

    symIndex = prev_symIndex;

    level--;

    return procAddr;
}

void constDeclaration() {
    if (getToken(lexIndex) == constsym) {
        do{
            char tmpName[11];

            int token = getToken(lexIndex+=1);

            if (token != identsym) {
                printf("Error: constant, variable, procedure, call, and read keywords must be followed by identifier\n");
                exit(0);
            } else if (SYMBOLTABLECHECK() != -1) {
                
                printf("Error: symbol name %s has already been declared\n", lexemeList[lexIndex].tokenName);
                exit(0);
            } else {
                // Save name
                strcpy(tmpName, lexemeList[lexIndex].tokenName);

                token = getToken(lexIndex+=1);
                if (token != eqsym) {
                    printf("Error: constants must be assigned with =\n");
                    exit(0);

                } else {
                    token = getToken(lexIndex+=1);
                    if (token != numbersym) {
                        printf("Error: constants must be assigned an integer value\n");
                        exit(0);
                    } 
                    // Add to symbol table
                    enter(1, tmpName, lexemeList[lexIndex].number, level, 0);

                    token = getToken(lexIndex+=1);
                }
            }
        }while(getToken(lexIndex) == commasym);

        if (getToken(lexIndex) != semicolonsym) {
            printf("Error: constant, variable, and procedure declarations must be followed by a semicolon\n");
            exit(0);
        }

        int token = getToken(lexIndex+=1);
    } 
}

void varDeclaration() {
    if (getToken(lexIndex) == varsym) {
        

        do{ 
            char ident[12];
            
            int token = getToken(lexIndex+=1);
        
            if (token != identsym) {
                printf("Error: constant, variable, procedure, and read keywords must be followed by identifier\n");
                exit(0);
            } else if (SYMBOLTABLECHECK() != -1) {
                printf("Error: symbol name %s has already been declared\n", lexemeList[lexIndex].tokenName);
                exit(0);
            } 

            // Save name
            strcpy(ident, lexemeList[lexIndex].tokenName);

            numVars++;

            token = getToken(lexIndex+=1);

            enter(2, ident, 0, level, numVars+2);

            // Add to symbol table
            
            

        } while(getToken(lexIndex) == commasym);

        // printf("SHOULD BE SEMICOLON %d\n", getToken(lexIndex));

        if (getToken(lexIndex) != semicolonsym) {
            printf("Error: constant, procedure, and variable declarations must be followed by a semicolon\n");
            exit(0);
        }

        int token = getToken(lexIndex+=1);
    }
}

void procDeclaration() {
    if (getToken(lexIndex) == procsym) {
        int token = getToken(lexIndex+=1);

        if (token != identsym) {
            printf("Error: const, var, call, and procedure must be followed by identifier\n");
            exit(0);
        } else if (SYMBOLTABLECHECK() != -1) {

            // printf("token: %d \n", token);
            // printf("token index: %d \n", lexIndex);
            printf("Error: symbol name %s has already been declared\n", lexemeList[lexIndex].tokenName);
            exit(0);
        } 

        enter(3, lexemeList[lexIndex].tokenName, 0, level, lexemeList[lexIndex].tokenType);

        int procIndex = symIndex - 1;

        token = getToken(lexIndex+=1);

        if (token != semicolonsym) {
            printf("Error: const, var, call, and procedure must be followed by semicolon\n");
            exit(0);
        }

        token = getToken(lexIndex+=1);

        //printf("SHOULD BE BEGIN %d\n", token);

        int procAddr = block();

        symbolTable[procIndex]->addr = procAddr;

        // populate call array
        strcpy(callArray[indexCall].name, symbolTable[symIndex-1]->name);
        callArray[indexCall].destination = symbolTable[symIndex-1]->addr;
        indexCall++;

        token = getToken(lexIndex);

        if (token != semicolonsym) {
            printf("Error: procedure must be followed by semicolon\n");
            exit(0);
        } else {
            emit(OPR, 0, 0);
        }

        token = getToken(lexIndex+=1);
    }
  
}

void readStatement() {
    int token = getToken(lexIndex);

    if (token == identsym) {
        int tmpIndex = SYMBOLTABLECHECK();
         if (tmpIndex == -1) {
              printf("Error: undeclared identifier %s\n", lexemeList[lexIndex].tokenName);
              exit(0);
         } else if (symbolTable[tmpIndex]->kind != 2) {
                printf("Error: only var values may be altered\n");
                exit(0);
         } else {
              token = getToken(lexIndex+=1);
              if(token != becomessym) {
                    printf("Error: must be followed by :=\n");
                    exit(0);
              } else {
                    token = getToken(lexIndex+=1);

                    readExpression();
                    
                    // emit STO
                    emit(STO, level - symbolTable[tmpIndex]->level, symbolTable[tmpIndex]->addr);

                    return;
              }
         }
    } else if (token == callsym) {
        token = getToken(lexIndex+=1);

        if(token != identsym) {
            printf("Error: call must be followed by identifier\n");
            exit(0);
        } else {
           
            int tmpIndex = SYMBOLTABLECHECK();

            if(tmpIndex == -1) {
                printf("Error: undeclared identifier %s or out of scope\n", lexemeList[lexIndex].tokenName);
                exit(0);
            } else if(symbolTable[tmpIndex]->kind != 3) {
                printf("Error: call must be followed by procedure\n");
                exit(0);
            } else {
                int procIndex = symIndex-2;

                emit(CAL, level - symbolTable[tmpIndex]->level,procIndex);

                token = getToken(lexIndex+=1);
            }
        }
    } else if(token == beginsym) {
        do {
            token = getToken(lexIndex+=1);
            readStatement();
        } while (getToken(lexIndex) == semicolonsym);

        if (getToken(lexIndex) != endsym) {
            printf("Error: begin must be followed by end\n");
            exit(0);
        }
        token = getToken(lexIndex+=1);

        readStatement();

        return;
    } else if(token == ifsym) {
        token = getToken(lexIndex+=1);
        
        readCondition();

        int jpcIndex = codeIndex;

        emit(JPC, 0, 0);

        token = getToken(lexIndex);

        if(token != thensym) {
            printf("Error: if must be followed by then\n");
            exit(0);
        }
        token = getToken(lexIndex+=1);

        readStatement();
        code[jpcIndex]->m = codeIndex * 3;

        token = getToken(lexIndex);
        if(token == fisym) {
            token = getToken(lexIndex+=1);
        }

        return;
    } else if(token == whilesym) {
        token = getToken(lexIndex+=1);

        int loopIndex = codeIndex;

        readCondition();
        
        if(token != dosym) {
            printf("Error: while must be followed by do\n");
            exit(0);
        }
        int jpcIndex = codeIndex;
        // emit JPC
        emit(JPC, 0, 0);

        token = getToken(lexIndex+=1);

        readStatement();
        // emit JMP
        emit(JMP, 0, loopIndex * 3);

        // m parameter of jpc
        code[jpcIndex]->m = codeIndex * 3;

        return;
    } else if(token == readsym) {
        token = getToken(lexIndex+=1);

        if(token != identsym) {
            printf("Error: const, var, procedure, call, and read keywords must be followed by identifier\n");
            exit(0);
        }
        int tmpIndex = SYMBOLTABLECHECK();

        if(tmpIndex == -1) {
            printf("Error: undeclared identifier %s\n", lexemeList[lexIndex].tokenName);
            exit(0);
        } else if(symbolTable[tmpIndex]->kind != 2) {
            printf("Error: only var values may be altered\n");
            exit(0);
        } else {
            token = getToken(lexIndex+=1);
            // emit READ
            emit (SYS, 0, 2);

            //emit STO
            emit(STO, level - symbolTable[tmpIndex]->level, symbolTable[tmpIndex]->addr);

            return;
        }
    }  else if(token == writesym) {
        token = getToken(lexIndex+=1);

        readExpression();
        // emit write
        emit(SYS, 0, 1);

        return;
    }
}

void readCondition() {
    int token = getToken(lexIndex);

    if(token == oddsym) {
        token = getToken(lexIndex+=1);
        readExpression();
        // emit ODD
        emit(OPR, 0, ODD);
    } else {
        readExpression();
        token = getToken(lexIndex);
       
        if(token == eqsym) {
            token = getToken(lexIndex+=1);
            readExpression();
            // emit EQL
            emit(OPR, 0, EQL);
        } else if(token == neqsym) {
            token = getToken(lexIndex+=1);
            readExpression();
            // emit NEQ
            emit(OPR, 0, NEQ);
        } else if(token == lessym) {
            token = getToken(lexIndex+=1);
            
            if(token == eqsym) {
                token = getToken(lexIndex+=1);
                readExpression();
                // emit LEQ
                emit(OPR, 0, LEQ);
            } else {
                readExpression();
                // emit LSS
                emit(OPR, 0, LSS);
            }
        } else if(token == gtrsym) {

            token = getToken(lexIndex+=1);

            if(token == eqsym) {
                token = getToken(lexIndex+=1);
                readExpression();
                // emit GEQ
                emit(OPR, 0, GEQ);
            } else {
                readExpression();
                // emit GTR
                emit(OPR, 0, GTR);
            }
        } else {
            printf("Error: condition must contain comparison operator\n");
            exit(0);
        }
    }
}

void readExpression() {
    int token = getToken(lexIndex);

    if (token == minussym) {
        token = getToken(lexIndex+=1);
        readTerm();

        token = getToken(lexIndex);

        while (token == plussym || token == minussym) {
            if (token == plussym ) {
                token = getToken (lexIndex+=1);

                readTerm();

                token = getToken(lexIndex);

                // emit ADD
                emit(OPR, 0, ADD);
            } else {
                token = getToken(lexIndex+=1);

                readTerm();

                token = getToken(lexIndex);

                // emit SUB
                emit(OPR, 0, SUB);
            }
        }
    } else {
        if (getToken(lexIndex) == plussym) {
            token = getToken(lexIndex+=1);
        }

        readTerm();

        token = getToken(lexIndex);

        while (token == plussym || token == minussym) {
            if (token == plussym) {
                token = getToken(lexIndex+=1);

                readTerm();

                token = getToken(lexIndex);

                // emit ADD
                emit(OPR, 0, ADD);
            } else {
                token = getToken(lexIndex+=1);

                readTerm();

                token = getToken(lexIndex);

                // emit SUB
                emit(OPR, 0, SUB);
            }
        }
    }
}

void readTerm() {
    int token = getToken(lexIndex);
    
    readFactor();
    
    token = getToken(lexIndex);

    while(token == multsym || token == slashsym || token == modsym) {
        if(token == multsym) {
            token = getToken(lexIndex+=1);
            readFactor();
            // emit MUL
            emit(OPR, 0, MUL);
        } else if(token == slashsym){
            token = getToken(lexIndex+=1);
            readFactor();
            // emit DIV
            emit(OPR, 0, DIV);
        } else {
            token = getToken(lexIndex+=1);
            readFactor();
            // emit MOD
            emit(OPR, 0, MOD);
        }
    }
}

void readFactor() {
    int token = getToken(lexIndex);

    if (token == identsym) {

        int tmpIndex = SYMBOLTABLECHECK();

        if (tmpIndex == -1) {
            printf("Error: undeclared variable %s or out of scope\n", lexemeList[lexIndex].tokenName);
            exit(0);
        } else if (symbolTable[tmpIndex]->kind == 1) {
            // emit LIT
            emit(LIT, 0, symbolTable[tmpIndex]->val);
        } else if (symbolTable[tmpIndex]->kind == 2) {
            // emit LOD
            emit(LOD, level - symbolTable[tmpIndex]->level, symbolTable[tmpIndex]->addr);
        }

        token = getToken(lexIndex+=1);
    } else if (token == numbersym) {
        // emit LIT

        emit(LIT, 0, lexemeList[lexIndex].number);

        token = getToken(lexIndex+=1);

    } else if (token == lparentsym) {
        token = getToken(lexIndex+=1);
        readExpression();

        token = getToken(lexIndex);

        if (token != rparentsym) {
            printf("Error: right parenthesis must follow left parenthesis\n");
            exit(0);
        }

        token = getToken(lexIndex+=1);
    } else {
        printf("Error: arithmetic equations must contain operands, parentheses, numbers, or symbols\n");
        exit(0);
    }
}

void printInstructions(FILE * output){
    int len = codeIndex;
    printf("Line\tOP\tL\tM\n");
    for (int i = 0; i < codeIndex; i++) {
    
        printf("%d\t", i);
        switch (code[i]->op) {
            case 1:
                printf("LIT");
                break;
            case 2:
                printf("OPR");
                break;
            case 3:
                printf("LOD");
                break;
            case 4:
                printf("STO");
                break;
            case 5:
                printf("CAL");
                break;
            case 6:
                printf("INC");
                break;
            case 7:
                printf("JMP");
                break;
            case 8:
                printf("JPC");
                break;
            case 9:
                printf("SYS");
                break;
        }
        printf("\t%d\t%d\n", code[i]->l, code[i]->m);
        fprintf(output, "%d %d %d\n", code[i]->op, code[i]->l, code[i]->m);
    }

    fclose(output);
}

void printSymbolTable() {
    // setting mark to 1 after generating code
    for (int i = 0; i < symIndex; i++) {
        symbolTable[i]->mark = 1;
    }

    int len = symIndex;

    printf("Symbol Table: \n");
    printf("Kind | Name\t\t | Value | Level | Address | Mark\n");
    printf("---------------------------------------------------------\n");

    for (int i = 0; i < len; i++) {
        printf("%d    |", symbolTable[i]->kind);
        printf("\t\t       %s |     %d |", symbolTable[i]->name, symbolTable[i]->val);

        if (symbolTable[i]->kind == 2)
            printf("     %d |       %d |", symbolTable[i]->level, symbolTable[i]->addr);
        else
            printf("     - |       - |");

        printf("    %d\n", symbolTable[i]->mark);
  }
}

int main(int argc, char ** argv) {
    bool sameToken = false; //used in the event of multiple character tokens

    FILE *input = fopen(argv[1], "r");
    FILE * tokens = fopen("tokens.txt", "w");
    FILE * output = fopen("elf.txt", "w");

    int character = fgetc(input); //stores each character in file
    fclose(input);

    for (int i = 0; i < MAX_ARRAY_LENGTH; i++)
        callArray[i].destination = -1;
    
    // initializing array 
    lexemeList = (tokenStruct *) malloc(sizeof(tokenStruct) * 3000); 
    for (int i = 0; i < 3000; i ++) {
        lexemeList[i].error = -1;
        lexemeList[i].number = 0;
        strcpy(lexemeList[i].tokenName, "-1");
        lexemeList[i].tokenType = -1;
    }

    // reset file
    input = fopen(argv[1], "r");
    character = fgetc(input);

    while (character != EOF) {
        //ignore tabs, white spaces, and newlines
        if (iscntrl(character) || isspace(character)) {
            character = fgetc(input);
            continue;
        } else if (isalpha(character)) {
            int wordLen = 0;
            char word[12] = {}; //12 chars is error identifier wordLen

            word[wordLen] = character;
            sameToken = true;
            
            character = fgetc(input);
            wordLen++;
            while (isalpha(character) || isdigit(character)) {
                //errors out if identifier is longer than 11 characters
                if (wordLen > MAX_WORD) {
                    lexemeList[lexSize].error = 2;     
                    printf("Error: Name too long\n"); 
                    exit(0);
                    
                    character = fgetc(input);   // next character
                    sameToken = false; //now beginning to scan new token
                    break;
                }
                // building current word of token
                word[wordLen] = character;
                character = fgetc(input);
                wordLen++;
            }

            //check to see if word is a reserved word
            int isResWord = -1;
            for (int i = 0; i < 14; i++) {
                // if word is a reserved word
                if (strcmp(word, reservedWords[i]) == 0) {
                    strcpy(lexemeList[lexSize].tokenName, word); 
                    isResWord = i;
                }
            }

            assignResToken(isResWord, word);
            lexSize++;
        } else if (isdigit(character)) {
            // converting char to digit
            int digit = character - '0';
            int digitLen = 1;
            character = fgetc(input);
            sameToken = true;

            while (isdigit(character)) {
                // Number is too long
                if (digitLen > MAX_NUM) {
                    lexemeList[lexSize].error = 1;
                    printf("Error: Number too long\n");
                    exit(0);
                    character = fgetc(input);
                    break;
                }

                // converting char to digit for next place
                digit = (10 * digit) + (character - '0');
                character = fgetc(input);
                digitLen++;
            }
            // whole digit has been read
            lexemeList[lexSize].number = digit;
            lexemeList[lexSize].tokenType = numbersym;
            lexSize++;
        } else {
            //if the character is a special symbol
            sameToken = false;

            char invalidSymbol[1] = {0};
            invalidSymbol[0] = character;
            invalidSymbol[1] = '\0';

            int sym = -1;
            for (int i = 0; i < 13; i++) {
                if (character == specialSymbols[i])
                    sym = i;
            }
            assignSymbolToken (sym, invalidSymbol, input, sameToken, character);
        }
        if (sameToken == 0) character = fgetc(input);
    }

    // initialize symbol table
    for (int i = 0; i < MAX_SYMBOL_TABLE_SIZE; i++) {
        symbolTable[i] = (symbol *) malloc(sizeof(symbol));
        symbolTable[i]->kind = -1;
        strcpy(symbolTable[i]->name, "-1");
        symbolTable[i]->val = 0;
        symbolTable[i]->level = 0;
        symbolTable[i]->addr = -1;
        symbolTable[i]->mark = -1;
    }

    // initialize code array
    for (int i = 0; i < MAX_SYMBOL_TABLE_SIZE; i++) {
        code[i] = (instruction *) malloc(sizeof(instruction));
        code[i]->op = -1;
        code[i]->l = -1;
        code[i]->m = -1;
    }

    printSourceProgram(input);
   
    // parse
    program();
    
    printf("No errors, program is syntactically correct\n\n");
    printf("Assembly Code:\n\n");

    printInstructions(output);

    fclose(input);

    // free memory
    free(lexemeList);

    for (int i = 0; i < MAX_SYMBOL_TABLE_SIZE; i++) {
        free(symbolTable[i]);
    }
}