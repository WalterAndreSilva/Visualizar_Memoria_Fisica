#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "../share.h"

#include "shader.h"

const char* shader_body =
"uniform usampler2D myTexture;\n"
"uniform vec4 palette[PALETTE_SIZE];\n"
"\n"
"void main() {\n"
"    uvec4 texColor = texture(myTexture, gl_TexCoord[0].st);\n"
"    uint val = texColor.r;\n"
"    \n"
"    // Convertimos la macro a uint para compararla\n"
"    if (val < uint(PALETTE_SIZE)) {\n"
"        gl_FragColor = palette[val];\n"
"    } else {\n"
"        gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
"    }\n"
"}\n";

GLuint compile_shader()
{
    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

    char define_string[64];
    int palette_size = sizeof(palette) / sizeof(palette[0]);
    snprintf(define_string, sizeof(define_string), "#define PALETTE_SIZE %d\n", palette_size);

    const char* shader_strings[3] = {
        "#version 130\n",
        define_string,
        shader_body
    };

    glShaderSource(shader, 3, shader_strings, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("Error compilando shader: %s\n", infoLog);
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);

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
