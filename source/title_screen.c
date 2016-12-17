#include "global.h"
#include "drawing.h"
#include "field.h"
#include "main.h"
#include "menu.h"
#include "text.h"
#include "title_screen.h"

//Title TPL data
#include "title_tpl.h"
#include "title.h"

#define TITLE_BANNER_WIDTH 432
#define TITLE_BANNER_HEIGHT 72

static TPLFile titleTPL;
static GXTexObj titleTexture;
static unsigned int pressStartBlinkCounter;

static struct MenuItem titleMenuItems[] = {
    {"Start Game"},
    {"Exit to Homebrew Channel"},
};

static struct Menu titleMenu = {
    "Main Menu",
    titleMenuItems,
    ARRAY_LENGTH(titleMenuItems),
};

static void title_screen_main(void);
static void title_screen_draw(void);

static void draw_title_banner(void)
{
    int x = (gDisplayWidth - TITLE_BANNER_WIDTH) / 2;
    int y = 100;
    
    GX_LoadTexObj(&titleTexture, GX_TEXMAP0);
    GX_SetNumTevStages(1);
    
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
    
    GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, 1, 1);
    
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U16, 0);
    
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position2u16(x, y);
    GX_TexCoord2u16(0, 0);
    GX_Position2u16(x + TITLE_BANNER_WIDTH, y);
    GX_TexCoord2u16(TITLE_BANNER_WIDTH, 0);
    GX_Position2u16(x + TITLE_BANNER_WIDTH, y + TITLE_BANNER_HEIGHT);
    GX_TexCoord2u16(TITLE_BANNER_WIDTH, TITLE_BANNER_HEIGHT);
    GX_Position2u16(x, y + TITLE_BANNER_HEIGHT);
    GX_TexCoord2u16(0, TITLE_BANNER_HEIGHT);
    GX_End();
}

static void main_menu_main(void)
{
    if (gControllerPressedKeys & PAD_BUTTON_B)
    {
        set_main_callback(title_screen_main);
        set_draw_callback(title_screen_draw);
        pressStartBlinkCounter = 0;
    }
    else
    {
        switch (menu_process_input())
        {
            case 0:  //Start Game
                field_init();
                break;
            case 1:  //Exit to Homebrew Channel
                exit(0);
                break;
        }
    }
}

static void main_menu_draw(void)
{
    draw_title_banner();
    menu_draw();
}

static void title_screen_main(void)
{
    if (gControllerPressedKeys & PAD_BUTTON_START)
    {
        set_main_callback(main_menu_main);
        set_draw_callback(main_menu_draw);
    }
}

static void title_screen_draw(void)
{
    draw_title_banner();
    if (!(pressStartBlinkCounter & 0x10))
        text_draw_string(gDisplayWidth / 2, 200, true, "Press Start");
    pressStartBlinkCounter++;
}

void title_screen_init(void)
{
    menu_init(&titleMenu);
    drawing_set_2d_mode();
    set_main_callback(title_screen_main);
    set_draw_callback(title_screen_draw);
    pressStartBlinkCounter = 0;
}

void title_screen_load_textures(void)
{
    TPL_OpenTPLFromMemory(&titleTPL, (void *)title_tpl, title_tpl_size);
    TPL_GetTexture(&titleTPL, titleTextureId, &titleTexture);
    GX_InitTexObjFilterMode(&titleTexture, GX_NEAR, GX_NEAR);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_InvalidateTexAll();
}
