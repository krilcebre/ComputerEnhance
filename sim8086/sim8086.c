#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "instruction.h"

#define buffer_SIZE 1000000
#define INSTRUCTION_BYTES_LEN 2

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
    u8* buffer = (u8*)malloc(file_size * sizeof(u8));
    if (fread(buffer, 1, file_size, assembly_file) < INSTRUCTION_BYTES_LEN) {
        fprintf(stderr, "\nUnable to read bytes from file %s\n", file_name);
        goto buf_err;
    }
    fclose(assembly_file);

    fprintf(stdout, "bits 16\n\n");
    size_t bytes_read = 0;
    while (bytes_read < file_size) {
        u8 instruction_byte_1 = buffer[bytes_read];

        u8 opcode = 0;
        Instruction instruction = { 0 };
        if ((opcode = (instruction_byte_1 & OPCODE_MASK_6BIT) >> 2) == 0b100010) {          //MOV register/memory to/from register
            instruction = mov_reg_mem_to_or_from_reg(buffer, bytes_read);
        }
        else if ((opcode = (instruction_byte_1 & OPCODE_MASK_4BIT) >> 4) == 0b1011) {       //MOV immediate to register
            instruction = mov_immediate_to_reg(buffer, bytes_read);
        }
        else if ((opcode = (instruction_byte_1 & OPCODE_MASK_7BIT) >> 1) == 0b1100011) {    //MOV immediate to reg/memory 
            instruction = mov_immediate_to_reg_mem(buffer, bytes_read);
        }
        else if ((opcode = (instruction_byte_1 & OPCODE_MASK_7BIT) >> 1) == 0b1010001) {    //MOV ACCUMULATOR TO MEM
            instruction = mov_acc_to_mem(buffer, bytes_read);
        }
        else if ((opcode = (instruction_byte_1 & OPCODE_MASK_7BIT) >> 1) == 0b1010000) {    //MOV MEM TO ACCUMULATOR
            instruction = mov_mem_to_acc(buffer, bytes_read);
        }
        else {
            fprintf(stderr, "Unknown instruction code\n");
            goto buf_err;
        }

        bytes_read += instruction.bytes_count;
        print_instruction_two_operands(&instruction, stdout);
    }

    free(buffer);
    return EXIT_SUCCESS;

buf_err:
    if (buffer != NULL)
        free(buffer);
file_err:
    if (assembly_file != NULL)
        fclose(assembly_file);

    return EXIT_FAILURE;
}

