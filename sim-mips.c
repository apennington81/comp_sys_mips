//ECE 353 Lab 3
//Samuel Burgess
//Justin Forgue
//Aric Pennington
//Elias Phillips

//For visual studio
#define _CRT_SECURE_NO_WARNINGS

// List the full names of ALL group members at the top of your code.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>
//feel free to add here any additional library names you may need 
#define SINGLE 1
#define BATCH 0
#define REG_NUM 32

char *progScanner(char *line);
char *regNumberConverter(char *ptr);
struct inst parser(char *ptr);
enum opcode { ADD, ADDI, SUB, MULT, BEQ, LW, SW };
int instCounter = 0;

//defining instuction struct
struct inst {
	int op;
	int reg1;
	int reg2;
	int reg3;
	int imm;
};

//defining instruction memory array of inst structs
//not sure what size to make this yet
struct inst instMemory[100];

main(int argc, char *argv[]) {
	int sim_mode = 0;//mode flag, 1 for single-cycle, 0 for batch
	int c, m, n;
	int i;//for loop counter
	long mips_reg[REG_NUM];
	long pgm_c = 0;//program counter
	long sim_cycle = 0;//simulation cycle counter
	//define your own counter for the usage of each pipeline stage here
	int pipeCounter = 0;
	int test_counter = 0;
	FILE *input = NULL;
	FILE *output = NULL;
	printf("The arguments are:");

	for (i = 1; i < argc; i++) {
		printf("%s ", argv[i]);
	}
	printf("\n");
	if (argc == 7) {
		if (strcmp("-s", argv[1]) == 0) {
			sim_mode = SINGLE;
		}
		else if (strcmp("-b", argv[1]) == 0) {
			sim_mode = BATCH;
		}
		else {
			printf("Wrong sim mode chosen\n");
			exit(0);
		}

		m = atoi(argv[2]);
		n = atoi(argv[3]);
		c = atoi(argv[4]);
		input = fopen(argv[5], "r");
		output = fopen(argv[6], "w");
	}

	else {
		printf("Usage: ./sim-mips -s m n c input_name output_name (single-sysle mode)\n or \n ./sim-mips -b m n c input_name  output_name(batch mode)\n");
		printf("m,n,c stand for number of cycles needed by multiplication, other operation, and memory access, respectively\n");
		exit(0);
	}
	if (input == NULL) {
		printf("Unable to open input or output file\n");
		exit(0);
	}
	if (output == NULL) {
		printf("Cannot create output file\n");
		exit(0);
	}
	//initialize registers and program counter
	if (sim_mode == 1) {
		for (i = 0; i < REG_NUM; i++) {
			mips_reg[i] = 0;
		}
	}

	//start your code from here
	//multiple line pointers are not necessary
		//they are here to make sure that the line pointer is transferring through each function correctly
		//printf("Argv 5 is %s\n", argv[5]);
	char *linePtr = NULL;
	char *correctedLinePtr = NULL;
	char *secondCorrectedLinePtr = NULL;

	//max line size should only be 100
	linePtr = malloc(100 * sizeof(char));
	correctedLinePtr = malloc(100 * sizeof(char));
	secondCorrectedLinePtr = malloc(100 * sizeof(char));

	while (fgets(linePtr, 100, input)) {
		//printf("String input is %s \n", linePtr);
		//printf("Line pointer is point to char: %c\n", *linePtr);
		//Send the input to prog scanner for corrections
		correctedLinePtr = progScanner(linePtr);
		secondCorrectedLinePtr = regNumberConverter(correctedLinePtr);
		//Print the returned pointer
		//printf("Returned line pointer from regCon to main: %s", secondCorrectedLinePtr);
		//test parset function
		struct inst temp;
		temp = parser(correctedLinePtr);
		printf("Returned struct from parser (inst, reg1, reg2, reg3, imm): %d %d %d %d %d", temp.op, temp.reg1, temp.reg2, temp.reg3, temp.imm);
		printf("\nValue from instruction memory (inst, reg1, reg2, reg3, imm): %d %d %d %d %d", instMemory[instCounter - 1].op, instMemory[instCounter - 1].reg1,
			instMemory[instCounter - 1].reg2, instMemory[instCounter - 1].reg3, instMemory[instCounter - 1].imm);

	}
	fclose(input);
}

char *progScanner(char *line){

	char originalLine[100];
	strcpy(originalLine, line);
	char *correctLine[100];
	char *segment;
	int i = 0;

	segment = strtok(originalLine," ,()");
	while (segment != NULL){
		correctLine[i++] = segment;
		segment = strtok (NULL, " ,()");
	}

	char *opcode = correctLine[0];
	int k;
	int depth = 0;
	if((strncmp(opcode, "sw", 2 ) == 0) || (strncmp(opcode, "lw", 2 )) == 0){
		for (k = 0; line[k] != 0; k++){
			depth += line[k] == '(';
			depth -= line[k] == ')';
		}
		if (depth != 0) {
			puts("Unmatched parentheses");
			exit(0);
		}

	}

	char *newLine;
	newLine = malloc(100 * sizeof(char));
	int j = 1;
	strcpy (newLine, correctLine[0]);
	while(j<i){
		strcat(newLine, " ");
		strcat(newLine, correctLine[j]);
		j++;
	}

	return newLine;
}

