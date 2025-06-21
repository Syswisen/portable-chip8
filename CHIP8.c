#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <CHIP8.h>

void chip8_reset(struct chip8 *chip8)
{
    memset(chip8->memory, 0x00, sizeof(chip8->memory) / sizeof(*chip8->memory));
    memset(chip8->keyboard_state, 0, sizeof(chip8->keyboard_state) / sizeof(*chip8->keyboard_state));
    memset(chip8->screen_buffer, 0x00, sizeof(chip8->screen_buffer) / sizeof(*chip8->screen_buffer));
    memset(chip8->registers, 0x00, sizeof(chip8->registers) / sizeof(*chip8->registers));
    memset(chip8->stack, 0x0000, sizeof(chip8->stack) / sizeof(*chip8->stack));
    chip8->stack_pointer = 0x0000;
    chip8->current_instruction = 0x0000;
    chip8->program_counter = 0x0200;
    chip8->index_register = 0x0000;
    chip8->delay_timer = 0;
    chip8->sound_timer = 0;

    uint8_t font_data[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80, // F
    };
    for (int i = 0; i < sizeof(font_data) / sizeof(*font_data); i++) {
        chip8->memory[i] = font_data[i];
    }
}

int chip8_load_program(struct chip8 *chip8, const char *file_path)
{
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        return 0;
    }
    unsigned char byte_buffer;
    int bytes_of_memory = sizeof(chip8->memory) / sizeof(*chip8->memory);
    for (int i = 0x200; i < bytes_of_memory; i++) {
        if (fread(&byte_buffer, sizeof(unsigned char), 1, file) == 0) {
            break;
        }
        chip8->memory[i] = byte_buffer;
    }
    fclose(file);
    return 1;
}

int chip8_set_key_state(struct chip8 *chip8, int key_value, bool new_state)
{
    if (key_value < 0 | key_value > 0xF) {
        return 0;
    }

    chip8->keyboard_state[key_value] = new_state;
    return 1;
}

int chip8_tick_frame(struct chip8 *chip8)
{
    chip8_update_timers(chip8);

    chip8->vblank_state = true;  // Vblank state is true once per frame to accurately emulate the timing of draw instructions
    for (int i = 0; i < chip8->instructions_per_frame; i++) {
        chip8_fetch_next_instruction(chip8);
        if (!chip8_execute_current_instruction(chip8)) {
            return 0;
        }
        chip8->vblank_state = false;
    }

    memcpy(chip8->last_frame_keyboard_state, chip8->keyboard_state, sizeof(chip8->keyboard_state));

    return 1;
}

bool *chip8_get_pixel(struct chip8 *chip8, int x, int y)
{
    if (x < 0 | x >= chip8->screen_width | y < 0 | y >= chip8->screen_height) {
        return NULL;
    }

    return &chip8->screen_buffer[x + (y * chip8->screen_width)];
}

int chip8_get_screen_width(struct chip8 *chip8)
{
    return chip8->screen_width;
}

int chip8_get_screen_height(struct chip8 *chip8)
{
    return chip8->screen_height;
}

bool chip8_should_sound_play(struct chip8 *chip8)
{
    return (chip8->sound_timer > 0);
}

int chip8_set_instructions_per_frame(struct chip8 *chip8, int amount)
{
    if (amount < 0) {
        return 0;
    }

    chip8->instructions_per_frame = amount;
    return 1;
}

