#include "global.h"
#include "drawing.h"

void drawing_set_2d_mode(void)
{
    Mtx44 projectionMtx;
    Mtx positionMtx;
    
    guOrtho(projectionMtx, 0, gDisplayHeight, 0, gDisplayWidth, 0.0, 1.0);
    GX_LoadProjectionMtx(projectionMtx, GX_ORTHOGRAPHIC);
    guMtxIdentity(positionMtx);
    GX_LoadPosMtxImm(positionMtx, GX_PNMTX0);
}

void drawing_set_3d_mode(void)
{
    Mtx44 projectionMtx;
    
    guPerspective(projectionMtx, 60, (float)gDisplayWidth / (float)gDisplayHeight, 0.5, 300.0);
    GX_LoadProjectionMtx(projectionMtx, GX_PERSPECTIVE);
}
