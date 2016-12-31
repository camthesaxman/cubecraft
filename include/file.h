#ifndef GUARD_FILE_H
#define GUARD_FILE_H

#include "inventory.h"

#define SAVENAME_MAX 16  //Maximum length of a file name (including the null terminator)
#define SEED_MAX 16      //Maximum length of a seed string (including the null terminator)

struct Chunk;
struct BlockModification;
struct ChunkModification;

struct SaveFile
{
    char name[SAVENAME_MAX];
    char seed[SEED_MAX];
    int spawnX, spawnY, spawnZ;
    struct ItemSlot inventory[NUM_ITEM_SLOTS];
    struct ChunkModification *modifiedChunks;
    int modifiedChunksCount;
};

extern struct SaveFile gSaveFile;

void file_init(void);
void file_log(const char *fmt, ...);
void file_enumerate(bool (*callback)(const char *filename));
void file_create(const char *name);
void file_delete(const char *name);
void file_load_world(const char *name);
void file_save_world(void);

#endif //GUARD_FILE_H
