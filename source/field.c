#include "global.h"
#include "drawing.h"
#include "field.h"
#include "file.h"
#include "inventory.h"
#include "main.h"
#include "menu.h"
#include "text.h"
#include "title_screen.h"
#include "world.h"

//For collision detection
//The player's bounding box is a prism that encompasses everything within 
//playerPosition.x - PLAYER_RADIUS to playerPosition.x + PLAYER_RADIUS,
//playerPosition.z - PLAYER_RADIUS to playerPosition.z + PLAYER_RADIUS, and
//playerPosition.y to playerPosition.y + PLAYER_HEIGHT
#define PLAYER_RADIUS 0.25
#define PLAYER_HEIGHT 1.5

//How far the player's eyes are above where they are standing
#define EYE_LEVEL 1.5

//How far the block selection can reach
#define SELECT_RADIUS 4

enum
{
    STANDING,
    MIDAIR,
    SWIMMING,
};

struct Vec3i
{
    int x;
    int y;
    int z;
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
static bool selectedBlockActive;
static struct Vec3i selectedBlockPos;
static struct Vec3i selectedBlockFace;
static bool showDebugInfo;

static struct MenuItem pauseMenuItems[] = {
    {"Continue"},
    {"Quit"},
};

static struct Menu pauseMenu = {
    "Paused",
    pauseMenuItems,
    ARRAY_LENGTH(pauseMenuItems),
};

static void field_main(void);
static void field_draw(void);

//==================================================
// Pause Menu
//==================================================

static void exit_pause_menu(void)
{
    set_main_callback(field_main);
    set_draw_callback(field_draw);
}

static void pause_menu_main(void)
{
    if (gControllerPressedKeys & PAD_BUTTON_START)
    {
        menu_wait_close_anim(exit_pause_menu);
        return;
    }
    
    switch (menu_process_input())
    {
        case 0: //Continue
        case MENU_CANCEL:
            menu_wait_close_anim(exit_pause_menu);
            break;
        case 1: //Quit
            //Spawn at the current location next time
            gSaveFile.spawnX = floorf(playerPosition.x);
            gSaveFile.spawnY = floorf(playerPosition.y);
            gSaveFile.spawnZ = floorf(playerPosition.z);
            file_log("menu_process_input(): exiting world. position: %i, %i, %i", gSaveFile.spawnX, gSaveFile.spawnY, gSaveFile.spawnZ);
            world_close();
            menu_wait_close_anim(title_screen_init);
            break;
    }
}

static void pause_menu_draw(void)
{
    field_draw();
    menu_draw();
}

static void open_pause_menu(void)
{
    drawing_set_2d_mode();
    menu_init(&pauseMenu);
    set_main_callback(pause_menu_main);
    set_draw_callback(pause_menu_draw);
}

//==================================================
// Overworld Functions
//==================================================

//See "A Fast Voxel Traversal Algorithm for Ray Tracing" by John Amanatides and Andrew Woo
static void get_selected_block(void)
{
    struct Vec3f direction = {
        cosf(DegToRad(90 - yaw)) * cosf(DegToRad(pitch)),
        sinf(DegToRad(pitch)),
        -sinf(DegToRad(90 - yaw)) * cosf(DegToRad(pitch)),
    };
    float eyeY = playerPosition.y + EYE_LEVEL;
    //coordinates of block we are currently at
    struct Vec3i pos = {
        floorf(playerPosition.x),
        floorf(eyeY),
        floorf(playerPosition.z),
    };
    //How far we are from the eye coordinates
    struct Vec3i radius = {0, 0, 0};
    //increment/decrement coordinates
    struct Vec3i step = {0, 0, 0};
    //ray distance it takes to equal one unit in each direction
    struct Vec3f tDelta = {INFINITY, INFINITY, INFINITY};
    //ray distance it takes to move to next block boundary in each direction
    struct Vec3f tMax = {INFINITY, INFINITY, INFINITY};
    
    if (direction.x > 0.0)
    {
        step.x = 1;
        tDelta.x = fabsf(1.0 / direction.x);
        tMax.x = fabsf((floorf(playerPosition.x + 1.0) - playerPosition.x) / direction.x);
    }
    else if (direction.x < 0.0)
    {
        step.x = -1;
        tDelta.x = fabsf(1.0 / direction.x);
        tMax.x = fabsf((playerPosition.x - ceilf(playerPosition.x - 1.0)) / direction.x);
    }
    if (direction.y > 0.0)
    {        
        step.y = 1;
        tDelta.y = fabsf(1.0 / direction.y);
        tMax.y = fabsf((floorf(eyeY + 1.0) - eyeY) / direction.y);
    }
    else if (direction.y < -0.0)
    {
        step.y = -1;
        tDelta.y = fabsf(1.0 / direction.y);
        tMax.y = fabsf((eyeY - ceilf(eyeY - 1.0)) / direction.y);
    }
    if (direction.z > 0.0)
    {
        step.z = 1;
        tDelta.z = fabsf(1.0 / direction.z);
        tMax.z = fabsf((floorf(playerPosition.z + 1.0) - playerPosition.z) / direction.z);
    }
    else if (direction.z < -0.0)
    {
        step.z = -1;
        tDelta.z = fabsf(1.0 / direction.z);
        tMax.z = fabsf((playerPosition.z - ceilf(playerPosition.z - 1.0)) / direction.z);
    }
    
    selectedBlockActive = false;
    
    //for (int i = 0; i < SELECT_RADIUS; i++)
    while (radius.x * radius.x + radius.y * radius.y + radius.z * radius.z < SELECT_RADIUS * SELECT_RADIUS)
    {
        if (tMax.x < tMax.y)
        {
            if (tMax.x < tMax.z)
            {
                //increment x
                tMax.x += tDelta.x;
                pos.x += step.x;
                radius.x++;
                selectedBlockFace.x = -step.x;
                selectedBlockFace.y = 0;
                selectedBlockFace.z = 0;
            }
            else
            {
                goto increment_z;
            }
        }
        else
        {
            if (tMax.y < tMax.z)
            {
                //increment y
                tMax.y += tDelta.y;
                pos.y += step.y;
                radius.y++;
                selectedBlockFace.x = 0;
                selectedBlockFace.y = -step.y;
                selectedBlockFace.z = 0;
            }
            else
            {
                //increment z
              increment_z:
                tMax.z += tDelta.z;
                pos.z += step.z;
                radius.z++;
                selectedBlockFace.x = 0;
                selectedBlockFace.y = 0;
                selectedBlockFace.z = -step.z;
            }
        }
        
        if (BLOCK_IS_SOLID(world_get_block_at(pos.x, pos.y, pos.z)))
        {
            selectedBlockPos.x = pos.x;
            selectedBlockPos.y = pos.y;
            selectedBlockPos.z = pos.z;
            selectedBlockActive = true;
            return;
        }
    }
}

static void draw_crosshair(void)
{
    drawing_set_fill_color(255, 255, 255, 255);
    
    drawing_draw_line(gDisplayWidth / 2, gDisplayHeight / 2 + 10, gDisplayWidth / 2, gDisplayHeight / 2 - 10);
    drawing_draw_line(gDisplayWidth / 2 + 10, gDisplayHeight / 2, gDisplayWidth / 2 - 10, gDisplayHeight / 2);
}

static void draw_block_selection(void)
{
    u8 blockSelectionColor[] ATTRIBUTE_ALIGN(32) = {255, 255, 255};
    
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGB, GX_RGB8, 0);
    
