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
    struct ChunkModification *modifiedChunks;
    int modifiedChunksCount;
};

void file_init(void);
void file_log(const char *fmt, ...);
void file_enumerate(bool (*callback)(const char *filename));
void file_create(const char *name);
void file_delete(const char *name);
void file_load_world(struct SaveFile *saveFile, const char *name);
void file_save_world(struct SaveFile *saveFile);

#endif //GUARD_FILE_H