bool chip8_is_instruction_valid(struct chip8 *chip8, uint16_t instruction)
{
    uint8_t instruction_index = 0;
    void (**instruction_jump_table_pointer)(struct chip8 *chip8) = chip8->instruction_jump_table;
    int instruction_jump_table_length = 1;
    uint8_t most_significant_half_byte = instruction >> 12;
    switch (most_significant_half_byte) {
        case 0x0:
            instruction_index = instruction & 0x00FF;
            instruction_jump_table_pointer = chip8->zero_instruction_jump_table;
            instruction_jump_table_length = sizeof(chip8->zero_instruction_jump_table) / sizeof(*chip8->zero_instruction_jump_table);
            break;
        case 0x8:
            instruction_index = instruction & 0x000F;
            instruction_jump_table_pointer = chip8->eight_instruction_jump_table;
            instruction_jump_table_length = sizeof(chip8->eight_instruction_jump_table) / sizeof(*chip8->eight_instruction_jump_table);
            break;
        case 0xE:
            instruction_index = instruction & 0x00FF;
            instruction_jump_table_pointer = chip8->e_instruction_jump_table;
            instruction_jump_table_length = sizeof(chip8->e_instruction_jump_table) / sizeof(*chip8->e_instruction_jump_table);
            break;
        case 0xF:
            instruction_index = instruction & 0x00FF;
            instruction_jump_table_pointer = chip8->f_instruction_jump_table;
            instruction_jump_table_length = sizeof(chip8->f_instruction_jump_table) / sizeof(*chip8->f_instruction_jump_table);
            break;
    }
    if (instruction_index >= instruction_jump_table_length) {
        return false;
    }
    if (instruction_jump_table_pointer[instruction_index] == NULL) {
        return false;
    }
    return true;
}

void chip8_update_timers(struct chip8 *chip8)
{
    if (chip8->delay_timer > 0) {
        chip8->delay_timer -= 1;
    }

    if (chip8->sound_timer > 0) {
        chip8->sound_timer -= 1;
    }
}

void chip8_fetch_next_instruction(struct chip8 *chip8) 
{
    chip8->current_instruction = (chip8->memory[chip8->program_counter] << 8) | chip8->memory[chip8->program_counter + 1];
    chip8->program_counter += 2;
}

int chip8_execute_current_instruction(struct chip8 *chip8)
{
    /*
    The instruction jump table is used to efficiently execute the current instruction, by using the most significant half-byte of the 
    current instruction as the index to run the instruction's correct function. 
    However, when the most significant half-byte is 0 or 8 in hexadecimal, another function is run which will then run the correct function 
    by using the least significant half-byte as the index in another instruction jump table. The same also goes for when the most significant 
    half-byte is F or E, except the least significant byte is used as the index instead.
    The reason for this is because the CHIP-8 has instructions which are not decoded solely by checking the most significant half-byte.
    */
    
    if (!chip8_is_instruction_valid(chip8, chip8->current_instruction)) {
        return 0;
    }

    uint8_t most_significant_half_byte = chip8->current_instruction >> 12;
    chip8->instruction_jump_table[most_significant_half_byte](chip8);
    return 1;
}

void chip8_lookup_in_zero_table(struct chip8 *chip8)
{
    uint8_t least_significant_byte = chip8->current_instruction & 0x00FF;
    chip8->zero_instruction_jump_table[least_significant_byte](chip8);
}

void chip8_lookup_in_eight_table(struct chip8 *chip8)
{
    uint8_t least_significant_half_byte = chip8->current_instruction & 0x000F;
    chip8->eight_instruction_jump_table[least_significant_half_byte](chip8);
}

void chip8_lookup_in_e_table(struct chip8 *chip8)
{
    uint8_t least_significant_byte = chip8->current_instruction & 0x00FF;
    chip8->e_instruction_jump_table[least_significant_byte](chip8);
}

void chip8_lookup_in_f_table(struct chip8 *chip8)
{
    uint8_t least_significant_byte = chip8->current_instruction & 0x00FF;
    chip8->f_instruction_jump_table[least_significant_byte](chip8);
}

void chip8_instruction_00E0(struct chip8 *chip8)
{
    // Instruction: Clear screen
    if (!chip8->vblank_state) {
        chip8->program_counter -= 2;
        return;
    }
    memset(&(chip8->screen_buffer), 0x00, chip8->screen_width * chip8->screen_height);
}

void chip8_instruction_00EE(struct chip8 *chip8)
{
    // Instruction: Return from subroutine
    chip8->program_counter = chip8->stack[chip8->stack_pointer];
    chip8->stack[chip8->stack_pointer] = 0x0000;
    chip8->stack_pointer--;
}

