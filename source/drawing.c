#include "global.h"
#include "drawing.h"

static u8 fillColor[4] ATTRIBUTE_ALIGN(32) = {255, 0, 0, 255};
static int textureWidth;
static int textureHeight;

void drawing_set_2d_mode(void)
{
    Mtx44 projectionMtx;
    Mtx positionMtx;
    
    guOrtho(projectionMtx, 0, gDisplayHeight, 0, gDisplayWidth, 0.0, 1.0);
    GX_LoadProjectionMtx(projectionMtx, GX_ORTHOGRAPHIC);
    guMtxIdentity(positionMtx);
    GX_LoadPosMtxImm(positionMtx, GX_PNMTX0);
}

void drawing_set_3d_mode(void)
{
    Mtx44 projectionMtx;
    
    guPerspective(projectionMtx, 60, (float)gDisplayWidth / (float)gDisplayHeight, 0.2, 300.0);
    GX_LoadProjectionMtx(projectionMtx, GX_PERSPECTIVE);
}

void drawing_draw_solid_rect(int x, int y, int width, int height)
{
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position2u16(x, y);
    GX_Color4u8(fillColor[0], fillColor[1], fillColor[2], fillColor[3]);
    GX_Position2u16(x + width, y);
    GX_Color4u8(fillColor[0], fillColor[1], fillColor[2], fillColor[3]);
    GX_Position2u16(x + width, y + height);
    GX_Color4u8(fillColor[0], fillColor[1], fillColor[2], fillColor[3]);
    GX_Position2u16(x, y + height);
    GX_Color4u8(fillColor[0], fillColor[1], fillColor[2], fillColor[3]);
    GX_End();
}

void drawing_draw_outline_rect(int x, int y, int width, int height)
{
    GX_Begin(GX_LINESTRIP, GX_VTXFMT0, 5);
    GX_Position2u16(x, y);
    GX_Color4u8(fillColor[0], fillColor[1], fillColor[2], fillColor[3]);
    GX_Position2u16(x + width, y);
    GX_Color4u8(fillColor[0], fillColor[1], fillColor[2], fillColor[3]);
    GX_Position2u16(x + width, y + height);
    GX_Color4u8(fillColor[0], fillColor[1], fillColor[2], fillColor[3]);
    GX_Position2u16(x, y + height);
    GX_Color4u8(fillColor[0], fillColor[1], fillColor[2], fillColor[3]);
    GX_Position2u16(x, y);
    GX_Color4u8(fillColor[0], fillColor[1], fillColor[2], fillColor[3]);
    GX_End();
}

void drawing_draw_textured_rect(int x, int y, int width, int height)
{
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position2u16(x, y);
    GX_TexCoord2u16(0, 0);
    GX_Position2u16(x + width, y);
    GX_TexCoord2u16(textureWidth, 0);
    GX_Position2u16(x + width, y + height);
    GX_TexCoord2u16(textureWidth, textureHeight);
    GX_Position2u16(x, y + height);
    GX_TexCoord2u16(0, textureHeight);
    GX_End();
}

void drawing_draw_line(int x1, int y1, int x2, int y2)
{
    GX_Begin(GX_LINES, GX_VTXFMT0, 2);
    GX_Position2u16(x1, y1);
    GX_Color4u8(fillColor[0], fillColor[1], fillColor[2], fillColor[3]);
    GX_Position2u16(x2, y2);
    GX_Color4u8(fillColor[0], fillColor[1], fillColor[2], fillColor[3]);
    GX_End();
}

void drawing_set_fill_color(u8 r, u8 g, u8 b, u8 a)
{
    fillColor[0] = r;
    fillColor[1] = g;
    fillColor[2] = b;
    fillColor[3] = a;
    
    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
}

void drawing_set_fill_texture(GXTexObj *texture, int width, int height)
{
    textureWidth = width;
    textureHeight = height;
    
    GX_SetNumTevStages(1);
    GX_LoadTexObj(texture, GX_TEXMAP0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
    GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, 1, 1);
    
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U16, 0);
}
