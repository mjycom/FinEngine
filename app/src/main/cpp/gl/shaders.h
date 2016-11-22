//
// Created by iFinVer on 2016/11/22.
//

#ifndef MYOPENGLES_GL_SHADERS_H
#define MYOPENGLES_GL_SHADERS_H

#include <GLES2/gl2.h>

class ShaderBase {
public:
    const GLfloat vertices[16] =
            {
                    -1.0f, 1.0f,   // Position 0
                    0.0f, 0.0f,   // TexCoord 0
                    -1.0f, -1.0f,  // Position 1
                    0.0f, 1.0f,   // TexCoord 1
                    1.0f, -1.0f,  // Position 2
                    1.0f, 1.0f,   // TexCoord 2
                    1.0f, 1.0f,   // Position 3
                    1.0f, 0.0f    // TexCoord 3
            };

    const GLshort indices[6] = {0, 1, 2, 0, 2, 3};

    const int indicesNum = 6;

    const GLint componentVertexPos = 2;

    const GLint componentTexCoord = 2;

    const GLint stride = 4;

    ShaderBase() {}
};

class ShaderYuv : ShaderBase {
public:
    const char *vertexShader;
    const char *fragmentShader;

    ShaderYuv() {
        vertexShader =
                "attribute vec4 aPosition;   \n"
                        "attribute vec2 aTexCoord;   \n"
                        "varying vec2 vTexCoord;     \n"
                        "void main(){                \n"
                        "   gl_Position = aPosition; \n"
                        "   vTexCoord = aTexCoord;   \n"
                        "}                           \n";

        fragmentShader =
                "precision highp float;                         \n"
                        "varying vec2 vTexCoord;                        \n"
                        "uniform sampler2D yTexture;                    \n"
                        "uniform sampler2D uvTexture;                   \n"
                        "void main(){                                   \n"
                        "   float r,g,b,y,u,v;                          \n"
                        "   y = texture2D(yTexture,vTexCoord).r;        \n"
                        "   u = texture2D(uvTexture,vTexCoord).a - 0.5; \n"
                        "   v = texture2D(uvTexture,vTexCoord).r - 0.5; \n"
                        "   r = y + 1.13983*v;                          \n"
                        "   g = y - 0.39465*u - 0.58060*v;              \n"
                        "   b = y + 2.03211*u;                          \n"
                        "   gl_FragColor = vec4(r, g, b, 1.0);          \n"
                        "}                                              \n";

    }

    ~ShaderYuv() {
        vertexShader = nullptr;
        fragmentShader = nullptr;
    }
};

#endif //MYOPENGLES_GL_SHADERS_H
