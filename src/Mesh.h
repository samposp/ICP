#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp> 
#include <glm/ext.hpp>

#include "Vertex.h"
#include "ShaderProgram.hpp"

class Mesh {
public:
    // mesh data
    glm::vec3 origin{};
    glm::vec3 orientation{};
    glm::vec3 size{};
    
    GLuint texture_id{0}; // texture id=0  means no texture
    GLenum primitive_type = GL_POINT;
    ShaderProgram &shader;
    
    // mesh material
    glm::vec4 ambient_material{1.0f}; //white, non-transparent 
    glm::vec4 diffuse_material{1.0f}; //white, non-transparent 
    glm::vec4 specular_material{1.0f}; //white, non-transparent
    float reflectivity{1.0f}; 

    // indirect (indexed) draw 
    Mesh(GLenum primitive_type, ShaderProgram& shader, std::vector<Vertex> const& vertices, std::vector<GLuint> const& indices, glm::vec3 const& origin, glm::vec3 const& orientation, glm::vec3 const& size, GLuint const texture_id = 0) :
        primitive_type(primitive_type),
        shader(shader),
        vertices(vertices),
        indices(indices),
        origin(origin),
        orientation(orientation),
        texture_id(texture_id),
        size(size)
    {
        GLuint prog_h = shader.getID();
        glCreateVertexArrays(1, &VAO);

        // Set Vertex Attribute to explain OpenGL how to interpret the data
        GLint position_attrib_location = glGetAttribLocation(prog_h, "aPos");
        if (position_attrib_location == -1)
            std::cerr << "Position of 'aPos' not found" << std::endl;
        else {
            glVertexArrayAttribFormat(VAO, position_attrib_location, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Position));
            glVertexArrayAttribBinding(VAO, position_attrib_location, 0);
            glEnableVertexArrayAttrib(VAO, position_attrib_location);
        }
        // Set end enable Vertex Attribute for Normal
        GLint normal_attrib_location = glGetAttribLocation(prog_h, "aNormal");
        if (normal_attrib_location == -1)
            std::cerr << "Position of 'aNormal' not found" << std::endl;
        else {
            glVertexArrayAttribFormat(VAO, normal_attrib_location, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Normal));
            glVertexArrayAttribBinding(VAO, normal_attrib_location, 0);
            glEnableVertexArrayAttrib(VAO, normal_attrib_location);
        }
        // Set end enable Vertex Attribute for Texture Coordinates
        GLint tex_attrib_location = glGetAttribLocation(prog_h, "aTex");
        if (tex_attrib_location == -1)
            std::cerr << "Position of 'aTex' not found" << std::endl;
        else {
            glVertexArrayAttribFormat(VAO, tex_attrib_location, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, TexCoords));
            glVertexArrayAttribBinding(VAO, tex_attrib_location, 0);
            glEnableVertexArrayAttrib(VAO, tex_attrib_location);
        }
        
        // Create and fill data
        glCreateBuffers(1, &VBO); // Vertex Buffer Object
        glCreateBuffers(1, &EBO); // Element Buffer Object
        glNamedBufferData(VBO, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        glNamedBufferData(EBO, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        //Connect together
        glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(Vertex));
        glVertexArrayElementBuffer(VAO, EBO);
    }


    
    void draw(glm::vec3 const& offset = glm::vec3(0.0f), glm::vec3 const& rotation = glm::vec3(0.0f), glm::vec3 const& scale = glm::vec3(1.0f)) {
 		if (VAO == 0) {
			std::cerr << "VAO not initialized!\n";
			return;
		}
        shader.activate();

        // compute complete transformation
        glm::mat4 t = glm::translate(glm::mat4(1.0f), origin);
        glm::mat4 rx = glm::rotate(glm::mat4(1.0f), orientation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 ry = glm::rotate(glm::mat4(1.0f), orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rz = glm::rotate(glm::mat4(1.0f), orientation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 s = glm::scale(glm::mat4(1.0f), size);

        glm::mat4 m_off = glm::translate(glm::mat4(1.0f), offset);
        glm::mat4 m_rx = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 m_ry = glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 m_rz = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 m_s = glm::scale(glm::mat4(1.0f), scale);

        glm::mat4 model_matrix =   s * rz * ry * rx * t * m_s * m_rz * m_ry * m_rx * m_off;

        shader.setUniform("uM_m", model_matrix);

        glBindTextureUnit(0, texture_id);
        shader.setUniform("tex0", 0);

        glBindVertexArray(VAO);
        glDrawElements(primitive_type, indices.size(), GL_UNSIGNED_INT, 0);
        
    }


	void clear(void) {
        texture_id = 0;
        primitive_type = GL_POINT;
        // TODO: clear rest of the member variables to safe default
        

        shader.clear();
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteVertexArrays(1, &VAO);

        
    };

private:

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    unsigned int VAO{0}, VBO{0}, EBO{0};
};
  


