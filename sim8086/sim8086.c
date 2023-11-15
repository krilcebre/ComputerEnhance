#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

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

#define FILE_BUFFER_SIZE 1000000
#define INSTRUCTION_BYTES_LEN 2

#define MAX_STR_LEN 32

typedef int8_t i8;
typedef int16_t i16;

typedef uint8_t u8;
typedef uint16_t u16;

static char const* byte_registers_map[] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
static char const* word_registers_map[] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
static char const* relative_address_registers[] = { "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx" };

void create_memory_displacement_operand(char* const buffer, size_t max_size, char const* const reg_mem_value, int displacement);

int main(int argc, char const* argv[]) {
    assert(argc == 2);

    char const* const file_name = argv[1];
    FILE* assembly_file = fopen(file_name, "rb");
    if (assembly_file == NULL) {
        fprintf(stderr, "\nUnable to open a file %s\n", file_name);
        goto file_err;
    }

    if (fseek(assembly_file, 0, SEEK_END) != 0) {
        fprintf(stderr, "\nfseek() failed for file %s", file_name);
        goto file_err;
    }

    //get file size in bytes
    long file_size = ftell(assembly_file);
    if (file_size < INSTRUCTION_BYTES_LEN) {
        fprintf(stderr, "\nFile %s is too small. It needs to have at least one CPU instruction (2 bytes)", file_name);
        goto file_err;
    }

    if (fseek(assembly_file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "\nfseek() failed for file %s", file_name);
        goto file_err;
    }

    //load file into memory
    u8* file_buffer = (u8*)malloc(file_size * sizeof(u8));
    if (fread(file_buffer, 1, file_size, assembly_file) < INSTRUCTION_BYTES_LEN) {
        fprintf(stderr, "\nUnable to read bytes from file %s\n", file_name);
        goto buf_err;
    }
    fclose(assembly_file);

    fprintf(stdout, "bits 16\n\n");
    size_t bytes_read = 0;
    while (bytes_read < file_size) {
        
        u8 instruction_byte_1 = file_buffer[bytes_read++];
        u8 instruction_byte_2 = file_buffer[bytes_read++];

        u8 opcode = 0;
        if ((opcode = (instruction_byte_1 & OPCODE_MASK_6BIT) >> 2) == 0b100010) { //MOV register/memory to/from register
            u8 d_value = (instruction_byte_1 & D_MASK) >> 1;
            u8 w_value = (instruction_byte_1 & W_MASK_6BIT);

            u8 mod_value = (instruction_byte_2 & MOD_MASK) >> 6;
            u8 reg_value = (instruction_byte_2 & REG_MASK_6BIT) >> 3;
            u8 rm_value = (instruction_byte_2 & RM_MASK);

            char const** registers = (w_value == 1)
                ? word_registers_map
                : byte_registers_map;

            char* dest_operand[MAX_STR_LEN] = { 0 };
            char* src_operand[MAX_STR_LEN] = { 0 };
            switch (mod_value) {
            case 0b11: { //REG TO REG   
                strncpy(dest_operand, registers[(d_value == 1) ? reg_value : rm_value], MAX_STR_LEN - 1);
                strncpy(src_operand, registers[(d_value == 1) ? rm_value : reg_value], MAX_STR_LEN - 1);

                break;
            }
            case 0b01: { //8-bit displacement
                i8 displacement_value = file_buffer[bytes_read++]; //displacement byte
                char const* mem_address = relative_address_registers[rm_value];
                if (d_value == 1) {
                    strncpy(dest_operand, registers[reg_value], MAX_STR_LEN - 1);
                    create_memory_displacement_operand(src_operand, sizeof src_operand, mem_address, displacement_value);
                }
                else {
                    strncpy(src_operand, registers[reg_value], MAX_STR_LEN - 1);
                    create_memory_displacement_operand(dest_operand, sizeof dest_operand, mem_address, displacement_value);
                }
                break;
            }
            case 0b10: { //16-bit displacement
                u8 displacement_lo = file_buffer[bytes_read++];
                u8 displacement_hi = file_buffer[bytes_read++];
                i16 displacement = (i16)((displacement_hi << 8) | displacement_lo);
                char const* mem_address = relative_address_registers[rm_value];

                if (d_value == 1) {
                    strncpy(dest_operand, registers[reg_value], MAX_STR_LEN - 1);
                    create_memory_displacement_operand(src_operand, sizeof src_operand, mem_address, displacement);
                }
                else
                {
                    strncpy(src_operand, registers[reg_value], MAX_STR_LEN - 1);
                    create_memory_displacement_operand(dest_operand, sizeof dest_operand, mem_address, displacement);
                }
                break;
            }
            case 0b00: { //no-didplacement, except when R/M is 110
                if (rm_value == 0b110) {
                    u8 displacement_lo = file_buffer[bytes_read++];
                    u8 displacement_hi = file_buffer[bytes_read++];
                    u16 displacement_value = (displacement_hi << 8) | displacement_lo;
                    char const* mem_address = relative_address_registers[rm_value];

                    if (d_value == 1) {
                        strncpy(dest_operand, registers[reg_value], MAX_STR_LEN - 1);
                        snprintf(src_operand, sizeof src_operand, "[%d]", (i16)displacement_value);
                    }
                    else {
                        strncpy(src_operand, registers[reg_value], MAX_STR_LEN - 1);
                        snprintf(dest_operand, sizeof dest_operand, "[%d]", (i16)displacement_value);
                    }
                }
                else {
                    char const* mem_address = relative_address_registers[rm_value];

                    if (d_value == 1) {
                        strncpy(dest_operand, registers[reg_value], MAX_STR_LEN - 1);
                        strncpy(src_operand, mem_address, MAX_STR_LEN - 1);
                    }
                    else {
                        strncpy(src_operand, registers[reg_value], MAX_STR_LEN - 1);
                        strncpy(dest_operand, mem_address, MAX_STR_LEN - 1);
                    }
                }

                break;
            }
            }

            fprintf(stdout, "mov %s, %s\n", dest_operand, src_operand);
        }
        else if ((opcode = (instruction_byte_1 & OPCODE_MASK_4BIT) >> 4) == 0b1011) { //MOV immediate to register
            u8 w_value = (instruction_byte_1 & W_MASK_4BIT) >> 3;
            u8 reg_value = instruction_byte_1 & REG_MASK_4BIT;

            if (w_value == 0) {
                u8 data_value = instruction_byte_2;
                char const* reg_name = byte_registers_map[reg_value];
                fprintf(stdout, "mov %s, %d\n", reg_name, data_value);
            }
            else {
                u8 data_byte_lo = instruction_byte_2;
                u8 data_byte_hi = file_buffer[bytes_read++];
                u16 data_value = (data_byte_hi << 8) | data_byte_lo;
                char const* reg_name = word_registers_map[reg_value];
                fprintf(stdout, "mov %s, %d\n", reg_name, (i16)data_value);
            }
        }
        else if ((opcode = (instruction_byte_1 & OPCODE_MASK_7BIT) >> 1) == 0b1100011) { //MOV immediate to reg/memory 
            u8 w_value = instruction_byte_1 & W_MASK_6BIT;
            u8 mod_value = (instruction_byte_2 & MOD_MASK) >> 6;
            u8 rm_value = instruction_byte_2 & RM_MASK;

            char* dest_operand[64] = { 0 };
            char* src_operand[64] = { 0 };
            switch (mod_value) {
            case 0b11: { //WRITE IMMEDIATE TO REGISTER
                if (w_value == 0) {
                    char* reg = byte_registers_map[rm_value];
                    i8 immediate = (i8)file_buffer[bytes_read++];
                    snprintf(dest_operand, sizeof dest_operand, "%s", reg);
                    snprintf(src_operand, sizeof src_operand, "byte %d", immediate);
                } 
                else {
                    char* reg = word_registers_map[rm_value];
                    u8 data_lo = file_buffer[bytes_read++];
                    u8 data_hi = file_buffer[bytes_read++];
                    i16 immediate = (i16)((data_hi << 8) | data_lo);
                    snprintf(dest_operand, sizeof dest_operand, "%s", reg);
                    snprintf(src_operand, sizeof src_operand, "word %d", immediate);
                }

                break;
            }
            case 0b10: { //WRITE IMMEDIATE using 16-bit displacement
                u8 displacement_lo = file_buffer[bytes_read++];
                u8 displacement_hi = file_buffer[bytes_read++];
                i16 displacement = (i16)((displacement_hi << 8) | displacement_lo);
                char* reg = relative_address_registers[rm_value];

                if (w_value == 0) {
                    i8 immediate_value = file_buffer[bytes_read++];
                    create_memory_displacement_operand(dest_operand, sizeof dest_operand, reg, displacement);
                    snprintf(src_operand, sizeof src_operand, "byte %d", immediate_value);
                }
                else {
                    u8 data_lo = file_buffer[bytes_read++];
                    u8 data_hi = file_buffer[bytes_read++];
                    i16 immediate_value = (i16)((data_hi << 8) | data_lo);
                    create_memory_displacement_operand(dest_operand, sizeof dest_operand, reg, displacement);
                    snprintf(src_operand, sizeof src_operand, "word %d", immediate_value);
                }

                break;
            }
            case 0b01: { //WRITE IMMEDIATE using 8 bit displacement
                i8 displacement = (i8)file_buffer[bytes_read++];
                char* reg = relative_address_registers[rm_value];

                if (w_value == 0) {
                    i8 immediate_value = file_buffer[bytes_read++];
                    create_memory_displacement_operand(dest_operand, sizeof dest_operand, reg, displacement);
                    snprintf(src_operand, sizeof src_operand, "byte %d", immediate_value);
                }
                else {
                    u8 data_lo = file_buffer[bytes_read++];
                    u8 data_hi = file_buffer[bytes_read++];
                    i16 immediate_value = (i16)((data_hi << 8) | data_lo);
                    create_memory_displacement_operand(dest_operand, sizeof dest_operand, reg, displacement);
                    snprintf(src_operand, sizeof src_operand, "word %d", immediate_value);
                }
                break;
            }
            case 0b00: { //WRITE IMMEDIATE to mem location without displacement
                char* reg = relative_address_registers[rm_value];

                if (w_value == 0) {
                    i8 immediate_value = file_buffer[bytes_read++];
                    create_memory_displacement_operand(dest_operand, sizeof dest_operand, reg, 0);
                    snprintf(src_operand, sizeof src_operand, "byte %d", immediate_value);
                }
                else {
                    u8 data_lo = file_buffer[bytes_read++];
                    u8 data_hi = file_buffer[bytes_read++];
                    i16 immediate_value = (i16)((data_hi << 8) | data_lo);
                    create_memory_displacement_operand(dest_operand, sizeof dest_operand, reg, 0);
                    snprintf(src_operand, sizeof src_operand, "word %d", immediate_value);
                }
                break;
            }
            }

            fprintf(stdout, "mov %s, %s\n", dest_operand, src_operand);

        }
        else if ((opcode = (instruction_byte_1 & OPCODE_MASK_6BIT) >> 2) == 0b101000) { //MOV ACCUMULATOR
            u8 d_value = (instruction_byte_1 & D_MASK) >> 1;
            u8 w_value = instruction_byte_1 & W_MASK_6BIT;

            char* reg = (w_value == 0) ? byte_registers_map[0] : word_registers_map[0];

            u16 addr_value = 0;
            if (w_value == 0) { 
                addr_value = instruction_byte_2;
            }
            else { //read 1 more bytes, otherwise we have already read whole instruction
                u8 addr_lo = instruction_byte_2;
                u8 addr_hi = file_buffer[bytes_read++];
                addr_value = (addr_hi << 8) | addr_lo;
            }

            (d_value == 0)
                ? fprintf(stdout, "mov %s, [%d]\n", reg, addr_value)
                : fprintf(stdout, "mov [%d], %s\n", addr_value, reg);
        }
        else {
            fprintf(stderr, "Unknown instruction code\n");
            goto buf_err;
        }
    }

    free(file_buffer);
    return EXIT_SUCCESS;

buf_err:
    if (file_buffer != NULL)
        free(file_buffer);
file_err:
    if (assembly_file != NULL)
        fclose(assembly_file);

    return EXIT_FAILURE;
}

void create_memory_displacement_operand(char* const buf, size_t max_size, char const* const reg_mem_value, int displacement) {
    if (displacement < 0)
        snprintf(buf, max_size, "[%s - %d]", reg_mem_value, abs(displacement));
    else if (displacement > 0)
        snprintf(buf, max_size, "[%s + %d]", reg_mem_value, displacement);
    else
        snprintf(buf, max_size, "[%s]", reg_mem_value);
}