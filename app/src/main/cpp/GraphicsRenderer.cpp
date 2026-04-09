/*
 * OpenXR Controller Demo - Graphics Renderer Implementation
 */

#include "GraphicsRenderer.h"
#include <android/log.h>
#include <cmath>
#include <cstring>

#define LOG_TAG "GraphicsRenderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Vertex shader
const char* vertexShaderSource = R"(
    #version 300 es
    layout(location = 0) in vec3 aPosition;
    uniform mat4 uMVPMatrix;
    uniform vec4 uColor;
    out vec4 vColor;
    void main() {
        gl_Position = uMVPMatrix * vec4(aPosition, 1.0);
        vColor = uColor;
    }
)";

// Fragment shader
const char* fragmentShaderSource = R"(
    #version 300 es
    precision mediump float;
    in vec4 vColor;
    out vec4 fragColor;
    void main() {
        fragColor = vColor;
    }
)";

// Simple triangle for testing (at z = -2)
const float triangleVertices[] = {
    // XYZ for each vertex
    -0.5f, -0.5f, -2.0f,  // bottom left
     0.5f, -0.5f, -2.0f,  // bottom right
     0.0f,  0.5f, -2.0f   // top center
};

GraphicsRenderer::GraphicsRenderer() = default;

GraphicsRenderer::~GraphicsRenderer() {
    Shutdown();
}

bool GraphicsRenderer::Initialize() {
    LOGI("Initializing graphics renderer...");

    if (!CreateShaders()) {
        LOGE("Failed to create shaders");
        return false;
    }

    if (!CreateGeometry()) {
        LOGE("Failed to create geometry");
        return false;
    }

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    LOGI("Graphics renderer initialized");
    return true;
}

void GraphicsRenderer::Shutdown() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_ibo != 0) {
        glDeleteBuffers(1, &m_ibo);
        m_ibo = 0;
    }
    if (m_shaderProgram != 0) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
}

GLuint GraphicsRenderer::CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = new char[infoLen];
            glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
            LOGE("Shader compilation failed: %s", infoLog);
            delete[] infoLog;
        }
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint GraphicsRenderer::CreateProgram(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = new char[infoLen];
            glGetProgramInfoLog(program, infoLen, nullptr, infoLog);
            LOGE("Program linking failed: %s", infoLog);
            delete[] infoLog;
        }
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

bool GraphicsRenderer::CreateShaders() {
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    if (vertexShader == 0) return false;

    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    m_shaderProgram = CreateProgram(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (m_shaderProgram == 0) return false;

    // Get uniform locations
    m_mvpMatrixLoc = glGetUniformLocation(m_shaderProgram, "uMVPMatrix");
    m_colorLoc = glGetUniformLocation(m_shaderProgram, "uColor");
    m_positionLoc = glGetAttribLocation(m_shaderProgram, "aPosition");

    LOGI("Shader locations: mvp=%d, color=%d, position=%d", m_mvpMatrixLoc, m_colorLoc, m_positionLoc);

    return true;
}

bool GraphicsRenderer::CreateGeometry() {
    LOGI("Creating geometry: positionLoc=%d", m_positionLoc);

    // Create VBO with triangle data
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

    // Create VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // Bind VBO to VAO
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // Set up vertex attributes (use location 0 since shader uses layout(location = 0))
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);

    m_indexCount = 3; // Triangle has 3 vertices

    LOGI("Geometry created: vao=%d, vbo=%d, vertices=%d", m_vao, m_vbo, m_indexCount);
    return true;
}

