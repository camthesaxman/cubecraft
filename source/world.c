#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
#include "world.h"

//Blocks TPL data
#include "blocks_tpl.h"
#include "blocks.h"

extern void build_chunk_display_list(struct Chunk *chunk);
extern void render_chunk_immediate(struct Chunk *chunk);

struct Vertex
{
    int x;
    int y;
    int z;
};

struct Face
{
    int tile;
    struct Vertex vertices[4];
};

#define CHUNK_TABLE_WIDTH 16  //must be a power of two and larger than the viewing radius
#define CHUNK_TABLE_CAPACITY (CHUNK_TABLE_WIDTH * CHUNK_TABLE_WIDTH)

static struct Chunk chunkTable[CHUNK_TABLE_WIDTH][CHUNK_TABLE_WIDTH];

static TPLFile blocksTPL;
static GXTexObj blocksTexture;

static int wrap_table_index(int index)
{
    index = ((unsigned int)index % CHUNK_TABLE_WIDTH);
    assert(index >= 0);
    assert(index < CHUNK_TABLE_WIDTH);
    return index;
}

void world_render_chunk(struct Chunk *chunk)
{
    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD1, GX_TEXMAP1, GX_COLORNULL);
    
    //assert(chunk->dispList != NULL);
    //GX_CallDispList(chunk->dispList, chunk->dispListSize);
    render_chunk_immediate(chunk);
}

static void generate_land(struct Chunk *chunk)
{
    int x, y, z;
    
    //TODO: generate heightmap
    for (x = 0; x < CHUNK_WIDTH; x++)
    {
        for (z = 0; z < CHUNK_WIDTH; z++)
        {
            int landHeight = 5;
            
            for (y = 0; y < landHeight; y++)
                chunk->blocks[x][y][z] = BLOCK_STONE;
            for (; y < CHUNK_HEIGHT; y++)
                chunk->blocks[x][y][z] = BLOCK_AIR;
        }
    }
}

static void load_changes(struct Chunk *chunk)
{
    //TODO: Implement
}

static void delete_chunk(struct Chunk *chunk)
{
    printf("deleting chunk (%i, %i)\n", chunk->x, chunk->z);
    chunk->active = false;
    free(chunk->dispList);
    chunk->dispList = NULL;
}

static void generate_chunk(struct Chunk *chunk, int x, int z)
{
    printf("generating chunk (%i, %i)\n", x, z);
    chunk->active = true;
    chunk->x = x;
    chunk->z = z;
    generate_land(chunk);
    load_changes(chunk);
    //build_chunk_display_list(chunk);
}

void world_init(void)
{
    memset(chunkTable, 0, sizeof(chunkTable));
    TPL_OpenTPLFromMemory(&blocksTPL, (void *)blocks_tpl, blocks_tpl_size);
    TPL_GetTexture(&blocksTPL, blocksTextureId, &blocksTexture);
    GX_InitTexObjFilterMode(&blocksTexture, GX_NEAR, GX_NEAR);
    GX_SetNumTexGens(1);
    GX_SetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX1, GX_IDENTITY);
    GX_LoadTexObj(&blocksTexture, GX_TEXMAP1);
    GX_InvalidateTexAll();
}

void world_close(void)
{
    for (int i = 0; i < CHUNK_TABLE_WIDTH; i++)
    {
        for (int j = 0; j < CHUNK_TABLE_WIDTH; j++)
        {
            if (chunkTable[i][j].active)
                delete_chunk(&chunkTable[i][j]);
        }
    }
}

struct Chunk *world_get_chunk(int x, int z)
{
    struct Chunk *chunk = &chunkTable[wrap_table_index(x)][wrap_table_index(z)];
    
    if (chunk->active)
    {
        if (chunk->x == x && chunk->z == z)
            return chunk;
        else
            delete_chunk(chunk);
    }
    generate_chunk(chunk, x, z);
    return chunk;
}

struct Chunk *world_get_chunk_containing(float x, float z)
{
    int chunkX = floorf(x / CHUNK_WIDTH);
    int chunkZ = floorf(z / CHUNK_WIDTH);
    
    return world_get_chunk(chunkX, chunkZ);
}

int world_to_chunk_coord(float x)
{
    int ret = floorf(x / CHUNK_WIDTH);
    
    assert(x >= (float)ret * CHUNK_WIDTH);
    assert(x < (float)ret * CHUNK_WIDTH + CHUNK_WIDTH);
    return ret;
}
