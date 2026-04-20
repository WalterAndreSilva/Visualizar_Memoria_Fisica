#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "share.h"
#include "user_app/text.h"
#include "user_app/shader.h"
#include "user_app/callback.h"

static const char *pathname = "/proc/ku_mmap";
static uint8_t *map_ptr;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;
    mouse_button_callback_fun(window, button, action);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    cursor_position_callback_fun(window, xpos, ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    (void)window;
    (void)xoffset;
    scroll_callback_fun(yoffset);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;
    key_callback_fun(window, key, action, map_ptr);
}

int main(void)
{
    int fd;
    double tiempoAnterio;
    int conatadorFrames;

    printf("Open %s...\n", pathname);
    fd = open(pathname, O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("Error al abrir el archivo proc");
        return 1;
    }

    if (!glfwInit()) return -1;

    // Obtenemos el monitor principal y su resolución para arrancar maximizados
    GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);

    // Le pasamos el monitor para que tome control exclusivo de la pantalla
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "RAM map", primary_monitor, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Registrar callbacks del ratón
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);

    // Iniciamos GLEW
    GLenum err = glewInit();
    if(GLEW_OK != err){
        fprintf(stderr,"Error inicializando GLEW: %s\n", glewGetErrorString(err));
        return -1;
    }
    printf("Usando GLEW %s\n", glewGetString(GLEW_VERSION));

    glfwSwapInterval(V_SYNC);

    // Mapeamos el buffer del kernel a nuestro espacio de usuario
    map_ptr = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_ptr == MAP_FAILED) {
        perror("Error en mmap");
        close(fd);
        return -1;
    }
    float vertices[] = {-1.0f,-1.0f,1.0f,-1.0f,1.0f,1.0f,-1.0f,1.0f};
    float texCoords[] = {0.0f,1.0f,1.0f,1.0f,1.0f,0.0f,0.0f,0.0f};

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    // NEAREST para ver los píxeles duros
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

    // Creamos la textura vacía en la GPU, unsigned int (8bits)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, WIDTH, HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);

    // Usamos Shader en OpenGL
    GLuint shaderProgram = compile_shader();
    glUseProgram(shaderProgram);
    GLint textureLocation = glGetUniformLocation(shaderProgram, "myTexture");
    glUniform1i(textureLocation, 0);

    tiempoAnterio = glfwGetTime();
    conatadorFrames = 0;
    while (!glfwWindowShouldClose(window)) {

        press_hold_keys(window);

        // Obtener el tamaño actual de la ventana
        int current_w, current_h;
        glfwGetFramebufferSize(window, &current_w, &current_h);
        if (current_h == 0) current_h = 1;
        glViewport(0, 0, current_w, current_h);
        float aspect = (float)current_w / (float)current_h;

        // Aplicar corrección
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        if (current_w >= current_h) {
            glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
        } else {
            glOrtho(-1.0, 1.0, -1.0 / aspect, 1.0 / aspect, -1.0, 1.0);
        }

        glClear(GL_COLOR_BUFFER_BIT);

        // Aplicar transformaciones de cámara
        apply_transformation();


        // Actualizar textura y dibujar
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_BYTE, map_ptr);

        glDrawArrays(GL_QUADS, 0, 4);

        if (get_show_info()) {
            glUseProgram(0);
            glDisable(GL_TEXTURE_2D);

            show_hud(map_ptr);

            // Restaurar estado
            glDisable(GL_BLEND);
            glEnable(GL_TEXTURE_2D);
            glUseProgram(shaderProgram);

            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        // Calculo FPS
        double tiempoActual = glfwGetTime();
        conatadorFrames ++;
        if (tiempoActual-tiempoAnterio >= 1.0){
            double fps = conatadorFrames / (tiempoActual-tiempoAnterio);
            uint8_t akps = map_ptr[INDEX_AKPS]; // actualizaciones del kernel por segundo
            char titulo[256];
            snprintf(titulo,sizeof(titulo), "RAM map - FPS: %.1f, AKPS: %hhu", fps, akps);
            glfwSetWindowTitle(window, titulo);
            tiempoAnterio = tiempoActual;
            conatadorFrames = 0;
        }
    }

    munmap(map_ptr, BUFFER_SIZE);
    glDeleteTextures(1, &textureID);
    glfwDestroyWindow(window);
    glfwTerminate();
    close(fd);

    printf("Close\n");
    return EXIT_SUCCESS;
}
