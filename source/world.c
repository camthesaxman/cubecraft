#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
#include "world.h"

//Blocks TPL data
#include "blocks_tpl.h"
#include "blocks.h"

#define CHUNK_RENDER_RADIUS 10
#define CHUNK_TABLE_WIDTH 16  //must be a power of two and larger than the render radius
#define CHUNK_TABLE_CAPACITY (CHUNK_TABLE_WIDTH * CHUNK_TABLE_WIDTH)

enum
{
    TILE_STONE,
    TILE_SAND,
    TILE_DIRT,
    TILE_GRASS_SIDE,
    TILE_GRASS,
    TILE_WOOD,
    TILE_TREE_SIDE,
    TILE_TREE_TOP,
    TILE_LEAVES,
};

enum
{
    DIR_X_FRONT,
    DIR_X_BACK,
    DIR_Y_FRONT,
    DIR_Y_BACK,
    DIR_Z_FRONT,
    DIR_Z_BACK,
};

static const u8 blockTiles[][6] =
{
	[BLOCK_STONE]     = {TILE_STONE,      TILE_STONE,      TILE_STONE,    TILE_STONE,    TILE_STONE,      TILE_STONE},
	[BLOCK_SAND]      = {TILE_SAND,       TILE_SAND,       TILE_SAND,     TILE_SAND,     TILE_SAND,       TILE_SAND},
	[BLOCK_DIRT]      = {TILE_DIRT,       TILE_DIRT,       TILE_DIRT,     TILE_DIRT,     TILE_DIRT,       TILE_DIRT},
	[BLOCK_GRASS] = {TILE_GRASS_SIDE, TILE_GRASS_SIDE, TILE_GRASS,    TILE_DIRT,     TILE_GRASS_SIDE, TILE_GRASS_SIDE},
	[BLOCK_WOOD]      = {TILE_WOOD,       TILE_WOOD,       TILE_WOOD,     TILE_WOOD,     TILE_WOOD,       TILE_WOOD},
    [BLOCK_TREE]      = {TILE_TREE_SIDE,  TILE_TREE_SIDE,  TILE_TREE_TOP, TILE_TREE_TOP, TILE_TREE_SIDE,  TILE_TREE_SIDE},
    [BLOCK_LEAVES]    = {TILE_LEAVES,     TILE_LEAVES,     TILE_LEAVES,   TILE_LEAVES,   TILE_LEAVES,     TILE_LEAVES}
};

struct Vertex
{
    s16 x, y, z;
};

struct Face
{
    int tile;
    struct Vertex vertexes[4];
};

static struct Face *facesList;
static int facesListCount;
static int facesListCapacity;

static TPLFile blocksTPL;
static GXTexObj blocksTexture;

static u16 worldSeed = 54321;
static struct Chunk chunkTable[CHUNK_TABLE_WIDTH][CHUNK_TABLE_WIDTH];

static void load_chunk_changes(struct Chunk *chunk);

//==================================================
// Land Generation
//==================================================

#define WAVELENGTH 32
#define LAND_HEIGHT_MIN 20
#define LAND_HEIGHT_MAX 36
#define MAX_TREES 4

static u16 random(u16 a)
{
    u32 val = a ^ worldSeed;
    val = val * 1103515245 >> 16;
    return val;
}

//Returns a random float between 0.0 and 1.0
static float rand_hash_frac(int a, int b)
{
    u16 hash = random(a) ^ random(b);
    
    return (float)hash / 65535.0;
}

static float lerp(float a, float b, float x)
{
    return a + x * (b - a);
}

static void make_tree(struct Chunk *chunk, int x, int y, int z)
{
    int i, j;
    
    for (i = -1; i <= 1; i++)
    {
        for (j = -1; j <= 1; j++)
        {
            chunk->blocks[x + i][y + 3][z + j] = BLOCK_LEAVES;
            chunk->blocks[x + i][y + 6][z + j] = BLOCK_LEAVES;
        }
    }
    for (i = -2; i <= 2; i++)
    {
        assert(x + i >= 0);
        assert(x + i < CHUNK_WIDTH);
        for (j = -2; j <= 2; j++)
        {
            assert(z + j >= 0);
            assert(z + j < CHUNK_WIDTH);
            chunk->blocks[x + i][y + 4][z + j] = BLOCK_LEAVES;
            chunk->blocks[x + i][y + 5][z + j] = BLOCK_LEAVES;
        }
    }
    for (int i = 0; i < 4; i++)
        chunk->blocks[x][y++][z] = BLOCK_TREE;
    
}

