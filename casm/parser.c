#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "lexer.h"
#include "parser.h"


typedef struct {
    Token* tokens;
    int position;
    int count;
    bool hasError;
} Parser;

#define MAX_LABELS 256
typedef struct {
    char name[32];
    int memory_offset;
} LabelDefinition;

static LabelDefinition label_definitions[MAX_LABELS];
static int label_definitions_count = 0;

void error(const char* msg);
static uint16_t parse_instruction(Parser* parser, int memory_offset);
static void add_opcode(OpcodeArray* opcode_array, uint16_t opcode, int memory_offset);
static uint16_t parse_register(Parser* parser);
static uint16_t parse_immediate(Parser* parser);
void collect_label_definitions(Parser* parser);
static void parse_label_definition(Parser* parser, Token* token, int memory_offset);
static int find_label_memory_offset(const char* label);
static int find_label_index(const char* label);

static void expect_token(Parser* parser, const char* expected);
static bool is_register(const char* value);
static int is_immediate(const char* value);
static Token* cur_token(Parser* parser);
static Token* next_token(Parser* parser);
static bool is_eof_token(Token* token);

static uint16_t handle_ld(Parser* parser);
static uint16_t handle_ld_v(Parser* parser, Token* token_first_param, Token* token_second_param);
static uint16_t handle_ld_i(Token* token_first_param, Token* token_second_param);
static uint16_t handle_ld_f(Token* token_first_param, Token* token_second_param);
static uint16_t handle_ld_b(Token* token_first_param, Token* token_second_param);
static uint16_t handle_ld_i_addr(Parser* parser, Token* token_first_param, Token* token_second_param);
static uint16_t handle_ld_dt(Token* token_first_param, Token* token_second_param);
static uint16_t handle_ld_st(Token* token_first_param, Token* token_second_param);

static uint16_t handle_cls(Parser* parser);
static uint16_t handle_ret(Parser* parser);
static uint16_t handle_sys(Parser* parser);
static uint16_t handle_call(Parser* parser);
static uint16_t handle_add(Parser* parser);
static uint16_t handle_jp(Parser* parser);
static uint16_t handle_se(Parser* parser);
static uint16_t handle_sne(Parser* parser);
static uint16_t handle_8xy_instruction(Parser* parser, uint8_t instruction);
static uint16_t handle_ex_instruction(Parser* parser, uint8_t instruction);
static uint16_t handle_rnd(Parser* parser);
static uint16_t handle_drw(Parser* parser);
static uint16_t handle_shl_shr(Parser* parser, const char* type);

void error(Parser* parser, Token* token, const char* fmt, ...) {
    parser->hasError = true;
    va_list args;
    va_start(args, fmt);
    printf("Error <%d>: ", token->line);
    vprintf(fmt, args);
    va_end(args);
}

bool parse(TokenArray* token_array, OpcodeArray* opcode_array) {
    Parser parser;
    parser.tokens = token_array->tokens;
    parser.position = 0;
    parser.count = token_array->count;
    parser.hasError = false;

    opcode_array->opcodes = malloc(16 * sizeof(Opcode));
    opcode_array->count = 0;
    opcode_array->capacity = 16;

    // Collect label definitions before parsing the program
    collect_label_definitions(&parser);

    int memory_offset = 0x200; // Start of the program memory in CHIP-8

    while (parser.position < parser.count-1) { // -1: dont consider EOF token
        uint16_t opcode = parse_instruction(&parser, memory_offset);
        if (opcode != NULL) {
            add_opcode(opcode_array, opcode, memory_offset);
            memory_offset += 2; // Increment the memory_offset by 2 since opcodes are 2 bytes long
        }
    }

    return !parser.hasError;
}

