#include <string.h>
#include "global.h"
#include "menu.h"
#include "text.h"

#define NUM_GLYPHS 128
#define CHAR_WIDTH 16
#define CHAR_HEIGHT 32
#define NUMGLYPHS 128
#define PADDING 10

static const struct Menu *currentMenu;
static int menuX, menuY;
static int menuWidth, menuHeight;
static int numChars;
static int selection;
static bool analogStickHeld;

void menu_init(const struct Menu *menu)
{
    currentMenu = menu;
    selection = 0;
    numChars = 0;
    menuWidth = 0;
    for (int i = 0; i < currentMenu->nItems; i++)
    {
        int len = strlen(currentMenu->items[i].text);
        
        menuWidth = MAX(menuWidth, len * CHAR_WIDTH + 2 * PADDING);
        numChars += len;
    }
    menuHeight = currentMenu->nItems * CHAR_HEIGHT + (currentMenu->nItems + 1) * PADDING;
    menuX = (gDisplayWidth - menuWidth) / 2;
    menuY = (gDisplayHeight - menuHeight) / 2;
}

int menu_process_input(void)
{
    int analogStickDir = 0;
    
    if (gAnalogStickY > 10 || gAnalogStickY < -10)
    {
        if (!analogStickHeld)
        {
            analogStickDir = gAnalogStickY;
            analogStickHeld = true;
        }
    }
    else
    {
        analogStickHeld = false;
    }
    
    if (gControllerPressedKeys & PAD_BUTTON_A)
    {
        return selection;
    }
    else if (analogStickDir > 0 || (gControllerPressedKeys & PAD_BUTTON_UP))
    {
        selection--;
        if (selection < 0)
            selection = currentMenu->nItems - 1;
    }
    else if (analogStickDir < 0 || (gControllerPressedKeys & PAD_BUTTON_DOWN))
    {
        selection++;
        if (selection > currentMenu->nItems - 1)
            selection = 0;
    }
    return -1;
}

void menu_draw(void)
{
    int numChars = 0;
    int textX;
    int textY = menuY + PADDING;
    int selectionRectLeft = menuX + PADDING;
    int selectionRectRight = menuX + menuWidth - PADDING;
    int selectionRectTop = menuY + PADDING + selection * (PADDING + CHAR_HEIGHT);
    int selectionRectBottom = selectionRectTop + CHAR_HEIGHT;
    u8 menuBackgroundColor[] ATTRIBUTE_ALIGN(32) = {255, 0, 0, 80};
    u8 selectionRectColor[] ATTRIBUTE_ALIGN(32) = {255, 255, 255};
    
    for (int i = 0; i < currentMenu->nItems; i++)
        numChars += strlen(currentMenu->items[i].text);
    
    /*
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX8);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetArray(GX_VA_CLR0, menuBackgroundColor, 4 * sizeof(u8));
    
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position2s16(menuX, menuY);
    GX_Color1x8(0);
    GX_Position2s16(menuX + menuWidth, menuY);
    GX_Color1x8(0);
    GX_Position2s16(menuX + menuWidth, menuY + menuHeight);
    GX_Color1x8(0);
    GX_Position2s16(menuX, menuY + menuHeight);
    GX_Color1x8(0);
    GX_End();
    */
    
    GX_LoadTexObj(&fontTexture, GX_TEXMAP0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4 * numChars);
    for (int i = 0; i < currentMenu->nItems; i++)
    {
        textX = (gDisplayWidth - strlen(currentMenu->items[i].text) * CHAR_WIDTH) / 2;
        
        for (const char *c = currentMenu->items[i].text; *c != '\0'; c++)
        {
            int glyphIndex = *c - ' ';
            float glyphLeft = (float)glyphIndex / (float)NUM_GLYPHS;
            float glyphRight = (float)(glyphIndex + 1) / (float)NUM_GLYPHS;
            
            GX_Position2s16(textX, textY);
            GX_TexCoord2f32(glyphLeft, 0.0);
            GX_Position2s16(textX + CHAR_WIDTH, textY);
            GX_TexCoord2f32(glyphRight, 0.0);
            GX_Position2s16(textX + CHAR_WIDTH, textY + CHAR_HEIGHT);
            GX_TexCoord2f32(glyphRight, 1.0);
            GX_Position2s16(textX, textY + CHAR_HEIGHT);
            GX_TexCoord2f32(glyphLeft, 1.0);
            
            textX += CHAR_WIDTH;
        }
        textY += CHAR_HEIGHT + PADDING;
    }
    GX_End();
    
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX8);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGB, GX_RGB8, 0);
    GX_SetArray(GX_VA_CLR0, selectionRectColor, 3 * sizeof(u8));
    
    GX_Begin(GX_LINESTRIP, GX_VTXFMT0, 5);
    GX_Position2s16(selectionRectLeft, selectionRectTop);
    GX_Color1x8(0);
    GX_Position2s16(selectionRectRight, selectionRectTop);
    GX_Color1x8(0);
    GX_Position2s16(selectionRectRight, selectionRectBottom);
    GX_Color1x8(0);
    GX_Position2s16(selectionRectLeft, selectionRectBottom);
    GX_Color1x8(0);
    GX_Position2s16(selectionRectLeft, selectionRectTop);
    GX_Color1x8(0);
    GX_End();
}