void chip8_instruction_1NNN(struct chip8 *chip8)
{
    // Instruction: Jump to NNN
    uint16_t jump_address = chip8->current_instruction & 0x0FFF;
    chip8->program_counter = jump_address;
}

void chip8_instruction_2NNN(struct chip8 *chip8)
{
    // Instruction: Call subroutine
    chip8->stack_pointer++;
    chip8->stack[chip8->stack_pointer] = chip8->program_counter;
    uint16_t subroutine_address = chip8->current_instruction & 0x0FFF;
    chip8->program_counter = subroutine_address;
}

void chip8_instruction_3XNN(struct chip8 *chip8)
{
    // Instruction: Skip next instruction if register X == NN
    uint8_t register_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t value_to_compare = chip8->current_instruction & 0x00FF;
    if (chip8->registers[register_index] == value_to_compare) {
        chip8->program_counter += 2;
    }
}

void chip8_instruction_4XNN(struct chip8 *chip8)
{
    // Instruction: Skip next instruction if register X != NN
    uint8_t register_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t value_to_compare = chip8->current_instruction & 0x00FF;
    if (chip8->registers[register_index] != value_to_compare) {
        chip8->program_counter += 2;
    }
}

void chip8_instruction_5XY0(struct chip8 *chip8)
{
    // Instruction: If register X == register Y, skip next instruction
    uint8_t register_x_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t register_y_index = (chip8->current_instruction & 0x00F0) >> 4;
    if (chip8->registers[register_x_index] == chip8->registers[register_y_index]) {
        chip8->program_counter += 2;
    }
}

void chip8_instruction_6XNN(struct chip8 *chip8)
{
    // Instruction: Set register X to NN
    uint8_t register_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t value_to_set = chip8->current_instruction & 0x00FF;    
    chip8->registers[register_index] = value_to_set;
}

void chip8_instruction_7XNN(struct chip8 *chip8)
{
    // Instruction: Add NN to register X
    uint8_t register_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t value_to_add = chip8->current_instruction & 0x00FF;
    chip8->registers[register_index] += value_to_add;
}

void chip8_instruction_8XY0(struct chip8 *chip8)
{
    // Instruction: Set register X to register Y
    uint8_t register_x_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t register_y_index = (chip8->current_instruction & 0x00F0) >> 4;
    chip8->registers[register_x_index] = chip8->registers[register_y_index];
}

void chip8_instruction_8XY1(struct chip8 *chip8)
{
    // Instruction: Set register X to the result of bitwise register X OR register Y
    uint8_t register_x_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t register_y_index = (chip8->current_instruction & 0x00F0) >> 4;
    chip8->registers[register_x_index] |= chip8->registers[register_y_index];
    chip8->registers[0xF] = 0;
}

void chip8_instruction_8XY2(struct chip8 *chip8)
{
    // Instruction: Set register X to the result of bitwise register X AND register Y
    uint8_t register_x_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t register_y_index = (chip8->current_instruction & 0x00F0) >> 4;
    chip8->registers[register_x_index] &= chip8->registers[register_y_index];
    chip8->registers[0xF] = 0;
}

void chip8_instruction_8XY3(struct chip8 *chip8)
{
    // Instruction: Set register X to the result of bitwise register X XOR register Y
    uint8_t register_x_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t register_y_index = (chip8->current_instruction & 0x00F0) >> 4;
    chip8->registers[register_x_index] ^= chip8->registers[register_y_index];
    chip8->registers[0xF] = 0;
}

void chip8_instruction_8XY4(struct chip8 *chip8)
{
    // Instruction: Add register Y to register X
    uint8_t register_x_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t register_y_index = (chip8->current_instruction & 0x00F0) >> 4;
    uint8_t result = chip8->registers[register_x_index] + chip8->registers[register_y_index];
    uint8_t carry = result < chip8->registers[register_x_index];
    chip8->registers[register_x_index] = result;
    chip8->registers[0xF] = carry;
}

