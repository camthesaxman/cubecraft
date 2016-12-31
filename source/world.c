#include "global.h"
#include "file.h"
#include "world.h"

//Blocks TPL data
#include "blocks_tpl.h"
#include "blocks.h"

#define CHUNK_RENDER_RANGE 10
#define CHUNK_TABLE_WIDTH 16  //must be a power of two and larger than the render range
#define CHUNK_TABLE_CAPACITY (CHUNK_TABLE_WIDTH * CHUNK_TABLE_WIDTH)

#define TEX_BLOCK_WIDTH 16
#define TEX_BLOCK_HEIGHT 16

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
    TILE_GC_FRONT,
    TILE_GC_TOP,
    TILE_GC_SIDE,
    TILE_GC_BACK,
    TILE_GC_BOTTOM,
    NUM_TILES,
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
    [BLOCK_STONE]     = {TILE_STONE,      TILE_STONE,      TILE_STONE,    TILE_STONE,     TILE_STONE,      TILE_STONE},
    [BLOCK_SAND]      = {TILE_SAND,       TILE_SAND,       TILE_SAND,     TILE_SAND,      TILE_SAND,       TILE_SAND},
    [BLOCK_DIRT]      = {TILE_DIRT,       TILE_DIRT,       TILE_DIRT,     TILE_DIRT,      TILE_DIRT,       TILE_DIRT},
    [BLOCK_GRASS]     = {TILE_GRASS_SIDE, TILE_GRASS_SIDE, TILE_GRASS,    TILE_DIRT,      TILE_GRASS_SIDE, TILE_GRASS_SIDE},
    [BLOCK_WOOD]      = {TILE_WOOD,       TILE_WOOD,       TILE_WOOD,     TILE_WOOD,      TILE_WOOD,       TILE_WOOD},
    [BLOCK_TREE]      = {TILE_TREE_SIDE,  TILE_TREE_SIDE,  TILE_TREE_TOP, TILE_TREE_TOP,  TILE_TREE_SIDE,  TILE_TREE_SIDE},
    [BLOCK_LEAVES]    = {TILE_LEAVES,     TILE_LEAVES,     TILE_LEAVES,   TILE_LEAVES,    TILE_LEAVES,     TILE_LEAVES},
    [BLOCK_GAMECUBE]  = {TILE_GC_SIDE,    TILE_GC_SIDE,    TILE_GC_TOP,   TILE_GC_BOTTOM, TILE_GC_FRONT,   TILE_GC_BACK}
};

struct Vertex
{
    s16 x, y, z;
};

struct Face
{
    int tile;
    int light;
    struct Vertex vertexes[4];
};

static struct Face *facesList;
static int facesListCount;
static int facesListCapacity;

static TPLFile blocksTPL;
static GXTexObj blocksTexture;

static u16 worldSeed;
static struct Chunk chunkTable[CHUNK_TABLE_WIDTH][CHUNK_TABLE_WIDTH];

static void load_chunk_changes(struct Chunk *chunk);
static void build_chunk_display_list(struct Chunk *chunk);

//==================================================
// Land Generation
//==================================================

