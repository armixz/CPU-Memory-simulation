#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

// Instruction define
//#define Load_value 				1
//#define Load_addr 				2
//#define LoadInd_addr 			3
//#define LoadIdxX_addr			4
//#define LoadIdxY_addr			5
//#define LoadSpX					6			
//#define Store_addr				7
//#define Get						8
//#define Put_port				9
//#define AddX					10
//#define AddY					11
//#define SubX					12
//#define SubY					13
//#define CopyToX					14
//#define CopyFromX				15
//#define CopyToY					16
//#define CopyFromY				17
//#define CopyToSp				18
//#define CopyFromSp				19
//#define Jump_addr				20
//#define JumpIfEqual_addr		21
//#define JumpIfNotEqual_addr		22
//#define Call_addr				23
//#define Ret						24
//#define IncX					25
//#define DecX					26
//#define Push					27
//#define Pop						28
//#define Int						29
//#define IRet					30
//#define End 					50

#define LOAD_VALUE 		1
#define LOAD_ADDR 		2
#define LOAD_IND_ADDR 	3
#define LOAD_IDX_X_ADDR	4
#define LOAD_IDX_Y_ADDR	5
#define LOAD_SP_X		6			
#define STORE_ADDR		7
#define GET				8
#define PUT_PORT		9
#define ADD_X			10
#define ADD_Y			11
#define SUB_X			12
#define SUB_Y			13
#define COPY_TO_X		14
#define COPY_FROM_X		15
#define COPY_TO_Y		16
#define COPY_FROM_Y		17
#define COPY_TO_SP		18
#define COPY_FROM_SP	19
#define JUMP_ADDR		20
#define JUMP_IF_EQUAL_ADDR		21
#define JUMP_IF_NOT_EQUAL_ADDR	22
#define CALL_ADDR		23
#define RET				24
#define INC_X			25
#define DEC_X			26
#define PUSH			27
#define POP				28
#define INT				29
#define I_RET			30
#define END 			50

#define Boolean char
#define DEFAULT_TIME_SET 1000
#define USER_MODE   0
#define KERNEL_MODE 1
#define USER_ADDRESS 	0		// Beginning position of user program
#define USER_STACK 		1000	// Beginning position of user stack, count down
#define INT_ADDRESS 	1500	// Beginning position of int instruction interrupt handler
#define TIMER_ADDRESS   1000	// Beginning position of timer interrupt handler
#define SYS_STACK 		2000	// Beginning position of system stack, count down
#define MEMORY_SIZE 2000       // Total size of memory
#define LINE_BUFFER_SIZE 100   // Max size for each line in a specific file

// Global variable to simulate CPU register
int PC, SP, IR, AC, X, Y;	// Special-function register
int COUNTER;				// Special-function register: to count the instruction CPU run so far
int counterSet;				// Set counter parameter
int memory[MEMORY_SIZE];
Boolean mode;				// USER_MODE or KERNEL_MODE


void CPUInit(void)
{
	PC = USER_ADDRESS;      // Set Program Counter to user memory
	SP = USER_STACK;        // Set Stack Pointer to user stack
	IR = 0;					// No instruction

	// Clear operand register
	AC = 0;
	X = 0;
	Y = 0;
	COUNTER = 0;            // Store the number of instructions that CPU runs so far

	mode = USER_MODE;		// USER_MODE as default value because CPU runs from 0x0

	// Get timer parameter and check the parameter
	printf("Set the timer parameter\n");
	scanf("%d", &counterSet);
	while (getchar() != '\n');    // Clear buffer in stdin
	while (counterSet <= 0)
	{   // Parameter check
		counterSet = DEFAULT_TIME_SET;	// Set count to default value
		printf("Invalid timer parameter, set to default value: %d!\n", DEFAULT_TIME_SET);
	}
}

