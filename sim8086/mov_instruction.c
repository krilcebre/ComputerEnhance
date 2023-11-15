#include "instruction.h"

static void create_memory_displacement_operand(u8* const buffer, size_t max_size, u8 const* const reg_mem_value, int displacement);

Instruction mov_reg_mem_to_or_from_reg(u8* const buffer, size_t const offset) {
    size_t bytes_read = offset;
    u8 instruction_byte_1 = buffer[bytes_read++];
    u8 instruction_byte_2 = buffer[bytes_read++];

    u8 d_value = (instruction_byte_1 & D_MASK) >> 1;
    u8 w_value = (instruction_byte_1 & W_MASK_6BIT);

    u8 mod_value = (instruction_byte_2 & MOD_MASK) >> 6;
    u8 reg_value = (instruction_byte_2 & REG_MASK_6BIT) >> 3;
    u8 rm_value = (instruction_byte_2 & RM_MASK);

    char const** registers = (w_value == 1)
        ? word_registers_map
        : byte_registers_map;

    char* first_operand[INSTR_OPRND_MAX_LEN] = { 0 };
    char* second_operand[INSTR_OPRND_MAX_LEN] = { 0 };
    switch (mod_value) {
    case 0b11: { //REG TO REG   
        strncpy(first_operand, registers[(d_value == 1) ? reg_value : rm_value], sizeof first_operand);
        strncpy(second_operand, registers[(d_value == 1) ? rm_value : reg_value], sizeof second_operand);

        break;
    }
    case 0b01: { //8-bit displacement
        i8 displacement_value = buffer[bytes_read++]; //displacement byte
        char const* mem_address = memory_access_registers_map[rm_value];
        if (d_value == 1) {
            strncpy(first_operand, registers[reg_value], sizeof first_operand);
            create_memory_displacement_operand(second_operand, sizeof second_operand, mem_address, displacement_value);
        }
        else {
            strncpy(second_operand, registers[reg_value], sizeof second_operand);
            create_memory_displacement_operand(first_operand, sizeof first_operand, mem_address, displacement_value);
        }
        break;
    }
    case 0b10: { //16-bit displacement
        u8 displacement_lo = buffer[bytes_read++];
        u8 displacement_hi = buffer[bytes_read++];
        i16 displacement = (i16)((displacement_hi << 8) | displacement_lo);
        char const* mem_address = memory_access_registers_map[rm_value];

        if (d_value == 1) {
            strncpy(first_operand, registers[reg_value], sizeof first_operand);
            create_memory_displacement_operand(second_operand, sizeof second_operand, mem_address, displacement);
        }
        else
        {
            strncpy(second_operand, registers[reg_value], sizeof second_operand);
            create_memory_displacement_operand(first_operand, sizeof first_operand, mem_address, displacement);
        }
        break;
    }
    case 0b00: { //no-didplacement, except when R/M is 110
        if (rm_value == 0b110) {
            u8 displacement_lo = buffer[bytes_read++];
            u8 displacement_hi = buffer[bytes_read++];
            u16 displacement_value = (displacement_hi << 8) | displacement_lo;
            char const* mem_address = memory_access_registers_map[rm_value];

            if (d_value == 1) {
                strncpy(first_operand, registers[reg_value], sizeof first_operand);
                snprintf(second_operand, sizeof second_operand, "[%d]", (i16)displacement_value);
            }
            else {
                strncpy(second_operand, registers[reg_value], sizeof second_operand);
                snprintf(first_operand, sizeof first_operand, "[%d]", (i16)displacement_value);
            }
        }
        else {
            char const* mem_address = memory_access_registers_map[rm_value];

            if (d_value == 1) {
                strncpy(first_operand, registers[reg_value], sizeof first_operand);
                strncpy(second_operand, mem_address, sizeof second_operand);
            }
            else {
                strncpy(second_operand, registers[reg_value], sizeof second_operand);
                strncpy(first_operand, mem_address, sizeof first_operand);
            }
        }

        break;
    }
    }

    Instruction instruction = {
        .bytes_count = bytes_read - offset
    };
    
    strncpy(instruction.operation, "mov", sizeof instruction.operation);
    strncpy(instruction.first_operand, first_operand, sizeof first_operand);
    strncpy(instruction.second_operand, second_operand, sizeof second_operand);

    return instruction;
}

