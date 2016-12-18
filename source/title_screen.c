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
    
    drawing_set_fill_texture(&titleTexture, TITLE_BANNER_WIDTH, TITLE_BANNER_HEIGHT);
    drawing_draw_textured_rect(x, y, TITLE_BANNER_WIDTH, TITLE_BANNER_HEIGHT);
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
    if (!(pressStartBlinkCounter & 0x20))
    {
        text_init();
        text_draw_string(gDisplayWidth / 2, 300, true, "Press Start");
    }
    pressStartBlinkCounter++;
}

void title_screen_init(void)
{
    menu_init(&titleMenu);
    drawing_set_2d_mode();
    text_set_font_size(16, 32);
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
