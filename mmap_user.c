#define _POSIX_C_SOURCE 200809L

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "share.h"
#include "user_app/text.h"
#include "user_app/shader.h"
#include "user_app/callback.h"
#include "user_app/record.h"

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
    int start_width;
    int start_height;
    GLFWmonitor* target_monitor;
    GLuint VAO, VBO;
    struct timespec req;
    double fps = 0.0;
    int frameCounter = 0;

    #if CAPT_VIDEO
    double frame_start_time;
    const double target_fps = TARGET_FPS;
    const double target_frame_time = 1.0 / target_fps;
    #endif

    if (!glfwInit()){
        perror("Error initializing GLFW.\n");
        return -1;
    }

    #if FORCE_WIN_TEXTURE
        start_width = FORCE_WIDTH;
        start_height = FORCE_HEIGHT;
        target_monitor = NULL;
        printf("Forcing window size to texture: %dx%d.\n", start_width, start_height);
    #else
        GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = NULL;
        if (primary_monitor) {
            mode = glfwGetVideoMode(primary_monitor);
        }
        start_width = mode ? mode->width : 800;
        start_height = mode ? mode->height : 800;
        target_monitor = primary_monitor;
        printf("Screen resolution: %dx%d.\n", start_width, start_height);
    #endif

    GLFWwindow* window = glfwCreateWindow(start_width, start_height, "RAM memory visualization", target_monitor, NULL);

    if (!window) {
        perror("Error creating GLFW window.\n");
        glfwTerminate();
        return -1;
    }

    #if FORCE_WIN_TEXTURE
        // Evita redimencionar. Mantiene relacion pixel 1:1
        glfwSetWindowSizeLimits(window, FORCE_WIDTH, FORCE_HEIGHT, FORCE_WIDTH, FORCE_HEIGHT);
    #endif

    glfwMakeContextCurrent(window);

    // Registrar callbacks
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);

    // Iniciamos GLEW
    GLenum err = glewInit();
    if(GLEW_OK != err){
        fprintf(stderr,"Error initializing GLEW: %s\n", glewGetErrorString(err));
        glfwTerminate();
        return -1;
    }
    printf("Using GLEW %s\n", glewGetString(GLEW_VERSION));
    glfwSwapInterval(V_SYNC);

    // Mapeamos el buffer del kernel a nuestro espacio de usuario
    printf("Open %s...\n", pathname);
    fd = open(pathname, O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("Error opening file");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    map_ptr = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_ptr == MAP_FAILED) {
        perror("Error in mmap");
        close(fd);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    //  (((total_pages*4096)/1024)/1024)/1024 = total_pages/262144
    unsigned long total_pages = 0;
    req.tv_sec = 0;
    req.tv_nsec = 100000L;
    for(int i=0; i<10; i++){
        memcpy(&total_pages, &map_ptr[INDEX_TOTAL_PAGES], sizeof(unsigned long));
        if(total_pages>0) break;
        nanosleep(&req, NULL);
    }
    double total_ram_gb = ((double)total_pages)/262144;
    printf("Total pages: %lu \nTotal RAM:  %.2f GB\n", total_pages, total_ram_gb);

    // Configuracion OpenGL
    float quadVertices[] = {
        -1.0f, -1.0f,   0.0f, 1.0f,
        1.0f, -1.0f,   1.0f, 1.0f,
        -1.0f,  1.0f,   0.0f, 0.0f,
        1.0f,  1.0f,   1.0f, 0.0f
    };
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glEnable(GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, WIDTH, HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);

    // Shader en OpenGL
    GLuint shaderProgram = compile_shader();
    glUseProgram(shaderProgram);
    if (shaderProgram == 0){
        munmap(map_ptr, BUFFER_SIZE);
        close(fd);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    GLint textureLocation = glGetUniformLocation(shaderProgram, "myTexture");
    glUniform1i(textureLocation, 0);

    // VAO y VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    int fb_w, fb_h;
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    float aspect = (float)fb_w / (float)fb_h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Background color
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    #if CAPT_VIDEO
    int init_error = init_video_capture(start_width, start_height);
    if (init_error){
        glDeleteTextures(1, &textureID);
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
        munmap(map_ptr, BUFFER_SIZE);
        close(fd);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    #endif

    previosTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        #if CAPT_VIDEO
            frame_start_time = glfwGetTime();
        #endif

        // Correccion de estiramiento y desplazminetos
        press_hold_keys(window);

        glClear(GL_COLOR_BUFFER_BIT);
        glfwGetFramebufferSize(window, &fb_w, &fb_h);
        if (fb_h == 0) fb_h = 1;
        glViewport(0, 0, fb_w, fb_h);
        aspect = (float)fb_w / (float)fb_h;
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        apply_transformation();

        float tex_aspect = (float)WIDTH / (float)HEIGHT;
        glScalef(tex_aspect, 1.0f, 1.0f);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RED_INTEGER, GL_UNSIGNED_BYTE, map_ptr);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        if (get_show_info()) {
            glUseProgram(0);
            glDisable(GL_TEXTURE_2D);

            show_hud(map_ptr,fps);

            glDisable(GL_BLEND);
            glEnable(GL_TEXTURE_2D);
            glUseProgram(shaderProgram);
        }

        #if CAPT_VIDEO
        record_frame();
        #endif

        glfwSwapBuffers(window);
        glfwPollEvents();

        // Calculo FPS
        double currentTime = glfwGetTime();
        frameCounter ++;
        if (currentTime-previosTime >= 1.0){
            fps = frameCounter;

            #if FORCE_WIN_TEXTURE
            uint8_t kups = map_ptr[INDEX_KUPS]; // actualizaciones del kernel por segundo
            char title[128];
                #if CAPT_VIDEO
                snprintf(title,sizeof(title), "RAM map:  KUPS: %hhu, FPS: %.1f, REC [%.1f FPS]",kups, fps, target_fps);
                #else
                snprintf(title,sizeof(title), "RAM map:  KUPS: %hhu, FPS: %.1f, NO REC",kups, fps);
                #endif
            glfwSetWindowTitle(window, title);
            #endif

            previosTime = currentTime;
            frameCounter = 0;
        }
        #if CAPT_VIDEO
            double frame_end_time = glfwGetTime();
            double frame_duration = frame_end_time - frame_start_time;
            double wait_time = target_frame_time - frame_duration;
            if (wait_time > 0.0) {
                req.tv_sec = (time_t)wait_time;
                req.tv_nsec = (long)((wait_time - req.tv_sec) * 1000000000.0);
                nanosleep(&req, NULL);
            }
        #endif
    }
    #if CAPT_VIDEO
    clean_video_capture();
    #endif

    glDeleteTextures(1, &textureID);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    munmap(map_ptr, BUFFER_SIZE);
    glfwDestroyWindow(window);
    glfwTerminate();
    close(fd);

    printf("Close\n");
    return EXIT_SUCCESS;
}
