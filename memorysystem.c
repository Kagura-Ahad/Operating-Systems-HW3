#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MEMSIZE 500
#define MAX_STACK_SIZE 300
#define MAX_HEAP_SIZE 300
#define MAX_NUM_OF_FRAMES 5
#define MIN_FRAME_SIZE 10
#define MAX_FRAME_SIZE 80
#define INT_VARIABLE_SIZE 4
#define DOUBLE_VARIABLE_SIZE 8
#define CHAR_VARIABLE_SIZE 1

struct framestatus {
	int number;               // frame number
	char name[8];             // function name representing the frame
	int functionaddress;      // address of function in code section (will be randomly generated in this case)
	int frameaddress;         // starting address of frame belonging to this header in Stack
	int used;                 // a boolean value indicating wheter the frame status entry is in use or not
};

struct allocated {
	char name[8];
	int startaddress;
	struct allocated * next;
};

struct freelist {
	int start;         // start address of free region
	int size;                 // size of free region
	struct freelist * next;   // pointer to next free region
};

struct var_tracker {
	int index_of_lower_occupancy;
	int index_of_higher_occupancy;
};

int extractInteger(char arr[], int size);

// Function to print the contents of the linked list
void printFreelist(struct freelist *headFreeList) {
    struct freelist *currentNode = headFreeList;

    printf("Contents of the Free list:\n");

    // Traverse the linked list and print the contents of each node
    while (currentNode != NULL) {
        printf("Start Address: %d \n Size: %d\n", currentNode->start, currentNode->size);
        currentNode = currentNode->next;
    }
}

void printAllocatedList(struct allocated *headAllocatedList, char *memory) {
    struct allocated *currentNode = headAllocatedList;

    printf("Contents of the allocated list:\n");

    // Traverse the linked list and print the contents of each node
    while (currentNode != NULL) {
		char sizeOfThisAllocatedRegionInCharArray[] = {memory[currentNode->startaddress - 8], memory[currentNode->startaddress - 7], memory[currentNode->startaddress - 6], memory[currentNode->startaddress - 5]};
		int sizeOfThisAllocatedRegion = extractInteger(sizeOfThisAllocatedRegionInCharArray, INT_VARIABLE_SIZE);
        printf("Name: %s \n Start Address: %d\n Size: %d\n", currentNode->name, currentNode->startaddress, sizeOfThisAllocatedRegion);
        currentNode = currentNode->next;
    }
}

// Function to check if the the frame on top of the stack has occupied the minimum space required for a frame
int is_callable(char *memory, int *current_stack_pointer)
{
	for (int i = *current_stack_pointer - MIN_FRAME_SIZE; i <= *current_stack_pointer; ++i)
	{
		if (memory[i] == 'U')
		{
			return 1;
		}
	}
	return 0;
}

int countDecimalPlaces(double fraction) {
    int count = 0;
    while (1) {
        count++;
        fraction *= 10;
        fraction = fraction - (int)fraction;
        if (fraction < 0.00001) {
            break;
        }
    }
    return count;
}

int getIntegerLength(int number) {
    int length = 0;
    while (number != 0) {
        number /= 10;
        length++;
    }
    return length;
}

void calculateDivisor(int number, int *divisor) {
    *divisor = 1;
    while (number / *divisor >= 10) {
        *divisor *= 10;
    }
}

void appendDigitsToArray(int number, int divisor, char *array, int *index) {
    while (divisor > 0) {
        int digit = (number / divisor) % 10;
        array[*index] = '0' + digit;
        (*index)++;
        divisor /= 10;
    }
}

