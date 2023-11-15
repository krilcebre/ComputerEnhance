#include "instruction.h"

void print_instruction_two_operands(struct Instruction const* const instruction, FILE* const stream) {
	fprintf(stream, "%s %s,%s\n", instruction->operation, instruction->first_operand, instruction->second_operand);
}