#define WAVELENGTH 32
#define LAND_HEIGHT_MIN 51
#define LAND_HEIGHT_MAX 67
#define STONE_LEVEL 53
#define SAND_LEVEL 56
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
    unsigned int x = chunk->x * CHUNK_WIDTH;
    unsigned int z = chunk->z * CHUNK_WIDTH;
    u16 randVal = 65535.0 * rand_hash_frac(x, z);
    int gameCubeX;
    int gameCubeY;
    int gameCubeZ;
    int y;
    
    //Calculate land height
    for (int i = 0; i < CHUNK_WIDTH; i++)
    {
        unsigned int x1 = ((x + i) / WAVELENGTH) * WAVELENGTH;
        unsigned int x2 = x1 + WAVELENGTH;
        float xBlend = (float)(x + i - x1) / (float)(WAVELENGTH);
        for (int j = 0; j < CHUNK_WIDTH; j++)
        {
            unsigned int z1 = ((z + j) / WAVELENGTH) * WAVELENGTH;
            unsigned int z2 = z1 + WAVELENGTH;
            float zBlend = (float)(z + j - z1) / (float)(WAVELENGTH);
            float a = lerp(rand_hash_frac(x1, z1), rand_hash_frac(x2, z1), xBlend);
            float b = lerp(rand_hash_frac(x1, z2), rand_hash_frac(x2, z2), xBlend);
            heightmap[i][j] = LAND_HEIGHT_MIN + (float)(LAND_HEIGHT_MAX - LAND_HEIGHT_MIN) * lerp(a, b, zBlend);
        }
    }
    
    //Add terrain
    for (x = 0; x < CHUNK_WIDTH; x++)
    {
        for (z = 0; z < CHUNK_WIDTH; z++)
        {
            int landHeight = heightmap[x][z];
            
            assert(landHeight >= LAND_HEIGHT_MIN);
            assert(landHeight <= LAND_HEIGHT_MAX);
            for (y = 0; y < landHeight; y++)
            {
                if (y < STONE_LEVEL)
                    chunk->blocks[x][y][z] = BLOCK_STONE;
                else if (y < SAND_LEVEL)
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
    
    //Add trees
    for (int i = 0; i < MAX_TREES; i++)
    {
        //We want to keep things simple and avoid having the tree leaves overlap into neighboring chunks
        randVal = random(randVal);
        x = 2 + (randVal % (CHUNK_WIDTH - 4));
        randVal = random(randVal);
        z = 2 + (randVal % (CHUNK_WIDTH - 4));
        y = heightmap[x][z];
       
        //Trees should only grow on grass
        if (chunk->blocks[x][y - 1][z] == BLOCK_GRASS)
            make_tree(chunk, x, y, z);
    }
    
    //Bury one secret Gamecube in each chunk
    randVal = random(randVal);
    gameCubeX = randVal % CHUNK_WIDTH;
    randVal = random(randVal);
    gameCubeY = randVal;
    randVal = random(randVal);
    gameCubeZ = randVal % CHUNK_WIDTH;
    gameCubeY %= heightmap[gameCubeX][gameCubeZ] - 1;
    assert(gameCubeX >= 0);
    assert(gameCubeX < CHUNK_WIDTH);
    assert(gameCubeZ >= 0);
    assert(gameCubeZ < CHUNK_WIDTH);
    assert(gameCubeY >= 0);
    assert(gameCubeY < CHUNK_HEIGHT);
    assert(heightmap[gameCubeX][gameCubeZ] > 0);
    chunk->blocks[gameCubeX][gameCubeY][gameCubeZ] = BLOCK_GAMECUBE;
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
    struct ChunkModification *mod;
    
    chunk->modificationIndex = -1;
    
    //Search the save file for modifications to this chunk
    for (int i = 0; i < gSaveFile.modifiedChunksCount; i++)
    {
        if (gSaveFile.modifiedChunks[i].x == chunk->x
         && gSaveFile.modifiedChunks[i].z == chunk->z)
        {
            chunk->modificationIndex = i;
            break;
        }
    }
    
    if (chunk->modificationIndex == -1)
        return;  //Chunk was not modified
    
    file_log("load_chunk_changes(): applying chunk changes for chunk %i, %i", chunk->x, chunk->z);
    mod = &gSaveFile.modifiedChunks[chunk->modificationIndex];
    for (int i = 0; i < mod->modifiedBlocksCount; i++)
    {
        struct BlockModification *blockMod = &mod->modifiedBlocks[i];
        
        //file_log("load_chunk_changes(): changing block %i, %i, %i", blockMod->x, blockMod->y, blockMod->z);
        assert(blockMod->x >= 0);
        assert(blockMod->x < CHUNK_WIDTH);
        assert(blockMod->y >= 0);
        assert(blockMod->y < CHUNK_HEIGHT);
        assert(blockMod->z >= 0);
        assert(blockMod->z < CHUNK_WIDTH);
        chunk->blocks[blockMod->x][blockMod->y][blockMod->z] = blockMod->type;
    }
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

static void add_block_modification(struct Chunk *chunk, int x, int y, int z, int type)
{
    int i;
    struct ChunkModification *mod;
    
    //Ensure this chunk is in the list
    for (i = 0; i < gSaveFile.modifiedChunksCount; i++)
    {
        if (gSaveFile.modifiedChunks[i].x == chunk->x
         && gSaveFile.modifiedChunks[i].z == chunk->z)
            break;
    }
    
    if (i == gSaveFile.modifiedChunksCount)
    {
        //Doesn't exist. Let's create it
        assert(chunk->modificationIndex == -1);
        chunk->modificationIndex = i;
        gSaveFile.modifiedChunks = realloc(gSaveFile.modifiedChunks,
                                              (gSaveFile.modifiedChunksCount + 1) * sizeof(struct ChunkModification));
        gSaveFile.modifiedChunks[i].x = chunk->x;
        gSaveFile.modifiedChunks[i].z = chunk->z;
        gSaveFile.modifiedChunks[i].modifiedBlocks = NULL;
        gSaveFile.modifiedChunks[i].modifiedBlocksCount = 0;
        gSaveFile.modifiedChunksCount++;
    }
    
    //Add the block modification
    assert(chunk->modificationIndex != -1);
    mod = &gSaveFile.modifiedChunks[chunk->modificationIndex];
    
    mod->modifiedBlocks = realloc(mod->modifiedBlocks, (mod->modifiedBlocksCount + 1) * sizeof(struct BlockModification));
    mod->modifiedBlocks[mod->modifiedBlocksCount].x = x;
    mod->modifiedBlocks[mod->modifiedBlocksCount].y = y;
    mod->modifiedBlocks[mod->modifiedBlocksCount].z = z;
    mod->modifiedBlocks[mod->modifiedBlocksCount].type = type;
    mod->modifiedBlocksCount++;
}

void world_set_block(int x, int y, int z, int type)
{
    struct Chunk *chunk = world_get_chunk_containing(x, z);
    int blockX = world_to_block_coord(x);
    int blockY = y;
    int blockZ = world_to_block_coord(z);
    
    chunk->blocks[blockX][blockY][blockZ] = type;
    free(chunk->dispList);
    build_chunk_display_list(chunk);
    
    //Patch up any leftover or newly exposed faces on chunk borders
    if (blockX == 0)
    {
        struct Chunk *neighborChunk = world_get_chunk(chunk->x - 1, chunk->z);
        
        free(neighborChunk->dispList);
        build_chunk_display_list(neighborChunk);
    }
    else if (blockX == CHUNK_WIDTH - 1)
    {
        struct Chunk *neighborChunk = world_get_chunk(chunk->x + 1, chunk->z);
        
        free(neighborChunk->dispList);
        build_chunk_display_list(neighborChunk);
    }
    if (blockZ == 0)
    {
        struct Chunk *neighborChunk = world_get_chunk(chunk->x, chunk->z - 1);
        
        free(neighborChunk->dispList);
        build_chunk_display_list(neighborChunk);
    }
    else if (blockZ == CHUNK_WIDTH - 1)
    {
        struct Chunk *neighborChunk = world_get_chunk(chunk->x, chunk->z + 1);
        
        free(neighborChunk->dispList);
        build_chunk_display_list(neighborChunk);
    }
    
    add_block_modification(chunk, blockX, blockY, blockZ, type);
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
            face.light = 13;
            break;
        case DIR_X_BACK:  //drawn clockwise looking x+
            face.vertexes[0] = (struct Vertex){0, 1, 0};
            face.vertexes[1] = (struct Vertex){0, 1, 1};
            face.vertexes[2] = (struct Vertex){0, 0, 1};
            face.vertexes[3] = (struct Vertex){0, 0, 0};
            face.light = 9;
            break;
        case DIR_Y_FRONT:  //drawn clockwise looking y-
            face.vertexes[0] = (struct Vertex){0, 0, 0};
            face.vertexes[1] = (struct Vertex){1, 0, 0};
            face.vertexes[2] = (struct Vertex){1, 0, 1};
            face.vertexes[3] = (struct Vertex){0, 0, 1};
            face.light = 15;
            break;
        case DIR_Y_BACK:  //drawn clockwise looking y+
            face.vertexes[0] = (struct Vertex){0, 0, 0};
            face.vertexes[1] = (struct Vertex){0, 0, 1};
            face.vertexes[2] = (struct Vertex){1, 0, 1};
            face.vertexes[3] = (struct Vertex){1, 0, 0};
            face.light = 5;
            break;
        case DIR_Z_FRONT: //drawn clockwise looking z-
            face.vertexes[0] = (struct Vertex){0, 1, 0};
            face.vertexes[1] = (struct Vertex){1, 1, 0};
            face.vertexes[2] = (struct Vertex){1, 0, 0};
            face.vertexes[3] = (struct Vertex){0, 0, 0};
            face.light = 11;
            break;
        case DIR_Z_BACK:  //drawn clockwise looking z+
            face.vertexes[0] = (struct Vertex){1, 1, 0};
            face.vertexes[1] = (struct Vertex){0, 1, 0};
            face.vertexes[2] = (struct Vertex){0, 0, 0};
            face.vertexes[3] = (struct Vertex){1, 0, 0};
            face.light = 7;
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
    
    build_exposed_faces_list(chunk);
    
    //The GX_DRAW_QUADS command takes up 3 bytes.
    //Each face is a quad with 4 vertexes.
    //Each vertex takes up three u16 for the position coordinate, one u8 for the color index, and two u16 for the texture coordinate.
    //Because of the write gathering pipe, an extra 63 bytes are needed.
    listSize = 3 + facesListCount * 4 * (3 * sizeof(s16) + sizeof(u8) + 2 * sizeof(u16)) + 63;
    //The list size also must be a multiple of 32, so round up to the next multiple of 32.
    listSize = round_up(listSize, 32);
    
    chunk->dispList = memalign(32, listSize);
    //Remove this block of memory from the CPU's cache because the write gather pipe is used to write the commands
    DCInvalidateRange(chunk->dispList, listSize);
    
    GX_BeginDispList(chunk->dispList, listSize); 
    
    GX_Begin(GX_QUADS, GX_VTXFMT1, 4 * facesListCount);
    for (int i = 0; i < facesListCount; i++)
    {
        struct Face *face = &facesList[i];
        u16 texCoords[4][2] = {
            {0, 0},
            {1, 0},
            {1, 1},
            {0, 1}
        };
        
        for (int j = 0; j < 4; j++)
        {
            struct Vertex *vertex = &face->vertexes[j];
            
            GX_Position3s16(x + vertex->x, vertex->y, z + vertex->z);
            GX_Color1x8(face->light);
            GX_TexCoord2u16(texCoords[j][0] + face->tile, texCoords[j][1]);
        }
    }
    GX_End();
    
    chunk->dispListSize = GX_EndDispList();
    assert(chunk->dispListSize != 0);
    free(facesList);
}

u8 lightLevels[16 * 3] ATTRIBUTE_ALIGN(32) = {
    0,   0,   0,
    16,  16,  16,
    32,  32,  32,
    48,  48,  48,
    64,  64,  64,
    80,  80,  80,
    96,  96,  96,
    112, 112, 112,
    128, 128, 128,
    144, 144, 144,
    160, 160, 160,
    176, 176, 176,
    192, 192, 192,
    208, 208, 208,
    224, 224, 224,
    240, 240, 240,
};

void world_render_chunks_at(float x, float z)
{
    int chunkX = world_to_chunk_coord(x);
    int chunkZ = world_to_chunk_coord(z);
    
    GX_LoadTexObj(&blocksTexture, GX_TEXMAP0);
    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, TEX_BLOCK_WIDTH, TEX_BLOCK_HEIGHT);
    
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX8);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGB, GX_RGB8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_U16, 0);
    GX_SetArray(GX_VA_CLR0, lightLevels, 3 * sizeof(u8));
    
    for (int i = -CHUNK_RENDER_RANGE / 2; i <= CHUNK_RENDER_RANGE / 2; i++)
    {
        for (int j = -CHUNK_RENDER_RANGE / 2; j <= CHUNK_RENDER_RANGE / 2; j++)
        {
            struct Chunk *chunk = world_get_chunk(chunkX + i, chunkZ + j);
            
            if (chunk->dispList == NULL)
                build_chunk_display_list(chunk);
            GX_CallDispList(chunk->dispList, chunk->dispListSize);
        }
    }
    
    GX_SetNumTevStages(1);
}

//==================================================
// Setup Functions
//==================================================

void world_init(void)
{
    assert(gSaveFile.name != NULL);
    memset(chunkTable, 0, sizeof(chunkTable));
    
    //Hash the seed string.
    worldSeed = 0;
    for (char *c = gSaveFile.seed; *c != '\0'; c++)
    {
        int shift = ((c - gSaveFile.seed) % sizeof(u16)) * CHAR_BIT;
        
        worldSeed |= *c << shift;
    }
    file_log("world_init(): starting world. name = '%s', seed = '%s', spawn = (%i, %i, %i), modifiedChunksCount = %i",
      gSaveFile.name, gSaveFile.seed, gSaveFile.spawnX, gSaveFile.spawnY, gSaveFile.spawnZ, gSaveFile.modifiedChunksCount);
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
    assert(gSaveFile.name != NULL);
    file_log("world_close(): saving world '%s'", gSaveFile.name);
    file_save_world();
    
    for (int i = 0; i < CHUNK_TABLE_WIDTH; i++)
    {
        for (int j = 0; j < CHUNK_TABLE_WIDTH; j++)
        {
            if (chunkTable[i][j].active)
                delete_chunk(&chunkTable[i][j]);
        }
    }
}