void doubleToCharArray(double num, char *castedIntValue, int is_negative) {
    int integerPart = (int)num;
    double decimalPart = num - integerPart;

    int intPartLength = getIntegerLength(integerPart);
    int decimalPartLength = countDecimalPlaces(decimalPart);

    for (int i = 0; i < decimalPartLength; i++) {
        decimalPart *= 10;
    }
    int decimalPartInt = (int)decimalPart;

    int totalLength = intPartLength + decimalPartLength;

    int divisorForIntPart, divisorForDecimalPart;
    calculateDivisor(integerPart, &divisorForIntPart);
    calculateDivisor(decimalPartInt, &divisorForDecimalPart);

    int arrIndex = 8 - totalLength - 1;
	if (is_negative == 1)
	{
		castedIntValue[arrIndex - 1] = '-';
	}
    appendDigitsToArray(integerPart, divisorForIntPart, castedIntValue, &arrIndex);
    castedIntValue[arrIndex] = '.';
    arrIndex++;
    appendDigitsToArray(decimalPartInt, divisorForDecimalPart, castedIntValue, &arrIndex);
}

void intToCharArray(int num, char* casted_int_value, int is_negative)
{
    int length = getIntegerLength(num);

    int divisor = 1;
    while (num / divisor >= 10)
    {
        divisor *= 10;
    }

    int arr_index = 4 - length;
	if (is_negative == 1)
	{
		casted_int_value[arr_index - 1] = '-';
	}
    while (divisor > 0)
    {
        int digit = (num / divisor) % 10;
        casted_int_value[arr_index] = '0' + digit;
        arr_index += 1;
        divisor /= 10;
    }
}

int extractInteger(char arr[], int size)
{
  int number = 0;
  for (int i = 0; i < size; i++)
  {
    if (arr[i] >= '0' && arr[i] <= '9')
    {
      number = number * 10 + (arr[i] - '0');
    }
  }
  return number;
}

void createFrame(char *functionName, char *functionAddress, char *memory, int *frame_number, struct var_tracker **variables_of_frames_on_stack_tracker, int *current_stack_pointer)
{
	//updating the number of frames on stack
	(*frame_number) = (*frame_number) + 1;

	// Allocate memory for an instance of var_tracker struct on the heap
	struct var_tracker *new_var_tracker = (struct var_tracker *)malloc(sizeof(struct var_tracker));

	// Initialize the integers in the var_tracker struct to proper positions
	new_var_tracker->index_of_lower_occupancy = *current_stack_pointer;
    new_var_tracker->index_of_higher_occupancy = *current_stack_pointer - 1;

    // Store the address of the new_var_tracker struct into the first index of ptr_array
    variables_of_frames_on_stack_tracker[(*frame_number) - 1] = new_var_tracker;

	//labelling the minimum size of frame number of bytes of memory alloted for this frame as unused
	for (int i = *current_stack_pointer - MIN_FRAME_SIZE; i < *current_stack_pointer; ++i)
	{
        memory[i] = 'U';
    }

	//cast frame number to char array
	char casted_frame_number[4] = {'0','0','0','0'};
	intToCharArray(*frame_number, casted_frame_number, 0);

	//function name is already a char array

	//function address is already a char array

	//cast frame address to char array
	char casted_frame_address[4] = {'0','0','0','0'};
	intToCharArray(*current_stack_pointer, casted_frame_address, 0); 

	//cast used to char
	char casted_used = '1';

	//copying the frame number of this frame onto the frame list
	memcpy(&memory[(MEMSIZE - (*frame_number * 21)) + 0], casted_frame_number, 4);

	//copying the function name of this frame onto the frame list
	memcpy(&memory[(MEMSIZE - (*frame_number * 21)) + 4], functionName, 8);

	//copying the function address of this frame onto the frame list
	memcpy(&memory[(MEMSIZE - (*frame_number * 21)) + 12], functionAddress, 4);

	//copying the frame address of this frame onto the frame list
	memcpy(&memory[(MEMSIZE - (*frame_number * 21)) + 16], casted_frame_address, 4);

	//copying the used of this frame onto the frame list
	memcpy(&memory[(MEMSIZE - (*frame_number * 21)) + 20], &casted_used, 1);

	//marking the frames below this frame as not on top of stack
	int top_of_stack_bit_correcter = MEMSIZE - 1;
	for (int i = 0; i < *frame_number - 1; ++i)
	{
		memory[top_of_stack_bit_correcter] = '0';
		top_of_stack_bit_correcter -= 21;
	}
}

