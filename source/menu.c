#include "global.h"
#include "drawing.h"
#include "main.h"
#include "menu.h"
#include "text.h"

#define CHAR_WIDTH 16
#define CHAR_HEIGHT 32
#define PADDING 10
#define ANIM_SPEED 64

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

static bool menuActive = false;
static const struct Menu *currentMenu;
static struct Rectangle menuRect;
static struct Rectangle titleRect;
static struct Rectangle itemsRect;
static int selection;
static bool analogStickHeld;
static bool menuClosing;
static bool menuAnimActive;
static int menuAnimCounter;

static bool msgBoxActive = false;
static const char *msgBoxText;
static bool msgBoxClosing;
static bool msgBoxAnimActive;
static int msgBoxAnimCounter;

//==================================================
// Menu
//==================================================

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
    menuActive = true;
    menuClosing = false;
    menuAnimActive = true;
    menuAnimCounter = 0;
}

void menu_close(void)
{
    assert(false);  //TODO: remove this function
}

int menu_process_input(void)
{
    int analogStickDir = 0;
    
    if (gAnalogStickY > 50 || gAnalogStickY < -50)
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
        return selection;
    else if (gControllerPressedKeys & PAD_BUTTON_B)
        return MENU_CANCEL;
    
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
    return MENU_NORESULT;
}

static void draw_menu(void)
{
    struct Rectangle selectionRect;
    int animOffsetX = 0;
    
    if (menuAnimActive)
    {
        if (menuClosing)
        {
            animOffsetX = -menuAnimCounter * ANIM_SPEED;
            if (menuRect.x + menuRect.width + animOffsetX < 0)
                menuAnimActive = 0;
        }
        else
        {
            int x = gDisplayWidth - menuAnimCounter * ANIM_SPEED;
            
            if (x > menuRect.x)
                animOffsetX = x - menuRect.x;
            else
                menuAnimActive = false;
        }
        menuAnimCounter++;
    }
    
    drawing_set_fill_color(0, 0, 0, 150);
    drawing_draw_solid_rect(animOffsetX + titleRect.x, titleRect.y, titleRect.width, titleRect.height);
    drawing_draw_solid_rect(animOffsetX + itemsRect.x, itemsRect.y, itemsRect.width, itemsRect.height);
    
    text_set_font_size(16, 32);
    text_init();
    text_draw_string(animOffsetX + titleRect.x + PADDING, titleRect.y + PADDING, false, currentMenu->title);
    for (int i = 0; i < currentMenu->nItems; i++)
        text_draw_string(animOffsetX + itemsRect.x + PADDING, itemsRect.y + PADDING + i * (CHAR_HEIGHT + PADDING), false, currentMenu->items[i].text);
        
    selectionRect.x = itemsRect.x + PADDING;
    selectionRect.y = itemsRect.y + PADDING + selection * (CHAR_HEIGHT + PADDING);
    selectionRect.width = itemsRect.width - 2 * PADDING;
    selectionRect.height = CHAR_HEIGHT;
    
    drawing_set_fill_color(255, 255, 255, 255);
    drawing_draw_outline_rect(animOffsetX + selectionRect.x, selectionRect.y, selectionRect.width, selectionRect.height);
}

//==================================================
// Message Box
//==================================================

void menu_msgbox_init(const char *text)
{
    msgBoxText = text;
    msgBoxActive = true;
    msgBoxClosing = false;
    msgBoxAnimActive = true;
    msgBoxAnimCounter = 0;
}

void menu_msgbox_close(void)
{
    msgBoxActive = false;
}

bool menu_msgbox_is_open(void)
{
    return msgBoxActive;
}

bool menu_msgbox_process_input(void)
{
    if (!msgBoxAnimActive)
    {
        if (msgBoxClosing)  //finished closing animation
        {
            msgBoxActive = false;
            return true;
        }
        else if (gControllerPressedKeys & PAD_BUTTON_A)
        {
            //start closing animation
            msgBoxClosing = true;
            msgBoxAnimActive = true;
            msgBoxAnimCounter = 0;
        }
    }
    return false;
}

static void draw_msgbox(void)
{
    int width = 400;
    int height = 200;
    int x = (gDisplayWidth - width) / 2;
    int y = (gDisplayHeight - height) / 2;
    int animOffsetY = 0;
    
    if (msgBoxAnimActive)
    {
        if (msgBoxClosing)
        {
            animOffsetY = msgBoxAnimCounter * ANIM_SPEED;
            if (y + animOffsetY > gDisplayHeight)
                msgBoxAnimActive = false;
        }
        else
        {
            animOffsetY = (gDisplayHeight - msgBoxAnimCounter * ANIM_SPEED) - y;
            if (animOffsetY < 0)
            {
                animOffsetY = 0;
                msgBoxAnimActive = false;
            }
        }
        msgBoxAnimCounter++;
    }
    
    drawing_set_fill_color(0, 0, 0, 150);
    drawing_draw_solid_rect(x, animOffsetY + y, width, height);
    text_init();
    text_set_font_size(16, 32);
    text_draw_string(x + width / 2, animOffsetY + y + height / 2, TEXT_HCENTER | TEXT_VCENTER, msgBoxText);
}

void menu_draw(void)
{
    if (menuActive)
        draw_menu();
    if (msgBoxActive)
        draw_msgbox();
}

static void (*animDoneCallback)(void);

static void menu_wait_main_callback(void)
{
    assert(menuClosing);
    if (!menuAnimActive)
    {
        menuActive = false;
        animDoneCallback();
    }
}

void menu_wait_close_anim(void (*callback)(void))
{
    assert(menuActive);
    menuAnimActive = true;
    menuAnimCounter = 0;
    menuClosing = true;
    animDoneCallback = callback;
    set_main_callback(menu_wait_main_callback);
}
