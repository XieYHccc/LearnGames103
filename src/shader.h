#ifndef SHADER_H
#define SHADER_H

#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{   
public:
    // constructor reads and builds the shader
    Shader(const char* vertexPath, const char* fragmentPath);

    // use/activate the shader
    void use();

    // utility uniform functions
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setMat4(const std::string &name, glm::mat4 mat) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
    void setVec3(const std::string &name, const glm::vec3& vec);
    // the program ID
    unsigned int ID;

private:
    // utility function for checking shader compilation/linking errors
    // ---------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, std::string type);
};

#endif