void chip8_instruction_8XY5(struct chip8 *chip8)
{
    // Instruction: Subtract register Y from register X
    uint8_t register_x_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t register_y_index = (chip8->current_instruction & 0x00F0) >> 4;
    uint8_t result = chip8->registers[register_x_index] - chip8->registers[register_y_index];
    uint8_t carry = result < chip8->registers[register_x_index];
    chip8->registers[register_x_index] = result;
    chip8->registers[0xF] = carry;
}

void chip8_instruction_8XY6(struct chip8 *chip8)
{
    // Instruction: Set register X to register Y, shift register X one bit right, and then set register F to the shifted bit
    uint8_t register_x_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t register_y_index = (chip8->current_instruction & 0x00F0) >> 4;
    uint8_t result = chip8->registers[register_y_index] >> 1;
    uint8_t shifted_bit = chip8->registers[register_y_index] & 1;
    chip8->registers[register_x_index] = result;
    chip8->registers[0xF] = shifted_bit;
}

void chip8_instruction_8XY7(struct chip8 *chip8)
{
    // Instruction: Subtract register X from register Y and store result in register X
    uint8_t register_x_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t register_y_index = (chip8->current_instruction & 0x00F0) >> 4;
    uint8_t result = chip8->registers[register_y_index] - chip8->registers[register_x_index];
    uint8_t carry = result < chip8->registers[register_y_index];
    chip8->registers[register_x_index] = result;
    chip8->registers[0xF] = carry;
}

void chip8_instruction_8XYE(struct chip8 *chip8)
{
    // Instruction: Set register X to register Y, shift register X one bit left, and then set register F to the shifted bit
    uint8_t register_x_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t register_y_index = (chip8->current_instruction & 0x00F0) >> 4;
    uint8_t result = chip8->registers[register_y_index] << 1;
    uint8_t shifted_bit = (chip8->registers[register_y_index] & 128) >> 7;
    chip8->registers[register_x_index] = result;
    chip8->registers[0xF] = shifted_bit;
}

void chip8_instruction_9XY0(struct chip8 *chip8)
{
    // Instruction: Skip next instruction if register X != register Y
    uint8_t register_x_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t register_y_index = (chip8->current_instruction & 0x00F0) >> 4;
    if (chip8->registers[register_x_index] != chip8->registers[register_y_index]) {
        chip8->program_counter += 2;
    }
}

void chip8_instruction_ANNN(struct chip8 *chip8)
{
    // Instruction: Set index register to NNN
    uint16_t address_to_set = chip8->current_instruction & 0x0FFF;
    chip8->index_register = address_to_set;
}

void chip8_instruction_BNNN(struct chip8 *chip8)
{
    // Instruction: Jump to NNN + register 0
    uint16_t jump_address = (chip8->current_instruction & 0x0FFF) + chip8->registers[0];
    chip8->program_counter = jump_address;
}

void chip8_instruction_CXNN(struct chip8 *chip8)
{
    // Instruction: Generate a random value from 0 to 255, bitwise AND it with NN, and then store it in register X
    uint8_t register_index = (chip8->current_instruction & 0x0F00) >> 8;
    uint8_t and_value = chip8->current_instruction & 0x00FF;
    int random_value = rand() % 256;
    random_value &= and_value;
    chip8->registers[register_index] = random_value;
}

void chip8_instruction_DXYN(struct chip8 *chip8)
{
    // Instruction: Draw sprite from memory
    if (!chip8->vblank_state) {
        chip8->program_counter -= 2;
        return;
    }

    uint8_t clear_flag = 0;
    uint8_t register_y_index = (chip8->current_instruction & 0x00F0) >> 4;
    int y = chip8->registers[register_y_index] % chip8->screen_height;
    int row_count = chip8->current_instruction & 0x000F;
    for (int row = 0; row < row_count; row++) {
        uint8_t register_x_index = (chip8->current_instruction & 0x0F00) >> 8;
        int x = chip8->registers[register_x_index] % chip8->screen_width;
        int pixel_row = chip8->memory[chip8->index_register + row];
        for (int column = 0; column < 8; column++) {
            bool sprite_pixel = (pixel_row >> (7 - column)) & 1;
            bool *screen_pixel = chip8_get_pixel(chip8, x, y);
            if (sprite_pixel & *screen_pixel) {
                *screen_pixel = 0;
                clear_flag = 1;
            }
            else if (sprite_pixel == 1) {
                *screen_pixel = 1;
            }
            x++;
            if (x >= chip8->screen_width) {
                break;
            }
        }
        y++;
        if (y >= chip8->screen_height) {
            break;
        }
    }
    chip8->registers[0xF] = clear_flag;
}

