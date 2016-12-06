#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "world.h"

void build_chunk_display_list_flat(struct Chunk *chunk)
{
    int blockFace = 4;
    float texLeft = (float)blockFace / 8.0;
    float texRight = (float)(blockFace + 1) / 8.0;
    int x, z;
    const size_t listSize = 4 * 1024; //TODO: calculate actual size needed
    
    chunk->dispList = memalign(32, listSize);
    memset(chunk->dispList, 0, listSize);
    DCInvalidateRange(chunk->dispList, listSize);
    
    GX_BeginDispList(chunk->dispList, listSize);
    
    GX_Begin(GX_QUADS, GX_VTXFMT1, 4 * CHUNK_WIDTH * CHUNK_WIDTH);
    for (int i = 0; i < CHUNK_WIDTH; i++)
    {
        x = chunk->x * CHUNK_WIDTH + i;
        for (int j = 0; j < CHUNK_WIDTH; j++)
        {
            z = chunk->z * CHUNK_WIDTH + j;
            
            GX_Position3s16(x, 0, z);
            GX_TexCoord2f32(texLeft, 0.0);
            GX_Position3s16(x + 1, 0, z);
            GX_TexCoord2f32(texRight, 0.0);
            GX_Position3s16(x + 1, 0, z + 1);
            GX_TexCoord2f32(texRight, 1.0);
            GX_Position3s16(x, 0, z + 1);
            GX_TexCoord2f32(texLeft, 1.0);
        }
    }
    GX_End();
    
    chunk->dispListSize = GX_EndDispList();
    assert(chunk->dispListSize != 0);
}

void render_chunk_displist_flat(struct Chunk *chunk)
{
    assert(chunk->dispList != NULL);
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
    printf("display list size = %u\n", chunk->dispListSize);
    GX_CallDispList(chunk->dispList, chunk->dispListSize);
}

struct Vertex
{
    s16 x, y, z;
};

struct Face
{
    struct Vertex vertexes[4];
};

struct Face *facesList;
int facesListCount;
int facesListCapacity;

enum
{
    DIR_X,
    DIR_Y,
    DIR_Z,
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

static void add_face(int x, int y, int z, int direction)
{
    struct Face face;
    
    //puts("add_face");
    switch (direction)
    {
        case DIR_X:
            face.vertexes[0] = (struct Vertex){0, 0, 0};
            face.vertexes[1] = (struct Vertex){0, 1, 0};
            face.vertexes[2] = (struct Vertex){0, 1, 1};
            face.vertexes[3] = (struct Vertex){0, 0, 1};
            break;
        case DIR_Y:
            face.vertexes[0] = (struct Vertex){0, 0, 0};
            face.vertexes[1] = (struct Vertex){0, 0, 1};
            face.vertexes[2] = (struct Vertex){1, 0, 1};
            face.vertexes[3] = (struct Vertex){1, 0, 0};
            break;
        case DIR_Z:
            face.vertexes[0] = (struct Vertex){0, 0, 0};
            face.vertexes[1] = (struct Vertex){1, 0, 0};
            face.vertexes[2] = (struct Vertex){1, 1, 0};
            face.vertexes[3] = (struct Vertex){0, 1, 0};
            break;
        default:
            assert(false);
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
    
    for (int x = 0; x < CHUNK_WIDTH; x++)
    {
        for (int y = 0; y < CHUNK_HEIGHT; y++)
        {
            for (int z = 0; z < CHUNK_WIDTH; z++)
            {
                if (BLOCK_IS_SOLID(chunk->blocks[x][y][z]))
                {
                    if (x > 0 && !BLOCK_IS_SOLID(chunk->blocks[x - 1][y][z]))
                        add_face(x, y, z, DIR_X);
                    if (y > 0 && !BLOCK_IS_SOLID(chunk->blocks[x][y - 1][z]))
                        add_face(x, y, z, DIR_Y);
                    if (z > 0 && !BLOCK_IS_SOLID(chunk->blocks[x][y][z - 1]))
                        add_face(x, y, z, DIR_Z);
                }
                else  //block is not solid
                {
                    if (x > 0 && BLOCK_IS_SOLID(chunk->blocks[x - 1][y][z]))
                        add_face(x, y, z, DIR_X);
                    if (y > 0 && BLOCK_IS_SOLID(chunk->blocks[x][y - 1][z]))
                        add_face(x, y, z, DIR_Y);
                    if (z > 0 && BLOCK_IS_SOLID(chunk->blocks[x][y][z - 1]))
                        add_face(x, y, z, DIR_Z);
                }
            }
        }
    }
}

//TODO: Get display lists working properly. Doing this shit every frame is O(MG) SLOW!!!
//TODO: Get indexed texture coordinates working
void render_chunk_immediate(struct Chunk *chunk)
{
    int x = chunk->x * CHUNK_WIDTH;
    int z = chunk->z * CHUNK_WIDTH;
    int blockFace = 4;
    float texLeft = (float)blockFace / 8.0;
    float texRight = (float)(blockFace + 1) / 8.0;
    f32 texCoords[] ATTRIBUTE_ALIGN(32) = {
        texLeft,  0.0,
        texLeft,  1.0,
        texRight, 1.0,
        texRight, 0.0,
    };
    
    build_exposed_faces_list(chunk);
    
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    //GX_SetVtxDesc(GX_VA_TEX1, GX_INDEX8);
    GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
    //GX_SetArray(GX_VA_TEX1, texCoords, 2 * sizeof(f32));    
    
    GX_Begin(GX_QUADS, GX_VTXFMT1, 4 * facesListCount);
    for (int i = 0; i < facesListCount; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            struct Vertex *vertex = &facesList[i].vertexes[j];
            
            GX_Position3s16(x + vertex->x, vertex->y, z + vertex->z);
            //GX_TexCoord1x8(j);
            GX_TexCoord2f32(texCoords[j * 2], texCoords[j * 2 + 1]);
        }
    }
    GX_End();
    
    free(facesList);
}

void render_chunk_immediate_flat(struct Chunk *chunk)
{
    int blockFace = 0;
    float texLeft = (float)blockFace / 8.0;
    float texRight = (float)(blockFace + 1) / 8.0;
    int x, z;
    
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
    
    GX_Begin(GX_QUADS, GX_VTXFMT1, 4 * CHUNK_WIDTH * CHUNK_WIDTH);
    for (int i = 0; i < CHUNK_WIDTH; i++)
    {
        x = chunk->x * CHUNK_WIDTH + i;
        for (int j = 0; j < CHUNK_WIDTH; j++)
        {
            z = chunk->z * CHUNK_WIDTH + j;
            
            GX_Position3s16(x, 0, z);
            GX_TexCoord2f32(texLeft, 0.0);
            GX_Position3s16(x + 1, 0, z);
            GX_TexCoord2f32(texRight, 0.0);
            GX_Position3s16(x + 1, 0, z + 1);
            GX_TexCoord2f32(texRight, 1.0);
            GX_Position3s16(x, 0, z + 1);
            GX_TexCoord2f32(texLeft, 1.0);
        }
    }
    GX_End();
}
