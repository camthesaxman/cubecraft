#ifndef GUARD_TEXT_H
#define GUARD_TEXT_H

void text_init(void);
void text_draw_string(int x, int y, bool center, char *string);
void text_draw_string_formatted(int x, int y, bool center, char *fmt, ...);

#endif //GUARD_TEXT_H
