#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main() {
    //const char* code =
    //    "CLS\n"             // Clear the screen
    //    "SNE V0"     // SNE V0, byte: Skip next instruction if V0 != 0x42
    //    "SNE V0, V1\n"       // SNE V0, V1: Skip next instruction if V0 != V1
    //    "OR V0, V1\n"        // OR V0, V1: V0 = V0 | V1
    //    "AND V0, V1\n"       // AND V0, V1: V0 = V0 & V1
    //    "XOR V0, V1\n"       // XOR V0, V1: V0 = V0 ^ V1
    //    "SUB V0, V1\n"       // SUB V0, V1: V0 = V0 - V1
    //    "SHR V0\n"           // SHR V0: V0 = V0 >> 1
    //    "SUBN V0, V1\n"      // SUBN V0, V1: V0 = V1 - V0
    //    "SHL V0\n"           // SHL V0: V0 = V0 << 1
    //    "RND V0, 0x42\n"     // RND V0, byte: V0 = random byte & 0x42
    //    "SKP V0\n"           // SKP V0: Skip next instruction if key with the value of V0 is pressed
    //    "SKNP V0\n"          // SKNP V0: Skip next instruction if key with the value of V0 is not pressed
    //    "DRW V0, V1, 0x5\n"  // DRW V0, V1, nibble: Draw 5-byte sprite at (V0, V1)
    //    ;

    const char* code =
        "CLS\n"             // Clear the screen
        "SYS 0x200\n"       // SYS with immediate address: SYS 0x200
        "CALL label1\n"     // CALL with label: CALL label1
        "RET\n"             // Return from subroutine
        "label1:\n"         // Label definition
        "CLS\n"             // Clear the screen (inside label1)
        "SYS label2\n"      // SYS with label: SYS label2
        "CALL 0x20A\n"      // CALL with immediate address: CALL 0x20A
        "RET\n"             // Return from subroutine
        "label2:\n"         // Label definition
        "CLS\n"             // Clear the screen (inside label2)
        ;

    // Lexer
    TokenArray token_array = tokenize(code);
    printf("Tokens:\n");
    for (int i = 0; i < token_array.count; i++) {
        printf("Token %d (line %d): %s\n", i, token_array.tokens[i].line, token_array.tokens[i].value);
    }

    // Parser
    OpcodeArray opcode_array;
    bool success = parse(&token_array, &opcode_array);

    if (success) {
        printf("\nOpcodes:\n");
        for (int i = 0; i < opcode_array.count; i++) {
            printf("Opcode %d (offset 0x%03X): 0x%04X\n", i, opcode_array.opcodes[i].memory_offset, opcode_array.opcodes[i].value);
        }
    }

    // Free memory
    free_token_array(&token_array);
    free_opcode_array(&opcode_array);

    getchar();
    return 0;
}