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
extern void render_chunk_display_list(struct Chunk *chunk);

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

#define CHUNK_RENDER_RADIUS 10
#define CHUNK_TABLE_WIDTH 16  //must be a power of two and larger than the render radius
#define CHUNK_TABLE_CAPACITY (CHUNK_TABLE_WIDTH * CHUNK_TABLE_WIDTH)

static struct Chunk chunkTable[CHUNK_TABLE_WIDTH][CHUNK_TABLE_WIDTH];

static TPLFile blocksTPL;
static GXTexObj blocksTexture;

static u16 worldSeed = 54321;

static int wrap_table_index(int index)
{
    index = ((unsigned int)index % CHUNK_TABLE_WIDTH);
    assert(index >= 0);
    assert(index < CHUNK_TABLE_WIDTH);
    return index;
}

static u16 random(u16 a)
{
    unsigned int val = a ^ worldSeed;
    val = val * 1103515245 + 12345;
    return val;
}

//Returns a random float between 0 and 1
static float rand_hash(int a, int b)
{
    u16 hash = random(a) ^ random(b);
    
    return (float)hash / 65535.0;
}

static float lerp(float a, float b, float x)
{
    return a + x * (b - a);
}

#define WAVELENGTH 32

static void generate_land(struct Chunk *chunk)
{
    float noisemap[CHUNK_WIDTH][CHUNK_WIDTH];
    int x = chunk->x * CHUNK_WIDTH;
    int z = chunk->z * CHUNK_WIDTH;
    int y;
    
    for (int i = 0; i < CHUNK_WIDTH; i++)
    {
        int x1 = floor((float)(x + i) / WAVELENGTH) *WAVELENGTH;
        int x2 = x1 + WAVELENGTH;
        float xBlend = (float)(x + i - x1) / (float)(WAVELENGTH);
        for (int j = 0; j < CHUNK_WIDTH; j++)
        {
            int z1 = floor((float)(z + j) / WAVELENGTH) * WAVELENGTH;
            int z2 = z1 + WAVELENGTH;
            float zBlend = (float)(z + j - z1) / (float)(WAVELENGTH);
            float a = lerp(rand_hash(x1, z1), rand_hash(x2, z1), xBlend);
            float b = lerp(rand_hash(x1, z2), rand_hash(x2, z2), xBlend);
            noisemap[i][j] = lerp(a, b, zBlend);
        }
    }
    
    //TODO: generate heightmap
    for (x = 0; x < CHUNK_WIDTH; x++)
    {
        for (z = 0; z < CHUNK_WIDTH; z++)
        {
            int landHeight = 20.0 + 16.0 * noisemap[x][z];
            
            for (y = 0; y < landHeight; y++)
            {
                if (y < 22)
                    chunk->blocks[x][y][z] = BLOCK_STONE;
                else if (y < 25)
                    chunk->blocks[x][y][z] = BLOCK_SAND;
                else if (y == landHeight - 1)
                    chunk->blocks[x][y][z] = BLOCK_DIRTGRASS;
                else
                    chunk->blocks[x][y][z] = BLOCK_DIRT;
            }
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
}

void world_init(void)
{
    memset(chunkTable, 0, sizeof(chunkTable));
}

void world_load_textures(void)
{
    TPL_OpenTPLFromMemory(&blocksTPL, (void *)blocks_tpl, blocks_tpl_size);
    TPL_GetTexture(&blocksTPL, blocksTextureId, &blocksTexture);
    GX_InitTexObjFilterMode(&blocksTexture, GX_NEAR, GX_NEAR);
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

void world_render_chunks_at(float x, float z)
{
    int chunkX = world_to_chunk_coord(x);
    int chunkZ = world_to_chunk_coord(z);
    
    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD1, GX_TEXMAP1, GX_COLORNULL);
    
    for (int i = -CHUNK_RENDER_RADIUS / 2; i <= CHUNK_RENDER_RADIUS / 2; i++)
    {
        for (int j = -CHUNK_RENDER_RADIUS / 2; j <= CHUNK_RENDER_RADIUS / 2; j++)
        {
            struct Chunk *chunk = world_get_chunk(chunkX + i, chunkZ + j);
            
            if (chunk->dispList == NULL)
                build_chunk_display_list(chunk);
            render_chunk_display_list(chunk);
        }
    }
}