//int readMemory(int addr, int wpd, int rpd)
//{
//	// Memory Protection
//	if (mode == USER_MODE && addr >= TIMER_ADDRESS)
//	{
//		printf("Memory violation: accessing system address %d in user mode\n", addr);
//		write(wpd, "E", sizeof(char));    // Error occur, exit processes
//		exit(-1);
//	}
//
//	int tmp;
//
//	write(wpd, "r", sizeof(char));    // Control signal
//	write(wpd, &addr, sizeof(addr));  // Address
//	read(rpd, &tmp, sizeof(tmp));     // Returned data
//
//	return tmp;
//}

int readMemory(int addr, int wpd, int rpd)
{
	// Memory Protection
	if (mode == USER_MODE && addr >= TIMER_ADDRESS)
	{
		printf("Memory violation: accessing system address %d in user mode\n", addr);
		write(wpd, "E", sizeof(char));    // Error occur, exit processes
		exit(-1);
	}

	int tmp;

	write(wpd, "r", sizeof(char));    // Control signal
	write(wpd, &addr, sizeof(addr));  // Address
	read(rpd, &tmp, sizeof(tmp));     // Returned data

	return tmp;
}

int fetch(int wtpd, int rdpd)
{
	return readMemory(PC++, wtpd, rdpd);
}



void writeMemory(int addr, int data, int wpd)
{
	// Memory Protection
	if (mode == USER_MODE && addr >= TIMER_ADDRESS)
	{
		printf("Memory violation: accessing system address %d in user mode\n", addr);
		write(wpd, "E", sizeof(char));    // Error occur, exit processes
		exit(-1);
	}

	write(wpd, "w", sizeof(char));    // Control signal 
	write(wpd, &addr, sizeof(addr));  // Address 
	write(wpd, &data, sizeof(data));  // Data
}



void MemoryInit()
{
	// Load a program into memory
	int offset = 0;

	printf("Input file name\n");
	char fileName[20];
	scanf("%s", fileName);       // Get file name from user
	while (getchar() != '\n');    // Clear rest buffer in stdin

	FILE* fp = fopen(fileName, "r");
	while (fp == NULL)			// Make sure the file open correctly.
	{
		printf("Error! File does not exist\nInput file name again!\n");
		scanf("%s", fileName);
		while (getchar() != '\n');
		fp = fopen(fileName, "r");
	}

	while (!feof(fp))	// Load the whole file into memory, end until EOF appears
	{
		char buff[LINE_BUFFER_SIZE] = "";
		fgets(buff, sizeof(buff), fp);         // Read line into buffer
		// Data line
		if (buff[0] >= '0' && buff[0] <= '9')
			memory[offset++] = atoi(buff);	   // Transfer char into instruction & store
		else if (buff[0] == '.')
			offset = atoi(&buff[1]);
	}
}

void runMemory(int wtpd, int rdpd)
{
	char command;        // 'r': read from memory; 'w': write into memory; 'E': end process
	int address, data;

	// read command from pipe
	read(rdpd, &command, sizeof(command));

	while (command != 'E')
	{
		switch (command)
		{
			// Read instruction/data from memory
		case 'r':
			read(rdpd, &address, sizeof(address));                   // Read PC from pipe
			write(wtpd, &memory[address], sizeof(memory[address]));  // Write the content of address into pipe
			break;

			// Write data into memory
		case 'w':
			read(rdpd, &address, sizeof(address));   // Read address from pipe
			read(rdpd, &data, sizeof(address));		 // Read the data that processor want to save from pipe
			memory[address] = data;
			break;

		default:
			printf("Unexpected command: %c\n", command);
			exit(-1);
			break;
		}
		read(rdpd, &command, sizeof(command));				// Read next command from CPU
	}
}

void loadMemoryTest() {
	printf(" Test Memory Now: \n");
	int offset = 0;
	printf("size of Memory is: %d\n", sizeof(memory) / sizeof(int));
	while (offset < sizeof(memory) / sizeof(int))
		printf("%d, %d\n", offset++, memory[offset]);
}


