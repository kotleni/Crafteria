#ifndef BAKEDCHUNKPART_H
#define BAKEDCHUNKPART_H

#include <vector>
#include "../GL/glad.h"

#include "BlocksIds.h"

class BakedChunkPart {
public:
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;
    BlockID blockID;

    GLuint vao, vbo, ebo;
    bool isBuffered; // TODO: Replace by checking VAO...
    bool isSolid;
    bool isFlora;

    [[nodiscard]] bool hasBuffered() const;

    void bufferMesh();
};

#endif //BAKEDCHUNKPART_H
