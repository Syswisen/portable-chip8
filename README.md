# Portable CHIP-8
A highly portable CHIP-8 emulator compatible with C versions C99 and later, written using only the standard library.

Input, graphics, and sound are not handled by the emulator, but instead must be handled by the user by using the provided functions to register input, display graphics, and play sound.

Note that the emulator aims to accurately emulate the original CHIP-8 for the COSMAC VIP.
# Usage
Initialize the emulator by declaring a struct variable of type chip8 and then calling ```chip8_initialize```.
```c
int chip8_initialize(struct chip8 *chip8, int instructions_per_frame)
// instructions_per_frame: Specifies the amount of instructions that will be executed per frame. Must be a value greater than 0.
// Return: 0 if the value of instructions_per_frame is invalid.
```

Load a program into the emulator using ```chip8_load_program```. Note that this function does not reset the emulator beforehand, but only loads a program into memory.
```c
int chip8_load_program(struct chip8 *chip8, const char *file_path)
// Return: 1 on success and 0 on failure.
```

Register input into the emulator using ```chip8_set_key_state```.
```c
int chip8_set_key_state(struct chip8 *chip8, int key_value, bool new_state)
// key_value: The key's value. Must be a value in the range of 0 to F.
// new_state: The state to set the key to. 1 for pressed and 0 for unpressed.
// Return: 0 if the value of key_value is invalid.
```

Tick the emulator by a frame by calling ```chip8_tick_frame```. For accurate timer emulation, call the function 60 times per second.
```c
int chip8_tick_frame(struct chip8 *chip8)
// Return: 0 if an invalid instruction is encountered.
```

Render the emulator's screen using ```chip8_get_pixel``` to get the value of individual screen pixels. Use ```chip8_get_screen_width``` and ```chip8_get_screen_height``` for looping over screen pixels.
```c
bool *chip8_get_pixel(struct chip8 *chip8, int x, int y)
// Return: 0 if the given coordinates are out of screen bounds.

int chip8_get_screen_width(struct chip8 *chip8)

int chip8_get_screen_height(struct chip8 *chip8)
```

Implement sound emulation using ```chip8_should_sound_play``` to get if sound should be played.
```c
bool chip8_should_sound_play(struct chip8 *chip8)
```

Set the amount of instructions that the emulator executes per frame using ```chip8_set_instructions_per_frame```.
```c
int chip8_set_instructions_per_frame(struct chip8 *chip8, int amount)
// amount: Must be a value greater than 0.
// Return: 0 if the value of amount is invalid.
```

Reset the emulator by calling ```chip8_reset```.
```c
void chip8_reset(struct chip8 *chip8)
```

Debugging functionality such as instruction-level stepping can be implemented using the following functions:

```c
void chip8_update_timers(struct chip8 *chip8)
// Note: Should be called 60 times per second.
```

```c
void chip8_fetch_next_instruction(struct chip8 *chip8)
// Note: Should be called (chip8.instructions_per_frame * 60) times per second.
```

```c
int chip8_execute_current_instruction(struct chip8 *chip8)
// Note: Should always be called after calling chip8_fetch_next_instruction
// Return: 0 if the current instruction is invalid.
```

```c
bool chip8_is_instruction_valid(struct chip8 *chip8, uint16_t instruction)
```