void create_integer_local_variable(char *variable_name, int int_value, char *memory, int *frame_number_of_the_frame_which_is_on_top_of_the_stack, struct var_tracker **variables_of_frames_on_stack_tracker, int *current_stack_pointer)
{
	//cast integer value to char array
	int is_negative = 0;
	char casted_int_value[INT_VARIABLE_SIZE] = {'0', '0', '0', '0'};
	if (int_value < 0)
	{
		is_negative = 1;
		casted_int_value[0] = ' ';
		casted_int_value[1] = ' ';
		casted_int_value[2] = ' ';
		casted_int_value[3] = ' ';
		int_value = int_value * -1;
	}
	intToCharArray(int_value, casted_int_value, is_negative);

	//copying the variable value of this frame onto the frame list
	memcpy(&memory[*current_stack_pointer - INT_VARIABLE_SIZE], casted_int_value, INT_VARIABLE_SIZE);

	// Modifying the value of relevant pointers
	*current_stack_pointer = *current_stack_pointer - INT_VARIABLE_SIZE;
	variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1]->index_of_lower_occupancy = variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1]->index_of_lower_occupancy - INT_VARIABLE_SIZE;
}

void create_double_local_variable(char *variable_name, double double_value, char *memory, int *frame_number_of_the_frame_which_is_on_top_of_the_stack, struct var_tracker **variables_of_frames_on_stack_tracker, int *current_stack_pointer)
{
	//TODO: condition to check if the given double value is negative or not and if it is negative then it shouldnt be greater than 7 bytes and if it is positive then it shouldnt be greater than 8 bytes

	//cast double value to char array
	int is_negative = 0;
	char casted_double_value[DOUBLE_VARIABLE_SIZE] = {'0', '0', '0', '0', '0', '0', '0', '0'};
	if (double_value < 0)
	{
		is_negative = 1;
		casted_double_value[0] = ' ';
		casted_double_value[1] = ' ';
		casted_double_value[2] = ' ';
		casted_double_value[3] = ' ';
		casted_double_value[4] = ' ';
		casted_double_value[5] = ' ';
		casted_double_value[6] = ' ';
		casted_double_value[7] = ' ';
		double_value = double_value * -1;
	}
	doubleToCharArray(double_value, casted_double_value, is_negative);

	//copying the variable value of this frame onto the frame list
	memcpy(&memory[*current_stack_pointer - DOUBLE_VARIABLE_SIZE], casted_double_value, DOUBLE_VARIABLE_SIZE);

	// Modifying the value of relevant pointers
	*current_stack_pointer = *current_stack_pointer - DOUBLE_VARIABLE_SIZE;
	variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1]->index_of_lower_occupancy = variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1]->index_of_lower_occupancy - DOUBLE_VARIABLE_SIZE;
}

void create_char_local_variable(char *variable_name, char char_value, char *memory, int *frame_number_of_the_frame_which_is_on_top_of_the_stack, struct var_tracker **variables_of_frames_on_stack_tracker, int *current_stack_pointer)
{
	//copying the variable value of this frame onto the frame list
	memcpy(&memory[*current_stack_pointer - CHAR_VARIABLE_SIZE], &char_value, CHAR_VARIABLE_SIZE);

	// Modifying the value of relevant pointers
	*current_stack_pointer = *current_stack_pointer - CHAR_VARIABLE_SIZE;
	variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1]->index_of_lower_occupancy = variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1]->index_of_lower_occupancy - CHAR_VARIABLE_SIZE;
}

