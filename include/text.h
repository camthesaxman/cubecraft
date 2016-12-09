#ifndef GUARD_TEXT_H
#define GUARD_TEXT_H

#define TEX_GLYPH_WIDTH 8
#define TEX_GLYPH_HEIGHT 16

extern GXTexObj fontTexture;

void text_load_textures(void);
void text_draw_string(int x, int y, bool center, char *string);
void text_draw_string_formatted(int x, int y, bool center, char *fmt, ...);

#endif //GUARD_TEXT_H
