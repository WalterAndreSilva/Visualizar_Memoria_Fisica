#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>


#define PAGE_SIZE 4096

// --- AJUSTES PARA 16 GB DE RAM ---
// 16 GB = 4,194,304 páginas.
// Una cuadrícula de 2048x2048 nos da exactamente 4,194,304 píxeles.
#define WIDTH 2048
#define HEIGHT 2048

// El tamaño de mmap ahora debe cubrir la textura entera (4.19 MB)
// OJO: Tu módulo del kernel debe tener EXP_RESERVE en 11 (8 MB) para soportar esto.
#define MMAP_SIZE (WIDTH * HEIGHT) 

static const char *pathname = "/proc/lkmc_mmap";

const char* fragment_shader_source =
"uniform sampler2D myTexture;\n"
"void main() {\n"
"    // Leer el color actual de la textura\n"
"    vec4 texColor = texture2D(myTexture, gl_TexCoord[0].st);\n"
"    \n"
"    // Si el valor es 255 (blanco, normalizado = 1.0), pintar de rojo\n"
"    if (texColor.r >= 0.99) {\n"
"        gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
"    } else {\n"
"        // Si no, usar el valor como gris (R=G=B)\n"
"        gl_FragColor = vec4(texColor.r, texColor.r, texColor.r, 1.0);\n"
"    }\n"
"}\n";


GLuint compile_shader(const char* source) {
    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Opcional: Revisar errores de compilación
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

    glDeleteShader(shader); // Ya está enlazado, podemos liberar la memoria del shader suelto
    return program;
}

int main(int argc, char **argv)
{
    int fd;

    printf("Open %s...\n", pathname);
    fd = open(pathname, O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("Error al abrir el archivo proc");
        return 1;
    }

    if (!glfwInit()) return -1;

    // Creamos la ventana (puedes redimensionarla, OpenGL escalará la textura de 2048x2048)
    GLFWwindow* window = glfwCreateWindow(1200, 1200, "Mapa de RAM (16 GB)", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Iniciamos GLEW
    GLenum err = glewInit();
    if(GLEW_OK != err){
        fprintf(stderr,"Error inicializando GLEW: %s\n", glewGetErrorString(err));
        return -1;
    }
    printf("Usando GLEW %s\n", glewGetString(GLEW_VERSION));

    // Mapeamos el buffer del kernel a nuestro espacio de usuario
    unsigned char *map_ptr = mmap(NULL, MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_ptr == MAP_FAILED) {
        perror("Error en mmap. Revisa EXP_RESERVE en el kernel.");
        close(fd);
        return -1;
    }

    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    float texCoords[] = {
        0.0f, 1.0f, // Invertimos Y para que el origen esté arriba
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f
    };

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // NEAREST para ver los píxeles duros
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

    // Creamos la textura vacía en la GPU, formato de un solo canal (GL_LUMINANCE)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, WIDTH, HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

    // --------------------------------------------------------------
    GLuint shaderProgram = compile_shader(fragment_shader_source);

    // Decirle a OpenGL que use este programa para todos los dibujos siguientes
    glUseProgram(shaderProgram);

    // Conectar la textura al shader (0 corresponde al primer sampler de textura activado)
    GLint textureLocation = glGetUniformLocation(shaderProgram, "myTexture");
    glUniform1i(textureLocation, 0);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        // AQUÍ ESTÁ LA MAGIA DEL RENDIMIENTO:
        // En lugar de iterar con un for(), usamos glTexSubImage2D.
        // Esto le dice al driver de video que chupe los datos directamente desde 'map_ptr' 
        // hacia la memoria de la GPU, interpretando 1 byte = 1 nivel de gris.
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_LUMINANCE, GL_UNSIGNED_BYTE, map_ptr);

        // Dibujar
        glDrawArrays(GL_QUADS, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    munmap(map_ptr, MMAP_SIZE);
    glDeleteTextures(1, &textureID);
    glfwDestroyWindow(window);
    glfwTerminate();
    close(fd);

    printf("Close\n");
    return EXIT_SUCCESS;
}
