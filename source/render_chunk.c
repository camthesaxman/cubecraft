#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "world.h"

void render_chunk_displist(struct Chunk *chunk)
{
    assert(chunk->dispList != NULL);
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
    GX_CallDispList(chunk->dispList, chunk->dispListSize);
}

enum
{
    STONE_FACE,
    SAND_FACE,
    DIRT_FACE,
    DIRTGRASS_FACE,
    GRASS_FACE,
    WOOD_FACE,
};

const u8 blockFaceTiles[][6] = {
	[BLOCK_STONE] = {STONE_FACE, STONE_FACE, STONE_FACE, STONE_FACE, STONE_FACE, STONE_FACE},
	[BLOCK_SAND] = {SAND_FACE, SAND_FACE, SAND_FACE, SAND_FACE, SAND_FACE, SAND_FACE},
	[BLOCK_DIRT] = {DIRT_FACE, DIRT_FACE, DIRT_FACE, DIRT_FACE, DIRT_FACE, DIRT_FACE},
	[BLOCK_DIRTGRASS] = {DIRTGRASS_FACE, DIRTGRASS_FACE, GRASS_FACE, DIRT_FACE, DIRTGRASS_FACE, DIRTGRASS_FACE},
	[BLOCK_WOOD] = {WOOD_FACE, WOOD_FACE, WOOD_FACE, WOOD_FACE, WOOD_FACE, WOOD_FACE}
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

struct Face *facesList;
int facesListCount;
int facesListCapacity;

enum
{
    DIR_X_FRONT,
    DIR_X_BACK,
    DIR_Y_FRONT,
    DIR_Y_BACK,
    DIR_Z_FRONT,
    DIR_Z_BACK,
};

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
    
    face.tile = blockFaceTiles[block][direction];
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

//TODO: Patch up exposed faces between chunks
static void build_exposed_faces_list(struct Chunk *chunk)
{
    facesList = NULL;
    facesListCount = 0;
    facesListCapacity = 0;
    struct Chunk *prevChunkX = world_get_chunk(chunk->x - 1, chunk->z);
    struct Chunk *prevChunkZ = world_get_chunk(chunk->x, chunk->z - 1);
    
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

void build_chunk_display_list(struct Chunk *chunk)
{
    size_t listSize;
    int x = chunk->x * CHUNK_WIDTH;
    int z = chunk->z * CHUNK_WIDTH;
    int blockFace = 0;
    float texLeft = (float)blockFace / 8.0;
    float texRight = (float)(blockFace + 1) / 8.0;
    f32 texCoords[] ATTRIBUTE_ALIGN(32) = {
        texLeft,  0.0,
        texRight, 0.0,
        texRight, 1.0,
        texLeft, 1.0,
    };
    
    build_exposed_faces_list(chunk);
    
    listSize = 3 + facesListCount * 4 * (3 * sizeof(s16) + 2 * sizeof(f32)) + 64;
    listSize = round_up(listSize, 32);
    chunk->dispList = memalign(32, listSize);
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
            GX_TexCoord2f32(texCoords[j * 2] + (float)face->tile / 8.0, texCoords[j * 2 + 1]);
        }
    }
    GX_End();
    
    chunk->dispListSize = GX_EndDispList();
    printf("size of display list: estimated %i, actual %i bytes", listSize, chunk->dispListSize);
    assert(chunk->dispListSize != 0);
    
    free(facesList);
}
