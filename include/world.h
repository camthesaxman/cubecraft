#ifndef GUARD_WORLD_H
#define GUARD_WORLD_H

#define CHUNK_WIDTH 16  //Must be a power of two
#define CHUNK_HEIGHT 256

enum
{
	BLOCK_AIR,
	BLOCK_WATER,
	BLOCK_STONE,
	BLOCK_SAND,
	BLOCK_DIRT,
	BLOCK_DIRTGRASS,
	BLOCK_WOOD
};

struct Chunk
{
    bool active;
    int x;
    int z;
    void *dispList;
    size_t dispListSize;
    u8 blocks[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_WIDTH];  //blocks[x][y][z]
};

void world_init(void);
void world_close(void);
void world_render_chunk(struct Chunk *chunk);
struct Chunk *world_get_chunk(int x, int z);
struct Chunk *world_get_chunk_containing(float x, float z);
int world_to_chunk_coord(float x);

#endif //GUARD_WORLD_H