static uint16_t parse_instruction(Parser* parser, int memory_offset) {
    Token* token = next_token(parser);
    uint16_t opcode = 0;

    if (token->value[strlen(token->value) - 1] == ':') {
        return NULL; // Do not generate an opcode for the label definition
    }
    else if (strcmp(token->value, "LD") == 0) {
        opcode = handle_ld(parser);
    }
    else if (strcmp(token->value, "ADD") == 0) {
        opcode = handle_add(parser);
    }
    else if (strcmp(token->value, "JP") == 0) {
        opcode = handle_jp(parser);
    }
    else if (strcmp(token->value, "SE") == 0) {
        opcode = handle_se(parser);
    }
    else if (strcmp(token->value, "CLS") == 0) {
        opcode = handle_cls(parser);
    }
    else if (strcmp(token->value, "RET") == 0) {
        opcode = handle_ret(parser);
    }
    else if (strcmp(token->value, "SYS") == 0) {
        opcode = handle_sys(parser);
    }
    else if (strcmp(token->value, "CALL") == 0) {
        opcode = handle_call(parser);
    }
    else if (strcmp(token->value, "SNE") == 0) {
        opcode = handle_sne(parser);
    }
    else if (strcmp(token->value, "OR") == 0) {
        opcode = handle_8xy_instruction(parser, 0x1);
    }
    else if (strcmp(token->value, "AND") == 0) {
        opcode = handle_8xy_instruction(parser, 0x2);
    }
    else if (strcmp(token->value, "XOR") == 0) {
        opcode = handle_8xy_instruction(parser, 0x3);
    }
    else if (strcmp(token->value, "SKP") == 0) {
        opcode = handle_ex_instruction(parser, 0x9E);
    }
    else if (strcmp(token->value, "SKNP") == 0) {
        opcode = handle_ex_instruction(parser, 0xA1);
    }
    else if (strcmp(token->value, "SUB") == 0) {
        opcode = handle_8xy_instruction(parser, 0x5);
    }
    else if (strcmp(token->value, "SUBN") == 0) {
        opcode = handle_8xy_instruction(parser, 0x7);
    }
    else if (strcmp(token->value, "SHL") == 0) {
        return handle_shl_shr(parser, "SHL");
    }
    else if (strcmp(token->value, "SHR") == 0) {
        return handle_shl_shr(parser, "SHR");
    }
    else if (strcmp(token->value, "RND") == 0) {
        opcode = handle_rnd(parser);
    }
    else if (strcmp(token->value, "DRW") == 0) {
        opcode = handle_drw(parser);
    }
    else if (strcmp(token->value, "EOF") == 0) {
        return NULL;
    }
    else {
        error(parser, token, "unknown instruction: '%s'\n", token->value);
        return NULL;
    }

    return opcode;
}

void collect_label_definitions(Parser* parser) {
    int memory_offset = 0x200; // Start of the program memory in CHIP-8
    int position = 0;

    // -1: dont consider EOF token
    while (position < parser->count-1) {
        Token* token = next_token(parser);

        if (token->value[strlen(token->value) - 1] == ':') {
            parse_label_definition(parser, token, memory_offset);
            // Increment the memory_offset by 2 since opcodes are 2 bytes long
            // Do not increment if the label is the last token in the token array
            if (position < parser->count - 2) { // -2: dont consider EOF token
                memory_offset += 2;
            }
        }
        else {
            memory_offset += 2;
        }

        position++;
    }
    parser->position = 0;
}

static void parse_label_definition(Parser* parser, Token* token, int memory_offset) {
    for (int i = 0; i < label_definitions_count; i++) {
        if (strcmp(token->value, label_definitions[i].name) == 0) {
            error(parser, token, "label '%s' already defined\n", token->value);
            return; // Label has already been defined, skip it
        }
    }

    if (label_definitions_count >= MAX_LABELS) {
        error(parser, token, "maximum number of labels reached: max %d\n", token->value, MAX_LABELS);
        return;
    }

    if (memory_offset > 0xFFF) {
        error(parser, token, "memory offset (0x%X) for label '%s' exceeds the 12-bit address space\n", memory_offset, token->value);
        return;
    }

    size_t len = strlen(token->value);
    strncpy(label_definitions[label_definitions_count].name, token->value, len);
    label_definitions[label_definitions_count].name[len - 1] = '\0'; // replace the colon with a null terminator
    label_definitions[label_definitions_count].memory_offset = memory_offset;
    label_definitions_count++;
}

