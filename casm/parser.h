#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>

/*
* Chip 8 opcodes:
* 
*           Fx07 - LD Vx, DT
*           Fx0A - LD Vx, K
*           Fx15 - LD DT, Vx
*           Fx18 - LD ST, Vx
*           Fx29 - LD F, Vx
*           Fx33 - LD B, Vx
*           Fx55 - LD [I], Vx
*           Fx65 - LD Vx, [I]
*           6xkk - LD Vx, byte
*           Annn - LD I, addr
*           1nnn - JP addr
*           Bnnn - JP V0, addr
*           7xkk - ADD Vx, byte
*           Fx1E - ADD I, Vx
*           3xkk - SE Vx, byte
*           5xy0 - SE Vx, Vy
*           00E0 - CLS
*           00EE - RET
*           0nnn - SYS addr
*           2nnn - CALL addr
*           8xy4 - ADD Vx, Vy
*           8xy0 - LD Vx, Vy
* 
*           4xkk - SNE Vx, byte
*           8xy1 - OR Vx, Vy
*           8xy2 - AND Vx, Vy
*           8xy3 - XOR Vx, Vy
*           Ex9E - SKP Vx
*           ExA1 - SKNP Vx
*           8xy5 - SUB Vx, Vy
*           8xy6 - SHR Vx {, Vy}
*           8xy7 - SUBN Vx, Vy
*           8xyE - SHL Vx {, Vy}
*           9xy0 - SNE Vx, Vy
*           Cxkk - RND Vx, byte
*           Dxyn - DRW Vx, Vy, nibble
*
*/

typedef struct {
    uint16_t value;
    int memory_offset;
} Opcode;

typedef struct {
    Opcode* opcodes;
    int count;
    int capacity;
} OpcodeArray;

bool parse(TokenArray* token_array, OpcodeArray* opcode_array);
void free_opcode_array(OpcodeArray* opcode_array);

#endif // PARSER_H