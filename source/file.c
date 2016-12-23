#include "global.h"
#include "file.h"
#include "world.h"

#define CHUNK_MOD_SIZE (4 * sizeof(uint32_t))

typedef unsigned char byte;

//TODO: Use actual error handling rather than assert

static const char fileMagic[] = "CUBECRAFTvALPHA";
static const char savePath[] = "/apps/cubecraft/worlds";
static const char logFilePath[] = "/apps/cubecraft/log.txt";

static bool can_open_root_dir(void)
{
    DIR *rootDir = opendir("/");
    if (rootDir != NULL)
    {
        closedir(rootDir);
        return true;
    }
    else
    {
        return false;
    }
}

static void ensure_worlds_dir(void)
{
    DIR *saveDir = opendir(savePath);
    
    if (saveDir != NULL)
    {
        closedir(saveDir);
    }
    else
    {
        //Directory doesn't exist. Create it.
        mkdir(savePath, 0777); //This 0777 is in octal
        
        //Make sure we can open it
        saveDir = opendir(savePath);
        assert(saveDir != NULL);
        closedir(saveDir);
    }
}

void file_init(void)
{
    assert(fatInitDefault());
    assert(can_open_root_dir());
    ensure_worlds_dir();
    remove(logFilePath);
}

void file_log(const char *fmt, ...)
{
    FILE *logFile = fopen(logFilePath, "a");
    va_list args;
    
    va_start(args, fmt);
    vfprintf(logFile, fmt, args);
    va_end(args);
    fputs("\r\n", logFile);
    fclose(logFile);
}

void file_enumerate(bool (*callback)(const char *filename))
{
    DIR *saveDir = opendir(savePath);
    struct dirent *d;
    
    assert(saveDir != NULL);
    while ((d = readdir(saveDir)) != NULL)
    {
        if (d->d_name[0] != '.')
        {
            if (!callback(d->d_name))
                break;
        }
    }
    closedir(saveDir);
}

static size_t file_size(FILE *file)
{
    fseek(file, 0, SEEK_END);
    return ftell(file);
}

//Generate serialize and deserialize functions for integer types
#define X(typename)                                                             \
static void serialize_##typename(byte **dst, typename x)                        \
{                                                                               \
    for (int i = 0; i < sizeof(x); i++)                                         \
        (*dst)[i] = (x >> (CHAR_BIT * (sizeof(x) - i - 1))) & UCHAR_MAX;        \
    (*dst) += sizeof(x);                                                        \
}                                                                               \
                                                                                \
static typename deserialize_##typename(byte **src)                              \
{                                                                               \
    typename x = 0;                                                             \
                                                                                \
    for (int i = 0; i < sizeof(x); i++)                                         \
        x |= (*src)[i] << (CHAR_BIT * (sizeof(x) - i - 1));                     \
    (*src) += sizeof(x);                                                        \
    return x;                                                                   \
}

X(int32_t)
X(uint32_t)
X(uint8_t)

#undef X

static void serialize_string(byte **dst, const char *string, int length)
{
    for (int i = 0; i < length; i++)
        *((*dst)++) = string[i];
}

static void deserialize_string(byte **src, char *dst, int length)
{
    for (int i = 0; i < length; i++)
        dst[i] = *((*src)++);
}

void file_load_world(struct SaveFile *save, const char *name)
{
    char *path = malloc(strlen(savePath) + 1 + strlen(name) + 1);
    FILE *file;
    size_t size;
    byte *buffer;
    byte *ptr;
    byte *blockData;
    int magicSize = sizeof(fileMagic);
    char magic[magicSize];
    
    strcpy(path, savePath);
    strcat(path, "/");
    strcat(path, name);
    
    file_log("file_load_world(): loading world '%s' from file '%s'", name, path);
    
    file = fopen(path, "r");
    assert(file != NULL);
    size = file_size(file);
    fseek(file, 0, SEEK_SET);
    
    buffer = malloc(size);
    memset(buffer, 0, size);
    ptr = buffer;
    fread(buffer, 1, size, file);
    
    //Verify magic value
    deserialize_string(&ptr, magic, magicSize - 1);
    magic[magicSize - 1] = '\0';
    if (strcmp(magic, fileMagic))
    {
        file_log("file_load_world(): magic value does not match (expected '%s', got '%s')", fileMagic, magic);
        goto close;
    }
    
    //Read name and seed
    deserialize_string(&ptr, save->name, SAVENAME_MAX);
    deserialize_string(&ptr, save->seed, SEED_MAX);
    
    //Read spawn location
    save->spawnX = deserialize_int32_t(&ptr);
    save->spawnY = deserialize_int32_t(&ptr);
    save->spawnZ = deserialize_int32_t(&ptr);
    
    //Read modified chunk data
    save->modifiedChunksCount = deserialize_uint32_t(&ptr);
    save->modifiedChunks = malloc(save->modifiedChunksCount * sizeof(struct ChunkModification));
    blockData = ptr + save->modifiedChunksCount * CHUNK_MOD_SIZE;
    for (int i = 0; i < save->modifiedChunksCount; i++)
    {
        struct ChunkModification *chunkMod = &save->modifiedChunks[i];
        
        chunkMod->x = deserialize_int32_t(&ptr);
        chunkMod->z = deserialize_int32_t(&ptr);
        chunkMod->modifiedBlocksCount = deserialize_uint32_t(&ptr);
        chunkMod->modifiedBlocks = malloc(chunkMod->modifiedBlocksCount * sizeof(struct BlockModification));
        
        //Read modified block data
        for (int j = 0; j < chunkMod->modifiedBlocksCount; j++)
        {
            struct BlockModification *blockMod = &chunkMod->modifiedBlocks[j];
            
            blockMod->x = deserialize_uint8_t(&blockData);
            blockMod->y = deserialize_uint8_t(&blockData);
            blockMod->z = deserialize_uint8_t(&blockData);
            blockMod->type = deserialize_uint8_t(&blockData);
        }
    }
    
  close:
    free(buffer);
    fclose(file);
}

