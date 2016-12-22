#ifndef GUARD_DRAWING_H
#define GUARD_DRAWING_H

void drawing_set_2d_mode(void);
void drawing_set_3d_mode(void);
void drawing_draw_solid_rect(int x, int y, int width, int height);
void drawing_draw_outline_rect(int x, int y, int width, int height);
void drawing_draw_textured_rect(int x, int y, int width, int height);
void drawing_draw_line(int x1, int y1, int x2, int y2);
void drawing_set_fill_color(u8 r, u8 g, u8 b, u8 a);
void drawing_set_fill_texture(GXTexObj *texture, int width, int height);

#endif //GUARD_DRAWING_H