void deleteFrame(char *memory, int *frame_number_of_the_frame_which_is_on_top_of_the_stack, struct var_tracker **variables_of_frames_on_stack_tracker, int *current_stack_pointer)
{
	//for the case when the frame being deleted has occupied less space than minimum space required for a frame
	for (int i = variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1]->index_of_higher_occupancy - 9; i < variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1]->index_of_lower_occupancy; ++i)
	{
		memory[i] = ' ';
	}
	
	//marking the memory used by the frame on top of the stack as free
	for (int i = variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1]->index_of_lower_occupancy; i <= variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1]->index_of_higher_occupancy; ++i)
	{
		memory[i] = ' ';
	}

	//updating the stack pointer
	*current_stack_pointer = variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1]->index_of_higher_occupancy + 1;

	// deallocating the memory of the var_tracker struct of the frame which is on top of the stack
	variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1]->index_of_lower_occupancy = 0;
	variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1]->index_of_higher_occupancy = 0;
	free(variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1]);

	//setting the pointer to the var_tracker struct of the frame which is on top of the stack to NULL
	variables_of_frames_on_stack_tracker[(*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1] = NULL;

	//marking the frame below the frame which is on top of the stack as the top of the stack
	if (*frame_number_of_the_frame_which_is_on_top_of_the_stack > 1)
	{
		memory[MEMSIZE - (((*frame_number_of_the_frame_which_is_on_top_of_the_stack) - 1) * 21) + 20] = '1';
	}

	//clearing the metadata of the frame which is on top of the stack
	for (int i = 0; i < 21; ++i)
	{
		memory[MEMSIZE - ((*frame_number_of_the_frame_which_is_on_top_of_the_stack) * 21) + i] = ' ';
	}

	//updating the counter which keeps track of the which frame is on top of the stack
	*frame_number_of_the_frame_which_is_on_top_of_the_stack = *frame_number_of_the_frame_which_is_on_top_of_the_stack - 1;
}

void create_character_buffer_on_heap(struct freelist** head, struct allocated** headAllocatedList, char *variable_name, int size_of_buffer, char *memory, int *current_stack_pointer, int *frame_number_of_the_frame_which_is_on_top_of_the_stack, struct var_tracker **variables_of_frames_on_stack_tracker)
{
	struct freelist * freeListTraverser = *head;
	while (freeListTraverser != NULL)
	{
		if (freeListTraverser->size >= size_of_buffer + 8)
		{
			//creating a new node for the allocated list
			struct allocated * newAllocatedNode = (struct allocated *)malloc(sizeof(struct allocated));
			strcpy(newAllocatedNode->name, variable_name);
			newAllocatedNode->startaddress = freeListTraverser->start + 8;
			newAllocatedNode->next = NULL;

			//adding the new node to the allocated list
			if (*headAllocatedList == NULL)
			{
				*headAllocatedList = newAllocatedNode;
			}
			else
			{
				struct allocated * allocatedListTraverser = *headAllocatedList;
				while (allocatedListTraverser->next != NULL)
				{
					allocatedListTraverser = allocatedListTraverser->next;
				}
				allocatedListTraverser->next = newAllocatedNode;
			}

			//copying the size of the buffer and the magic number into the memory right before the data of the buffer
			char casted_size_of_buffer[4] = {'0', '0', '0', '0'};
			intToCharArray(size_of_buffer, casted_size_of_buffer, 0);
			memccpy(&memory[freeListTraverser->start], casted_size_of_buffer, 4, 4);

			char casted_magic_number[4] = {'0', '0', '0', '0'};
			int magic_number = rand() % 10000;
			intToCharArray(magic_number, casted_magic_number, 0);
			memccpy(&memory[freeListTraverser->start + 4], casted_magic_number, 4, 4);

			create_integer_local_variable(variable_name, freeListTraverser->start + 8, memory, frame_number_of_the_frame_which_is_on_top_of_the_stack, variables_of_frames_on_stack_tracker, current_stack_pointer);

			//marking the memory used by the buffer as allocated
			for (int i = newAllocatedNode->startaddress; i < newAllocatedNode->startaddress + size_of_buffer; ++i)
			{
				memory[i] = 'A';
			}

			//updating the free list
			if (freeListTraverser->size == size_of_buffer + 8)
			{
				if (freeListTraverser == *head)
				{
					*head = freeListTraverser->next;
					if (*head == NULL)
					{
						*head = (struct freelist*)malloc(sizeof(struct freelist));
						(*head)->start = newAllocatedNode->startaddress + size_of_buffer;
						(*head)->size = 0;
						(*head)->next = NULL;
					}
				}
				else
				{
					struct freelist * freeListTraverser2 = *head;
					while (freeListTraverser2->next != freeListTraverser)
					{
						freeListTraverser2 = freeListTraverser2->next;
					}
					freeListTraverser2->next = freeListTraverser->next;
				}
			}
			else
			{
				freeListTraverser->start = freeListTraverser->start + (size_of_buffer + 8);
				freeListTraverser->size = freeListTraverser->size - (size_of_buffer + 8);
			}

			//returning the address of the allocated memory
			return;
		}
		freeListTraverser = freeListTraverser->next;
	}
}

