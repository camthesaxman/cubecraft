#include <math.h>
#include "global.h"
#include "drawing.h"
#include "field.h"
#include "main.h"
#include "text.h"
#include "title_menu.h"
#include "world.h"

#define PLAYER_RADIUS 0.5

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
    struct Chunk *chunk;
    struct Chunk *neighborChunk;
    struct Vec3f newPosition;
    float fracX, fracZ;
    int intX, intY, intZ;
    bool hitX = false;
    bool hitZ = false;
    
    //We're not yet prepared to handle these cases
    assert(fabsf(motion.x) <= 1.0);
    assert(fabsf(motion.y) <= 1.0);
    assert(fabsf(motion.z) <= 1.0);
    newPosition.x = playerPosition.x + motion.x;
    newPosition.y = playerPosition.y + motion.y;
    newPosition.z = playerPosition.z + motion.z;
    chunk = world_get_chunk_containing(newPosition.x, newPosition.z);
    
    intX = world_to_block_coord(newPosition.x);
    intY = floorf(newPosition.y);
    intZ = world_to_block_coord(newPosition.z);
    fracX = newPosition.x - floorf(newPosition.x);
    fracZ = newPosition.z - floorf(newPosition.z);
    
    hitX = BLOCK_IS_SOLID(chunk->blocks[intX][intY][intZ]);
    if (!hitX)
    {
        if (fracX < PLAYER_RADIUS)
        {
            //Check x - 1
            if (intX > 0)
            {
                hitX = BLOCK_IS_SOLID(chunk->blocks[intX - 1][intY][intZ]);
            }
            else
            {
                neighborChunk = world_get_chunk(chunk->x - 1, chunk->z);
                hitX = BLOCK_IS_SOLID(neighborChunk->blocks[CHUNK_WIDTH - 1][intY][intZ]);
            }
        }
        else if (fracX > 1.0 - PLAYER_RADIUS)
        {
            //Check x + 1
            if (intX < CHUNK_WIDTH - 1)
            {
                hitX = BLOCK_IS_SOLID(chunk->blocks[intX + 1][intY][intZ]);
            }
            else
            {
                neighborChunk = world_get_chunk(chunk->x + 1, chunk->z);
                hitX = BLOCK_IS_SOLID(neighborChunk->blocks[0][intY][intZ]);
            }
        }
    }
    //clamp x position
    if (hitX)
    {
        //text_draw_string(0, 0, false, "X Collision");
        //TODO: find maximum X position
        newPosition.x = playerPosition.x;
        intX = world_to_block_coord(newPosition.x);
    }
    
    hitZ = BLOCK_IS_SOLID(chunk->blocks[intX][intY][intZ]);
    if (!hitZ)
    {
        if (fracZ < PLAYER_RADIUS)
        {
            //Check z - 1
            if (intZ > 0)
            {
                hitZ = BLOCK_IS_SOLID(chunk->blocks[intX][intY][intZ - 1]);
            }
            else
            {
                neighborChunk = world_get_chunk(chunk->x, chunk->z - 1);
                hitZ = BLOCK_IS_SOLID(neighborChunk->blocks[intX][intY][CHUNK_WIDTH - 1]);
            }
        }
        else if (fracZ > 1.0 - PLAYER_RADIUS)
        {
            //Check z + 1
            if (intZ < CHUNK_WIDTH - 1)
            {
                hitZ = BLOCK_IS_SOLID(chunk->blocks[intX][intY][intZ + 1]);
            }
            else
            {
                neighborChunk = world_get_chunk(chunk->x, chunk->z + 1);
                hitZ = BLOCK_IS_SOLID(neighborChunk->blocks[intX][intY][0]);
            }
        }
    }
    //clamp z position
    if (hitZ)
    {
        //text_draw_string(0, 16, false, "Z Collision");
        newPosition.z = playerPosition.z;
        intZ = world_to_block_coord(newPosition.z);
    }
    
    if (state == MIDAIR) //We only need to check hitting the ground when the player is in midair
    {
        chunk = world_get_chunk_containing(newPosition.x, newPosition.z);
        if (BLOCK_IS_SOLID(chunk->blocks[intX][intY][intZ]))
        {
            state = STANDING;
            yVelocity = 0.0;
            newPosition.y = intY + 1;
        }
    }
    else if (state == STANDING)
    {
        assert(yVelocity == 0.0);
        assert((float)intY == newPosition.y);
        chunk = world_get_chunk_containing(newPosition.x, newPosition.z);
        if (!BLOCK_IS_SOLID(chunk->blocks[intX][intY - 1][intZ]))
            state = MIDAIR;
    }
    
    playerPosition = newPosition;
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
