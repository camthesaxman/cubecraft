#include <math.h>
#include "global.h"
#include "drawing.h"
#include "field.h"
#include "main.h"
#include "text.h"
#include "title_menu.h"
#include "world.h"

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

static void render_scene(void)
{
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
    
    world_render_chunks_at(playerPosition.x, playerPosition.z);
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
        up = 0.1;
    else if (gControllerHeldKeys & PAD_BUTTON_B)
        up = -0.1;
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
    struct Chunk *chunk;
    int x, y, z;
    
    world_init();
    playerPosition.x = 0.0;
    playerPosition.z = 0.0;
    yaw = 0.0;
    pitch = 0.0;
    chunk = world_get_chunk_containing(playerPosition.x, playerPosition.z);
    x = (unsigned int)floor(playerPosition.x) % CHUNK_WIDTH;
    z = (unsigned int)floor(playerPosition.z) % CHUNK_WIDTH;
    
    for (y = CHUNK_HEIGHT -1; y >= 0; y--)
    {
        if (BLOCK_IS_SOLID(chunk->blocks[x][y][z]))
        {
            y++;
            break;
        }
    }
    playerPosition.y = y;
    
    set_main_callback(field_main);
    set_draw_callback(field_draw);
}