void delete_character_buffer_on_heap(struct freelist** head, struct allocated** headAllocatedList, char *variable_name, char *memory)
{
	struct allocated * allocatedListTraverser = *headAllocatedList;
	while (allocatedListTraverser != NULL)
	{
		if (strcmp(allocatedListTraverser->name, variable_name) == 0)
		{
			//extracting the size of the allocated region
			char sizeOfThisAllocatedRegionInCharArray[] = {memory[allocatedListTraverser->startaddress - 8], memory[allocatedListTraverser->startaddress - 7], memory[allocatedListTraverser->startaddress - 6], memory[allocatedListTraverser->startaddress - 5]};
			int sizeOfThisAllocatedRegion = extractInteger(sizeOfThisAllocatedRegionInCharArray, 4);

			//marking the memory used by the buffer as free
			for (int i = allocatedListTraverser->startaddress - 8; i < allocatedListTraverser->startaddress + sizeOfThisAllocatedRegion; ++i)
			{
				memory[i] = '0';
			}

			//updating the free list
			struct freelist * newFreeNode = (struct freelist *)malloc(sizeof(struct freelist));
			newFreeNode->start = allocatedListTraverser->startaddress - 8;
			newFreeNode->size = 8 + sizeOfThisAllocatedRegion;
			newFreeNode->next = NULL;

			if (*head == NULL)
			{
				*head = newFreeNode;
			}
			else
			{
				struct freelist * freeListTraverser = *head;
				while (freeListTraverser->next != NULL)
				{
					freeListTraverser = freeListTraverser->next;
				}
				freeListTraverser->next = newFreeNode;
			}

			//updating the allocated list
			if (allocatedListTraverser == *headAllocatedList)
			{
				*headAllocatedList = allocatedListTraverser->next;
			}
			else
			{
				struct allocated * allocatedListTraverser2 = *headAllocatedList;
				while (allocatedListTraverser2->next != allocatedListTraverser)
				{
					allocatedListTraverser2 = allocatedListTraverser2->next;
				}
				allocatedListTraverser2->next = allocatedListTraverser->next;
			}
			free(allocatedListTraverser);
			//returning the address of the allocated memory
			return;
		}
		allocatedListTraverser = allocatedListTraverser->next;
	}
}

void show_memory_map(char *memory, int number_of_frames_on_stack, struct var_tracker **variables_of_frames_on_stack_tracker, int current_stack_size, int current_heap_size, int current_stack_pointer, struct freelist *headFreeList, struct allocated *headAllocatedList)
{
	printf("MEMORY MAP:\n");
	printf("------------------------------------------------------------------\n");
	printf("------------------------------------------------------------------\n");
	printf("------------------------------------------------------------------\n");
	printf("Stack:\n");
	printf("------------------------------------------------------------------\n");
	printf("Stack Size: %d\n", current_stack_size);
	printf("Stack Pointer: %d\n", current_stack_pointer);
	printf("------------------------------------------------------------------\n");
	printf("Heap:\n");
	printf("------------------------------------------------------------------\n");
	printf("Heap Size: %d\n", current_heap_size);
	printf("------------------------------------------------------------------\n");
	printf("Frames:\n");
	printf("------------------------------------------------------------------\n");
	for (int i = 0; i < number_of_frames_on_stack; ++i)
	{
		if (variables_of_frames_on_stack_tracker[i] == NULL)
		{
			printf("No more than %d stack frames have been initialized yet.\n", i);
			break;
		}
		else
		{
			printf("Frame number: %d\n", i + 1);
			printf("Frame's higher occupancy: %d\n", variables_of_frames_on_stack_tracker[i]->index_of_higher_occupancy);
			printf("Frame's lower occupancy: %d\n", variables_of_frames_on_stack_tracker[i]->index_of_lower_occupancy);
		}
	}
	printf("------------------------------------------------------------------\n");
	printf("Free List:\n");
	printf("------------------------------------------------------------------\n");
	printFreelist(headFreeList);
	printf("------------------------------------------------------------------\n");
	printf("Allocated List:\n");
	printf("------------------------------------------------------------------\n");
	printAllocatedList(headAllocatedList, memory);
	printf("------------------------------------------------------------------\n");
	printf("Memory looks like the below representation:\n");
	for (int i = 0; i < 500; ++i)
	{
		printf("%d) %c \n", i, memory[i]);	
	}
}

