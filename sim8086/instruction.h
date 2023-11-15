#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define INSTR_OP_MAX_LEN		16
#define INSTR_OPRND_MAX_LEN		64

#define OPCODE_MASK_7BIT    0b11111110
#define OPCODE_MASK_6BIT    0b11111100
#define OPCODE_MASK_4BIT    0b11110000
#define D_MASK              0b00000010
#define W_MASK_6BIT         0b00000001
#define W_MASK_4BIT         0b00001000

#define MOD_MASK            0b11000000
#define REG_MASK_6BIT       0b00111000
#define REG_MASK_4BIT       0b00000111
#define RM_MASK             0b00000111

typedef int8_t i8;
typedef int16_t i16;

typedef uint8_t u8;
typedef uint16_t u16;

typedef struct Instruction Instruction;
struct Instruction {
	size_t bytes_count;
	u8 operation[INSTR_OP_MAX_LEN];
	u8 first_operand[INSTR_OPRND_MAX_LEN];
	u8 second_operand[INSTR_OPRND_MAX_LEN];
};

static u8 const* byte_registers_map[] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
static u8 const* word_registers_map[] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
static u8 const* memory_access_registers_map[] = { "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx" };

Instruction mov_reg_mem_to_or_from_reg(u8* const buffer, size_t const offset);
Instruction mov_immediate_to_reg_mem(u8* const buffer, size_t const offset);
Instruction mov_immediate_to_reg(u8* const buffer, size_t const offset);
Instruction mov_mem_to_acc(u8* const buffer, size_t const offset);
Instruction mov_acc_to_mem(u8* const buffer, size_t const offset);

void print_instruction_two_operands(Instruction const* const instruction, FILE* const stream);