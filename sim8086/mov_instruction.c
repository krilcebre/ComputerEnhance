#include "instruction.h"

Instruction mov_mem_to_acc(u8* const buffer, size_t offset) {
    size_t bytes_read = offset;
    u8 instruction_byte_1 = buffer[bytes_read++];
    u8 instruction_byte_2 = buffer[bytes_read++];

    u8 w_value = instruction_byte_1 & W_MASK_6BIT;
    char* reg = (w_value == 0) ? byte_registers_map[0] : word_registers_map[0];

    u16 addr_value = 0;
    if (w_value == 0) {
        addr_value = instruction_byte_2;
    }
    else { //read 1 more bytes, otherwise we have already read whole instruction
        u8 addr_lo = instruction_byte_2;
        u8 addr_hi = buffer[bytes_read++];
        addr_value = (addr_hi << 8) | addr_lo;
    }

    Instruction instruction = {
        .bytes_count = bytes_read - offset
    };

    strncpy(instruction.operation, "mov", sizeof instruction.operation);
    strncpy(instruction.second_operand, reg, sizeof instruction.second_operand);
    snprintf(instruction.first_operand, sizeof instruction.first_operand, "[%d]", addr_value);

    return instruction;
}

Instruction mov_acc_to_mem(u8* const buffer, size_t offset) {
    size_t bytes_read = offset;
    u8 instruction_byte_1 = buffer[bytes_read++];
    u8 instruction_byte_2 = buffer[bytes_read++];

    u8 w_value = instruction_byte_1 & W_MASK_6BIT;

    char* reg = (w_value == 0) ? byte_registers_map[0] : word_registers_map[0];

    u16 addr_value = 0;
    if (w_value == 0) {
        addr_value = instruction_byte_2;
    }
    else { //read 1 more bytes, otherwise we have already read whole instruction
        u8 addr_lo = instruction_byte_2;
        u8 addr_hi = buffer[bytes_read++];
        addr_value = (addr_hi << 8) | addr_lo;
    }

    Instruction instruction = {
        .bytes_count = bytes_read - offset
    };

    strncpy(instruction.operation, "mov", sizeof instruction.operation);
    strncpy(instruction.first_operand, reg, sizeof instruction.first_operand);
    snprintf(instruction.second_operand, sizeof instruction.second_operand, "[%d]", addr_value);

    return instruction;
}
