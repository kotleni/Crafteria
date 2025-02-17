#ifndef H_WORLD
#define H_WORLD

#include <thread>

#include "constants.h"
#include "PerlinNoise.h"
#include "BlocksIds.h"
#include "Player.h"
#include "Chunk.h"
#include "Math/Vec3i.h"
#include "BlocksSource.h"
#include "World/Generator/AbstractWorldGenerator.h"
#include "World/Generator/DefaultWorldGenerator.h"

class World: public BlocksSource {
private:
    std::vector<std::thread> threads;

    std::mutex mutex;
public:
    Player *player;
    int seedValue;
    AbstractWorldGenerator *generator;
    std::vector<Chunk *> chunks;

    World(int seedValue);

    int xx = 0;
    int zz = 0;
    int max_xz = 8;

    bool isChunkExist(Vec3i chunkPos);

    void markChunkToUnload(Chunk *chunk);

    // TODO: Review all allocable things
    void unloadChunk(Chunk *chunk);

    bool areNeighborsGenerated(const Vec3i &chunkPos);
    void updateChunks();

    void generateFilledChunk(Vec3i pos);

    Block *getBlock(Vec3i pos) override;
    void setBlock(BlockID id, Vec3i pos) override;
};

#endif //H_WORLD