void chip8_instruction_EX9E(struct chip8 *chip8)
{
    // Instruction: Skip next instruction if the key corresponding to the value in register X is pressed
    uint8_t register_index = (chip8->current_instruction & 0x0F00) >> 8;
    if (chip8->keyboard_state[chip8->registers[register_index]]) {
        chip8->program_counter += 2;
    }
}

void chip8_instruction_EXA1(struct chip8 *chip8)
{
    // Instruction: Skip next instruction if the key corresponding to the value in register X is pressed
    uint8_t register_index = (chip8->current_instruction & 0x0F00) >> 8;
    if (!(chip8->keyboard_state[chip8->registers[register_index]])) {
        chip8->program_counter += 2;
    }
}

void chip8_instruction_FX07(struct chip8 *chip8)
{
    // Instruction: Set register X to the current value of the delay timer
    uint8_t register_index = (chip8->current_instruction & 0x0F00) >> 8;
    chip8->registers[register_index] = chip8->delay_timer;
}

void chip8_instruction_FX0A(struct chip8 *chip8)
{
    // Instruction: Wait until the key corresponding to the value in register X is pressed and then released
    uint8_t register_index = (chip8->current_instruction & 0x0F00) >> 8;
    int key_value = chip8->registers[register_index];
    bool is_key_released = chip8->last_frame_keyboard_state[key_value] & !(chip8->keyboard_state[key_value]);
    if (!is_key_released) {
        chip8->program_counter -= 2;
    }
}

void chip8_instruction_FX15(struct chip8 *chip8)
{
    // Instruction: Set the delay timer to register X
    uint8_t register_index = (chip8->current_instruction & 0x0F00) >> 8;
    chip8->delay_timer = chip8->registers[register_index];
}

void chip8_instruction_FX18(struct chip8 *chip8)
{
    // Instruction: Set the sound timer to register X
    uint8_t register_index = (chip8->current_instruction & 0x0F00) >> 8;
    chip8->sound_timer = chip8->registers[register_index];
}

void chip8_instruction_FX1E(struct chip8 *chip8)
{
    // Instruction: Add register X to index register
    uint8_t register_index = (chip8->current_instruction & 0x0F00) >> 8;
    chip8->index_register += chip8->registers[register_index];
}

void chip8_instruction_FX29(struct chip8 *chip8)
{
    // Instruction: Point the index register to the first byte in memory of the character value in register X
    uint8_t register_index = (chip8->current_instruction & 0x0F00) >> 8;
    int character_value = chip8->registers[register_index];
    chip8->index_register = chip8->memory[character_value * 5];
}

void chip8_instruction_FX33(struct chip8 *chip8)
{
    // Instruction: Store the three decimal digits of register X left to right starting at the location pointed to by the index register
    int digits[3] = {0};
    
    int i = 2;
    int register_value = chip8->registers[(chip8->current_instruction & 0x0F00) >> 8];
    while (register_value != 0) {
        digits[i] = register_value % 10;
        i--;
        register_value /= 10;
    }

    chip8->memory[chip8->index_register] = digits[0];
    chip8->memory[chip8->index_register + 1] = digits[1];
    chip8->memory[chip8->index_register + 2] = digits[2];
}

void chip8_instruction_FX55(struct chip8 *chip8)
{
    // Instruction: Store registers 0 through X in memory starting at the location pointed to by the index register 
    uint8_t register_x_index = (chip8->current_instruction & 0x0F00) >> 8;
    for (int i = 0; i < register_x_index + 1; i++) {
        chip8->memory[chip8->index_register] = chip8->registers[i];
        chip8->index_register++;
    }
}

