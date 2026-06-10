#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "../share.h"
#include "shader.h"

const char* vertex_shader_body =
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 1) in vec2 aTexCoord;\n"
"\n"
"out vec2 TexCoord;\n"
"\n"
"void main() {\n"
"    // Multiplicamos el vértice por las matrices nativas de OpenGL\n"
"    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4(aPos, 0.0, 1.0);\n"
"    TexCoord = aTexCoord;\n"
"}\n";

const char* fragment_shader_body =
"in vec2 TexCoord;\n"
"out vec4 FragColor;\n"
"\n"
"uniform usampler2D myTexture;\n"
"uniform vec4 palette[PALETTE_SIZE];\n"
"\n"
"void main() {\n"
"    uvec4 texColor = texture(myTexture, TexCoord);\n"
"    uint val = texColor.r;\n"
"    \n"
"    if (val < uint(PALETTE_SIZE)) {\n"
"        FragColor = palette[val];\n"
"    } else {\n"
"        FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
"    }\n"
"}\n";

GLuint compile_shader()
{
    GLint success;
    char infoLog[512];

    char define_string[64];
    int palette_size = sizeof(palette) / sizeof(palette[0]);
    snprintf(define_string, sizeof(define_string), "#define PALETTE_SIZE %d\n", palette_size);

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    if (vertex_shader == 0) vertex_shader = glCreateShader(0x8B31);

    const char* vertex_strings[2] = {
        "#version 330 compatibility\n",
        vertex_shader_body
    };
    glShaderSource(vertex_shader, 2, vertex_strings, NULL);
    glCompileShader(vertex_shader);

    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
        printf("Error compilando Vertex Shader: %s\n", infoLog);
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragment_strings[3] = {
        "#version 330 compatibility\n",
        define_string,
        fragment_shader_body
    };
    glShaderSource(fragment_shader, 3, fragment_strings, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
        printf("Error compilando Fragment Shader: %s\n", infoLog);
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("Error en el Linkeo del Programa: %s\n", infoLog);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glUseProgram(program);

    GLint palette_location = glGetUniformLocation(program, "palette");
    if (palette_location != -1) {
        glUniform4fv(palette_location, palette_size, &palette[0][0]);
    } else {
        printf("Advertencia: No se encontró la variable 'palette' en el shader.\n");
    }

    glUseProgram(0);

    return program;
}
