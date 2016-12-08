#ifndef GUARD_TEXT_H
#define GUARD_TEXT_H

extern GXTexObj fontTexture;

void text_load_textures(void);
void text_draw_string(int x, int y, bool center, char *string);
void text_draw_string_formatted(int x, int y, bool center, char *fmt, ...);

#endif //GUARD_TEXT_H
