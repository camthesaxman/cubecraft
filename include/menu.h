#ifndef GUARD_MENU_H
#define GUARD_MENU_H

struct MenuItem
{
    char *text;
};

struct Menu
{
    char *title;
    struct MenuItem *items;
    int nItems;
};

void menu_init(const struct Menu *menu);
int menu_process_input(void);
void menu_draw(void);

#endif //GUARD_MENU_H
