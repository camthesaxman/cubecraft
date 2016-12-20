#ifndef GUARD_TEXT_H
#define GUARD_TEXT_H

#define TEXT_HCENTER (1 << 0)
#define TEXT_VCENTER (1 << 1)

void text_load_textures(void);
void text_set_font_size(int height, int width);
void text_init(void);
void text_draw_string(int x, int y, int alignment, const char *string);
void text_draw_string_formatted(int x, int y, int alignment, const char *fmt, ...);

#endif //GUARD_TEXT_H
