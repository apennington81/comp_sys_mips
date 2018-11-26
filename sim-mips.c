/* Name: Samuel Burgess
 *	 	 Justin Forgue
 *		 Aric Pennington
 *		 Elias Phillips
 * Date: 11/26/2018
 * Course Number: ECE353
 * Course Name: Computer Systems Lab 3
 * Project: Pipelined Machine
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>
#define SINGLE 1
#define BATCH 0
#define REG_NUM 32


/* Prototype functions */
int str_len(char str[]);
int compare(char *str1, char *str2);
char * my_strcpy(char *strDest, const char *strSrc);
char * my_strcat(char *dest, const char *src);
char *progScanner(char *line);
char *regNumberConverter(char *instruction);
struct inst parser(char *ptr);
void IF(struct inst temp);
void ID();
void EX();
void Mem();
void WB();

/* Global Variables */
int c, m, n;
int branchPending = 0;
enum opcode { ADD, ADDI, SUB, MULT, BEQ, LW, SW, haltSim };
long mips_reg[REG_NUM];
long dataMem[512];
long pgm_c = 0; //Program counter
int halt = 0;
int IFcyc = 0;
int IDcyc = 0;
int EXcyc = 0;
int MEMcyc = 0;
int WBcyc = 0;


struct inst {
	int op;
	int reg1;
	int reg2;
	int reg3;
	int imm;
};

struct latch
{
	struct inst instruction;
	int full;
	long data;
};

/* This is how we pass data between stages */
struct latch IFID;
struct latch IDEX;
struct latch EXMEM;
struct latch MEMWB;

/*
 * Function: main
 * -------------------
 *  Sets up the arguments and opens the file to be read. Calls all of the
 *  main functions in the pipeline
 */