static void generate_land(struct Chunk *chunk)
{
    int heightmap[CHUNK_WIDTH][CHUNK_WIDTH];
    int x = chunk->x * CHUNK_WIDTH;
    int z = chunk->z * CHUNK_WIDTH;
    u16 randVal = random(x) ^ random(z);
    int y;
    
    for (int i = 0; i < CHUNK_WIDTH; i++)
    {
        int x1 = floorf((float)(x + i) / WAVELENGTH) *WAVELENGTH;
        int x2 = x1 + WAVELENGTH;
        float xBlend = (float)(x + i - x1) / (float)(WAVELENGTH);
        for (int j = 0; j < CHUNK_WIDTH; j++)
        {
            int z1 = floorf((float)(z + j) / WAVELENGTH) * WAVELENGTH;
            int z2 = z1 + WAVELENGTH;
            float zBlend = (float)(z + j - z1) / (float)(WAVELENGTH);
            float a = lerp(rand_hash_frac(x1, z1), rand_hash_frac(x2, z1), xBlend);
            float b = lerp(rand_hash_frac(x1, z2), rand_hash_frac(x2, z2), xBlend);
            heightmap[i][j] = (float)LAND_HEIGHT_MIN + (float)(LAND_HEIGHT_MAX - LAND_HEIGHT_MIN) * lerp(a, b, zBlend);
        }
    }
    
    for (x = 0; x < CHUNK_WIDTH; x++)
    {
        for (z = 0; z < CHUNK_WIDTH; z++)
        {
            int landHeight = heightmap[x][z];
            
            assert(landHeight >= LAND_HEIGHT_MIN);
            assert(landHeight <= LAND_HEIGHT_MAX);
            for (y = 0; y < landHeight; y++)
            {
                if (y < 22)
                    chunk->blocks[x][y][z] = BLOCK_STONE;
                else if (y < 25)
                    chunk->blocks[x][y][z] = BLOCK_SAND;
                else if (y == landHeight - 1)
                    chunk->blocks[x][y][z] = BLOCK_GRASS;
                else
                    chunk->blocks[x][y][z] = BLOCK_DIRT;
            }
            for (; y < CHUNK_HEIGHT; y++)
                chunk->blocks[x][y][z] = BLOCK_AIR;
        }
    }
    
    for (int i = 0; i < MAX_TREES; i++)
    {
        //We want to keep things simple and avoid having the tree leaves overlap into neighboring chunks
        x = 2 + (random(z) % (CHUNK_WIDTH - 4));
        z = 2 + (random(x) % (CHUNK_WIDTH - 4));
        y = heightmap[x][z];
       
        //Trees should only grow on grass
        if (chunk->blocks[x][y - 1][z] == BLOCK_GRASS)
            make_tree(chunk, x, y, z);
    }
}

//==================================================
// Chunk Management
//==================================================

static int wrap_table_index(int index)
{
    index = ((unsigned int)index % CHUNK_TABLE_WIDTH);
    assert(index >= 0);
    assert(index < CHUNK_TABLE_WIDTH);
    return index;
}

static void load_chunk_changes(struct Chunk *chunk)
{
    //TODO: Implement
}

static void create_chunk(struct Chunk *chunk, int x, int z)
{
    printf("generating chunk (%i, %i)\n", x, z);
    chunk->active = true;
    chunk->x = x;
    chunk->z = z;
    generate_land(chunk);
    load_chunk_changes(chunk);
}

