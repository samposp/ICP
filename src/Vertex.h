#pragma once

#include <glm/glm.hpp> 

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;

    bool operator == (const Vertex& v1) const {
        return (Position == v1.Position
            && Normal == v1.Normal
            && TexCoords == v1.TexCoords);
    }
};

