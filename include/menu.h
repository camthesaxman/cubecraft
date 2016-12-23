#ifndef GUARD_MENU_H
#define GUARD_MENU_H

struct MenuItem
{
    char *text;
};

struct Menu
{
    const char *title;
    struct MenuItem *items;
    int nItems;
};

#define MENU_CANCEL -2
#define MENU_NORESULT -1

void menu_init(const struct Menu *menu);
void menu_close(void);
int menu_process_input(void);
void menu_msgbox_init(const char *text);
void menu_msgbox_close(void);
bool menu_msgbox_process_input(void);
void menu_draw(void);

#endif //GUARD_MENU_H
