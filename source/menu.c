#include <string.h>
#include "global.h"
#include "menu.h"
#include "text.h"

#define CHAR_WIDTH 16
#define CHAR_HEIGHT 32
#define PADDING 10

/* Menu layout
 *
 * +-----------------------------+
 * |              A              |
 * |           padding           |
 * |              V              |
 * |<padding>   TITLE   <padding>|
 * |              A              |
 * |           padding           |
 * |              V              |
 * +-----------------------------+
 *                A
 *             padding
 *                V
 * +-----------------------------+
 * |              A              |
 * |           padding           |
 * |              V              |
 * |<padding>MENU ITEM 1<padding>|
 * |              A              |
 * |           padding           |
 * |              V              |
 * |<padding>MENU ITEM 2<padding>|
 * |              A              |
 * |           padding           |
 * |              V              |
 * +-----------------------------+
 */

struct Box
{
    int left;
    int right;
    int top;
    int bottom;
};

struct Rectangle
{
    int x, y;
    int width, height;
};

static const struct Menu *currentMenu;
static struct Rectangle menuRect;
static struct Rectangle titleRect;
static struct Rectangle itemsRect;
static int numChars;
static int selection;
static bool analogStickHeld;

static u8 menuBackgroundColor[] ATTRIBUTE_ALIGN(32) = {0, 0, 0, 80};
static u8 selectionRectColor[] ATTRIBUTE_ALIGN(32) = {255, 255, 255};

void menu_init(const struct Menu *menu)
{
    int len;
    int width = 0;
    
    titleRect.height = PADDING + CHAR_HEIGHT + PADDING;
    itemsRect.height = PADDING + menu->nItems * (CHAR_HEIGHT + PADDING);
    
    //Get width and numChars
    len = strlen(menu->title);
    numChars = len;
    width = PADDING + CHAR_WIDTH * len + PADDING;
    for (int i = 0; i < menu->nItems; i++)
    {
        int len = strlen(menu->items[i].text);
        
        numChars += len;
        width = MAX(width, PADDING + CHAR_WIDTH * len + PADDING);
    }
    menuRect.width = width;
    titleRect.width = width;
    itemsRect.width = width;
    menuRect.height = titleRect.height + PADDING + itemsRect.height;
    
    //Calculate position
    menuRect.x = (gDisplayWidth - menuRect.width) / 2;
    menuRect.y = (gDisplayHeight - menuRect.height) / 2;
    titleRect.x = menuRect.x;
    titleRect.y = menuRect.y;
    itemsRect.x = menuRect.x;
    itemsRect.y = menuRect.y + titleRect.height + PADDING;
    
    currentMenu = menu;
    selection = 0;
    analogStickHeld = false;
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

static void menu_draw_string(int x, int y, const char *string)
{
    for (const char *c = string; *c != '\0'; c++)
    {
        int glyph = *c - ' ';
        
        GX_Position2u16(x, y);
        GX_TexCoord2u16(glyph, 0);
        GX_Position2u16(x + CHAR_WIDTH, y);
        GX_TexCoord2u16(glyph + 1, 0);
        GX_Position2u16(x + CHAR_WIDTH, y + CHAR_HEIGHT);
        GX_TexCoord2u16(glyph + 1, 1);
        GX_Position2u16(x, y + CHAR_HEIGHT);
        GX_TexCoord2u16(glyph, 1);
        
        x += CHAR_WIDTH;
    }
}

void menu_draw(void)
{
    struct Rectangle selectionRect;
    
    GX_SetNumTevStages(1);
    
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX8);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetArray(GX_VA_CLR0, menuBackgroundColor, 4 * sizeof(u8));
    
    GX_Begin(GX_QUADS, GX_VTXFMT0, 8);
    //Title background
    GX_Position2u16(titleRect.x, titleRect.y);
    GX_Color1x8(0);
    GX_Position2u16(titleRect.x + titleRect.width, titleRect.y);
    GX_Color1x8(0);
    GX_Position2u16(titleRect.x + titleRect.width, titleRect.y + titleRect.height);
    GX_Color1x8(0);
    GX_Position2u16(titleRect.x, titleRect.y + titleRect.height);
    GX_Color1x8(0);
    //Items background
    GX_Position2u16(itemsRect.x, itemsRect.y);
    GX_Color1x8(0);
    GX_Position2u16(itemsRect.x + itemsRect.width, itemsRect.y);
    GX_Color1x8(0);
    GX_Position2u16(itemsRect.x + itemsRect.width, itemsRect.y + itemsRect.height);
    GX_Color1x8(0);
    GX_Position2u16(itemsRect.x, itemsRect.y + itemsRect.height);
    GX_Color1x8(0);
    GX_End();
    
    GX_LoadTexObj(&fontTexture, GX_TEXMAP0);
    GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, TEX_GLYPH_WIDTH, TEX_GLYPH_HEIGHT);
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U16, 0);
    
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4 * numChars);
    menu_draw_string(titleRect.x + PADDING, titleRect.y + PADDING, currentMenu->title);
    for (int i = 0; i < currentMenu->nItems; i++)
        menu_draw_string(itemsRect.x + PADDING, itemsRect.y + PADDING + i * (CHAR_HEIGHT + PADDING), currentMenu->items[i].text);
    GX_End();
    
    selectionRect.x = itemsRect.x + PADDING;
    selectionRect.y = itemsRect.y + PADDING + selection * (CHAR_HEIGHT + PADDING);
    selectionRect.width = itemsRect.width - 2 * PADDING;
    selectionRect.height = CHAR_HEIGHT;
    
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX8);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGB, GX_RGB8, 0);
    GX_SetArray(GX_VA_CLR0, selectionRectColor, 3 * sizeof(u8));
    
    GX_Begin(GX_LINESTRIP, GX_VTXFMT0, 5);
    GX_Position2u16(selectionRect.x, selectionRect.y);
    GX_Color1x8(0);
    GX_Position2u16(selectionRect.x + selectionRect.width, selectionRect.y);
    GX_Color1x8(0);
    GX_Position2u16(selectionRect.x + selectionRect.width, selectionRect.y + selectionRect.height);
    GX_Color1x8(0);
    GX_Position2u16(selectionRect.x, selectionRect.y + selectionRect.height);
    GX_Color1x8(0);
    GX_Position2u16(selectionRect.x, selectionRect.y);
    GX_Color1x8(0);
    GX_End();
}