static void delete_chunk(struct Chunk *chunk)
{
    printf("deleting chunk (%i, %i)\n", chunk->x, chunk->z);
    chunk->active = false;
    free(chunk->dispList);
    chunk->dispList = NULL;
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
    create_chunk(chunk, x, z);
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

int world_to_block_coord(float x)
{
    int ret = (unsigned int)floorf(x) % CHUNK_WIDTH;
    
    assert(ret >= 0);
    assert(ret < CHUNK_WIDTH);
    return ret;
}

int world_get_block_at(float x, float y, float z)
{
    struct Chunk *chunk = world_get_chunk_containing(x, z);
    int blockX = world_to_block_coord(x);
    int blockY = floorf(y);
    int blockZ = world_to_block_coord(z);
    
    assert(blockX >= 0);
    assert(blockX < CHUNK_WIDTH);
    assert(blockY >= 0);
    assert(blockY < CHUNK_HEIGHT);
    assert(blockZ >= 0);
    assert(blockZ < CHUNK_WIDTH);
    return chunk->blocks[blockX][blockY][blockZ];
}

//==================================================
// Rendering
//==================================================

/* Block faces
 *
 *           A
 *           |y+
 *        (0,1,0)-------------(1,1,0)
 *          /|                   |
 *         / |                   |
 *        /  |                   |
 *       /   |                   |
 *      /    |       DIR_Z       |
 *  (0,1,1)  |                   |
 *     |     |                   |
 *     |DIR_X|                   |
 *     |     |                   |   x+
 *     |  (0,0,0)-------------(1,0,0)-->
 *     |    /                   /
 *     |   /                   /
 *     |  /       DIR_Y       /
 *     | /                   /
 *     |/                   /
 *  (0,0,1)-------------(1,0,1)
 *    /z+
 *   V
 */

static void add_face(int x, int y, int z, int direction, int block)
{
    struct Face face;
    
    face.tile = blockTiles[block][direction];
    switch (direction)
    {
        case DIR_X_FRONT:  //drawn clockwise looking x-
            face.vertexes[0] = (struct Vertex){0, 1, 1};
            face.vertexes[1] = (struct Vertex){0, 1, 0};
            face.vertexes[2] = (struct Vertex){0, 0, 0};
            face.vertexes[3] = (struct Vertex){0, 0, 1};
            break;
        case DIR_X_BACK:  //drawn clockwise looking x+
            face.vertexes[0] = (struct Vertex){0, 1, 0};
            face.vertexes[1] = (struct Vertex){0, 1, 1};
            face.vertexes[2] = (struct Vertex){0, 0, 1};
            face.vertexes[3] = (struct Vertex){0, 0, 0};
            break;
        case DIR_Y_FRONT:  //drawn clockwise looking y-
            face.vertexes[0] = (struct Vertex){0, 0, 0};
            face.vertexes[1] = (struct Vertex){1, 0, 0};
            face.vertexes[2] = (struct Vertex){1, 0, 1};
            face.vertexes[3] = (struct Vertex){0, 0, 1};
            break;
        case DIR_Y_BACK:  //drawn clockwise looking y+
            face.vertexes[0] = (struct Vertex){0, 0, 0};
            face.vertexes[1] = (struct Vertex){0, 0, 1};
            face.vertexes[2] = (struct Vertex){1, 0, 1};
            face.vertexes[3] = (struct Vertex){1, 0, 0};
            break;
        case DIR_Z_FRONT: //drawn clockwise looking z-
            face.vertexes[0] = (struct Vertex){0, 1, 0};
            face.vertexes[1] = (struct Vertex){1, 1, 0};
            face.vertexes[2] = (struct Vertex){1, 0, 0};
            face.vertexes[3] = (struct Vertex){0, 0, 0};
            break;
        case DIR_Z_BACK:  //drawn clockwise looking z+
            face.vertexes[0] = (struct Vertex){1, 1, 0};
            face.vertexes[1] = (struct Vertex){0, 1, 0};
            face.vertexes[2] = (struct Vertex){0, 0, 0};
            face.vertexes[3] = (struct Vertex){1, 0, 0};
            break;
        default:
            assert(false);  //bad direction parameter
    }
    for (int i = 0; i < 4; i++)
    {
        face.vertexes[i].x += x;
        face.vertexes[i].y += y;
        face.vertexes[i].z += z;
    }
    if (facesListCount == facesListCapacity)
    {
        facesListCapacity += 32;
        facesList = realloc(facesList, facesListCapacity * sizeof(struct Face));
    }
    facesList[facesListCount] = face;
    facesListCount++;
}

static void build_exposed_faces_list(struct Chunk *chunk)
{
    facesList = NULL;
    facesListCount = 0;
    facesListCapacity = 0;
    struct Chunk *prevChunkX = world_get_chunk(chunk->x - 1, chunk->z);
    struct Chunk *prevChunkZ = world_get_chunk(chunk->x, chunk->z - 1);
    
    //For each block, add the face of the block preceding it in the x, y, and z directions, if visible
    for (int x = 0; x < CHUNK_WIDTH; x++)
    {
        for (int y = 0; y < CHUNK_HEIGHT; y++)
        {
            for (int z = 0; z < CHUNK_WIDTH; z++)
            {
                int currBlock = chunk->blocks[x][y][z];
                int prevBlockX = (x == 0) ? prevChunkX->blocks[CHUNK_WIDTH - 1][y][z] : chunk->blocks[x - 1][y][z];
                int prevBlockZ = (z == 0) ? prevChunkZ->blocks[x][y][CHUNK_WIDTH - 1] : chunk->blocks[x][y][z - 1];
                
                if (BLOCK_IS_SOLID(currBlock))
                {
                    if (!BLOCK_IS_SOLID(prevBlockX))
                        add_face(x, y, z, DIR_X_BACK, currBlock);
                    if (y > 0 && !BLOCK_IS_SOLID(chunk->blocks[x][y - 1][z]))
                        add_face(x, y, z, DIR_Y_BACK, currBlock);
                    if (!BLOCK_IS_SOLID(prevBlockZ))
                        add_face(x, y, z, DIR_Z_BACK, currBlock);
                }
                else
                {
                    if (BLOCK_IS_SOLID(prevBlockX))
                        add_face(x, y, z, DIR_X_FRONT, prevBlockX);
                    if (y > 0 && BLOCK_IS_SOLID(chunk->blocks[x][y - 1][z]))
                        add_face(x, y, z, DIR_Y_FRONT, chunk->blocks[x][y - 1][z]);
                    if (BLOCK_IS_SOLID(prevBlockZ))
                        add_face(x, y, z, DIR_Z_FRONT, prevBlockZ);
                }
            }
        }
    }
}

static inline int round_up(int number, int multiple)
{
    return ((number + multiple - 1) / multiple) * multiple;
}

static void build_chunk_display_list(struct Chunk *chunk)
{
    size_t listSize;
    int x = chunk->x * CHUNK_WIDTH;
    int z = chunk->z * CHUNK_WIDTH;
    int blockFace = 0;
    float texLeft = (float)blockFace / 9.0;
    float texRight = (float)(blockFace + 1) / 9.0;
    f32 texCoords[] ATTRIBUTE_ALIGN(32) = {
        texLeft,  0.0,
        texRight, 0.0,
        texRight, 1.0,
        texLeft, 1.0,
    };
    
    build_exposed_faces_list(chunk);
    
    //The GX_DRAW_QUADS command takes up 3 bytes.
    //Each face is a quad with 4 vertexes.
    //Each vertex takes up three "u16"s for the position coordinate and two "f32"s for the texture coordinate.
    //Because of the write gathering pipe, an extra 63 bytes are needed.
    listSize = 3 + facesListCount * 4 * (3 * sizeof(s16) + 2 * sizeof(f32)) + 63;
    //The list size also must be a multiple of 32, so we round up.
    listSize = round_up(listSize, 32);
    
    chunk->dispList = memalign(32, listSize);
    //Remove this block of memory from the CPU's cache because the write gather pipe is used to write the commands
    DCInvalidateRange(chunk->dispList, listSize);
    
    GX_BeginDispList(chunk->dispList, listSize); 
    
    GX_Begin(GX_QUADS, GX_VTXFMT1, 4 * facesListCount);
    for (int i = 0; i < facesListCount; i++)
    {
        struct Face *face = &facesList[i];
        
        for (int j = 0; j < 4; j++)
        {
            struct Vertex *vertex = &face->vertexes[j];
            
            GX_Position3s16(x + vertex->x, vertex->y, z + vertex->z);
            GX_TexCoord2f32(texCoords[j * 2] + (float)face->tile / 9.0, texCoords[j * 2 + 1]);
        }
    }
    GX_End();
    
    chunk->dispListSize = GX_EndDispList();
    assert(chunk->dispListSize != 0);
    free(facesList);
}

void world_render_chunks_at(float x, float z)
{
    int chunkX = world_to_chunk_coord(x);
    int chunkZ = world_to_chunk_coord(z);
    
    GX_LoadTexObj(&blocksTexture, GX_TEXMAP0);
    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
    
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    
    for (int i = -CHUNK_RENDER_RADIUS / 2; i <= CHUNK_RENDER_RADIUS / 2; i++)
    {
        for (int j = -CHUNK_RENDER_RADIUS / 2; j <= CHUNK_RENDER_RADIUS / 2; j++)
        {
            struct Chunk *chunk = world_get_chunk(chunkX + i, chunkZ + j);
            
            if (chunk->dispList == NULL)
                build_chunk_display_list(chunk);
            GX_CallDispList(chunk->dispList, chunk->dispListSize);
        }
    }
}

//==================================================
// Setup Functions
//==================================================

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
