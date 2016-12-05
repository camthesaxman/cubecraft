#include <stdlib.h>
#include "global.h"
#include "drawing.h"
#include "field.h"
#include "main.h"
#include "text.h"
#include "title_menu.h"

static void title_menu_main(void)
{
    if (gControllerPressedKeys & PAD_BUTTON_A)
        field_init();
    else if (gControllerPressedKeys & PAD_BUTTON_B)
        exit(0);
}

static void title_menu_draw(void)
{
    text_draw_string(gDisplayWidth / 2, 100, true, "Welcome to CubeCraft!");
    text_draw_string(gDisplayWidth / 2, 200, true, "Press the A button to start");
    text_draw_string(gDisplayWidth / 2, 216, true, "Press the B button to exit to the Homebrew Channel");
}

void title_menu_init(void)
{
    drawing_set_2d_mode();
    set_main_callback(title_menu_main);
    set_draw_callback(title_menu_draw);
}
