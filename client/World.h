#ifndef H_WORLD
#define H_WORLD

#include <thread>

#include "constants.h"
#include "PerlinNoise.h"
#include "BlocksIds.h"
#include "Player.h"
#include "Chunk.h"
#include "Math/Vec3i.h"

class World {
private:
    siv::PerlinNoise perlin;
    std::vector<std::thread> threads;

    std::mutex mutex;
public:
    Player *player;
    int seedValue;
    std::vector<Chunk *> chunks;

    World(int seedValue);

    int xx = 0;
    int zz = 0;
    int max_xz = 8;

    bool isChunkExist(Vec3i chunkPos);

    // TODO: Review all allocable things
    void unloadChunk(Chunk *chunk);

    void updateChunks();

    void generateFilledChunk(Vec3i pos);

    void setBlock(BlockID id, Vec3i pos);
};

#endif //H_WORLD
