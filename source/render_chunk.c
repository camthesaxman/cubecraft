#include <stdio.h>
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

void render_chunk_immediate_flat(struct Chunk *chunk)
{
    int blockFace = 4;
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
