#include "global.h"
#include "text.h"

//Font TPL data
#include "font_tpl.h"
#include "font.h"

static TPLFile fontTPL;
static GXTexObj fontTexture;
static int fontHeight;
static int fontWidth;

void text_load_textures(void)
{
    TPL_OpenTPLFromMemory(&fontTPL, (void *)font_tpl, font_tpl_size);
    TPL_GetTexture(&fontTPL, fontTextureId, &fontTexture);
    GX_InitTexObjFilterMode(&fontTexture, GX_NEAR, GX_NEAR);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_InvalidateTexAll();
}

void text_set_font_size(int width, int height)
{
    fontHeight = height;
    fontWidth = width;
}

void text_init(void)
{
    GX_LoadTexObj(&fontTexture, GX_TEXMAP0);
    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
    GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, TEX_GLYPH_WIDTH, TEX_GLYPH_HEIGHT);
    
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U16, 0);
}

void text_draw_string(int x, int y, bool center, const char *string)
{
    int len = strlen(string);
    
    if (center)
        x -= len * fontWidth / 2;
    
    GX_Begin(GX_QUADS, GX_VTXFMT0, len * 4);
    for (int i = 0; i < len; i++)
    {
        int glyph = string[i] - ' ';
        
        GX_Position2u16(x, y);
        GX_TexCoord2u16(glyph, 0);
        GX_Position2u16(x + fontWidth, y);
        GX_TexCoord2u16(glyph + 1, 0);
        GX_Position2u16(x + fontWidth, y + fontHeight);
        GX_TexCoord2u16(glyph + 1, 1);
        GX_Position2u16(x, y + fontHeight);
        GX_TexCoord2u16(glyph, 1);
        
        x += fontWidth;
    }
    GX_End();
}

void text_draw_string_formatted(int x, int y, bool center, const char *fmt, ...)
{
    va_list args;
    size_t bufferSize = 2;
    size_t requiredBufSize;
    char *buffer = malloc(bufferSize);
    
    va_start(args, fmt);
    requiredBufSize = vsnprintf(buffer, bufferSize, fmt, args) + 1;
    va_end(args);
    if (requiredBufSize > bufferSize)
    {
        buffer = realloc(buffer, requiredBufSize);
        va_start(args, fmt);
        vsnprintf(buffer, requiredBufSize, fmt, args);
        va_end(args);
    }
    text_draw_string(x, y, center, buffer);
    free(buffer);
}
