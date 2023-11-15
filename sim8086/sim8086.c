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
    for (size_t i = 0; i < file_size; i += 2) {
        u8 instruction_byte_1 = file_buffer[i];
        u8 instruction_byte_2 = file_buffer[i + 1];

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

            char* dest_name[MAX_STR_LEN] = { 0 };
            char* src_name[MAX_STR_LEN] = { 0 };
            switch (mod_value) {
            case 0b11: { //REG TO REG
                (d_value == 1) 
                    ? strncpy(dest_name, registers[reg_value], MAX_STR_LEN - 1) 
                    : strncpy(dest_name, registers[rm_value], MAX_STR_LEN - 1);

                (d_value != 1) 
                    ? strncpy(src_name, registers[reg_value], MAX_STR_LEN - 1) 
                    : strncpy(src_name, registers[rm_value], MAX_STR_LEN - 1);

                break;
            }
            case 0b01: { //8-bit displacement
                u8 displacement_value = file_buffer[i + 2]; //displacement byte
                char const* mem_address = relative_address_registers[rm_value];
                char const* output_format = ((i8)displacement_value > 0) ? "[%s + %d]" : "[%s - %d]";
                (d_value == 1) 
                    ? strncpy(dest_name, registers[reg_value], MAX_STR_LEN - 1) 
                    : (displacement_value != 0)
                        ? sprintf(dest_name, output_format, mem_address, abs((i8)displacement_value))
                        : sprintf(dest_name, "[%s]", mem_address);

                (d_value != 1)
                    ? strncpy(src_name, registers[reg_value], MAX_STR_LEN - 1)
                    : (displacement_value != 0)
                        ? sprintf(src_name, output_format, mem_address, abs((i8)displacement_value))
                        : sprintf(src_name, "[%s]", mem_address);

                i++;
                break;
            }
            case 0b10: { //16-bit displacement
                u8 displacement_lo = file_buffer[i + 2];
                u8 displacement_hi = file_buffer[i + 3];
                char const* mem_address = relative_address_registers[rm_value];
                u16 displacement_value = (displacement_hi << 8) | displacement_lo;
                char const* output_format = ((i16)displacement_value > 0) ? "[%s + %d]" : "[%s - %d]";

                (d_value == 1)
                    ? strncpy(dest_name, registers[reg_value], MAX_STR_LEN - 1)
                    : (displacement_value != 0)
                        ? sprintf(dest_name, output_format, mem_address, abs((i16)displacement_value))
                        : sprintf(dest_name, "[%s]", mem_address);

                (d_value != 1)
                    ? strncpy(src_name, registers[reg_value], MAX_STR_LEN - 1)
                    : (displacement_value != 0)
                        ? sprintf(src_name, output_format, mem_address, abs((i16)displacement_value))
                        : sprintf(src_name, "[%s]", mem_address);

                i += 2;
                break;
            }
            case 0b00: { //no-didplacement, except when R/M is 110
                if (rm_value == 0b110) {
                    u8 displacement_lo = file_buffer[i + 2];
                    u8 displacement_hi = file_buffer[i + 3];
                    char const* mem_address = relative_address_registers[rm_value];
                    u16 displacement_value = (displacement_hi << 8) | displacement_lo;

                    (d_value == 1) 
                        ? strncpy(dest_name, registers[reg_value], MAX_STR_LEN - 1) 
                        : sprintf(dest_name, "[%d]", (i16)displacement_value);

                    (d_value != 1) 
                        ? strncpy(src_name, registers[reg_value], MAX_STR_LEN - 1) 
                        : sprintf(src_name, "[%d]", (i16)displacement_value);

                    i += 2;
                }
                else {
                    char const* mem_address = relative_address_registers[rm_value];

                    (d_value == 1) 
                        ? strncpy(dest_name, registers[reg_value], MAX_STR_LEN - 1) 
                        : strncpy(dest_name, mem_address, MAX_STR_LEN - 1);

                    (d_value != 1) 
                        ? strncpy(src_name, registers[reg_value], MAX_STR_LEN - 1) 
                        : strncpy(src_name, mem_address, MAX_STR_LEN - 1);
                }

                break;
            }
            }

            fprintf(stdout, "mov %s, %s\n", dest_name, src_name);
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
                u8 data_byte_hi = file_buffer[i + 2];
                u16 data_value = (data_byte_hi << 8) | data_byte_lo;
                char const* reg_name = word_registers_map[reg_value];
                i++;
                fprintf(stdout, "mov %s, %d\n", reg_name, (i16)data_value);
            }
        }
        else if ((opcode = (instruction_byte_1 & OPCODE_MASK_7BIT) >> 1) == 0b1100011) { //MOV immediate to reg/memory 
            u8 w_value = instruction_byte_1 & W_MASK_6BIT;
            u8 mod_value = (instruction_byte_2 & MOD_MASK) >> 6;
            u8 rm_value = instruction_byte_2 & RM_MASK;

            char* instruction[128] = { 0 };
            switch (mod_value) {
            case 0b11: { //WRITE IMMEDIATE TO REGISTER
                if (w_value == 0) {
                    char* reg = byte_registers_map[rm_value];
                    snprintf(instruction, sizeof instruction, "mov %s, byte %d", reg, (i8)file_buffer[i + 2]);
                    i++;
                } 
                else {
                    char* reg = word_registers_map[rm_value];
                    u8 data_lo = file_buffer[i + 2];
                    u8 data_hi = file_buffer[i + 3];
                    snprintf(instruction, sizeof instruction, "mov %s, word %d", reg, (i16)((data_hi << 8) | data_lo));
                    i += 2;
                }

                break;
            }
            case 0b10: { //WRITE IMMEDIATE using 16-bit displacement
                u8 displacement_lo = file_buffer[i + 2];
                u8 displacement_hi = file_buffer[i + 3];
                i16 displacement = (i16)((displacement_hi << 8) | displacement_lo);
                char* reg = relative_address_registers[rm_value];

                if (w_value == 0) {
                    i8 immediate_value = file_buffer[i + 4];

                    if (displacement < 0)
                        snprintf(instruction, sizeof instruction, "mov [%s - %d], byte %d", reg, abs(displacement), immediate_value);
                    else if (displacement > 0)
                        snprintf(instruction, sizeof instruction, "mov [%s + %d], byte %d", reg, displacement, immediate_value);
                    else
                        snprintf(instruction, sizeof instruction, "mov [%s], byte %d", reg, immediate_value);
                        
                    i += 3;
                }
                else {
                    u8 data_lo = file_buffer[i + 4];
                    u8 data_hi = file_buffer[i + 5];
                    i16 immediate_value = (i16)((data_hi << 8) | data_lo);

                    if (displacement < 0)
                        snprintf(instruction, sizeof instruction, "mov [%s - %d], word %d", reg, abs(displacement), immediate_value);
                    else if (displacement > 0)
                        snprintf(instruction, sizeof instruction, "mov [%s + %d], word %d", reg, displacement, immediate_value);
                    else
                        snprintf(instruction, sizeof instruction, "mov [%s], word %d", reg, immediate_value);
                    
                    i += 4;
                }

                break;
            }
            case 0b01: { //WRITE IMMEDIATE using 8 bit displacement
                i8 displacement = (i8)file_buffer[i + 2];
                char* reg = relative_address_registers[rm_value];

                if (w_value == 0) {
                    i8 immediate_value = file_buffer[i + 3];

                    if (displacement < 0)
                        snprintf(instruction, sizeof instruction, "mov [%s - %d], byte %d", reg, abs(displacement), immediate_value);
                    else if (displacement > 0)
                        snprintf(instruction, sizeof instruction, "mov [%s + %d], byte %d", reg, displacement, immediate_value);
                    else
                        snprintf(instruction, sizeof instruction, "mov [%s], byte %d", reg, immediate_value);

                    i += 2;
                }
                else {
                    u8 data_lo = file_buffer[i + 3];
                    u8 data_hi = file_buffer[i + 4];
                    i16 immediate_value = (i16)((data_hi << 8) | data_lo);

                    if (displacement < 0)
                        snprintf(instruction, sizeof instruction, "mov [%s - %d], word %d", reg, abs(displacement), immediate_value);
                    else if (displacement > 0)
                        snprintf(instruction, sizeof instruction, "mov [%s + %d], word %d", reg, displacement, immediate_value);
                    else
                        snprintf(instruction, sizeof instruction, "mov [%s], word %d", reg, immediate_value);

                    i += 3;
                }
                break;
            }
            case 0b00: { //WRITE IMMEDIATE to mem location without displacement
                char* reg = relative_address_registers[rm_value];

                if (w_value == 0) {
                    i8 immediate_value = file_buffer[i + 2];
                    snprintf(instruction, sizeof instruction, "mov [%s], byte %d", reg, immediate_value);
                    i += 1;
                }
                else {
                    u8 data_lo = file_buffer[i + 2];
                    u8 data_hi = file_buffer[i + 3];
                    i16 immediate_value = (i16)((data_hi << 8) | data_lo);
                    snprintf(instruction, sizeof instruction, "mov [%s], word %d", reg, immediate_value);
                    i += 2;
                }

                break;
            }
            }

            fprintf(stdout, "%s\n", instruction);

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
                u8 addr_hi = file_buffer[i + 2];
                addr_value = (addr_hi << 8) | addr_lo;
                i++;
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