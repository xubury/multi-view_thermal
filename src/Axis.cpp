#include "Axis.hpp"
#include "glbinding/gl/gl.h"

using namespace gl;

Axis::Axis(const glm::mat4 &model) : RenderTarget(model), m_VAO(0) {
    // draw X, Y, Z lines with Red, Green, Blue colors
    // note that the first three columns are the point position
    // the last three columnn are the colors
    float vertices[] = {
            0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
            1.f, 0.f, 0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 0.f, 0.f, 1.f, 0.f,
            0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 0.f, 0.f, 1.f,
            0.f, 0.f, 1.f, 0.f, 0.f, 1.f, };
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    unsigned int VBO = 0;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


    glEnableVertexAttribArray(0);
    // in file axis.vs, there is a statement
    // layout (location = 0) in vec3 vertex;
    // the first argument 0 means (location = 0)
    glVertexAttribPointer(0,                 // location = 0 in vertex shader file
                          3,                 // the position has X,Y,Z 3 elements
                          GL_FLOAT,          // element type
                          GL_FALSE,          // do not normalize
                          6 * sizeof(float), // Stride
                          (void *) nullptr); // an offset of into the array, it is 0
    glEnableVertexAttribArray(1);
    // in file axis.vs, there is a statement
    // layout (location = 1) in vec3 color;
    // the first argument 1 means (location = 1)
    glVertexAttribPointer(1,                 // location = 1 in vertex shader file
                          3,                 // the color has RGB 3 elements
                          GL_FLOAT,          // element type
                          GL_FALSE,          // do not normalize
                          6 * sizeof(float), // Stride
                          (void *) (3 * sizeof(float))); // an offset of into the array, it is 3 * sizeof(float)
    // color columns are behind the XYZ columns

    glBindVertexArray(0);
}

void Axis::DrawArray() {
    glBindVertexArray(m_VAO);
    // the last parameter of glDrawArrays has to be the number of vertices
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);
}

