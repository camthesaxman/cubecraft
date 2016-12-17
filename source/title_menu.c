#include "global.h"
#include "drawing.h"
#include "field.h"
#include "main.h"
#include "menu.h"
#include "text.h"
#include "title_menu.h"

static struct MenuItem titleMenuItems[] = {
    {"Start Game"},
    {"Exit to Homebrew Channel"},
};

static struct Menu titleMenu = {
    "Main Menu",
    titleMenuItems,
    ARRAY_LENGTH(titleMenuItems),
};

static void title_menu_main(void)
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

static void title_menu_draw(void)
{
    menu_draw();
}

void title_menu_init(void)
{
    menu_init(&titleMenu);
    drawing_set_2d_mode();
    set_main_callback(title_menu_main);
    set_draw_callback(title_menu_draw);
}