int main ()
{
	// Initializing the counter to keep track of number of frames on stack
	int number_of_frames_on_stack = 0;

	// Initializing the integers which keep track of the initial size of stack and heap
	int current_stack_size = 200;
	int current_heap_size = 100;

	// Buffer that will emulate stack and heap memory
	char memory[MEMSIZE];

	// Sets each element to the space character
	for (int i = 0; i < MEMSIZE; ++i)
	{
    	memory[i] = ' ';
	}

	//Initializing the stack pointer
	int current_stack_pointer = 395;

	// Initialising the array of pointers to var_tracker structs
	struct var_tracker **variables_of_frames_on_stack_tracker = (struct var_tracker **)malloc(MAX_NUM_OF_FRAMES * sizeof(struct var_tracker *));
	for (int i = 0; i < MAX_NUM_OF_FRAMES; ++i)
	{
		variables_of_frames_on_stack_tracker[i] = NULL;
	}

	// Initializing the free list of the heap memory
	struct freelist * headFreeList;
	headFreeList = (struct freelist*)malloc(sizeof(struct freelist));

	headFreeList->start = 0;
	headFreeList->size = 100;
	headFreeList->next = NULL;

	for (int i = 0; i < headFreeList->size; ++i)
	{
    	memory[i] = '0';
	}

	//Initializing the allocated list of the heap memory
	struct allocated * headAllocatedList;
	headAllocatedList = NULL;

	char input[50];
    char command[2];
    char argument1[50];
    char argument2[50];
	while (1)
	{
		printf(">");
		fgets(input, sizeof(input), stdin);
		int count = sscanf(input, "%s %s %s", command, argument1, argument2);
		if (count == 3)
		{
			if (strcmp(command, "CF") == 0)
			{
				int funct_name_alr_exist = 0;
				if (strlen(argument1) > 8)
				{
					printf("Function name too long\n");
				}
				else if(number_of_frames_on_stack == MAX_NUM_OF_FRAMES)
				{
					printf("cannot create another frame, maximum number of frames have reached\n");
				}
				else if ((current_stack_size > (MAX_STACK_SIZE - MIN_FRAME_SIZE)) || ((current_stack_pointer < current_stack_size - MIN_FRAME_SIZE) && ((MEMSIZE - current_stack_pointer) + MIN_FRAME_SIZE + current_heap_size > MEMSIZE)))
				{
					printf("stack overflow, not enough memory available for new function\n");
				}
				else if (is_callable(memory, &current_stack_pointer) == 1)
				{
					printf("Please occupy the space alotted to the frame you just created before creating another frame\n");
				}
				else
				{
					if (number_of_frames_on_stack > 0)
					{
						//checking if the function name already exists
						for (int i = 1; i <= number_of_frames_on_stack; ++i)
						{
							if (strcmp(argument1, &memory[(MEMSIZE - (i * 20)) + 4]) == 0)
							{
								funct_name_alr_exist = 1;
								break;
							}
						}
					}
					if (funct_name_alr_exist == 0)
					{
						createFrame(argument1, argument2, memory, &number_of_frames_on_stack, variables_of_frames_on_stack_tracker, &current_stack_pointer);
						if ((MEMSIZE - current_stack_pointer + MIN_FRAME_SIZE) > current_stack_size)
						{
							current_stack_size += ((MEMSIZE - current_stack_pointer + MIN_FRAME_SIZE) - current_stack_size);
						}
					}
					else if (funct_name_alr_exist == 1)
					{
						printf("Function name already exists\n");
					}
				}

			}
			else if (strcmp(command, "CI") == 0)
			{
				if (variables_of_frames_on_stack_tracker[0] == NULL)
				{
					printf("No frame has been created yet\n");
				}
				else
				{
					int int_value = atoi(argument2);
					if (int_value > 9999 || int_value < -999)
					{
						printf("Integer value out of range\n");
					}
					else if ((current_stack_size > (MAX_STACK_SIZE - INT_VARIABLE_SIZE)) || ((current_stack_pointer < current_stack_size - INT_VARIABLE_SIZE) && ((MEMSIZE - current_stack_pointer) + INT_VARIABLE_SIZE + current_heap_size > MEMSIZE)))
					{
						printf("stack overflow, not enough memory available for new integer variable\n");
					}
					else if (abs(variables_of_frames_on_stack_tracker[number_of_frames_on_stack - 1]->index_of_higher_occupancy - variables_of_frames_on_stack_tracker[number_of_frames_on_stack - 1]->index_of_lower_occupancy) > MAX_FRAME_SIZE - INT_VARIABLE_SIZE)
					{
						printf("the frame is full, cannot create more data on it\n");
					}
					else
					{
						create_integer_local_variable(argument1, int_value, memory, &number_of_frames_on_stack, variables_of_frames_on_stack_tracker, &current_stack_pointer);
						if ((MEMSIZE - current_stack_pointer + INT_VARIABLE_SIZE) > current_stack_size)
						{
							current_stack_size += ((MEMSIZE - current_stack_pointer + MIN_FRAME_SIZE) - current_stack_size);
						}
					}
				}
			}
			else if (strcmp(command, "CD") == 0)
			{
				if (variables_of_frames_on_stack_tracker[0] == NULL)
				{
					printf("No frame has been created yet\n");
				}
				else
				{
					double double_value = atof(argument2);
					if (strlen(argument2) > 8)
					{
						printf("Double value out of range\n");
					}
					else if ((current_stack_size > (MAX_STACK_SIZE - DOUBLE_VARIABLE_SIZE)) || ((current_stack_pointer < current_stack_size - DOUBLE_VARIABLE_SIZE) && ((MEMSIZE - current_stack_pointer) + DOUBLE_VARIABLE_SIZE + current_heap_size > MEMSIZE)))
					{
						printf("stack overflow, not enough memory available for new double variable\n");
					}
					else if (abs(variables_of_frames_on_stack_tracker[number_of_frames_on_stack - 1]->index_of_higher_occupancy - variables_of_frames_on_stack_tracker[number_of_frames_on_stack - 1]->index_of_lower_occupancy) > MAX_FRAME_SIZE - DOUBLE_VARIABLE_SIZE)
					{
						printf("the frame is full, cannot create more data on it\n");
					}
					else
					{
						create_double_local_variable(argument1, double_value, memory, &number_of_frames_on_stack, variables_of_frames_on_stack_tracker, &current_stack_pointer);
						if ((MEMSIZE - current_stack_pointer + DOUBLE_VARIABLE_SIZE) > current_stack_size)
						{
							current_stack_size += ((MEMSIZE - current_stack_pointer + MIN_FRAME_SIZE) - current_stack_size);
						}
					}
				}
			}
			else if (strcmp(command, "CC") == 0)
			{
				if (variables_of_frames_on_stack_tracker[0] == NULL)
				{
					printf("No frame has been created yet\n");
				}
				else
				{
					if (strlen(argument2) > 1)
					{
						printf("Character value too long\n");
					}
					else if ((current_stack_size > (MAX_STACK_SIZE - CHAR_VARIABLE_SIZE)) || ((current_stack_pointer < current_stack_size - CHAR_VARIABLE_SIZE) && ((MEMSIZE - current_stack_pointer) + CHAR_VARIABLE_SIZE + current_heap_size > MEMSIZE)))
					{
						printf("stack overflow, not enough memory available for new character variable\n");
					}
					else if (abs(variables_of_frames_on_stack_tracker[number_of_frames_on_stack - 1]->index_of_higher_occupancy - variables_of_frames_on_stack_tracker[number_of_frames_on_stack - 1]->index_of_lower_occupancy) > MAX_FRAME_SIZE - CHAR_VARIABLE_SIZE)
					{
						printf("the frame is full, cannot create more data on it\n");
					}
					else
					{
						create_char_local_variable(argument1, argument2[0], memory, &number_of_frames_on_stack, variables_of_frames_on_stack_tracker, &current_stack_pointer);
						if ((MEMSIZE - current_stack_pointer + CHAR_VARIABLE_SIZE) > current_stack_size)
						{
							current_stack_size += ((MEMSIZE - current_stack_pointer + MIN_FRAME_SIZE) - current_stack_size);
						}
					}
				}
			}
			else if (strcmp(command, "CH") == 0)
			{
				if (variables_of_frames_on_stack_tracker[0] == NULL)
				{
					printf("No frame has been created yet where are you going to store the pointer for this?\n");
				}
				else
				{
					struct freelist * FLT = headFreeList;
					while (FLT != NULL)
					{
						//condition to check if the free region is big enough to store the buffer on the heap
						if (FLT->size >= atoi(argument2) + 8)
						{
							create_character_buffer_on_heap(&headFreeList, &headAllocatedList, argument1, atoi(argument2), memory, &current_stack_pointer, &number_of_frames_on_stack, variables_of_frames_on_stack_tracker);
							//if region is big enough then break out of the loop
							break;
						}
						FLT = FLT->next;
					}
					if (FLT == NULL)
					{
						if (headFreeList->start + atoi(argument2) + current_stack_size > MEMSIZE)
						{
							printf("“the heap is full, cannot create more data\n");
						}
						else
						{
							current_heap_size = headFreeList->start + atoi(argument2) + 8;
							headFreeList->size = atoi(argument2) + 8;
							for (int i = headFreeList->start; i < headFreeList->size; ++i)
							{
								memory[i] = 'F';
							}
							create_character_buffer_on_heap(&headFreeList, &headAllocatedList, argument1, atoi(argument2), memory, &current_stack_pointer, &number_of_frames_on_stack, variables_of_frames_on_stack_tracker);
						}
					}
				}
			}
			else {
				printf("Invalid command\n");
			}
		}
		else if (count == 2)
		{
			if (strcmp(command, "DH") == 0)
			{
				struct allocated * ALT = headAllocatedList;
				while (ALT != NULL)
				{
					if (strcmp(ALT->name, argument1) == 0)
					{
						delete_character_buffer_on_heap(&headFreeList, &headAllocatedList, argument1, memory);
						break;
					}
					ALT = ALT->next;
				}
				if (ALT == NULL)
				{
					printf("the pointer is NULL or already de-allocated\n");
				}
			}
			else
			{
				printf("Invalid command\n");
			}
		}
		else if (count == 1)
		{
			if (strcmp(command, "DF") == 0)
			{
				if (variables_of_frames_on_stack_tracker[0] == NULL)
				{
					printf("stack is empty\n");
				}
				else
				{
					deleteFrame(memory, &number_of_frames_on_stack, variables_of_frames_on_stack_tracker, &current_stack_pointer);
				}
			}
			else if (strcmp(command, "SM") == 0)
			{
				show_memory_map(memory, number_of_frames_on_stack, variables_of_frames_on_stack_tracker, current_stack_size, current_heap_size, current_stack_pointer, headFreeList, headAllocatedList);
			}
			else
			{
				printf("Invalid command\n");
			}
		}
		else
		{
			printf("Invalid command\n");
		}
	}

	return 0;
}
