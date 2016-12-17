#ifndef GUARD_INVENTORY_H
#define GUARD_INVENTORY_H

#define NUM_ITEM_SLOTS 8
#define NO_ITEM = -1;

struct ItemSlot
{
    int type;
    int count;
};

extern struct ItemSlot inventory[NUM_ITEM_SLOTS];
extern int inventorySelection;

void inventory_draw(void);
void inventory_add_block(int type);
void inventory_init(void);
void inventory_load_textures(void);

#endif //GUARD_INVENTORY_H
