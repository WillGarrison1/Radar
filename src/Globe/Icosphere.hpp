#pragma once

#include <vector>
#include <glm/vec3.hpp>

class Icosphere
{
public:
    Icosphere();
    ~Icosphere();

private:
    std::vector<glm::vec3> vertices;
};