    GX_Begin(GX_LINESTRIP, GX_VTXFMT0, 5);
    if (selectedBlockFace.x != 0)
    {
        float x = selectedBlockPos.x + ((selectedBlockFace.x > 0) ? 1.01 : -0.01);
        
        GX_Position3f32(x, selectedBlockPos.y + 0.99, selectedBlockPos.z + 0.99);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
        GX_Position3f32(x, selectedBlockPos.y + 0.99, selectedBlockPos.z + 0.01);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
        GX_Position3f32(x, selectedBlockPos.y + 0.01, selectedBlockPos.z + 0.01);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
        GX_Position3f32(x, selectedBlockPos.y + 0.01, selectedBlockPos.z + 0.99);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
        GX_Position3f32(x, selectedBlockPos.y + 0.99, selectedBlockPos.z + 0.99);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
    }
    else if (selectedBlockFace.y != 0)
    {
        float y = selectedBlockPos.y + ((selectedBlockFace.y > 0) ? 1.01 : -0.01);
        
        GX_Position3f32(selectedBlockPos.x + 0.01, y, selectedBlockPos.z + 0.01);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
        GX_Position3f32(selectedBlockPos.x + 0.99, y, selectedBlockPos.z + 0.01);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
        GX_Position3f32(selectedBlockPos.x + 0.99, y, selectedBlockPos.z + 0.99);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
        GX_Position3f32(selectedBlockPos.x + 0.01, y, selectedBlockPos.z + 0.99);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
        GX_Position3f32(selectedBlockPos.x + 0.01, y, selectedBlockPos.z + 0.01);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
    }
    else if (selectedBlockFace.z != 0)
    {
        float z = selectedBlockPos.z + ((selectedBlockFace.z > 0) ? 1.01 : -0.01);
        
        GX_Position3f32(selectedBlockPos.x + 0.01, selectedBlockPos.y + 0.99, z);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
        GX_Position3f32(selectedBlockPos.x + 0.99, selectedBlockPos.y + 0.99, z);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
        GX_Position3f32(selectedBlockPos.x + 0.99, selectedBlockPos.y + 0.01, z);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
        GX_Position3f32(selectedBlockPos.x + 0.01, selectedBlockPos.y + 0.01, z);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
        GX_Position3f32(selectedBlockPos.x + 0.01, selectedBlockPos.y + 0.99, z);
        GX_Color3u8(blockSelectionColor[0], blockSelectionColor[1], blockSelectionColor[2]);
    }
    else
    {
        assert(false);  //One of the faces must be selected
    }
    GX_End();
}