main(int argc, char *argv[]) {
	double ifUtil;
	double idUtil;
	double exUtil;
	double memUtil;
	double wbUtil;

	int sim_mode = 0;   //Mode flag, 1 for single-cycle, 0 for batch

	int i = 0;          //For loop counter

	long sim_cycle = 1; //Simulation cycle counter

	int test_counter = 0;
	FILE *input = NULL;
	FILE *output = NULL;
	printf("The arguments are:");

	for (i = 1;i < argc;i++) {
		printf("%s ", argv[i]);
	}
	printf("\n");
	if (argc == 7) {
		if (compare("-s", argv[1]) == 0) {
			sim_mode = SINGLE;
		}
		else if (compare("-b", argv[1]) == 0) {
			sim_mode = 0;
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

	//Initialize registers and program counter
	for (i = 0;i < REG_NUM;i++) {
		mips_reg[i] = 0;
	}

	pgm_c = 0;
	char *linePtr = NULL;
	char *correctedLinePtr = NULL;
	char *secondCorrectedLinePtr = NULL;


	linePtr = (char*)malloc(100);
	correctedLinePtr = (char*)malloc(100);
	secondCorrectedLinePtr = (char*)malloc(100);
	int t = 0;


	struct inst IM[512];

	while (fgets(linePtr, 100, input)){
		correctedLinePtr = progScanner(linePtr);
		secondCorrectedLinePtr = regNumberConverter(correctedLinePtr);
		IM[t] = parser(secondCorrectedLinePtr);
		t++;
	}

	while (!halt) {
		WB();
		wbUtil = (double)WBcyc / (double)sim_cycle;
		Mem();
		memUtil = (double)MEMcyc / (double)sim_cycle;
		EX();
		exUtil = (double)EXcyc / (double)sim_cycle;
		ID();
		idUtil = (double)IDcyc / (double)sim_cycle;
		IF(IM[pgm_c]);
		ifUtil = (double)IFcyc / (double)sim_cycle;

		/*  Output code 2: the following code will output the register
         *  value to screen at every cycle and wait for the ENTER key
         *  to be pressed; this will make it proceed to the next cycle
         */
		if (sim_mode == 1) {
			printf("cycle: %d register value: ", sim_cycle);
			for (i = 1;i < REG_NUM;i++) {
				printf("%d  ", mips_reg[i]);
			}
			printf("program counter: %d\n", pgm_c*4);
			printf("press ENTER to continue\n");
			while (getchar() != '\n');

		}
		sim_cycle++;    //Increase cycle count each time

	}

	if (sim_mode == 0) {
		fprintf(output, "program name: %s\n", argv[5]);
		fprintf(output, "stage utilization: %f  %f  %f  %f  %f \n", ifUtil, idUtil, exUtil, memUtil, wbUtil);
		// Add the (double) stage_counter/sim_cycle for each
		// Stage following sequence IF ID EX MEM WB
		fprintf(output, "register values ");
		for (i = 1;i < REG_NUM;i++) {
			fprintf(output, "%d  ", mips_reg[i]);
		}
		fprintf(output, "%d\n", pgm_c*4);

	}
	//Close input and output files at the end of the simulation
	fclose(input);
	fclose(output);
	return 0;
}

/*
 * Function: str_len
 * -------------------
 *  Finds the length of the string
 *  returns: The length of the string
 */
int str_len(char str[]) {
	int i;
	for (i = 0; str[i] != '\0';++i);

	return i;
}


/*
 * Function: compare
 * -------------------
 *  Compare two strings to see if they are equal or not
 *  returns: return 0 if they are equal otherwise non-zero
 */
int compare(char *str1, char *str2) {
	while (*str1 && *str1 == *str2) {
		str1++;
		str2++;
	}
	return *str1 - *str2;
}


/*
 * Function: my_strcpy
 * -------------------
 *  Copy a string
 *  returns: The copied string
 */
char * my_strcpy(char *strDest, const char *strSrc){
	assert(strDest != NULL && strSrc != NULL);
	char *temp = strDest;
	while (*strDest++ = *strSrc++);
	return temp;
}

/*
 * Function: my_strcat
 * -------------------
 *  Appends a copy of the string to another string
 *  returns: string
 */
char * my_strcat(char *dest, const char *src){
	char *rdest = dest;

	while (*dest)
		dest++;
	while (*dest++ = *src++)
		;
	return rdest;
}

/*
 * Function: progScanner
 * -------------------
 *  Takes in each line and breaks it into segments
 *  with strtok at spaces, commas, and parentheses
 *  reassembles with only spaces between words
 *  Stops program if unmatched parentheses are found
 *  returns: Pointer to a character
 */
char *progScanner(char *line) {

	char originalLine[100];
	my_strcpy(originalLine, line);
	char *correctLine[100];
	char *segment;
	int i = 0;

	segment = strtok(originalLine, " ,()");
	while (segment != NULL) {
		correctLine[i++] = segment;
		segment = strtok(NULL, " ,()");
	}

	char *opcode = correctLine[0];
	int k;
	int depth = 0;
	if ((strncmp(opcode, "sw", 2) == 0) || (strncmp(opcode, "lw", 2)) == 0) {
		for (k = 0; line[k] != 0; k++) {
			depth += line[k] == '(';
			depth -= line[k] == ')';
		}
		if (depth != 0) {
			puts("Unmatched parentheses");
			exit(0);
		}

	}

	char *newLine;
	newLine = (char*)malloc(100);
	int j = 1;
	my_strcpy(newLine, correctLine[0]);
	while (j < i) {
		my_strcat(newLine, " ");
		my_strcat(newLine, correctLine[j]);
		j++;
	}

	return newLine;
}

/*
 * Function: regNumberConverter
 * -------------------
 *  Converts register names to numbers
 *  returns: Pointer to a character
 */
char *regNumberConverter(char *instruction) {
	char *registers1[16] = { "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7"};
	char *registers2[16] = {"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7","t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra" };
	char* newLine;
	int length = strlen(instruction);
	int j = 0;
	char* temp;
	char* regNumber;
	newLine = (char*)malloc(length);
	int g = 0;
	int i = 0;
	for (i = 0; i < length;i++)
	{
		//Find the delimeter
		if (instruction[i] == '$')
		{
			j = i;
			//Find the end
			while ((instruction[j] != ' ') && (j < length - 1))
			{
				j++;
			}

			//Copy the register into a temp var
			temp = (char*)malloc(j - (i));
			regNumber = (char*)malloc(j - (i));
			int h = 0;
			int x = 0;
			int k = 0;
			for (k = i + 1;k < j;k++)
			{
				if (isdigit(instruction[k]))
				{
					regNumber[h] = instruction[k];
					h++;
				}
				temp[x] = instruction[k];
				x++;

			}
			regNumber[h] = '\0';
			temp[x] = '\0';
			//If register value is not already a number, make it one
			if (compare(temp, regNumber))
			{
				int q = 0;
				for (q = 0; q < 16;q++)
				{
					if (!compare(registers1[q], temp))
					{
						snprintf(regNumber, sizeof(regNumber), "%d", q);
						break;
					}

					if (!compare(registers2[q], temp))
					{
						snprintf(regNumber, sizeof(regNumber), "%d", q+16);
						break;
					}
					snprintf(regNumber, sizeof(regNumber), "%d", -1);
				}
			}
			//Replace the instruction with the new register value
			int length2 = strlen(regNumber);
			int z = 0;
			for (z = 0; z < length2;z++)
			{
				newLine[g] = regNumber[z];
				g++;
			}
			i = i + x;
			//Check if the register value is not out of bounds
			assert(atoi(regNumber) < 32);
			assert(atoi(regNumber) != -1);

		}
			//Reconstruct the instruction
		else
		{
			newLine[g] = instruction[i];
			g++;
		}
	}
	newLine[g] = '\0';
	return newLine;
}

/*
 * Function: parser
 * -------------------
 *  Requires input like: add 3 4 5
 *  Uses output from regNumberConverter
 *  instruction as an inst struct
 *  test code to make sure ptr is received
 */
struct inst parser(char *ptr) {
	struct inst temp;
	temp.op = 0;
	temp.reg1 = 0;
	temp.reg2 = 0;
	temp.reg3 = 0;
	temp.imm = 0;

	//Modified code from class website
	int i = 0;
	char delimiters[] = " ";
	char ** instructionFields;

	instructionFields = (char **)malloc(100 * sizeof(char *));
	for (i = 0; i <= 4; i++)
		*(instructionFields + i) = (char *)malloc(20 * sizeof(char *));

	instructionFields[0] = strtok(ptr, delimiters);
	instructionFields[1] = strtok(NULL, delimiters);
	instructionFields[2] = strtok(NULL, delimiters);
	instructionFields[3] = strtok(NULL, delimiters);

	//Can't use switch case statements in c for strings, awesome...

	if (compare(instructionFields[0], "add") == 0) {
		temp.op = ADD;
		temp.reg1 = atoi(instructionFields[1]);
		temp.reg2 = atoi(instructionFields[2]);
		temp.reg3 = atoi(instructionFields[3]);
		assert(temp.reg1 != 0);

	}
	else if (compare(instructionFields[0], "addi") == 0) {
		if (atoi(instructionFields[3]) > 65535) {
			printf("Immediate field contains a number that is too large,");
			temp.imm = -1;  //Return -1 in the imm field to indicate and error for program to stop
			return temp;
		}

		temp.op = ADDI;
		temp.reg1 = atoi(instructionFields[1]);
		temp.reg2 = atoi(instructionFields[2]);
		temp.imm = atoi(instructionFields[3]);
		assert(temp.reg1 != 0);
	}
	else if (compare(instructionFields[0], "sub") == 0) {
		temp.op = SUB;
		temp.reg1 = atoi(instructionFields[1]);
		temp.reg2 = atoi(instructionFields[2]);
		temp.reg3 = atoi(instructionFields[3]);
		assert(temp.reg1 != 0);
	}
	else if (compare(instructionFields[0], "mul") == 0) {
		temp.op = MULT;
		temp.reg1 = atoi(instructionFields[1]);
		temp.reg2 = atoi(instructionFields[2]);
		temp.reg3 = atoi(instructionFields[3]);
		assert(temp.reg1 != 0);

	}
	else if (compare(instructionFields[0], "beq") == 0) {;
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
	else if (compare(instructionFields[0], "sw") == 0) {;
		temp.op = SW;
		temp.reg1 = atoi(instructionFields[1]);
		temp.reg2 = atoi(instructionFields[3]);
		temp.imm = atoi(instructionFields[2]);
		assert(temp.imm % 4 == 0);
		temp.imm = temp.imm / 4;
	}
	else if (compare(instructionFields[0], "lw") == 0) {
		temp.op = LW;
		temp.reg1 = atoi(instructionFields[1]);
		temp.reg2 = atoi(instructionFields[3]);
		temp.imm = atoi(instructionFields[2]);
		assert(temp.imm % 4 == 0);
		temp.imm = temp.imm / 4;
		assert(temp.reg1 != 0);
	}
	else if (compare(ptr, "haltSimulation") == 0)
	{
		temp.op = haltSim;
	}
	else {
		printf("\nIllegal opcode: %s", instructionFields[0]);
	}
	return temp;
}

/*
 * Function: IF
 * -------------------
 *  Fetches the instruction. Also waits for the stage to be free
 *  before allowing another instruction into the IF stage
 *  returns: nothing
 */
void IF(struct inst temp){
	static int IFstall;
	IFstall = 1;
	if (!IFID.full && !branchPending)
	{
		if (IFstall < c)
		{
			IFstall++;
		}
		else
		{
			IFstall = 1;
			IFID.instruction = temp;
			IFID.full = 1;
			pgm_c++;
			IFcyc += c;
		}
	}
}

/*
 * Function: ID
 * -------------------
 *  Decodes the instruction and
 *  checks for data and control hazards
 *  returns: nothing
 */
void ID(){
	int dataHazard;
	dataHazard = 0;
	if (!IDEX.full&&IFID.full)
	{
		if (EXMEM.instruction.reg1 == IFID.instruction.reg2&&EXMEM.full)
			dataHazard = 1;
		if (MEMWB.instruction.reg1 == IFID.instruction.reg2&&MEMWB.full)
			dataHazard = 1;
		if (EXMEM.instruction.reg1 == IFID.instruction.reg3&&EXMEM.full)
			dataHazard = 1;
		if (MEMWB.instruction.reg1 == IFID.instruction.reg3&&MEMWB.full)
			dataHazard = 1;
		if (IFID.instruction.op == ADD&&dataHazard == 0)
		{
			IDEX.instruction.op = ADD;
			IDEX.instruction.reg3 = mips_reg[IFID.instruction.reg3];
			IDEX.instruction.reg2 = mips_reg[IFID.instruction.reg2];
			IDEX.instruction.reg1 = IFID.instruction.reg1;
			IFID.full = 0;
			IDEX.full = 1;
			IDcyc++;

		}
		else if (IFID.instruction.op == ADDI && dataHazard == 0)
		{
			IDEX.instruction.op = ADDI;
			IDEX.instruction.imm = IFID.instruction.imm;
			IDEX.instruction.reg2 = mips_reg[IFID.instruction.reg2];
			IDEX.instruction.reg1 = IFID.instruction.reg1;
			IFID.full = 0;
			IDEX.full = 1;
			IDcyc++;
		}
		else if (IFID.instruction.op == SUB && dataHazard == 0)
		{

			IDEX.instruction.op = SUB;
			IDEX.instruction.reg3 = mips_reg[IFID.instruction.reg3];
			IDEX.instruction.reg2 = mips_reg[IFID.instruction.reg2];
			IDEX.instruction.reg1 = IFID.instruction.reg1;
			IFID.full = 0;
			IDEX.full = 1;
			IDcyc++;
		}
		else if (IFID.instruction.op == MULT && dataHazard == 0)
		{
			IDEX.instruction.op = MULT;
			IDEX.instruction.reg3 = mips_reg[IFID.instruction.reg3];
			IDEX.instruction.reg2 = mips_reg[IFID.instruction.reg2];
			IDEX.instruction.reg1 = IFID.instruction.reg1;
			IFID.full = 0;
			IDEX.full = 1;
			IDcyc++;
		}
		else if (IFID.instruction.op == LW && dataHazard == 0)
		{
			IDEX.instruction.op = LW;
			IDEX.instruction.reg1 = IFID.instruction.reg1;
			IDEX.instruction.reg2 = mips_reg[IFID.instruction.reg2];
			IDEX.instruction.imm = IFID.instruction.imm;
			IFID.full = 0;
			IDEX.full = 1;
			IDcyc++;
		}
		else if (IFID.instruction.op == SW && dataHazard == 0)
		{
			if (EXMEM.instruction.reg1 != IFID.instruction.reg1 || !EXMEM.full)
			{
				if (MEMWB.instruction.reg1 != IFID.instruction.reg1 || !MEMWB.full)
				{
					IDEX.instruction.op = SW;
					IDEX.instruction.reg1 = mips_reg[IFID.instruction.reg1];
					IDEX.instruction.reg2 = mips_reg[IFID.instruction.reg2];
					IDEX.instruction.imm = IFID.instruction.imm;
					IFID.full = 0;
					IDEX.full = 1;
					IDcyc++;
				}
			}

		}
		else if (IFID.instruction.op == BEQ && dataHazard == 0)
		{
			if (EXMEM.instruction.reg1 != IFID.instruction.reg1 || !EXMEM.full)
			{
				if (MEMWB.instruction.reg1 != IFID.instruction.reg1 || !MEMWB.full)
				{
					branchPending = 1;
					IDEX.instruction.op = BEQ;
					IDEX.instruction.reg1 = mips_reg[IFID.instruction.reg1];
					IDEX.instruction.reg2 = mips_reg[IFID.instruction.reg2];
					IDEX.instruction.imm = IFID.instruction.imm;
					IFID.full = 0;
					IDEX.full = 1;
					IDcyc++;
				}
			}
		}
		else if (IFID.instruction.op == haltSim)
		{
			IDEX.instruction.op = haltSim;
			IDEX.full = 1;
		}

	}
}

/*
 * Function: EX
 * -------------------
 *  Executes the intruction
 *  returns: nothing
 */
void EX(){
	static int stall = 1;
	if (!EXMEM.full&&IDEX.full)
	{
		if (IDEX.instruction.op == haltSim)
		{
			EXMEM.instruction.op = haltSim;
			EXMEM.full = 1;
			IDEX.full = 0;
		}

		if (IDEX.instruction.op == ADD)
		{
			if (stall < n)
			{
				stall++;
			}
			else
			{
				stall = 1;
				EXMEM.instruction.op = ADD;
				EXMEM.data = IDEX.instruction.reg2 + IDEX.instruction.reg3;
				EXMEM.instruction.reg1 = IDEX.instruction.reg1;
				EXMEM.full = 1;
				IDEX.full = 0;
				EXcyc += n;
			}
		}
		if (IDEX.instruction.op == SUB)
		{
			if (stall < n)
			{
				stall++;
			}
			else
			{
				stall = 1;
				EXMEM.instruction.op = SUB;
				EXMEM.data = IDEX.instruction.reg2 - IDEX.instruction.reg3;
				EXMEM.instruction.reg1 = IDEX.instruction.reg1;
				EXMEM.full = 1;
				IDEX.full = 0;
				EXcyc += n;
			}
		}
		if (IDEX.instruction.op == ADDI)
		{
			if (stall < n)
			{
				stall++;
			}
			else
			{
				stall = 1;
				EXMEM.instruction.op = ADDI;
				EXMEM.data = IDEX.instruction.reg2 + IDEX.instruction.imm;
				EXMEM.instruction.reg1 = IDEX.instruction.reg1;
				EXMEM.full = 1;
				IDEX.full = 0;
				EXcyc += n;
			}
		}
		if (IDEX.instruction.op == BEQ)
		{
			if (stall < n)
			{
				stall++;
			}
			else
			{
				stall = 1;
				EXMEM.instruction.op = BEQ;
				if (IDEX.instruction.reg2 - IDEX.instruction.reg1 == 0)
				{
					pgm_c = pgm_c + IDEX.instruction.imm;
				}
				branchPending = 0;
				EXMEM.full = 1;
				IDEX.full = 0;
				EXcyc += n;
			}

		}
		if (IDEX.instruction.op == SW)
		{
			if (stall < n)
			{
				stall++;
			}
			else
			{
				stall = 1;
				EXMEM.instruction.op = SW;
				EXMEM.data = IDEX.instruction.reg2 + IDEX.instruction.imm;
				EXMEM.instruction.reg1 = IDEX.instruction.reg1;
				EXMEM.full = 1;
				IDEX.full = 0;
				EXcyc += n;
			}

		}
		if (IDEX.instruction.op == LW)
		{
			if (stall < n)
			{
				stall++;
			}
			else
			{
				stall = 1;
				EXMEM.instruction.op = LW;
				EXMEM.data = IDEX.instruction.reg2 + IDEX.instruction.imm;
				EXMEM.instruction.reg1 = IDEX.instruction.reg1;
				EXMEM.full = 1;
				IDEX.full = 0;
				EXcyc += n;
			}

		}
		if (IDEX.instruction.op == MULT)
		{
			if (stall < m)
			{
				stall++;
			}
			else
			{
				stall = 1;
				EXMEM.instruction.op = MULT;
				EXMEM.data = IDEX.instruction.reg2*IDEX.instruction.reg3;
				EXMEM.instruction.reg1 = IDEX.instruction.reg1;
				EXMEM.full = 1;
				IDEX.full = 0;
				EXcyc += m;
			}
		}

	}

}

/*
 * Function: Mem
 * -------------------
 *  Read or writes from memory for lw and sw
 *  returns: nothing
 */
void Mem(){
	static int memstall = 1;
	if (EXMEM.full && !MEMWB.full)
	{
		if (EXMEM.instruction.op == SW)
		{
			if (memstall < c)
			{
				memstall++;
			}
			else
			{
				memstall = 1;
				dataMem[EXMEM.data] = EXMEM.instruction.reg1;
				MEMWB.instruction.op = SW;
				MEMWB.full = 1;
				EXMEM.full = 0;
				MEMcyc += c;
			}
			//pull data out of memory

		}
		else if (EXMEM.instruction.op == LW)
		{
			if (memstall < c)
			{
				memstall++;
			}
			else
			{
				memstall = 1;
				MEMWB.data = dataMem[EXMEM.data];
				MEMWB.instruction.reg1 = EXMEM.instruction.reg1;
				MEMWB.instruction.op = LW;
				MEMWB.full = 1;
				EXMEM.full = 0;
				MEMcyc += c;
			}

		}
		else
		{
			MEMWB.instruction.op = EXMEM.instruction.op;
			MEMWB.instruction.reg1 = EXMEM.instruction.reg1;
			MEMWB.instruction.reg2 = EXMEM.instruction.reg2;
			MEMWB.instruction.reg3 = EXMEM.instruction.reg3;
			MEMWB.instruction.imm = EXMEM.instruction.imm;
			MEMWB.data = EXMEM.data;
			MEMWB.full = 1;
			EXMEM.full = 0;

		}

	}

}

/*
 * Function: WB
 * -------------------
 *  Updates the register contents
 *  returns: nothing
 */
void WB(){
	if (MEMWB.full)
	{
		if (MEMWB.instruction.op == ADD || MEMWB.instruction.op == SUB || MEMWB.instruction.op == ADDI || MEMWB.instruction.op == MULT || MEMWB.instruction.op == LW)
		{
			mips_reg[MEMWB.instruction.reg1] = MEMWB.data;
			WBcyc++;
		}
		if (MEMWB.instruction.op == haltSim)
			halt = 1;

		MEMWB.full = 0;
	}

}
