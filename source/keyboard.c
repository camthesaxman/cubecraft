#include "global.h"
#include "drawing.h"
#include "keyboard.h"
#include "main.h"
#include "text.h"

#define STANDARD_KEY_WIDTH 40
#define STANDARD_KEY_HEIGHT 40
#define PADDING 10

struct KeyboardKey
{
    int width;
    int height;
    char *text;
    void (*func)(void);
};

struct KeyboardRow
{
    struct KeyboardKey *keys;
    int nKeys;
};

static void ok_func(void);
static void cancel_func(void);

struct KeyboardKey row1keys[] = {
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "1", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "2", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "3", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "4", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "5", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "6", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "7", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "8", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "9", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "0", NULL},
};

struct KeyboardKey row2keys[] = {
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "Q", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "W", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "E", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "R", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "T", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "Y", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "U", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "I", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "O", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "P", NULL},
};

struct KeyboardKey row3keys[] = {
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "A", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "S", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "D", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "F", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "G", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "H", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "J", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "K", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "L", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, ";", NULL},
};

struct KeyboardKey row4keys[] = {
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "Z", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "X", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "C", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "V", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "B", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "N", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "M", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, ",", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, ".", NULL},
    {STANDARD_KEY_WIDTH, STANDARD_KEY_HEIGHT, "/", NULL},
};

struct KeyboardKey row5keys[] = {
    {50, STANDARD_KEY_HEIGHT, "OK", ok_func},
    {100, STANDARD_KEY_HEIGHT, "Cancel", cancel_func},
};

struct KeyboardRow keyboard[] = {
    {row1keys, ARRAY_LENGTH(row1keys)},
    {row2keys, ARRAY_LENGTH(row2keys)},
    {row3keys, ARRAY_LENGTH(row3keys)},
    {row4keys, ARRAY_LENGTH(row4keys)},
    {row5keys, ARRAY_LENGTH(row5keys)},
};

static const char *title;
static char *textBuffer;
static int textBufferLength;
static int textBufferPos;
static int selectedKeyX, selectedKeyY;
static bool analogStickHeldX, analogStickHeldY;
static int result;

static void ok_func(void)
{
    result = KEYBOARD_OK;
}

static void cancel_func(void)
{
    result = KEYBOARD_CANCEL;
}

void keyboard_init(const char *message, char *buffer, int bufferLength)
{
    title = message;
    textBuffer = buffer;
    textBufferLength = bufferLength;
    textBufferPos = 0;
    selectedKeyX = 0;
    selectedKeyY = 0;
    analogStickHeldX = false;
    analogStickHeldY = false;
    result = 0;
}

static void draw_title(void)
{
    text_init();
    text_draw_string(gDisplayWidth / 2, 50, TEXT_HCENTER, title);
}

static void draw_text_entry(void)
{
    drawing_set_fill_color(0, 0, 0, 100);
    drawing_draw_solid_rect(50, 100, 400, 16 + 2 * PADDING);
    text_init();
    text_draw_string(50 + PADDING, 100 + PADDING, 0, textBuffer);
}

static void draw_keys(void)
{
    int x;
    int y;
    
    y = 150;
    drawing_set_fill_color(0, 0, 0, 100);
    for (int i = 0; i < ARRAY_LENGTH(keyboard); i++)
    {
        struct KeyboardKey *keys = keyboard[i].keys;
        
        x = 50;
        for (int j = 0; j < keyboard[i].nKeys; j++)
        {
            if (selectedKeyX == j && selectedKeyY == i)
                drawing_set_fill_color(255, 255, 255, 100);
            drawing_draw_solid_rect(x, y, keys[j].width, keys[j].height);
            if (selectedKeyX == j && selectedKeyY == i)
                drawing_set_fill_color(0, 0, 0, 100);
            x += keys[j].width + PADDING;
        }
        y += STANDARD_KEY_HEIGHT + PADDING;
    }
    
    y = 150;
    text_init();
    for (int i = 0; i < ARRAY_LENGTH(keyboard); i++)
    {
        struct KeyboardKey *keys = keyboard[i].keys;
        
        x = 50;
        for (int j = 0; j < keyboard[i].nKeys; j++)
        {
            text_draw_string(x + keys[j].width / 2, y + keys[j].height / 2,
                             TEXT_HCENTER | TEXT_VCENTER, keys[j].text);
            x += keys[j].width + PADDING;
        }
        y += STANDARD_KEY_HEIGHT + PADDING;
    }
}

void keyboard_draw(void)
{
    text_set_font_size(16, 32);
    draw_title();
    draw_text_entry();
    draw_keys();
}

int keyboard_process_input(void)
{
    int analogStickDirX = 0;
    int analogStickDirY = 0;
    
    if (gAnalogStickX > 50 || gAnalogStickX < -50)
    {
        if (!analogStickHeldX)
        {
            analogStickDirX = gAnalogStickX;
            analogStickHeldX = true;
        }
    }
    else
    {
        analogStickHeldX = false;
    }
    
    if (gAnalogStickY > 50 || gAnalogStickY < -50)
    {
        if (!analogStickHeldY)
        {
            analogStickDirY = gAnalogStickY;
            analogStickHeldY = true;
        }
    }
    else
    {
        analogStickHeldY = false;
    }
    
    if (gControllerPressedKeys & PAD_BUTTON_A)
    {
        struct KeyboardKey *key = &keyboard[selectedKeyY].keys[selectedKeyX];
        
        if (key->func != NULL)
        {
            //Call the key's special function
            key->func();
        }
        else
        {
            //Put the letter on the key into the text buffer
            if (textBufferPos < textBufferLength - 1)
            {
                textBuffer[textBufferPos] = key->text[0];
                textBufferPos++;
            }
        }
    }
    else if (gControllerPressedKeys & PAD_BUTTON_B)
    {
        //Backspace
        textBufferPos = MAX(0, textBufferPos - 1);
        for (char *c = textBuffer + textBufferPos; *c != '\0'; c++)
            *c = *(c + 1);
    }
    else if (analogStickDirX > 0 || (gControllerPressedKeys & PAD_BUTTON_RIGHT))
    {
        selectedKeyX++;
        if (selectedKeyX == keyboard[selectedKeyY].nKeys)
            selectedKeyX = 0;
    }
    else if (analogStickDirX < 0 || (gControllerPressedKeys & PAD_BUTTON_LEFT))
    {
        selectedKeyX--;
        if (selectedKeyX == -1)
            selectedKeyX = keyboard[selectedKeyY].nKeys - 1;
    }
    else if (analogStickDirY > 0 || (gControllerPressedKeys & PAD_BUTTON_UP))
    {
        puts("asdf");
        selectedKeyY--;
        if (selectedKeyY == -1)
            selectedKeyY = ARRAY_LENGTH(keyboard) - 1;
        selectedKeyX = MIN(selectedKeyX, keyboard[selectedKeyY].nKeys - 1);
    }
    else if (analogStickDirY < 0 || (gControllerPressedKeys & PAD_BUTTON_DOWN))
    {
        puts("foo");
        selectedKeyY++;
        if (selectedKeyY == ARRAY_LENGTH(keyboard))
            selectedKeyY = 0;
        selectedKeyX = MIN(selectedKeyX, keyboard[selectedKeyY].nKeys - 1);
    }
    
    printf("selected key: %i, %i\n", selectedKeyX, selectedKeyY);
    return result;
}