char *regNumberConverter(char *instruction){
    char registers1[32][5]={"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7","s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7","t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};
	char* newLine;
    int length = strlen(instruction);
    int j = 0;
    char* temp;
    char* regNumber;
    newLine = (char*)malloc(length);
    int g=0;
    for (int i = 0; i<length;i++)
    {
        //find the delimeter
        if(instruction[i]=='$')
        {
            j=i;
            //find the end
           while((instruction[j]!=' ')&&(j<length))
           {
               j++;
           }

           //copy the register into a temp var
           temp = (char*)malloc(j-(i));
           regNumber = (char*)malloc(j-(i));
           int h = 0;
           int x =0;
           for(int k = i+1;k<j;k++)
           {
               if(isdigit(instruction[k]))
                  {
                    regNumber[h]= instruction[k];
                    h++;
                  }
                    temp[x]= instruction[k];
                    x++;

           }
           regNumber[h] = '\0';
           temp[x] = '\0';
            //if register value is not already a number, make it one
            if(strcmp(temp,regNumber))
            {
                for(int q=0; q<32;q++)
                {
                    if(!strcmp(registers1[q],temp))
                    {
                    itoa(q, regNumber, 10);
                    break;
                    }
                itoa(-1, regNumber, 10);
                }

            }
            //replace the instruction with the new register value
            int length2 = strlen(regNumber);

            for(int z = 0; z<length2;z++)
            {
                newLine[g]=regNumber[z];
                g++;
            }
            i=i+x;



        //printf(regNumber);
        //check if the register value is not out of bounds
        assert(atoi(regNumber) < 32);
        assert(atoi(regNumber) != -1);

    }
      //reconstruct the instruction
    else
    {
        newLine[g]=instruction[i];
        g++;
    }
    }
    newLine[g]='\0';

	return newLine;
}

struct inst parser(char *ptr){
	//Requires input like: add 3 4 5

	//Uses output from regNumberConverter 
	//instruction as an inst struct
	//test code to make sure ptr is received
	//printf("Line input inside parser: %s", ptr);
	
	struct inst temp;
	temp.op = NULL;
	temp.reg1 = NULL;
	temp.reg2 = NULL;
	temp.reg3 = NULL;
	temp.imm = NULL;
	//int val1;
	//int val2;
	//int val3;
	//char *str = NULL;
	//str = malloc(100 * sizeof(char));
	//str = "sub";

	//modified code from class website
	int i;
	char delimiters[] = " ";  
	char ** instructionFields;

	instructionFields = (char **)malloc(100 * sizeof(char *));
	for (i = 0; i <= 4; i++)
		*(instructionFields + i) = (char *)malloc(20 * sizeof(char *));

	instructionFields[0] = strtok(ptr, delimiters);
	instructionFields[1] = strtok(NULL, delimiters);
	instructionFields[2] = strtok(NULL, delimiters);
	instructionFields[3] = strtok(NULL, delimiters);
	
	//printf("inputString[]=%s\n", ptr);
	printf("\n\nThe instruction line is: %s %s %s %s", instructionFields[0], instructionFields[1], 
		instructionFields[2], instructionFields[3]);

	//printf("The string is: %s", *str);
	//printf("opcode %s", opcode);
	//cant use switch case statements in c for strings, awesome...
	if (strcmp(instructionFields[0], "add") == 0){
		//format add rd = rs + rt
		//printf("ADD");
		temp.op = ADD;
		temp.reg1 = atoi(instructionFields[1]);
		temp.reg2 = atoi(instructionFields[2]);
		temp.reg3 = atoi(instructionFields[3]);
	}
	else if (strcmp(instructionFields[0], "addi") == 0){
		//format addi rt = rs + imm
		//printf("ADDI");
		if (atoi(instructionFields[3]) > 65535) {
			printf("Immediate field contains a number that is too large,");
			temp.imm = -1; //return -1 in the imm field to indicate and error for program to stop
			return temp;
		}

		temp.op = ADDI;
		temp.reg1 = atoi(instructionFields[1]);
		temp.reg2 = atoi(instructionFields[2]);
		temp.imm = atoi(instructionFields[3]);
	}
	else if (strcmp(instructionFields[0], "sub") == 0){
		//printf("SUB");
		//format sub rd = rs - rt
		temp.op = SUB;
		temp.reg1 = atoi(instructionFields[1]);
		temp.reg2 = atoi(instructionFields[2]);
		temp.reg3 = atoi(instructionFields[3]);
	}
	else if (strcmp(instructionFields[0], "mul") == 0){
		//format mult rd = rs * rt
		//printf("MULT");
		temp.op = MULT;
		temp.reg1 = atoi(instructionFields[1]);
		temp.reg2 = atoi(instructionFields[2]);
		temp.reg3 = atoi(instructionFields[3]);

	}
	else if (strcmp(instructionFields[0], "beq") == 0){
		//if rs = rt goto imm
		//format beq rs rt imm
		//printf("BEQ");
		if (atoi(instructionFields[3]) > 65535) {
			printf("Immediate field contains a number that is too large,");
			temp.imm = -1; //return -1 in the imm field to indicate and error for program to stop
			return temp;
		}

		temp.op = BEQ;
		temp.reg1 = atoi(instructionFields[1]);
		temp.reg2 = atoi(instructionFields[2]);
		temp.imm = atoi(instructionFields[3]);
	}
	else if (strcmp(instructionFields[0], "sw") == 0){
		//store rt into rs + imm
		//format sw rt imm rs
		//printf("SW");
		temp.op = SW;
		temp.reg1 = atoi(instructionFields[1]);
		temp.reg2 = atoi(instructionFields[3]);
		temp.imm = atoi(instructionFields[2]);
	}
	else if (strcmp(instructionFields[0], "lw") == 0){
		//load from rs + imm to rt
		//format lw rt imm rs
		//printf("LW");
		temp.op = LW;
		temp.reg1 = atoi(instructionFields[1]);
		temp.reg2 = atoi(instructionFields[3]);
		temp.imm = atoi(instructionFields[2]);
	}
	else{
		printf("\nIllegal opcode: %s", instructionFields[0]);
	}

	instMemory[instCounter] = temp;
	instCounter++;

	return temp;
}