void exeInstruction(int wtpd, int rdpd)
{
	switch (IR) {
		/* Load the value into the AC*/
	case LOAD_VALUE:
		AC = fetch(wtpd, rdpd);
		break;

		/* Load the value at the address into the AC */
	case LOAD_ADDR:
	{
		int addr = fetch(wtpd, rdpd);
		AC = readMemory(addr, wtpd, rdpd);
		break;
	}

	/* Load the value from the address found in the given address into AC */
	case LOAD_IND_ADDR:
	{
		int addr = fetch(wtpd, rdpd);
		addr = readMemory(addr, wtpd, rdpd);
		AC = readMemory(addr, wtpd, rdpd);
		break;
	}

	/* Load the value at (address + X) into the AC */
	case LOAD_IDX_X_ADDR:
	{
		// Calculate address
		int addr = fetch(wtpd, rdpd) + X;				// Base address + offset
		AC = readMemory(addr, wtpd, rdpd);
		break;
	}

	/* Load the value at (address + Y) into the AC */
	/* Similar with LOAD_IDX_X_ADDR */
	case LOAD_IDX_Y_ADDR:
	{
		int addr = fetch(wtpd, rdpd) + Y;
		AC = readMemory(addr, wtpd, rdpd);
		break;
	}

	/* Load from (SP + X) into the AC */
	case LOAD_SP_X:
	{
		int addr = SP + X;
		AC = readMemory(addr, wtpd, rdpd);
		break;
	}

	/* Store the value in the AC into the address */
	case STORE_ADDR:
	{
		int addr = fetch(wtpd, rdpd);
		writeMemory(addr, AC, wtpd);
		break;
	}

	/* Gets a random int from 1 to 100 into the AC */
	case GET:
		srand(time(NULL));      // Generate the seed
		AC = rand() % 100 + 1;  // Create a random number [1, 100]
		break;

		/* Put AC to the screen*/
		/* If port = 1, writes AC as an int to the screen */
		/* If port = 2, writes AC as a char to the screen */
	case PUT_PORT:
	{
		Boolean flag = fetch(wtpd, rdpd);
		if (flag == 1)
			printf("%d", AC);
		else if (flag == 2)
			printf("%c", AC);
		else
			printf("Parameter error: %d\n", flag);
		break;
	}

	/* Add the value in X to the AC */
	case ADD_X:
		AC += X;
		break;

		/* Add the value in Y to the AC */
	case ADD_Y:
		AC += Y;
		break;

		/* Subtract the value in X from the AC */
	case SUB_X:
		AC -= X;
		break;

		/* Subtract the value in Y from the AC */
	case SUB_Y:
		AC -= Y;
		break;

		/* Copy the value in the AC to X */
	case COPY_TO_X:
		X = AC;
		break;

		/* Copy the value in X to the AC */
	case COPY_FROM_X:
		AC = X;
		break;

		/* Copy the value in the AC to Y */
	case COPY_TO_Y:
		Y = AC;
		break;

		/* Copy the value in Y to the AC */
	case COPY_FROM_Y:
		AC = Y;
		break;

		/* Copy the value in AC to the SP */
	case COPY_TO_SP:
		SP = AC;
		break;

		/* Copy the value in SP to the AC */
	case COPY_FROM_SP:
		AC = SP;
		break;

		/* Jump to the address */
	case JUMP_ADDR:
		PC = fetch(wtpd, rdpd);
		break;

		/* Jump to the address only if the value in the AC is 0 */
	case JUMP_IF_EQUAL_ADDR:
	{
		int addr = fetch(wtpd, rdpd);
		if (AC == 0)
			PC = addr;
		break;
	}

	/* Jump to the address only if the value in the AC is not 0*/
	case JUMP_IF_NOT_EQUAL_ADDR:
	{
		int addr = fetch(wtpd, rdpd);
		if (AC != 0)
			PC = addr;
		break;
	}

	/* Function call: push return address onto stack, jump to the address */
	case CALL_ADDR:
	{
		int addr = fetch(wtpd, rdpd);   // Finish the whole instruction fetch

		SP--;							// Stack is grow down
		writeMemory(SP, PC, wtpd);      // Push PC onto stack

		PC = addr;						// Jump to the address

		break;
	}

	/* Pop return address from the stack, jump to the address */
	case RET:
		PC = readMemory(SP, wtpd, rdpd); // Read current content of the stack
		SP++;			                 // Stack shrink
		break;

		/* Increment the value in X*/
	case INC_X:
		X++;
		break;

		/* Decrement the value in X */
	case DEC_X:
		X--;
		break;

		/* Push AC onto stack */
	case PUSH:
		SP--;
		writeMemory(SP, AC, wtpd);
		break;

		/* Pop from stack into AC */
	case POP:
		AC = readMemory(SP, wtpd, rdpd);
		SP++;
		break;

		/* Perform system call */
	case INT:
		if (mode == USER_MODE)
		{
			mode = KERNEL_MODE;			// Set mode to kernel mode to access interrup handler
			int tmp = SP;				// Record user stack pointer
			SP = SYS_STACK;				// Stack Pointer switch to system stack

			SP--;
			writeMemory(SP, tmp, wtpd); // Save user SP, PC into system stack
			SP--;
			writeMemory(SP, PC, wtpd);
			PC = INT_ADDRESS;			// Set PC to timer interrupt handler
			break;
		}

		/* Return from system call */
	case I_RET:
		PC = readMemory(SP, wtpd, rdpd); // Pop PC, SP
		SP++;
		SP = readMemory(SP, wtpd, rdpd);
		mode = USER_MODE;                // Change mode
		break;

		/* End execution */
	case END:
		write(wtpd, "E", sizeof(char));  // Tell memory program finished so that memory can remove the process
		break;

	default:
		printf("Invalid instruction\n");
		write(wtpd, "E", sizeof(char));    // Error occur, exit processes
		exit(-1);
		break;
	}
}

