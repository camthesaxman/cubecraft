#include "global.h"
#include "drawing.h"
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
static int selection;
static bool analogStickHeld;

void menu_init(const struct Menu *menu)
{
    int len;
    int width = 0;
    
    titleRect.height = PADDING + CHAR_HEIGHT + PADDING;
    itemsRect.height = PADDING + menu->nItems * (CHAR_HEIGHT + PADDING);
    
    //Get width
    len = strlen(menu->title);
    width = PADDING + CHAR_WIDTH * len + PADDING;
    for (int i = 0; i < menu->nItems; i++)
    {
        int len = strlen(menu->items[i].text);
        
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

void menu_draw(void)
{
    struct Rectangle selectionRect;
    
    drawing_set_fill_color(0, 0, 0, 80);
    drawing_draw_solid_rect(titleRect.x, titleRect.y, titleRect.width, titleRect.height);
    drawing_draw_solid_rect(itemsRect.x, itemsRect.y, itemsRect.width, itemsRect.height);
    
    text_set_font_size(16, 32);
    text_init();
    text_draw_string(titleRect.x + PADDING, titleRect.y + PADDING, false, currentMenu->title);
    for (int i = 0; i < currentMenu->nItems; i++)
        text_draw_string(itemsRect.x + PADDING, itemsRect.y + PADDING + i * (CHAR_HEIGHT + PADDING), false, currentMenu->items[i].text);
        
    selectionRect.x = itemsRect.x + PADDING;
    selectionRect.y = itemsRect.y + PADDING + selection * (CHAR_HEIGHT + PADDING);
    selectionRect.width = itemsRect.width - 2 * PADDING;
    selectionRect.height = CHAR_HEIGHT;
    
    drawing_set_fill_color(255, 255, 255, 255);
    drawing_draw_outline_rect(selectionRect.x, selectionRect.y, selectionRect.width, selectionRect.height);
}