static uint16_t parse_register(Parser* parser, Token* token) {
    if (strlen(token->value) != 2 || token->value[0] != 'V') {
        error(parser, token, "invalid register '%s'\n", token->value);
        return NULL;
    }
    uint16_t reg = token->value[1] - '0';
    if (reg > 15 || reg < 0) {
        error(parser, token, "invalid register number '%s'\n", token->value);
        return NULL;
    }
    return reg;
}

static uint16_t parse_immediate(Parser* parser, Token* token) {
    char* end;
    int base = is_immediate(token->value);
    if (!base) {
        error(parser, token, "invalid immediate value: '%s'\n", token->value);
        return NULL;
    }
    long imm = strtol(token->value, &end, base);
    if (*end != '\0' || imm < 0) {
        error(parser, token, "invalid immediate value: '%s'\n", token->value);
        return NULL;
    }
    if (*end != '\0' || imm < 0 || imm > 0xFFF) {
        error(parser, token, "immediate value '%s' exceeds MAX (0xFFF)\n", token->value);
        return NULL;
    }
    return (uint16_t)imm;
}

static void add_opcode(OpcodeArray* opcode_array, uint16_t opcode, int memory_offset) {
    if (opcode_array->count >= opcode_array->capacity) {
        opcode_array->capacity *= 2;
        opcode_array->opcodes = realloc(opcode_array->opcodes, opcode_array->capacity * sizeof(Opcode));
    }
    opcode_array->opcodes[opcode_array->count].value = opcode;
    opcode_array->opcodes[opcode_array->count].memory_offset = memory_offset;
    opcode_array->count++;
}

static void expect_token(Parser* parser, const char* expected) {
    Token* token = cur_token(parser);
    if (strlen(token->value) != strlen(expected) || strcmp(token->value, expected) != 0) {
        error(parser, token, "expected '%s', but got '%s'\n", expected, token->value);
    }
    next_token(parser);
}

static Token* cur_token(Parser* parser) {
    return &parser->tokens[parser->position];
}

static Token* next_token(Parser* parser) {
    Token* token = cur_token(parser);
    
    if (parser->position >= parser->count) {
        return NULL;
    }

    if (is_eof_token(token)) {
        return token;
    }

    parser->position++;
    return token;
}

static bool is_eof_token(Token* token) {
    return strcmp(token->value, "EOF") == 0;
}

static bool is_register(const char* value) {
    if (strlen(value) != 2) {
        return false;
    }
    return value[0] == 'V' && (value[1] >= '0' && value[1] <= 'F');
}

static int is_immediate(const char* value) {
    int start = 0;
    int base = 10;

    if (value[0] == '0') {
        if (value[1] == 'x' || value[1] == 'X') {
            start = 2;
            base = 16;
        }
        else if (value[1] == 'b' || value[1] == 'B') {
            start = 2;
            base = 2;
        }
    }

    for (int i = start; value[i] != '\0'; i++) {
        if ((base == 16 && !isxdigit(value[i])) || (base == 10 && !isdigit(value[i])) || (base == 2 && (value[i] != '0' && value[i] != '1'))) {
            return 0;
        }
    }
    return base;
}

static int find_label_memory_offset(const char* label) {
    for (int i = 0; i < label_definitions_count; i++) {
        if (strcmp(label, label_definitions[i].name) == 0) {
            return label_definitions[i].memory_offset;
        }
    }
    return -1;
}

