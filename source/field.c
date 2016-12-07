#include <math.h>
#include "global.h"
#include "drawing.h"
#include "field.h"
#include "main.h"
#include "text.h"
#include "title_menu.h"
#include "world.h"

//For collision detection
//The player's bounding box is a prism that encompasses everything within 
//playerPosition.x - PLAYER_RADIUS to playerPosition.x + PLAYER_RADIUS,
//playerPosition.z - PLAYER_RADIUS to playerPosition.z + PLAYER_RADIUS, and
//playerPosition.y to playerPosition.y + PLAYER_HEIGHT
#define PLAYER_RADIUS 0.5
#define PLAYER_HEIGHT 1.5

enum
{
    STANDING,
    MIDAIR,
    SWIMMING,
};

struct Vec3f
{
    float x;
    float y;
    float z;
};

static struct Vec3f playerPosition;
static float yVelocity;
static float yaw;  //positive value means looking right
static float pitch;  //positive value means looking up
static int state;

static void field_main(void);
static void field_draw(void);

//==================================================
// Pause Menu
//==================================================

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

//==================================================
// Overworld Functions
//==================================================

//TODO: This collision check can be improved.
static void apply_motion_vector(struct Vec3f motion)
{
    //Clamp fall speed to avoid tunneling problem on large falls. This should be very rare.
    if (motion.y < -1.0)
        motion.y = -1.0;
    
    if (motion.x > 0.0)
    {
        if (BLOCK_IS_SOLID(world_get_block_at(playerPosition.x + motion.x + PLAYER_RADIUS, playerPosition.y, playerPosition.z)))
            motion.x = 0;
    }
    else if (motion.x < 0.0)
    {
        if (BLOCK_IS_SOLID(world_get_block_at(playerPosition.x + motion.x - PLAYER_RADIUS, playerPosition.y, playerPosition.z)))
            motion.x = 0;
    }
    
    if (motion.y > 0.0)
    {
        //TODO: Implement later when we are able to test this.
        assert(state == MIDAIR);
    }
    else if (motion.y < 0.0) //Player hit the ground
    {
        assert(state == MIDAIR);
        if (BLOCK_IS_SOLID(world_get_block_at(playerPosition.x, playerPosition.y + motion.y, playerPosition.z)))
        {
            state = STANDING;
            yVelocity = 0;
            motion.y = 0;
        }
    }
    
    if (motion.z > 0.0)
    {
        if (BLOCK_IS_SOLID(world_get_block_at(playerPosition.x, playerPosition.y, playerPosition.z + motion.z + PLAYER_RADIUS)))
            motion.z = 0;
    }
    else if (motion.z < 0.0)
    {
        if (BLOCK_IS_SOLID(world_get_block_at(playerPosition.x, playerPosition.y, playerPosition.z + motion.z - PLAYER_RADIUS)))
            motion.z = 0;
    }
    
    playerPosition.x += motion.x;
    playerPosition.y += motion.y;
    playerPosition.z += motion.z;
    
    if (state == STANDING)
    {
        if (!BLOCK_IS_SOLID(world_get_block_at(playerPosition.x, playerPosition.y - 1.0, playerPosition.z)))
        {
            state = MIDAIR;
        }
    }
}

static void field_main(void)
{
    struct Vec3f motion;
    float forward = 0.0;
    float right = 0.0;
    float up = 0.0;
    
    if (gControllerPressedKeys & PAD_BUTTON_START)
        open_pause_menu();
    if (state == STANDING)
    {
        assert(yVelocity == 0.0);
        if (gControllerPressedKeys & PAD_BUTTON_A)
        {
            state = MIDAIR;
            yVelocity = 0.18;
        }
    }
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
    
    switch (state)
    {
        case MIDAIR:
            yVelocity -= 0.01;
            break;
        case SWIMMING: //not implemented yet.
            break;
    }
    
    motion.x = right * sin(DegToRad(yaw + 90.0)) - forward * cos(DegToRad(yaw + 90.0));
    motion.y = yVelocity;
    motion.z = -forward * sin(DegToRad(yaw + 90.0)) - right * cos(DegToRad(yaw + 90.0));
    
    //drawing_set_2d_mode();
    apply_motion_vector(motion);
}

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
    
    world_render_chunks_at(playerPosition.x, playerPosition.z);
}

static char *get_state_text(void)
{
    switch (state)
    {
        case STANDING:
            return "STANDING";
            break;
        case MIDAIR:
            return "MIDAIR";
            break;
        case SWIMMING:
            return "SWIMMING";
            break;
        default:
            return "UNDEFINED";
    }
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
    text_draw_string_formatted(50, 82, false, "State: %s", get_state_text());
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
    state = STANDING;
    yVelocity = 0.0;
    
    set_main_callback(field_main);
    set_draw_callback(field_draw);
}
