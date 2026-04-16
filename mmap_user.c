#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "share.h"

static const char *pathname = "/proc/ku_mmap";
static unsigned char *map_ptr;

float zoom = 1.0f;
float offsetX = 0.0f;
float offsetY = 0.0f;

int is_fullscreen = 1;
int win_x = 100, win_y = 100;
int win_w = 800, win_h = 800;

int isDragging = 0;
double lastMouseX = 0.0;
double lastMouseY = 0.0;

const char* fragment_shader_source =
"#version 130\n"
"uniform usampler2D myTexture;\n"
"\n"
"const vec4 palette[11] = vec4[11](\n"
"    vec4(0.0, 0.0, 0.0, 0.0), // Valor 0: negro\n"
"    vec4(1.0, 0.0, 0.0, 1.0), // Valor 1: rojo\n"
"    vec4(0.0, 0.0, 1.0, 1.0), // Valor 2: azul\n"
"    vec4(0.6, 1.0, 0.6, 1.0), // Valor 3: verde claro\n"
"    vec4(0.0, 0.3, 0.0, 1.0), // Valor 4: verde oscuro\n"
"    vec4(1.0, 0.8, 0.0, 1.0), // Valor 5: amarillo\n"
"    vec4(1.0, 0.0, 1.0, 1.0), // Valor 6: magenta\n"
"    vec4(0.0, 0.8, 0.8, 1.0), // Valor 7: cian\n"
"    vec4(1.0, 0.4, 0.0, 1.0), // Valor 8: naranja\n"
"    vec4(0.5, 0.0, 0.5, 1.0), // Valor 9: morado\n"
"    vec4(0.6, 0.3, 0.1, 1.0)  // Valor 10: marron\n"
");\n"
"\n"
"void main() {\n"
"    uvec4 texColor = texture(myTexture, gl_TexCoord[0].st);\n"
"    uint val = texColor.r;\n"
"    \n"
"    if (val < 11u) {\n"
"        // Si el valor es correcto, pintamos de color\n"
"        gl_FragColor = palette[val];\n"
"    } else {\n"
"        // Si el valor no se reconoce, pintamos de blanco\n"
"        gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
"    }\n"
"}\n";

GLuint compile_shader(const char* source)
{
    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shader, 1, &source, NULL);
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
    return program;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isDragging = 1;
            // Guardamos la posición inicial del clic
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        } else if (action == GLFW_RELEASE) {
            isDragging = 0;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (isDragging) {
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        if (h == 0) h = 1;

        double deltaX = xpos - lastMouseX;
        double deltaY = ypos - lastMouseY;
        float moveX, moveY;

        if (w >= h) {
            moveX = (float)(deltaX * 2.0 / h) / zoom;
            moveY = (float)(deltaY * 2.0 / h) / zoom;
        } else {
            moveX = (float)(deltaX * 2.0 / w) / zoom;
            moveY = (float)(deltaY * 2.0 / w) / zoom;
        }

        offsetX += moveX;
        offsetY -= moveY;

        lastMouseX = xpos;
        lastMouseY = ypos;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    (void)window;
    (void)xoffset;
    if (yoffset > 0) zoom *= 1.1f;
    else if (yoffset < 0) zoom /= 1.1f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, 1);
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        if (!is_fullscreen) {
            // Guardar la posición y tamaño actual de la ventana
            glfwGetWindowPos(window, &win_x, &win_y);
            glfwGetWindowSize(window, &win_w, &win_h);
            // Obtener el monitor principal y su resolución actual
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            // Pasar a pantalla completa
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            is_fullscreen = 1;
        } else {
            // Restaurar al modo ventana utilizando las dimensiones guardadas
            glfwSetWindowMonitor(window, NULL, win_x, win_y, win_w, win_h, 0);
            is_fullscreen = 0;
        }
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS){
        map_ptr[INDEX_MODE] = (map_ptr[INDEX_MODE]+1) % 2;
        glfwSetWindowTitle(window, "Cambio de vista");
    }
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
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Mapa de RAM (16 GB)", primary_monitor, NULL);
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

    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    float texCoords[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f
    };

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
    GLuint shaderProgram = compile_shader(fragment_shader_source);
    glUseProgram(shaderProgram);
    GLint textureLocation = glGetUniformLocation(shaderProgram, "myTexture");
    glUniform1i(textureLocation, 0);

    tiempoAnterio = glfwGetTime();
    conatadorFrames = 0;
    while (!glfwWindowShouldClose(window)) {
        float panSpeed = 0.02f / zoom;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) offsetY -= panSpeed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) offsetY += panSpeed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) offsetX += panSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) offsetX -= panSpeed;

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
        glScalef(zoom, zoom, 1.0f);
        glTranslatef(offsetX, offsetY, 0.0f);

        // Actualizar textura y dibujar
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_BYTE, map_ptr);

        glDrawArrays(GL_QUADS, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // Calculo FPS
        double tiempoActual = glfwGetTime();
        conatadorFrames ++;
        if (tiempoActual-tiempoAnterio >= 1.0){
            double fps = conatadorFrames / (tiempoActual-tiempoAnterio);
            uint8_t akps = map_ptr[INDEX_AKPS]; // actualizaciones del kernel por segundo
            char titulo[256];
            char *mode;
            if (map_ptr[INDEX_MODE]==1) mode ="Uso";
            else mode = "Zonas";
            snprintf(titulo,sizeof(titulo), "Mapa de RAM - Vista: %s, FPS: %.1f, AKPS: %hhu",mode, fps, akps);
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
