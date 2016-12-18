#ifndef GUARD_TEXT_H
#define GUARD_TEXT_H

#define TEX_GLYPH_WIDTH 8
#define TEX_GLYPH_HEIGHT 16

void text_load_textures(void);
void text_set_font_size(int height, int width);
void text_init(void);
void text_draw_string(int x, int y, bool center, const char *string);
void text_draw_string_formatted(int x, int y, bool center, const char *fmt, ...);

#endif //GUARD_TEXT_H
