#include "global.h"
#include "file.h"
#include "drawing.h"
#include "inventory.h"
#include "text.h"
#include "world.h"

//Item Icons TPL Data
#include "item_icons_tpl.h"
#include "item_icons.h"

#define INVENTORY_TILE_WIDTH 64
#define INVENTORY_TILE_HEIGHT 64
#define ITEM_ICON_WIDTH 32
#define ITEM_ICON_HEIGHT 32

//Item icon IDs
enum
{
    ICON_STONE,
    ICON_SAND,
    ICON_DIRT,
    ICON_GRASS,
    ICON_WOOD,
    ICON_TREE,
    ICON_LEAVES,
    ICON_GAMECUBE
};

int inventorySelection;

static TPLFile itemIconsTPL;
static GXTexObj itemIconsTexture;
static const u8 itemIconTable[] =
{
    [BLOCK_STONE] = ICON_STONE,
    [BLOCK_SAND] = ICON_SAND,
    [BLOCK_DIRT] = ICON_DIRT,
    [BLOCK_GRASS] = ICON_GRASS,
    [BLOCK_WOOD] = ICON_WOOD,
    [BLOCK_TREE] = ICON_TREE,
    [BLOCK_LEAVES] = ICON_LEAVES,
    [BLOCK_GAMECUBE] = ICON_GAMECUBE
};

static int get_nonempty_slots_count(void)
{
    int n = 0;
    
    for (int i = 0; i < NUM_ITEM_SLOTS; i++)
    {
        if (gSaveFile.inventory[i].count > 0)
            n++;
    }
    return n;
}

void inventory_draw(void)
{
    int width = INVENTORY_TILE_WIDTH * NUM_ITEM_SLOTS;
    int height = INVENTORY_TILE_HEIGHT;
    int x = (gDisplayWidth - width) / 2;
    int y = gDisplayHeight - height;
    int selectionRectX = x + inventorySelection * INVENTORY_TILE_WIDTH;
    int selectionRectY = y;
    int iconOffsetX = (INVENTORY_TILE_WIDTH - ITEM_ICON_WIDTH) / 2;
    int iconOffsetY = (INVENTORY_TILE_HEIGHT - ITEM_ICON_HEIGHT) / 2;
    
    //Draw background
    drawing_set_fill_color(20, 20, 20, 180);
    drawing_draw_solid_rect(x, y, width, height);
    
    //Draw item icons
    GX_LoadTexObj(&itemIconsTexture, GX_TEXMAP0);
    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
    GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, ITEM_ICON_WIDTH, ITEM_ICON_HEIGHT);
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U16, 0);
    GX_Begin(GX_QUADS, GX_VTXFMT0, get_nonempty_slots_count() * 4);
    for (int i = 0; i < NUM_ITEM_SLOTS; i++)
    {
        if (gSaveFile.inventory[i].count > 0)
        {
            int icon = itemIconTable[gSaveFile.inventory[i].type];
            int iconX = x + i * INVENTORY_TILE_WIDTH + iconOffsetX;
            int iconY = y + iconOffsetY;
            
            GX_Position2u16(iconX, iconY);
            GX_TexCoord2u16(icon, 0);
            GX_Position2u16(iconX + ITEM_ICON_WIDTH, iconY);
            GX_TexCoord2u16(icon + 1, 0);
            GX_Position2u16(iconX + ITEM_ICON_WIDTH, iconY + ITEM_ICON_HEIGHT);
            GX_TexCoord2u16(icon + 1, 1);
            GX_Position2u16(iconX, iconY + ITEM_ICON_HEIGHT);
            GX_TexCoord2u16(icon, 1);
        }
    }
    GX_End();
    
    //Draw item quantities
    text_set_font_size(16, 32);
    text_init();
    for (int i = 0; i < NUM_ITEM_SLOTS; i++)
    {
        if (gSaveFile.inventory[i].count > 0)
            text_draw_string_formatted(x + i * INVENTORY_TILE_WIDTH, y, 0, "%i", gSaveFile.inventory[i].count);
    }
    
    //Draw selection rectangle
    drawing_set_fill_color(255, 255, 255, 255);
    drawing_draw_outline_rect(selectionRectX, selectionRectY, INVENTORY_TILE_WIDTH, INVENTORY_TILE_HEIGHT);
    
}

void inventory_add_block(int type)
{
    //Check to see if a slot with the item already exists
    for (int i = 0; i < NUM_ITEM_SLOTS; i++)
    {
        if (gSaveFile.inventory[i].type == type && gSaveFile.inventory[i].count < 99)
        {
            gSaveFile.inventory[i].count++;
            return;
        }
    }
    
    //Check for an empty slot
    for (int i = 0; i < NUM_ITEM_SLOTS; i++)
    {
        if (gSaveFile.inventory[i].count == 0)
        {
            gSaveFile.inventory[i].type = type;
            gSaveFile.inventory[i].count = 1;
            return;
        }
    }
}

void inventory_init(void)
{
    inventorySelection = 0;
}

void inventory_load_textures(void)
{
    TPL_OpenTPLFromMemory(&itemIconsTPL, (void *)item_icons_tpl, item_icons_tpl_size);
    TPL_GetTexture(&itemIconsTPL, itemIconsTextureId, &itemIconsTexture);
    GX_InitTexObjFilterMode(&itemIconsTexture, GX_NEAR, GX_NEAR);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_InvalidateTexAll();
}