// Helper: Multiply two 4x4 matrices: out = a * b
static void MultiplyMatrix4x4(const float* a, const float* b, float* out) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            out[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++) {
                out[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
}

// Helper: Create view matrix from pose (inverse of camera transform)
static void CreateViewMatrix(const XrPosef& pose, float* viewMatrix) {
    const XrQuaternionf& q = pose.orientation;
    const XrVector3f& p = pose.position;

    // Convert quaternion to rotation matrix
    float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

    float rot[16] = {
        1 - 2*(yy + zz), 2*(xy - wz), 2*(xz + wy), 0,
        2*(xy + wz), 1 - 2*(xx + zz), 2*(yz - wx), 0,
        2*(xz - wy), 2*(yz + wx), 1 - 2*(xx + yy), 0,
        0, 0, 0, 1
    };

    // View matrix = inverse(rotation) * inverse(translation)
    // For rotation matrix, inverse = transpose
    // For translation, inverse = -position
    viewMatrix[0] = rot[0]; viewMatrix[1] = rot[4]; viewMatrix[2] = rot[8]; viewMatrix[3] = 0;
    viewMatrix[4] = rot[1]; viewMatrix[5] = rot[5]; viewMatrix[6] = rot[9]; viewMatrix[7] = 0;
    viewMatrix[8] = rot[2]; viewMatrix[9] = rot[6]; viewMatrix[10] = rot[10]; viewMatrix[11] = 0;
    viewMatrix[12] = -(viewMatrix[0]*p.x + viewMatrix[4]*p.y + viewMatrix[8]*p.z);
    viewMatrix[13] = -(viewMatrix[1]*p.x + viewMatrix[5]*p.y + viewMatrix[9]*p.z);
    viewMatrix[14] = -(viewMatrix[2]*p.x + viewMatrix[6]*p.y + viewMatrix[10]*p.z);
    viewMatrix[15] = 1;
}

// Helper: Create projection matrix from FOV (OpenGL-style)
static void CreateProjectionMatrix(const XrFovf& fov, float nearZ, float farZ, float* projMatrix) {
    float tanLeft = tanf(fov.angleLeft);
    float tanRight = tanf(fov.angleRight);
    float tanUp = tanf(fov.angleUp);
    float tanDown = tanf(fov.angleDown);

    float tanWidth = tanRight - tanLeft;
    float tanHeight = tanUp - tanDown;

    float a = 2.0f / tanWidth;
    float b = 2.0f / tanHeight;
    float c = (tanRight + tanLeft) / tanWidth;
    float d = (tanUp + tanDown) / tanHeight;
    float e = -(farZ + nearZ) / (farZ - nearZ);
    float f = -(2.0f * farZ * nearZ) / (farZ - nearZ);

    projMatrix[0] = a;  projMatrix[1] = 0;  projMatrix[2] = 0;  projMatrix[3] = 0;
    projMatrix[4] = 0;  projMatrix[5] = b;  projMatrix[6] = 0;  projMatrix[7] = 0;
    projMatrix[8] = c;  projMatrix[9] = d;  projMatrix[10] = e; projMatrix[11] = -1;
    projMatrix[12] = 0; projMatrix[13] = 0; projMatrix[14] = f; projMatrix[15] = 0;
}

void GraphicsRenderer::Render(uint32_t eyeIndex, const XrView& view, uint32_t width, uint32_t height, const std::array<ControllerState, 2>& controllers) {
    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Set viewport using actual swapchain dimensions
    glViewport(0, 0, width, height);

    // Use shader program
    glUseProgram(m_shaderProgram);

    // Build view matrix from pose
    float viewMatrix[16];
    CreateViewMatrix(view.pose, viewMatrix);

    // Build projection matrix from FOV (near=0.01, far=100 to ensure 10m cube is visible)
    float projMatrix[16];
    CreateProjectionMatrix(view.fov, 0.01f, 100.0f, projMatrix);

    static int frameLog = 0;
    if (++frameLog % 60 == 0) {
        LOGI("Rendering eye %d, shader=%d, vao=%d", eyeIndex, m_shaderProgram, m_vao);
        LOGI("View: pos=(%.2f,%.2f,%.2f)", view.pose.position.x, view.pose.position.y, view.pose.position.z);
    }

    // Disable culling to see both sides
    glDisable(GL_CULL_FACE);

    // Clear depth buffer before rendering scene
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render multiple reference cubes at different positions for debugging
    RenderDebugCubes(viewMatrix, projMatrix);

    // Render controllers
    int activeCount = 0;
    for (int i = 0; i < 2; i++) {
        if (controllers[i].active) {
            RenderController(controllers[i], viewMatrix, projMatrix);
            activeCount++;
        }
    }
    static int frameCount = 0;
    if (++frameCount % 60 == 0) {
        LOGI("Render: %d active controllers", activeCount);
    }
}

void GraphicsRenderer::RenderController(const ControllerState& controller, const float* viewMatrix, const float* projMatrix) {
    glBindVertexArray(m_vao);

    // Build model matrix from grip pose
    const XrQuaternionf& q = controller.gripPose.orientation;
    const XrVector3f& p = controller.gripPose.position;

    // Convert quaternion to rotation matrix
    float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

    float rotMatrix[16] = {
        1 - 2*(yy + zz), 2*(xy - wz), 2*(xz + wy), 0,
        2*(xy + wz), 1 - 2*(xx + zz), 2*(yz - wx), 0,
        2*(xz - wy), 2*(yz + wx), 1 - 2*(xx + yy), 0,
        0, 0, 0, 1
    };

    // Scale controller based on grip value
    float scale = 0.05f + 0.05f * controller.gripValue;

    // Translation with scale applied to rotation
    float modelMatrix[16] = {
        rotMatrix[0] * scale, rotMatrix[1] * scale, rotMatrix[2] * scale, 0,
        rotMatrix[4] * scale, rotMatrix[5] * scale, rotMatrix[6] * scale, 0,
        rotMatrix[8] * scale, rotMatrix[9] * scale, rotMatrix[10] * scale, 0,
        p.x, p.y, p.z, 1
    };

    // Combine matrices: MVP = P * V * M
    float viewModel[16];
    float mvpMatrix[16];
    MultiplyMatrix4x4(viewMatrix, modelMatrix, viewModel);  // VM = V * M
    MultiplyMatrix4x4(projMatrix, viewModel, mvpMatrix);     // MVP = P * VM

    // Set color based on trigger value
    float trigger = controller.triggerValue;
    float color[4] = {
        0.2f + 0.8f * trigger,  // R: increases with trigger
        0.5f + 0.5f * controller.gripValue,  // G: increases with grip
        1.0f - 0.5f * trigger,  // B: decreases with trigger
        1.0f
    };

    // Set uniforms
    glUniformMatrix4fv(m_mvpMatrixLoc, 1, GL_FALSE, mvpMatrix);
    glUniform4fv(m_colorLoc, 1, color);

    // Draw triangle using glDrawArrays
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
}

void GraphicsRenderer::RenderDebugCube(const float* viewMatrix, const float* projMatrix,
                                        float x, float y, float z, float scale, const float* color) {
    glBindVertexArray(m_vao);

    float modelMatrix[16] = {
        scale, 0, 0, 0,
        0, scale, 0, 0,
        0, 0, scale, 0,
        x, y, z, 1
    };

    // Combine matrices: MVP = P * V * M
    float viewModel[16];
    float mvpMatrix[16];
    MultiplyMatrix4x4(viewMatrix, modelMatrix, viewModel);
    MultiplyMatrix4x4(projMatrix, viewModel, mvpMatrix);

    // Set uniforms
    glUniformMatrix4fv(m_mvpMatrixLoc, 1, GL_FALSE, mvpMatrix);
    glUniform4fv(m_colorLoc, 1, color);

    // Draw triangle using glDrawArrays (no index buffer needed)
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
}

void GraphicsRenderer::RenderDebugCubes(const float* viewMatrix, const float* projMatrix) {
    // Just render one big red triangle at center to test basic rendering
    float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};

    // Render a large triangle at z = -5, with scale 1.0
    RenderDebugCube(viewMatrix, projMatrix, 0, 0, -5, 1.0f, red);

    static int debugLogCount = 0;
    if (++debugLogCount % 60 == 0) {
        LOGI("Rendered debug triangle (frame %d)", debugLogCount);
    }
}

void GraphicsRenderer::RenderText(float x, float y, const char* text) {
    // Text rendering is complex, simplified for this demo
    // In a full implementation, you'd use a font atlas
}