static int find_label_index(const char* label) {
    for (int i = 0; i < label_definitions_count; i++) {
        if (strcmp(label, label_definitions[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

static uint16_t handle_ld_v(Parser* parser, Token* token_first_param, Token* token_second_param) {
    uint8_t register_x = token_first_param->value[1] - '0';
    uint16_t opcode;

    if (token_second_param->value[0] == 'V') {
        // 8xy0 - LD Vx, Vy
        uint8_t register_y = token_second_param->value[1] - '0';
        opcode = 0x8000 | (register_x << 8) | (register_y << 4);
    }
    else if (is_immediate(token_second_param->value)) {
        // 6xkk - LD Vx, byte
        uint8_t byte = parse_immediate(parser, token_second_param);
        opcode = 0x6000 | (register_x << 8) | byte;
    }
    else if (strcmp(token_second_param->value, "DT") == 0) {
        // Fx07 - LD Vx, DT
        opcode = 0xF000 | (register_x << 8) | 0x07;
    }
    else if (strcmp(token_second_param->value, "K") == 0) {
        // Fx0A - LD Vx, K
        opcode = 0xF000 | (register_x << 8) | 0x0A;
    }
    else if (strcmp(token_second_param->value, "[I]") == 0) {
        // Fx65 - LD Vx, [I]
        opcode = 0xF000 | (register_x << 8) | 0x65;
    }
    else {
        error(parser, token_second_param, "invalid second LD parameter '%s'\n", token_second_param->value);
        return NULL;
    }

    return opcode;
}

static uint16_t handle_ld_i(Parser* parser, Token* token_first_param, Token* token_second_param) {
    uint16_t address = parse_immediate(parser, token_second_param);
    uint16_t opcode = 0xA000 | address;
    return opcode;
}

static uint16_t handle_ld_f(Token* token_first_param, Token* token_second_param) {
    uint8_t register_x = token_second_param->value[1] - '0';
    uint16_t opcode = 0xF000 | (register_x << 8) | 0x29;
    return opcode;
}

static uint16_t handle_ld_b(Token* token_first_param, Token* token_second_param) {
    uint8_t register_x = token_second_param->value[1] - '0';
    uint16_t opcode = 0xF000 | (register_x << 8) | 0x33;
    return opcode;
}

static uint16_t handle_ld_dt(Token* token_first_param, Token* token_second_param) {
    uint8_t register_x = token_second_param->value[1] - '0';
    uint16_t opcode = 0xF000 | (register_x << 8) | 0x15;
    return opcode;
}

static uint16_t handle_ld_st(Token* token_first_param, Token* token_second_param) {
    uint8_t register_x = token_second_param->value[1] - '0';
    uint16_t opcode = 0xF000 | (register_x << 8) | 0x18;
    return opcode;
}

static uint16_t handle_ld_i_addr(Parser* parser, Token* token_first_param, Token* token_second_param) {
    uint8_t register_x = token_second_param->value[1] - '0';
    uint16_t opcode;

    if (strcmp(token_first_param->value, "[I]") == 0 && token_second_param->value[0] == 'V') {
        opcode = 0xF000 | (register_x << 8) | 0x55;
    }
    else if (token_second_param->value[0] == 'V' && strcmp(token_first_param->value, "V") == 0) {
        opcode = 0xF000 | (register_x << 8) | 0x65;
    }
    else {
        error(parser, token_second_param, "invalid second LD parameter '%s'\n", token_second_param->value);
        return NULL;
    }

    return opcode;
}

void free_opcode_array(OpcodeArray* opcode_array) {
    free(opcode_array->opcodes);
}

/*********************************************************************************
* Opcode handlers
*********************************************************************************/

static uint16_t handle_cls(Parser* parser) {
    return 0x00E0;
}

static uint16_t handle_ret(Parser* parser) {
    return 0x00EE;
}

static uint16_t handle_sys(Parser* parser) {
    Token* token = next_token(parser);
    uint16_t opcode = 0x0000;

    if (is_immediate(token->value)) {
        uint16_t address = parse_immediate(parser, token);
        opcode |= address;
    }
    else {
        int label_index = find_label_index(token->value);

        if (label_index == -1) {
            error(parser, token, "label '%s' not found\n", token->value);
            return NULL;
        }

        opcode |= label_definitions[label_index].memory_offset & 0x0FFF;
    }

    return opcode;
}

static uint16_t handle_call(Parser* parser) {
    Token* token = next_token(parser);
    uint16_t opcode = 0x2000;

    if (is_immediate(token->value)) {
        uint16_t address = parse_immediate(parser, token);
        opcode |= address;
    }
    else {
        int label_index = find_label_index(token->value);

        if (label_index == -1) {
            error(parser, token, "label '%s' not found\n", token->value);
            return NULL;
        }

        opcode |= label_definitions[label_index].memory_offset & 0x0FFF;
    }

    return opcode;
}

static uint16_t handle_jp(Parser* parser) {
    Token* token = next_token(parser);
    uint16_t opcode = 0x1000;

    if (token->value[0] == 'V') {
        if (token->value[1] == '0' && token->value[2] == ',') {
            opcode = 0xB000;
            expect_token(parser, ',');
            token = next_token(parser);
        }
        else {
            error(parser, token, "only register V0 is allowed for JP instruction\n");
            return NULL;
        }
    }

    if (is_immediate(token->value)) {
        uint16_t address = parse_immediate(parser, token);
        if (address > 0xFFF) {
            error(parser, token, "address (0x%X) exceeds the 12-bit address space\n", token->value);
            return NULL;
        }
        opcode |= address;
    }
    else {
        int label_index = find_label_index(token->value);

        if (label_index == -1) {
            error(parser, token, "label '%s' not found\n", token->value);
            return NULL;
        }

        opcode |= label_definitions[label_index].memory_offset & 0x0FFF;
    }

    return opcode;
}

static uint16_t handle_se(Parser* parser) {
    uint16_t opcode = 0x3000;
    opcode |= parse_register(parser, cur_token(parser)) << 8;
    expect_token(parser, ",");
    opcode |= parse_immediate(parser, cur_token(parser));
    return opcode;
}

static uint16_t handle_ld(Parser* parser) {
    // Get first parameter
    Token* token_first_param = next_token(parser);;
    // Expect a comma
    expect_token(parser, ",");
    // Get second parameter
    Token* token_second_param = next_token(parser);;

    uint16_t opcode;

    if (token_first_param->value[0] == 'V') {
        opcode = handle_ld_v(parser, token_first_param, token_second_param);
    }
    else if (token_first_param->value[0] == 'I') {
        opcode = handle_ld_i(parser, token_first_param, token_second_param);
    }
    else if (token_first_param->value[0] == 'F') {
        opcode = handle_ld_f(token_first_param, token_second_param);
    }
    else if (token_first_param->value[0] == 'B') {
        opcode = handle_ld_b(token_first_param, token_second_param);
    }
    else if (strcmp(token_first_param->value, "DT") == 0) {
        opcode = handle_ld_dt(token_first_param, token_second_param);
    }
    else if (strcmp(token_first_param->value, "ST") == 0) {
        opcode = handle_ld_st(token_first_param, token_second_param);
    }
    else if (strcmp(token_first_param->value, "[I]") == 0) {
        opcode = handle_ld_i_addr(parser, token_first_param, token_second_param);
    }
    else {
        error(parser, token_first_param, "invalid first LD parameter '%s'\n", token_first_param->value);
        return NULL;
    }

    return opcode;
}

static uint16_t handle_add(Parser* parser) {
    Token* token_first_param = next_token(parser);
    expect_token(parser, ",");

    // Save the next operand (to check if its a register, an immediate value or I)
    Token* token_second_param = next_token(parser);

    uint16_t opcode;

    if (is_register(token_first_param->value) && is_register(token_second_param->value)) {
        uint16_t x_register = parse_register(parser, token_first_param);
        uint16_t y_register = parse_register(parser, token_second_param);
        opcode = 0x8004 | (x_register << 8) | (y_register << 4);
    }
    else if (is_register(token_first_param->value) && is_immediate(token_second_param->value)) {
        uint16_t x_register = parse_register(parser, token_first_param);
        uint16_t immediate = parse_immediate(parser, token_second_param);
        opcode = 0x7000 | (x_register << 8) | immediate;
    }
    else if (strcmp(token_first_param->value, "I") == 0 && is_register(token_second_param->value)) {
        uint16_t x_register = parse_register(parser, token_second_param);
        opcode = 0xF01E | (x_register << 8);
    }
    else {
        error(parser, token_first_param, "invalid ADD parameters '%s' and '%s'\n", token_first_param->value, token_second_param->value);
        return 0;
    }

    return opcode;
}

static uint16_t handle_sne(Parser* parser) {
    uint16_t opcode;

    Token* token = next_token(parser);
    uint8_t vx = parse_register(parser, token) << 8;

    expect_token(parser, ",");

    token = next_token(parser);
    if (is_register(token->value)) {
        opcode = 0x9000 | vx | (parse_register(parser, token) << 4);
    }
    else if (is_immediate(token->value)) {
        opcode = 0x4000 | vx | parse_immediate(parser, token);
    }
    else {
        error(parser, token, "unexpected operand '%s' for instruction SNE\n", token->value);
        return NULL;
    }

    return opcode;
}

static uint16_t handle_8xy_instruction(Parser* parser, uint8_t instruction) {
    uint16_t opcode = 0x8000 | instruction;

    Token* token = next_token(parser);

    opcode |= parse_register(parser, token) << 8;

    expect_token(parser, ",");

    token = next_token(parser);
    opcode |= parse_register(parser, token) << 4;

    return opcode;
}

static uint16_t handle_ex_instruction(Parser* parser, uint8_t instruction) {
    uint16_t opcode = 0xE000 | instruction;
    Token* token = next_token(parser);
    opcode |= parse_register(parser, token) << 8;

    return opcode;
}

static uint16_t handle_rnd(Parser* parser) {
    uint16_t opcode = 0xC000;

    Token* token = next_token(parser);
    uint8_t vx = parse_register(parser, token) << 8;

    expect_token(parser, ",");

    token = next_token(parser);
    opcode |= vx | parse_immediate(parser, token);

    return opcode;
}

static uint16_t handle_drw(Parser* parser) {
    uint16_t opcode = 0xD000;

    Token* token = next_token(parser);

    uint8_t vx = parse_register(parser, token) << 8;
    uint8_t vy;

    expect_token(parser, ",");

    token = next_token(parser);
    vy = parse_register(parser, token) << 4;

    expect_token(parser, ",");

    token = next_token(parser);
    uint8_t nibble = parse_immediate(parser, token) & 0x0F;

    opcode |= vx | vy | nibble;

    return opcode;
}

static uint16_t handle_shl_shr(Parser* parser, const char* type) {
    Token* token = next_token(parser);
    if (token->value[0] != 'V') {
        error(parser, token, "expected 'V' for register in %s instruction\n", type);
        return NULL;
    }

    uint8_t vx = parse_register(parser, token);
    uint16_t opcode = (strcmp(type, "SHL") == 0) ? 0x800E : 0x8006;

    token = cur_token(parser);
    if (token->value[0] == ',') {
        expect_token(parser, ",");
        token = next_token(parser);
        if (token->value[0] != 'V') {
            error(parser, token, "expected 'V' for register in %s instruction\n", type);
            return NULL;
        }

        uint8_t vy = parse_register(parser, token);
        opcode |= (vy << 4);
    }

    opcode |= (vx << 8);
    return opcode;
}