void chip8_instruction_FX65(struct chip8 *chip8)
{
    // Instruction: Load memory starting at the location pointed to by the index register into registers 0 through X
    uint8_t register_x_index = (chip8->current_instruction & 0x0F00) >> 8;
    for (int i = 0; i < register_x_index + 1; i++) {
        chip8->registers[i] = chip8->memory[chip8->index_register];
        chip8->index_register++;
    }
}

int chip8_initialize(struct chip8 *chip8, int instructions_per_frame)
{
    chip8_reset(chip8);

    if (!chip8_set_instructions_per_frame(chip8, instructions_per_frame)) {
        return 0;
    }

    chip8->screen_width = 64;
    chip8->screen_height = 32;

    chip8->instructions_per_frame = instructions_per_frame;
    chip8->instruction_jump_table[0x0] = chip8_lookup_in_zero_table;
    chip8->instruction_jump_table[0x1] = chip8_instruction_1NNN;
    chip8->instruction_jump_table[0x2] = chip8_instruction_2NNN;
    chip8->instruction_jump_table[0x3] = chip8_instruction_3XNN;
    chip8->instruction_jump_table[0x4] = chip8_instruction_4XNN;
    chip8->instruction_jump_table[0x5] = chip8_instruction_5XY0;
    chip8->instruction_jump_table[0x6] = chip8_instruction_6XNN;
    chip8->instruction_jump_table[0x7] = chip8_instruction_7XNN;
    chip8->instruction_jump_table[0x8] = chip8_lookup_in_eight_table;
    chip8->instruction_jump_table[0x9] = chip8_instruction_9XY0;
    chip8->instruction_jump_table[0xA] = chip8_instruction_ANNN;
    chip8->instruction_jump_table[0xB] = chip8_instruction_BNNN;
    chip8->instruction_jump_table[0xC] = chip8_instruction_CXNN;
    chip8->instruction_jump_table[0xD] = chip8_instruction_DXYN;
    chip8->instruction_jump_table[0xE] = chip8_lookup_in_e_table;
    chip8->instruction_jump_table[0xF] = chip8_lookup_in_f_table;
    chip8->zero_instruction_jump_table[0xE0] = chip8_instruction_00E0;
    chip8->zero_instruction_jump_table[0xEE] = chip8_instruction_00EE;
    chip8->eight_instruction_jump_table[0x0] = chip8_instruction_8XY0;
    chip8->eight_instruction_jump_table[0x1] = chip8_instruction_8XY1;
    chip8->eight_instruction_jump_table[0x2] = chip8_instruction_8XY2;
    chip8->eight_instruction_jump_table[0x3] = chip8_instruction_8XY3;
    chip8->eight_instruction_jump_table[0x4] = chip8_instruction_8XY4;
    chip8->eight_instruction_jump_table[0x5] = chip8_instruction_8XY5; 
    chip8->eight_instruction_jump_table[0x6] = chip8_instruction_8XY6; 
    chip8->eight_instruction_jump_table[0x7] = chip8_instruction_8XY7; 
    chip8->eight_instruction_jump_table[0xE] = chip8_instruction_8XYE;
    chip8->e_instruction_jump_table[0x9E] = chip8_instruction_EX9E; 
    chip8->e_instruction_jump_table[0xA1] = chip8_instruction_EXA1;
    chip8->f_instruction_jump_table[0x07] = chip8_instruction_FX07; 
    chip8->f_instruction_jump_table[0x0A] = chip8_instruction_FX0A; 
    chip8->f_instruction_jump_table[0x15] = chip8_instruction_FX15; 
    chip8->f_instruction_jump_table[0x18] = chip8_instruction_FX18; 
    chip8->f_instruction_jump_table[0x1E] = chip8_instruction_FX1E; 
    chip8->f_instruction_jump_table[0x29] = chip8_instruction_FX29; 
    chip8->f_instruction_jump_table[0x33] = chip8_instruction_FX33; 
    chip8->f_instruction_jump_table[0x55] = chip8_instruction_FX55;
    chip8->f_instruction_jump_table[0x65] = chip8_instruction_FX65;

    return 1;
}
