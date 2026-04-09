/*
 * OpenXR Controller Demo - Graphics Renderer
 * OpenGL ES rendering
 */

#pragma once

#include <GLES3/gl3.h>
#include <openxr/openxr.h>
#include "OpenXRControllerApp.h"
#include <array>
#include <vector>

class GraphicsRenderer {
public:
    GraphicsRenderer();
    ~GraphicsRenderer();

    // Initialize OpenGL resources
    bool Initialize();

    // Render a frame for one eye
    void Render(uint32_t eyeIndex, const XrView& view, uint32_t width, uint32_t height, const std::array<ControllerState, 2>& controllers);

    // Clean up
    void Shutdown();

private:
    bool CreateShaders();
    bool CreateGeometry();
    void RenderController(const ControllerState& controller, const float* viewMatrix, const float* projMatrix);
    void RenderDebugCube(const float* viewMatrix, const float* projMatrix, float x, float y, float z, float scale, const float* color);
    void RenderDebugCubes(const float* viewMatrix, const float* projMatrix);
    void RenderText(float x, float y, const char* text);

    // Shader utilities
    GLuint CompileShader(GLenum type, const char* source);
    GLuint CreateProgram(GLuint vertexShader, GLuint fragmentShader);

private:
    // Shader program
    GLuint m_shaderProgram = 0;
    GLint m_mvpMatrixLoc = -1;
    GLint m_colorLoc = -1;
    GLint m_positionLoc = -1;

    // Controller geometry
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ibo = 0;
    GLsizei m_indexCount = 0;

    // Matrices
    std::vector<float> m_projectionMatrix;
    std::vector<float> m_viewMatrix;
};