static void apply_motion_vector(struct Vec3f motion)
{
    struct Vec3f newPosition = playerPosition;
    int xMin = floorf(playerPosition.x - PLAYER_RADIUS);
    int xMax = floorf(playerPosition.x + PLAYER_RADIUS);
    int yMin = floorf(playerPosition.y);
    int yMax = floorf(playerPosition.y + PLAYER_HEIGHT);
    int zMin = floorf(playerPosition.z - PLAYER_RADIUS);
    int zMax = floorf(playerPosition.z + PLAYER_RADIUS);
    bool testX = false;
    bool testY = false;
    bool testZ = false;
    int x, y, z;
    bool collided;
    
    //Clamp fall speed to avoid tunneling problem on large falls. This should be very rare.
    if (motion.y < -1.0)
        motion.y = -1.0;
    
    if (motion.x < 0.0)
    {
        x = floorf(playerPosition.x - PLAYER_RADIUS + motion.x);
        testX = true;
    }
    else if (motion.x > 0.0)
    {
        x = floorf(playerPosition.x + PLAYER_RADIUS + motion.x);
        testX = true;
    }
    
    if (motion.y < 0.0)
    {
        y = floorf(playerPosition.y + motion.y);
        testY = true;
    }
    else if (motion.y > 0.0)
    {
        y = floorf(playerPosition.y + PLAYER_HEIGHT + motion.y);
        testY = true;
    }
    
    if (motion.z < 0.0)
    {
        z = floorf(playerPosition.z - PLAYER_RADIUS + motion.z);
        testZ = true;
    }
    else if (motion.z > 0.0)
    {
        z = floorf(playerPosition.z + PLAYER_RADIUS + motion.z);
        testZ = true;
    }
    
    if (testX)
    {
        collided = false;
        for (int y = yMin; y <= yMax; y++)
        {
            for (int z = zMin; z <= zMax; z++)
            {
                if (BLOCK_IS_SOLID(world_get_block_at(x, y, z)))
                    collided = true; 
            }
        }
        if (!collided)
            newPosition.x += motion.x;
    }
    
    if (testY)
    {
        assert(state == MIDAIR);
        collided = false;
        for (int x = xMin; x <= xMax; x++)
        {
            for (int z = zMin; z <= zMax; z++)
            {
                if (BLOCK_IS_SOLID(world_get_block_at(x, y, z)))
                    collided = true;
            }
        }
        if (collided)
        {
            if (motion.y < 0.0)
            {
                newPosition.y = y + 1;
                state = STANDING;
            }
            yVelocity = 0.0;
        }
        else
        {
            newPosition.y += motion.y;
        }
    }
    
    if (testZ)
    {
        collided = false;
        for (int x = xMin; x <= xMax; x++)
        {
            for (int y = yMin; y <= yMax; y++)
            {
                if (BLOCK_IS_SOLID(world_get_block_at(x, y, z)))
                    collided = true;
            }
        }
        if (!collided)
            newPosition.z += motion.z;
    }
    
    playerPosition = newPosition;
    
    if (state == STANDING)
    {
        if (!BLOCK_IS_SOLID(world_get_block_at(playerPosition.x, playerPosition.y - 1.0, playerPosition.z)))
        {
            state = MIDAIR;
        }
    }
}

static int analog_stick_clamp(int value, int deadzone)
{
    if (value > 0)
    {
        if (value > deadzone)
            return value - deadzone;
    }
    else if (value < 0)
    {
        if (value < -deadzone)
            return value + deadzone;
    }
    
    return 0;
}