void runCPU(int wtpd, int rdpd)
{
	do {
		if (mode == USER_MODE)			// Timer works only if in user mode
			COUNTER++;

		IR = fetch(wtpd, rdpd);         // Fetch instruction to Instruction Register
		exeInstruction(wtpd, rdpd);		// Execute instruction

		// Check timer interrupt flag
		if (mode == USER_MODE && COUNTER == counterSet)
		{
			COUNTER = 0;				// Clear timer
			mode = KERNEL_MODE;			// Set mode to kernel mode to access interrupt handler

			int tmp = SP;				// Record user stack pointer
			SP = SYS_STACK;				// Stack Pointer switch to system stack
			SP--;
			writeMemory(SP, tmp, wtpd); // Save user SP into system stack
			SP--;
			writeMemory(SP, PC, wtpd);  // Save current PC into system stack
			PC = TIMER_ADDRESS;			// Set PC to timer interrupt handler		
		}
	} while (IR != END);

	// END
	write(wtpd, "E", sizeof(char));
}


int main(void)
{
	// Create pipe
	int rdpd[2];				// Read pipe descriptors, rdpd[0]: read - CPU, rdpd[1]: write - Memory
	int wtpd[2];				// Write pipe descriptors, wtpd[0]: read - Memory, wtpd[1]: write - CPU
	if (pipe(rdpd) == -1 || pipe(wtpd) == -1)  // Create pipe
	{
		printf("pipe() failed!\n");
		exit(1);
	}

	// Create two process: CPU, Memory
	int pid = fork();
	switch (pid)
	{
	case -1:
		/* Here pid is -1, the fork failed */
		/* Some possible reasons are that you're */
		/* out of process slots or virtual memory */
		printf("The fork failed!");
		exit(-1);

	case 0:
		// pid == 0 is child
		MemoryInit();
		runMemory(rdpd[1], wtpd[0]);	// Param:(write pd, read pd)
		exit(0);

	default:
		// pid > 0 is Parent
		CPUInit();
		runCPU(wtpd[1], rdpd[0]);		// Param:(write pd, read pd)
		waitpid(pid, NULL, 0);			// Waiting for memory process exit
		exit(0);
	}
}