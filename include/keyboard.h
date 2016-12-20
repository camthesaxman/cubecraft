#ifndef GUARD_KEYBOARD_H
#define GUARD_KEYBOARD_H

#define KEYBOARD_OK 1
#define KEYBOARD_CANCEL -1

void keyboard_init(const char *message, char *buffer, int bufferLength);
void keyboard_draw(void);
int keyboard_process_input(void);

#endif //GUARD_KEYBOARD_H