static void field_main(void)
{
    struct Vec3f motion;
    float forward = 0.0;
    float right = 0.0;
    
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
    
    if (gControllerPressedKeys & PAD_BUTTON_B)
    {
        if (selectedBlockActive)
        {
            int block = world_get_block_at(selectedBlockPos.x, selectedBlockPos.y, selectedBlockPos.z);
            
            world_set_block(selectedBlockPos.x, selectedBlockPos.y, selectedBlockPos.z, BLOCK_AIR);
            inventory_add_block(block);
        }
    }
    else if (gControllerPressedKeys & PAD_BUTTON_Y)
    {
        if (selectedBlockActive && gSaveFile.inventory[inventorySelection].count > 0)
        {
            world_set_block(selectedBlockPos.x + selectedBlockFace.x,
                            selectedBlockPos.y + selectedBlockFace.y,
                            selectedBlockPos.z + selectedBlockFace.z,
                            gSaveFile.inventory[inventorySelection].type);
            gSaveFile.inventory[inventorySelection].count--;
        }
    }
    
    if (gControllerPressedKeys & PAD_TRIGGER_Z)
        showDebugInfo = !showDebugInfo;
        
    if (gControllerPressedKeys & PAD_BUTTON_LEFT)
    {
        inventorySelection--;
        if (inventorySelection == -1)
            inventorySelection = NUM_ITEM_SLOTS - 1;
    }
    else if (gControllerPressedKeys & PAD_BUTTON_RIGHT)
    {
        inventorySelection++;
        if (inventorySelection == NUM_ITEM_SLOTS)
            inventorySelection = 0;
    }
    
    yaw += (float)analog_stick_clamp(gCStickX, 15) / 40.0;
    pitch += (float)analog_stick_clamp(gCStickY, 15) / 40.0;
    
    right = (float)analog_stick_clamp(gAnalogStickX, 15) / 1000.0;
    forward = (float)analog_stick_clamp(gAnalogStickY, 15) / 1000.0;
    
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
    
    motion.x = right * sinf(DegToRad(yaw + 90.0)) - forward * cosf(DegToRad(yaw + 90.0));
    motion.y = yVelocity;
    motion.z = -forward * sinf(DegToRad(yaw + 90.0)) - right * cosf(DegToRad(yaw + 90.0));
    
    apply_motion_vector(motion);
    get_selected_block();
}

static void render_scene(void)
{
    Mtx posMtx;
    Mtx rotMtx;
    Mtx yawRotMtx;
    Mtx pitchRotMtx;
    guVector axis;
    
    guMtxIdentity(posMtx);
    guMtxApplyTrans(posMtx, posMtx, -playerPosition.x, -(playerPosition.y + EYE_LEVEL), -playerPosition.z);
    
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
    if (selectedBlockActive)
        draw_block_selection();
    drawing_set_2d_mode();
    inventory_draw();
    if (showDebugInfo)
    {
        text_set_font_size(8, 16);
        text_init();
        text_draw_string_formatted(50, 50, 0, "Position: (%.2f, %.2f, %.2f), Chunk: (%i, %i)",
                                                  playerPosition.x, playerPosition.y, playerPosition.z, chunk->x, chunk->z);
        text_draw_string_formatted(50, 66, 0, "Camera angle: (%.2f, %.2f)",
                                                  yaw, pitch);
        if (selectedBlockActive)
            text_draw_string_formatted(50, 82, 0, "Selected block: (%i, %i, %i)", selectedBlockPos.x, selectedBlockPos.y, selectedBlockPos.z);
        else
            text_draw_string(50, 82, 0, "Selected block: none");
        text_draw_string_formatted(50, 98, 0, "State: %s, yVelocity = %.2f", get_state_text(), yVelocity);
        text_draw_string_formatted(50, 114, 0, "FPS: %i", gFramesPerSecond);
        text_draw_string_formatted(50, 130, 0, "World: %s, Seed: %s", gSaveFile.name, gSaveFile.seed);
    }
    draw_crosshair();
}

void field_init(void)
{
    struct Chunk *chunk;
    int x, y, z;
    
    world_init();
    file_log("field_init(): starting at position: %i, %i", gSaveFile.spawnX, gSaveFile.spawnZ);
    playerPosition.x = gSaveFile.spawnX + 0.5;
    playerPosition.z = gSaveFile.spawnZ + 0.5;
    yaw = 0.0;
    pitch = 0.0;
    chunk = world_get_chunk_containing(playerPosition.x, playerPosition.z);
    x = (unsigned int)floor(playerPosition.x) % CHUNK_WIDTH;
    z = (unsigned int)floor(playerPosition.z) % CHUNK_WIDTH;
    
    for (y = gSaveFile.spawnY; y >= 0; y--)
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
    selectedBlockActive = false;
    inventory_init();
    showDebugInfo = false;
    
    text_set_font_size(8, 16);
    set_main_callback(field_main);
    set_draw_callback(field_draw);
}