static size_t calc_save_size(struct SaveFile *save)
{
    size_t size = 0;
    
    size += sizeof(fileMagic) - 1;  //file signature
    size += SAVENAME_MAX;           //name
    size += SEED_MAX;               //seed
    size += 3 * sizeof(int32_t);    //spawn location
    size += sizeof(uint32_t);       //number of modified chunks
    
    //chunk data
    for (int i = 0; i < save->modifiedChunksCount; i++)
    {
        size += sizeof(int32_t);   //x
        size += sizeof(int32_t);   //z
        size += sizeof(uint32_t);  //number of modified blocks
        size += 4;                 //TODO: Find out why I need 4 extra bytes
        
        //block data
        size += save->modifiedChunks[i].modifiedBlocksCount * 4 * sizeof(uint8_t);
    }
    
    return size;
}

void file_save_world(struct SaveFile *save)
{
    char *path = malloc(strlen(savePath) + 1 + strlen(save->name) + 1);
    FILE *file;
    size_t size;
    byte *buffer;
    byte *ptr;
    byte *blockData;
    char nameBuffer[SAVENAME_MAX] = {'\0'};
    char seedBuffer[SEED_MAX] = {'\0'};
    
    assert(strlen(save->name) > 0);
    assert(strlen(save->seed) > 0);
    strcpy(path, savePath);
    strcat(path, "/");
    strcat(path, save->name);
    
    file_log("file_save_world(): saving world '%s' to file '%s'", save->name, path);
    
    //Allocate write buffer
    size = calc_save_size(save);
    buffer = malloc(size);
    ptr = buffer;
    
    file = fopen(path, "w");
    assert(file != NULL);
    
    //Write magic value
    serialize_string(&ptr, fileMagic, strlen(fileMagic));
    
    //Write name
    strcpy(nameBuffer, save->name);
    serialize_string(&ptr, nameBuffer, SAVENAME_MAX);
    
    //Write seed
    strcpy(seedBuffer, save->seed);
    serialize_string(&ptr, seedBuffer, SEED_MAX);
    
    //Write spawn location
    serialize_int32_t(&ptr, save->spawnX);
    serialize_int32_t(&ptr, save->spawnY);
    serialize_int32_t(&ptr, save->spawnZ);
    
    //Write modified chunk data
    serialize_uint32_t(&ptr, save->modifiedChunksCount);
     
    blockData = ptr + save->modifiedChunksCount * CHUNK_MOD_SIZE;
    for (int i = 0; i < save->modifiedChunksCount; i++)
    {
        struct ChunkModification *chunkMod = &save->modifiedChunks[i];
        
        serialize_int32_t(&ptr, chunkMod->x);
        serialize_int32_t(&ptr, chunkMod->z);
        serialize_uint32_t(&ptr, chunkMod->modifiedBlocksCount);
        
        //Write modified block data
        for (int j = 0; j < chunkMod->modifiedBlocksCount; j++)
        {
            struct BlockModification *blockMod = &chunkMod->modifiedBlocks[j];
            
            serialize_uint8_t(&blockData, blockMod->x);
            serialize_uint8_t(&blockData, blockMod->y);
            serialize_uint8_t(&blockData, blockMod->z);
            serialize_uint8_t(&blockData, blockMod->type);
        }
    }
    
    assert(size == blockData - buffer);  //Make darn sure our buffer was the correct size
    fwrite(buffer, blockData - buffer, 1, file);
    fclose(file);
}

void file_delete(const char *name)
{
    char *path = malloc(strlen(savePath) + 1 + strlen(name) + 1);
    
    strcpy(path, savePath);
    strcat(path, "/");
    strcat(path, name);
    
    file_log("file_delete(): deleting file '%s'", path);
    
    remove(path);
}