Instruction mov_immediate_to_reg_mem(u8* const buffer, size_t offset) {
    size_t bytes_read = offset;
    u8 instruction_byte_1 = buffer[bytes_read++];
    u8 instruction_byte_2 = buffer[bytes_read++];

    u8 w_value = instruction_byte_1 & W_MASK_6BIT;
    u8 mod_value = (instruction_byte_2 & MOD_MASK) >> 6;
    u8 rm_value = instruction_byte_2 & RM_MASK;

    char* first_operand[INSTR_OPRND_MAX_LEN] = { 0 };
    char* second_operand[INSTR_OPRND_MAX_LEN] = { 0 };
    switch (mod_value) {
    case 0b11: { //WRITE IMMEDIATE TO REGISTER
        if (w_value == 0) {
            char* reg = byte_registers_map[rm_value];
            i8 immediate = (i8)buffer[bytes_read++];
            snprintf(first_operand, sizeof first_operand, "%s", reg);
            snprintf(second_operand, sizeof second_operand, "byte %d", immediate);
        }
        else {
            char* reg = word_registers_map[rm_value];
            u8 data_lo = buffer[bytes_read++];
            u8 data_hi = buffer[bytes_read++];
            i16 immediate = (i16)((data_hi << 8) | data_lo);
            snprintf(first_operand, sizeof first_operand, "%s", reg);
            snprintf(second_operand, sizeof second_operand, "word %d", immediate);
        }

        break;
    }
    case 0b10: { //WRITE IMMEDIATE using 16-bit displacement
        u8 displacement_lo = buffer[bytes_read++];
        u8 displacement_hi = buffer[bytes_read++];
        i16 displacement = (i16)((displacement_hi << 8) | displacement_lo);
        char* reg = memory_access_registers_map[rm_value];

        if (w_value == 0) {
            i8 immediate_value = buffer[bytes_read++];
            create_memory_displacement_operand(first_operand, sizeof first_operand, reg, displacement);
            snprintf(second_operand, sizeof second_operand, "byte %d", immediate_value);
        }
        else {
            u8 data_lo = buffer[bytes_read++];
            u8 data_hi = buffer[bytes_read++];
            i16 immediate_value = (i16)((data_hi << 8) | data_lo);
            create_memory_displacement_operand(first_operand, sizeof first_operand, reg, displacement);
            snprintf(second_operand, sizeof second_operand, "word %d", immediate_value);
        }

        break;
    }
    case 0b01: { //WRITE IMMEDIATE using 8 bit displacement
        i8 displacement = (i8)buffer[bytes_read++];
        char* reg = memory_access_registers_map[rm_value];

        if (w_value == 0) {
            i8 immediate_value = buffer[bytes_read++];
            create_memory_displacement_operand(first_operand, sizeof first_operand, reg, displacement);
            snprintf(second_operand, sizeof second_operand, "byte %d", immediate_value);
        }
        else {
            u8 data_lo = buffer[bytes_read++];
            u8 data_hi = buffer[bytes_read++];
            i16 immediate_value = (i16)((data_hi << 8) | data_lo);
            create_memory_displacement_operand(first_operand, sizeof first_operand, reg, displacement);
            snprintf(second_operand, sizeof second_operand, "word %d", immediate_value);
        }
        break;
    }
    case 0b00: { //WRITE IMMEDIATE to mem location without displacement
        char* reg = memory_access_registers_map[rm_value];

        if (w_value == 0) {
            i8 immediate_value = buffer[bytes_read++];
            create_memory_displacement_operand(first_operand, sizeof first_operand, reg, 0);
            snprintf(second_operand, sizeof second_operand, "byte %d", immediate_value);
        }
        else {
            u8 data_lo = buffer[bytes_read++];
            u8 data_hi = buffer[bytes_read++];
            i16 immediate_value = (i16)((data_hi << 8) | data_lo);
            create_memory_displacement_operand(first_operand, sizeof first_operand, reg, 0);
            snprintf(second_operand, sizeof second_operand, "word %d", immediate_value);
        }
        break;
    }
    }
    
    Instruction instruction = {
        .bytes_count = bytes_read - offset
    };

    strncpy(instruction.operation, "mov", sizeof instruction.operation);
    strncpy(instruction.first_operand, first_operand, sizeof first_operand);
    strncpy(instruction.second_operand, second_operand, sizeof second_operand);

    return instruction;
}

Instruction mov_immediate_to_reg(u8* const buffer, size_t offset) {
    size_t bytes_read = offset;
    u8 instruction_byte_1 = buffer[bytes_read++];
    u8 instruction_byte_2 = buffer[bytes_read++];

    u8 w_value = (instruction_byte_1 & W_MASK_4BIT) >> 3;
    u8 reg_value = instruction_byte_1 & REG_MASK_4BIT;

    char* first_operand[INSTR_OPRND_MAX_LEN] = { 0 };
    char* second_operand[INSTR_OPRND_MAX_LEN] = { 0 };
    if (w_value == 0) {
        u8 data_value = instruction_byte_2;
        strncpy(first_operand, byte_registers_map[reg_value], sizeof first_operand);
        snprintf(second_operand, sizeof second_operand, "%d", data_value);
    }
    else {
        u8 data_byte_lo = instruction_byte_2;
        u8 data_byte_hi = buffer[bytes_read++];
        u16 data_value = (data_byte_hi << 8) | data_byte_lo;
        strncpy(first_operand, word_registers_map[reg_value], sizeof first_operand);
        snprintf(second_operand, sizeof second_operand, "%d", (i16)data_value);
    }

    Instruction instruction = {
        .bytes_count = bytes_read - offset
    };

    strncpy(instruction.operation, "mov", sizeof instruction.operation);
    strncpy(instruction.first_operand, first_operand, sizeof first_operand);
    strncpy(instruction.second_operand, second_operand, sizeof second_operand);

    return instruction;
}

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

static void create_memory_displacement_operand(char* const buffer, size_t max_size, char const* const reg_mem_value, int displacement) {
    if (displacement < 0)
        snprintf(buffer, max_size, "[%s - %d]", reg_mem_value, abs(displacement));
    else if (displacement > 0)
        snprintf(buffer, max_size, "[%s + %d]", reg_mem_value, displacement);
    else
        snprintf(buffer, max_size, "[%s]", reg_mem_value);
}
