#define _POSIX_C_SOURCE 200809L

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
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
    double previosTime;
    int frameCounter;

    int record_w = 0;
    int record_h = 0;
    int data_size = 0;
    FILE *ffmpeg = NULL;
    GLuint pbo[2] = {0, 0};
    double fps = 0.0;
    int pbo_index = 0;
    int pbo_next_index = 1;
    const double target_fps = TARGET_FPS;
    const double target_frame_time = 1.0 / target_fps;

    printf("Open %s...\n", pathname);
    fd = open(pathname, O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("Error al abrir el archivo proc");
        return 1;
    }

    if (!glfwInit()) return -1;

    // Obtenemos el monitor principal y su resolución para arrancar maximizados
    GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = NULL;

    if (primary_monitor) {
        mode = glfwGetVideoMode(primary_monitor);
    }

    printf("Screen resolution: %dx%d.\n", mode->width, mode->height);
    int start_width = mode ? mode->width : 800;
    int start_height = mode ? mode->height : 800;
    GLFWwindow* window = glfwCreateWindow(start_width, start_height, "RAM memory visualization", primary_monitor, NULL);

    if (!window) {
        fprintf(stderr, "Error al crear la ventana GLFW.\n");
        glfwTerminate();
        close(fd);
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
    printf("Using GLEW %s\n", glewGetString(GLEW_VERSION));

    glfwSwapInterval(V_SYNC);

    // Mapeamos el buffer del kernel a nuestro espacio de usuario
    map_ptr = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_ptr == MAP_FAILED) {
        perror("Error in mmap");
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

    unsigned long total_pages = *(unsigned long*)(&map_ptr[INDEX_TOTAL_PAGES]);
    //  (((total_pages*4096)/1024)/1024)/1024 = total_pages/262144
    double total_ram_gb = ((double)total_pages)/262144;
    printf("Total pages: %lu \nTotal RAM:  %.2f GB\n", total_pages, total_ram_gb);

    // Background color
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    if (CAPT_VIDEO){
        record_w = mode->width & ~1;
        record_h = mode->height & ~1;
        data_size = record_w * record_h * 3;

        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);

        char filename[128];
        strftime(filename, sizeof(filename), "RAM_map_%Y-%m-%d_%H-%M-%S.mp4", tm_info);
        printf("Starting recording in: %s\n", filename);

        glGenBuffers(2, pbo);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[0]);
        glBufferData(GL_PIXEL_PACK_BUFFER, data_size, 0, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[1]);
        glBufferData(GL_PIXEL_PACK_BUFFER, data_size, 0, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        char ffmpeg_cmd[512];
        snprintf(ffmpeg_cmd, sizeof(ffmpeg_cmd),
                 "ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgb24 "
                 "-s %dx%d -r %d -i - -vf vflip -c:v libx264 -preset ultrafast -crf 23 -pix_fmt yuv420p %s",
                 record_w, record_h, TARGET_FPS, filename);

        ffmpeg = popen(ffmpeg_cmd, "w");
        if (!ffmpeg) {
            perror("Error al abrir pipe a FFmpeg");
        }
    }

    previosTime = glfwGetTime();
    frameCounter = 0;
    while (!glfwWindowShouldClose(window)) {
        double frame_start_time = glfwGetTime();

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

            show_hud(map_ptr,fps);

            // Restaurar estado
            glDisable(GL_BLEND);
            glEnable(GL_TEXTURE_2D);
            glUseProgram(shaderProgram);

            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
        }


        if (CAPT_VIDEO){
            if (ffmpeg) {
                glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[pbo_index]);
                glPixelStorei(GL_PACK_ALIGNMENT, 1);
                glReadPixels(0, 0, record_w, record_h, GL_RGB, GL_UNSIGNED_BYTE, 0);

                glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[pbo_next_index]);
                GLubyte* src = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

                if (src) {
                    fwrite(src, 1, data_size, ffmpeg);
                    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                }
                glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

                pbo_index = (pbo_index + 1) % 2;
                pbo_next_index = (pbo_next_index + 1) % 2;
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        // Calculo FPS
        double currentTime = glfwGetTime();
        frameCounter ++;
        if (currentTime-previosTime >= 1.0){
            fps = frameCounter / (currentTime-previosTime);
            previosTime = currentTime;
            frameCounter = 0;
        }

        if (CAPT_VIDEO){
            double frame_end_time = glfwGetTime();
            double frame_duration = frame_end_time - frame_start_time;

            double wait_time = target_frame_time - frame_duration;
            if (wait_time > 0.0) {
                struct timespec req;
                req.tv_sec = (time_t)wait_time;
                req.tv_nsec = (long)((wait_time - req.tv_sec) * 1000000000.0);

                nanosleep(&req, NULL);
            }
        }
    }

    if(CAPT_VIDEO){
        if (ffmpeg) {
            pclose(ffmpeg);
        }
        glDeleteBuffers(2, pbo);
    }

    munmap(map_ptr, BUFFER_SIZE);
    glDeleteTextures(1, &textureID);
    glfwDestroyWindow(window);
    glfwTerminate();
    close(fd);

    printf("Close\n");
    return EXIT_SUCCESS;
}
