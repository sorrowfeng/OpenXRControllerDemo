/*
 * Copyright 2024 - 2024 PICO. All rights reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of PICO.
 * The intellectual and technical concepts contained herein are proprietary to PICO.
 * and may be covered by patents, patents in process, and are protected by trade
 * secret or copyright law. Dissemination of this information or reproduction of
 * this material is strictly forbidden unless prior written permission is obtained
 * from PICO.
 */

#ifndef PICONATIVEOPENXRSAMPLES_GRAPHICSCONSTANTS_H
#define PICONATIVEOPENXRSAMPLES_GRAPHICSCONSTANTS_H

#include <openxr/openxr.h>

namespace GraphicsConstants {
    struct Vertex {
        XrVector3f position;
        XrColor4f color;
    };

    // The version statement has come on first line.
    static const char* kSimpleVertexShaderGlsl = R"_(#version 320 es

    in vec3 VertexPos;
    in vec4 VertexColor;

    out vec4 PSVertexColor;

    uniform mat4 ModelViewProjection;

    void main() {
       gl_Position = ModelViewProjection * vec4(VertexPos, 1.0);
       PSVertexColor = VertexColor;
    }
    )_";

    // The version statement has come on first line.
    static const char* kSimpleFragmentShaderGlsl = R"_(#version 320 es

    in lowp vec4 PSVertexColor;
    out lowp vec4 FragColor;

    void main() {
       FragColor = PSVertexColor;
    }
    )_";

    static const char* KGuiVertexShaderGlsl = R"_(#version 320 es

    precision highp float;

    in vec3 VertexPos;
    in vec4 VertexColor;

    out vec2 TexCoords;

    uniform mat4 ModelViewProjection;

    void main() {
       gl_Position = ModelViewProjection * vec4(VertexPos, 1.0);
       TexCoords = vec2(VertexColor.x, VertexColor.y);
    }
    )_";

    static const char* kGuiFragmentShaderGlsl = R"_(#version 320 es

    precision highp float;

    in vec2 TexCoords;
    out vec4 FragColor;

    uniform sampler2D u_texture;

    void main() {
        FragColor = texture(u_texture, TexCoords);
    }
    )_";

    static const char* kSkyBoxVertexShaderGlsl = R"_(#version 320 es

    precision highp float;

    in vec3 VertexPos;

    out vec3 TexCoords;

    uniform mat4 ModelViewProjection;

    void main() {
//       gl_Position = ModelViewProjection * vec4(VertexPos, 1.0);
       vec4 pos = ModelViewProjection * vec4(VertexPos, 1.0);
       TexCoords = VertexPos;
       gl_Position = pos.xyww;
    }
    )_";

    static const char* kSkyBoxFragmentShaderGlsl = R"_(#version 320 es

    precision highp float;

    in vec3 TexCoords;
    out vec4 FragColor;

    uniform samplerCube u_texture;

    void main() {
        FragColor = texture(u_texture, TexCoords);
    }
    )_";

    static const char* kSimpleWireframeVertexShaderGlsl = R"_(#version 320 es

    precision mediump float;
    layout(location = 0) in vec3 VertexPos;
    layout(location = 1) in vec4 VertexColor;

    out vec3 PSVertexColor;
    uniform vec3 wireframeColor;
    uniform mat4 ModelViewProjection;

    void main() {
       gl_Position = ModelViewProjection * vec4(VertexPos, 1.0);
       PSVertexColor = wireframeColor;
    }
    )_";

    static const char* kSimpleWireframeGeometryShaderGlsl = R"_(#version 320 es

    precision mediump float;
    layout(triangles) in;
    layout(line_strip, max_vertices = 4) out;

    in vec3 PSVertexColor[];
    out vec3 fragColor;

    void main() {
        // generate triangles
        for (int i = 0; i < 3; ++i) {
            gl_Position = gl_in[i].gl_Position;
            fragColor = PSVertexColor[i];
            EmitVertex();
        }

        gl_Position = gl_in[0].gl_Position;
        EmitVertex();
        EndPrimitive();
    }
    )_";

    // The version statement has come on first line.
    static const char* kSimpleWireframeFragmentShaderGlsl = R"_(#version 320 es

    precision mediump float;
    in vec3 fragColor;
    out vec4 FragColor;

    void main() {
       FragColor = vec4(fragColor, 1);
    }
    )_";
}  // namespace GraphicsConstants

#endif  //PICONATIVEOPENXRSAMPLES_GRAPHICSCONSTANTS_H
