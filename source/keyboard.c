#include "global.h"
#include "drawing.h"
#include "keyboard.h"
#include "main.h"
#include "text.h"

#define STANDARD_KEY_WIDTH 36
#define STANDARD_KEY_HEIGHT 36
#define PADDING 8

struct KeyboardKey
{
    int width;
    char *text;
    void (*func)(void);  //Special function. Standard letter keys have this set to null.
};

struct KeyboardRow
{
    int x;
    struct KeyboardKey *keys;
    int nKeys;
};

static void backspace_func(void);
static void caps_func(void);
static void shift_func(void);
static void space_func(void);
static void ok_func(void);
static void cancel_func(void);

struct KeyboardKey row1keys[] = {
    {STANDARD_KEY_WIDTH, "1", NULL},
    {STANDARD_KEY_WIDTH, "2", NULL},
    {STANDARD_KEY_WIDTH, "3", NULL},
    {STANDARD_KEY_WIDTH, "4", NULL},
    {STANDARD_KEY_WIDTH, "5", NULL},
    {STANDARD_KEY_WIDTH, "6", NULL},
    {STANDARD_KEY_WIDTH, "7", NULL},
    {STANDARD_KEY_WIDTH, "8", NULL},
    {STANDARD_KEY_WIDTH, "9", NULL},
    {STANDARD_KEY_WIDTH, "0", NULL},
    {STANDARD_KEY_WIDTH, "-", NULL},
    {60, "<-", backspace_func},
};

struct KeyboardKey row2keys[] = {
    {STANDARD_KEY_WIDTH, "q", NULL},
    {STANDARD_KEY_WIDTH, "w", NULL},
    {STANDARD_KEY_WIDTH, "e", NULL},
    {STANDARD_KEY_WIDTH, "r", NULL},
    {STANDARD_KEY_WIDTH, "t", NULL},
    {STANDARD_KEY_WIDTH, "y", NULL},
    {STANDARD_KEY_WIDTH, "u", NULL},
    {STANDARD_KEY_WIDTH, "i", NULL},
    {STANDARD_KEY_WIDTH, "o", NULL},
    {STANDARD_KEY_WIDTH, "p", NULL},
    {STANDARD_KEY_WIDTH, "[", NULL},
    {STANDARD_KEY_WIDTH, "]", NULL},
};

struct KeyboardKey row3keys[] = {
    {100, "Caps", caps_func},
    {STANDARD_KEY_WIDTH, "a", NULL},
    {STANDARD_KEY_WIDTH, "s", NULL},
    {STANDARD_KEY_WIDTH, "d", NULL},
    {STANDARD_KEY_WIDTH, "f", NULL},
    {STANDARD_KEY_WIDTH, "g", NULL},
    {STANDARD_KEY_WIDTH, "h", NULL},
    {STANDARD_KEY_WIDTH, "j", NULL},
    {STANDARD_KEY_WIDTH, "k", NULL},
    {STANDARD_KEY_WIDTH, "l", NULL},
    {STANDARD_KEY_WIDTH, ";", NULL},
    {STANDARD_KEY_WIDTH, "'", NULL},
};

struct KeyboardKey row4keys[] = {
    {120, "Shift\0", shift_func},
    {STANDARD_KEY_WIDTH, "z", NULL},
    {STANDARD_KEY_WIDTH, "x", NULL},
    {STANDARD_KEY_WIDTH, "c", NULL},
    {STANDARD_KEY_WIDTH, "v", NULL},
    {STANDARD_KEY_WIDTH, "b", NULL},
    {STANDARD_KEY_WIDTH, "n", NULL},
    {STANDARD_KEY_WIDTH, "m", NULL},
    {STANDARD_KEY_WIDTH, ",", NULL},
    {STANDARD_KEY_WIDTH, ".", NULL},
    {STANDARD_KEY_WIDTH, "/", NULL},
};

struct KeyboardKey row5keys[] = {
    {300, "Space\0", space_func},
};

struct KeyboardKey row6keys[] = {
    {50, "OK", ok_func},
    {100, "Cancel", cancel_func},
};

struct KeyboardRow keyboard[] = {
    {60, row1keys, ARRAY_LENGTH(row1keys)},
    {80, row2keys, ARRAY_LENGTH(row2keys)},
    {0, row3keys, ARRAY_LENGTH(row3keys)},
    {0, row4keys, ARRAY_LENGTH(row4keys)},
    {150, row5keys, ARRAY_LENGTH(row5keys)},
    {0, row6keys, ARRAY_LENGTH(row6keys)},
};

static const char *title;
static char *textBuffer;
static int textBufferLength;
static int textBufferPos;
static int selectedKeyX, selectedKeyY;
static bool kbShift;
static bool kbCaps;
static bool analogStickHeldX, analogStickHeldY;
static int result;
static unsigned int caretBlinkCounter;

static void change_case(bool upper)
{
    for (int i = 0; i < ARRAY_LENGTH(keyboard); i++)
    {
        for (int j = 0; j < keyboard[i].nKeys; j++)
        {
            struct KeyboardKey *key = &keyboard[i].keys[j];
            
            //If there's no special function, then this is a letter key, and is affected by case.
            //BUG: The last letter in 5 letter words ("Shift" and "Space") seems to get affected by this.
            //I'm adding '\0' to the end of them to work around this.
            if (key->func == NULL)
                key->text[0] = upper ? toupper(key->text[0]) : tolower(key->text[0]);
        }
    }
}

