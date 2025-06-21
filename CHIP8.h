#ifndef CHIP8
#define CHIP8

#include <stdint.h>
#include <stdbool.h>

struct chip8 {  // Read-only
    uint8_t memory[4096];
    uint8_t registers[16];
    uint16_t index_register;
    uint16_t program_counter;
    uint16_t current_instruction;
    uint16_t stack[16];
    int stack_pointer; 
    int delay_timer; 
    int sound_timer; 
    bool keyboard_state[16];
    bool last_frame_keyboard_state[0xF];
    bool screen_buffer[64 * 32]; 
    int screen_width; 
    int screen_height; 
    bool vblank_state; 
    int instructions_per_frame; 
    
    void (*instruction_jump_table[0xF])(struct chip8 *chip8);
    void (*zero_instruction_jump_table[0xEF])(struct chip8 *chip8);
    void (*eight_instruction_jump_table[0xF])(struct chip8 *chip8);
    void (*e_instruction_jump_table[0xA2])(struct chip8 *chip8);
    void (*f_instruction_jump_table[0x66])(struct chip8 *chip8);
};

// The amount of instructions per frame must be greater than 0. Otherwise, the function will return 0
int chip8_initialize(struct chip8 *chip8, int instructions_per_frame);

// Returns 1 upon success, and 0 upon failure. NOTE: This function doesn't reset the CHIP-8 before loading the program
int chip8_load_program(struct chip8 *chip8, const char *file_path);

// The key value must be a value from 0 to F. Otherwise, the function will return 0
int chip8_set_key_state(struct chip8 *chip8, int key_value, bool new_state);

// Returns 0 if an invalid instruction was encountered. NOTE: This function should be called 60 times per second for accurate timer emulation
int chip8_tick_frame(struct chip8 *chip8); 

// The coordinates must be in screen bounds. Otherwise, the function will return NULL
bool *chip8_get_pixel(struct chip8 *chip8, int x, int y);

int chip8_get_screen_width(struct chip8 *chip8);

int chip8_get_screen_height(struct chip8 *chip8);

// Returns true when the sound timer is greater than 0
bool chip8_should_sound_play(struct chip8 *chip8);

// The amount must be greater than 0. Otherwise, the function will return 0
int chip8_set_instructions_per_frame(struct chip8 *chip8, int amount);

void chip8_reset(struct chip8 *chip8);

bool chip8_is_instruction_valid(struct chip8 *chip8, uint16_t instruction);

/*
The following three functions are called internally in chip8_tick_frame. 
They should only be used when instruction-level stepping is required(instruction step debug feature, etc.) 
*/

// This function should be called 60 times per second for accurate timer emulation
void chip8_update_timers(struct chip8 *chip8);

// This function should be called chip8.instructions_per_frame times per frame
void chip8_fetch_next_instruction(struct chip8 *chip8);

// Returns 0 if the current instruction is invalid. NOTE: This function should always be called after calling chip8_fetch_next_instruction
int chip8_execute_current_instruction(struct chip8 *chip8);

#endif