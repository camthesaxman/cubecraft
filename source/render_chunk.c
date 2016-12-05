#include <string.h>
#include "global.h"
#include "world.h"

/*
void build_chunk_display_list(struct Chunk *chunk)
{
    const int listSize = 65536; //TODO: calculate actual size needed
    int x = chunk->x * CHUNK_WIDTH;
    int z = chunk->z * CHUNK_WIDTH;
    
    chunk->dispList = memalign(32, listSize);
    memset(chunk->dispList, 0, listSize);
    DCInvalidateRange(chunk->dispList, listSize);
    
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
    
    GX_BeginDispList(chunk->dispList, listSize);
    
    float blockImgWidth = 1.0 / 64.0;
    float blockImgX = 4 * blockImgWidth;
    
    GX_Begin(GX_QUADS, GX_VTXFMT1, 4 * CHUNK_WIDTH * CHUNK_WIDTH);
    for (int i = 0; i < CHUNK_WIDTH; i++)
    {
        x = chunk->x * CHUNK_WIDTH + i;
        for (int j = 0; j < CHUNK_WIDTH; j++)
        {
            z = chunk->z * CHUNK_WIDTH + j;
            
            GX_Position3f32(x, 0, z);
            GX_TexCoord2f32(blockImgX, 0);
            GX_Position3f32(x + 1, 0, z);
            GX_TexCoord2f32(blockImgX + blockImgWidth, 0);
            GX_Position3f32(x + 1, 0, z + 1);
            GX_TexCoord2f32(blockImgX + blockImgWidth, 1);
            GX_Position3f32(x, 0, z + 1);
            GX_TexCoord2f32(blockImgX, 1);
        }
    }
    GX_End();
    
    chunk->dispListSize = GX_EndDispList();
    assert(chunk->dispListSize != 0);
}
*/

void render_chunk_immediate(struct Chunk *chunk)
{
    int x = chunk->x * CHUNK_WIDTH;
    int z = chunk->z * CHUNK_WIDTH;
    
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
    
    GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
    GX_Position3f32(x, 0, z);
    GX_TexCoord2f32(0, 0);
    GX_Position3f32(x + CHUNK_WIDTH, 0, z);
    GX_TexCoord2f32(1, 0);
    GX_Position3f32(x + CHUNK_WIDTH, 0, z + CHUNK_WIDTH);
    GX_TexCoord2f32(1, 1);
    GX_Position3f32(x, 0, z + CHUNK_WIDTH);
    GX_TexCoord2f32(0, 1);
    GX_End();
}
