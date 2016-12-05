#include <math.h>
#include "global.h"
#include "drawing.h"
#include "field.h"
#include "main.h"
#include "text.h"
#include "title_menu.h"
#include "world.h"

#define CHUNK_RENDER_RADIUS 3

static void field_main(void);
static void field_draw(void);

struct Vec3f
{
    float x;
    float y;
    float z;
};

static struct Vec3f playerPosition;
static float yaw;  //positive value means looking right
static float pitch;  //positive value means looking up

/*
static void draw_axes(void)
{
    s8 vertices[] ATTRIBUTE_ALIGN(32) = {
        0,  0,  0,   //origin
        10, 0,  0,   //x endpoint
        9,  1,  0,   //x left arrowhead
        9,  -1, 0,   //x right arrowhead
        0,  10, 0,   //y endpoint
        0,  9,  1,   //y left arrowhead
        0,  9,  -1,  //y right arrowhead
        0,  0,  10,  //z endpoint
        1,  0,  9,   //z left arrowhead
        -1, 0,  9,   //z right arrowhead
    };
    
    u8 colors[] ATTRIBUTE_ALIGN(32) = {
        0xFF, 0x00, 0x00, 0xFF,  //red
        0x00, 0xFF, 0x00, 0xFF,  //green
        0x00, 0x00, 0xFF, 0xFF,  //blue
    };
    
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_INDEX8);
    GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX8);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetArray(GX_VA_POS, vertices, 3 * sizeof(s8));
    GX_SetArray(GX_VA_CLR0, colors, 4 * sizeof(u8));
    
    GX_Begin(GX_LINES, GX_VTXFMT1, 6);
    //X axis
    GX_Position1x8(0);
    GX_Color1x8(0);
    GX_Position1x8(1);
    GX_Color1x8(0);
    //Y axis
    GX_Position1x8(0);
    GX_Color1x8(1);
    GX_Position1x8(4);
    GX_Color1x8(1);
    //Z axis
    GX_Position1x8(0);
    GX_Color1x8(2);
    GX_Position1x8(7);
    GX_Color1x8(2);
    GX_End();
    
    GX_Begin(GX_TRIANGLES, GX_VTXFMT1, 9);
    //X axis
    GX_Position1x8(1);
    GX_Color1x8(0);
    GX_Position1x8(2);
    GX_Color1x8(0);
    GX_Position1x8(3);
    GX_Color1x8(0);
    //Y axis
    GX_Position1x8(4);
    GX_Color1x8(1);
    GX_Position1x8(5);
    GX_Color1x8(1);
    GX_Position1x8(6);
    GX_Color1x8(1);
    //Z axis
    GX_Position1x8(7);
    GX_Color1x8(2);
    GX_Position1x8(8);
    GX_Color1x8(2);
    GX_Position1x8(9);
    GX_Color1x8(2);
    GX_End();
}
*/

static void render_scene(void)
{
    int x = world_to_chunk_coord(playerPosition.x);
    int z = world_to_chunk_coord(playerPosition.z);
    
    Mtx posMtx;
    Mtx rotMtx;
    Mtx yawRotMtx;
    Mtx pitchRotMtx;
    guVector axis;
    
    guMtxIdentity(posMtx);
    guMtxApplyTrans(posMtx, posMtx, -playerPosition.x, -(playerPosition.y + 1.5), -playerPosition.z);
    
    axis = (guVector){0.0, 1.0, 0.0};
    guMtxRotAxisDeg(yawRotMtx, &axis, yaw);
    
    axis = (guVector){-1.0, 0.0, 0.0};
    guMtxRotAxisDeg(pitchRotMtx, &axis, pitch);
    
    guMtxConcat(pitchRotMtx, yawRotMtx, rotMtx);
    guMtxConcat(rotMtx, posMtx, posMtx);
    GX_LoadPosMtxImm(posMtx, GX_PNMTX0);
    
    //draw_axes();
    
    for (int i = -CHUNK_RENDER_RADIUS / 2; i <= CHUNK_RENDER_RADIUS / 2; i++)
    {
        for (int j = -CHUNK_RENDER_RADIUS / 2; j <= CHUNK_RENDER_RADIUS / 2; j++)
            world_render_chunk(world_get_chunk(x + i, z + j));
    }
}

static void pause_menu_main(void)
{
    if (gControllerPressedKeys & PAD_BUTTON_START)
    {
        set_main_callback(field_main);
        set_draw_callback(field_draw);
    }
    else if (gControllerPressedKeys & PAD_BUTTON_B)
    {
        world_close();
        title_menu_init();
    }
}

static void pause_menu_draw(void)
{
    text_draw_string(gDisplayWidth / 2, 100, true, "Game Paused");
    text_draw_string(gDisplayWidth / 2, 200, true, "Press the START button to resume");
    text_draw_string(gDisplayWidth / 2, 216, true, "Press the B button to go back to the title screen");
}

static void open_pause_menu(void)
{
    drawing_set_2d_mode();
    set_main_callback(pause_menu_main);
    set_draw_callback(pause_menu_draw);
}

static void field_main(void)
{
    float forward = 0.0;
    float right = 0.0;
    float up = 0.0;
    
    if (gControllerPressedKeys & PAD_BUTTON_START)
        open_pause_menu();
    if (gControllerHeldKeys & PAD_BUTTON_A)
        up = 0.5;
    else if (gControllerHeldKeys & PAD_BUTTON_B)
        up = -0.5;
    if (gCStickX > 10 || gCStickX < -10)
        yaw += (float)gCStickX / 100.0;
    if (gCStickY > 10 || gCStickY < -10)
        pitch += (float)gCStickY / 100.0;
    if (gAnalogStickX > 10 || gAnalogStickX < -10)
        right = (float)gAnalogStickX / 1000.0;
    if (gAnalogStickY > 10 || gAnalogStickY < -10)
        forward = (float)gAnalogStickY / 1000.0;
    
    //Wrap yaw to -180 to 180
    if (yaw > 180.0)
        yaw -= 360.0;
    else if (yaw < -180.0)
        yaw += 360.0;
    //Restrict pitch range to -90 to 90
    if (pitch > 90.0)
        pitch = 90.0;
    else if (pitch < -90.0)
        pitch = -90.0;
    
    playerPosition.z -= forward * sin(DegToRad(yaw + 90.0));
    playerPosition.z -= right * cos(DegToRad(yaw + 90.0));
    playerPosition.x += right * sin(DegToRad(yaw + 90.0));
    playerPosition.x -= forward * cos(DegToRad(yaw + 90.0));
    playerPosition.y += up;
}

static void field_draw(void)
{
    struct Chunk *chunk = world_get_chunk_containing(playerPosition.x, playerPosition.z);
    
    drawing_set_3d_mode();
    render_scene();
    drawing_set_2d_mode();
    text_draw_string_formatted(50, 50, false, "Position: (%.2f, %.2f, %.2f), Chunk: (%i, %i)",
                                              playerPosition.x, playerPosition.y, playerPosition.z, chunk->x, chunk->z);
    text_draw_string_formatted(50, 66, false, "Camera angle: (%.2f, %.2f)",
                                              yaw, pitch);
}

void field_init(void)
{
    playerPosition.x = 0.0;
    playerPosition.y = 0.0;
    playerPosition.z = 0.0;
    yaw = 0.0;
    pitch = 0.0;
    
    world_init();
    set_main_callback(field_main);
    set_draw_callback(field_draw);
}