static void add_char(int c)
{
    if (textBufferPos < textBufferLength - 1)
    {
        textBuffer[textBufferPos] = c;
        textBufferPos++;
    }
    
    if (!(gControllerHeldKeys & PAD_TRIGGER_L))
    {
        kbShift = false;
        change_case(kbCaps);
    }
}

static void backspace_func(void)
{
    textBufferPos = MAX(0, textBufferPos - 1);
    for (char *c = textBuffer + textBufferPos; *c != '\0'; c++)
        *c = *(c + 1);
}

static void ok_func(void)
{
    result = KEYBOARD_OK;
}

static void cancel_func(void)
{
    result = KEYBOARD_CANCEL;
}

static void caps_func(void)
{
    kbCaps = !kbCaps;
    change_case(kbCaps);
}

static void shift_func(void)
{
    kbShift = true;
    change_case(!kbCaps);
}

static void space_func(void)
{
    add_char(' ');
}

void keyboard_init(const char *message, char *buffer, int bufferLength)
{
    title = message;
    textBuffer = buffer;
    textBufferLength = bufferLength;
    textBufferPos = strlen(buffer);
    selectedKeyX = 0;
    selectedKeyY = 0;
    kbShift = false;
    kbCaps = false;
    analogStickHeldX = false;
    analogStickHeldY = false;
    result = 0;
    caretBlinkCounter = 0;
}

static void draw_title(void)
{
    text_init();
    text_draw_string(gDisplayWidth / 2, 50, TEXT_HCENTER, title);
}

static void draw_text_entry(void)
{
    int x = 20;
    int y = 100;
    
    drawing_set_fill_color(0, 0, 0, 150);
    drawing_draw_solid_rect(x, y, 600, 32 + 2 * PADDING);
    text_init();
    text_draw_string(x + PADDING, y + PADDING, 0, textBuffer);
    drawing_set_fill_color(255, 255, 255, 255);
    if (!(caretBlinkCounter & 0x20))
    {
        drawing_draw_line(x + PADDING + 16 * textBufferPos, y + PADDING,
                          x + PADDING + 16 * textBufferPos, y + PADDING + 32);
    }
    caretBlinkCounter++;
}

static void draw_keys(void)
{
    int x;
    int y;
    
    y = 162;
    drawing_set_fill_color(0, 0, 0, 150);
    for (int i = 0; i < ARRAY_LENGTH(keyboard); i++)
    {
        struct KeyboardKey *keys = keyboard[i].keys;
        
        x = 20 + keyboard[i].x;
        for (int j = 0; j < keyboard[i].nKeys; j++)
        {
            bool altColor = false;
            
            if (selectedKeyX == j && selectedKeyY == i)
            {
                altColor = true;
                drawing_set_fill_color(255, 255, 255, 150);
            }
            else
            {
                if ((i == 2 && j == 0 && kbCaps)   //Caps is active
                 || (i == 3 && j == 0 && kbShift)) //Shift is active
                {
                    altColor = true;
                    drawing_set_fill_color(150, 150, 150, 150);
                }
            }
            drawing_draw_solid_rect(x, y, keys[j].width, STANDARD_KEY_HEIGHT);
            if (altColor)
                drawing_set_fill_color(0, 0, 0, 150);
            x += keys[j].width + PADDING;
        }
        y += STANDARD_KEY_HEIGHT + PADDING;
    }
    
    y = 162;
    text_init();
    for (int i = 0; i < ARRAY_LENGTH(keyboard); i++)
    {
        struct KeyboardKey *keys = keyboard[i].keys;
        
        x = 20 + keyboard[i].x;
        for (int j = 0; j < keyboard[i].nKeys; j++)
        {
            text_draw_string(x + keys[j].width / 2, y + STANDARD_KEY_HEIGHT / 2,
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
    
    result = 0;
    
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
    
    if (gControllerPressedKeys & PAD_TRIGGER_L)
    {
        kbShift = true;
        change_case(!kbCaps);
    }
    else if (gControllerReleasedKeys & PAD_TRIGGER_L)
    {
        kbShift = false;
        change_case(kbCaps);
    }
    
    if (gControllerPressedKeys & PAD_BUTTON_A)
    {
        struct KeyboardKey *key = &keyboard[selectedKeyY].keys[selectedKeyX];
        
        if (key->func != NULL)
            key->func();
        else
            add_char(key->text[0]);
    }
    else if (gControllerPressedKeys & PAD_BUTTON_B)
    {
        backspace_func();
    }
    else if (gControllerPressedKeys & PAD_BUTTON_START)
    {
        //Move to OK button
        selectedKeyX = 0;
        selectedKeyY = 5;
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
        selectedKeyY--;
        if (selectedKeyY == -1)
            selectedKeyY = ARRAY_LENGTH(keyboard) - 1;
        selectedKeyX = MIN(selectedKeyX, keyboard[selectedKeyY].nKeys - 1);
    }
    else if (analogStickDirY < 0 || (gControllerPressedKeys & PAD_BUTTON_DOWN))
    {
        selectedKeyY++;
        if (selectedKeyY == ARRAY_LENGTH(keyboard))
            selectedKeyY = 0;
        selectedKeyX = MIN(selectedKeyX, keyboard[selectedKeyY].nKeys - 1);
    }
    
    printf("selected key: %i, %i\n", selectedKeyX, selectedKeyY);
    return result;
}
