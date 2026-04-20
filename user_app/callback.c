#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "../share.h"

#include "callback.h"

static int show_info = 0;

static int is_fullscreen = 1;
static int win_x = 100, win_y = 100;
static int win_w = 800, win_h = 800;

static float zoom = 1.0f;
static float offsetX = 0.0f;
static float offsetY = 0.0f;

static int isDragging = 0;
static double lastMouseX = 0.0;
static double lastMouseY = 0.0;

int get_show_info(){
    return show_info;
}

void mouse_button_callback_fun(GLFWwindow* window, int button, int action)
{
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

void cursor_position_callback_fun(GLFWwindow* window, double xpos, double ypos)
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

        if (offsetX < MIN_X) offsetX = MIN_X;
        if (offsetX > MAX_X) offsetX = MAX_X;
        if (offsetY < MIN_Y) offsetY = MIN_Y;
        if (offsetY > MAX_Y) offsetY = MAX_Y;

        lastMouseX = xpos;
        lastMouseY = ypos;
    }
}

void scroll_callback_fun(double yoffset)
{
    if (yoffset > 0) zoom *= 1.1f;
    else if (yoffset < 0) zoom /= 1.1f;

    if (zoom < MIN_ZOOM) zoom = MIN_ZOOM;
    if (zoom > MAX_ZOOM) zoom = MAX_ZOOM;
}

void key_callback_fun(GLFWwindow* window, int key, int action, uint8_t *map_ptr)
{
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE){
            glfwSetWindowShouldClose(window, 1);
        } else if (key == GLFW_KEY_F) {
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
        } else if (key == GLFW_KEY_Z){
            map_ptr[INDEX_MODE] = (map_ptr[INDEX_MODE] == 1)? 0:1;
        } else if (key == GLFW_KEY_S){
            map_ptr[INDEX_MODE] = (map_ptr[INDEX_MODE] == 2)? 0:2;
        } else if (key == GLFW_KEY_1 ) {
            if(map_ptr[INDEX_MODE] != 0) map_ptr[INDEX_MODE] = 0;
            map_ptr[INDEX_VIEW] ^= MASK_FREE;
        } else if (key == GLFW_KEY_2 ) {
            if(map_ptr[INDEX_MODE] != 0) map_ptr[INDEX_MODE] = 0;
            map_ptr[INDEX_VIEW] ^= MASK_PGTB;
        } else if (key == GLFW_KEY_3 ) {
            if(map_ptr[INDEX_MODE] != 0) map_ptr[INDEX_MODE] = 0;
            map_ptr[INDEX_VIEW] ^= MASK_RESE;
        } else if (key == GLFW_KEY_4 ) {
            if(map_ptr[INDEX_MODE] != 0) map_ptr[INDEX_MODE] = 0;
            map_ptr[INDEX_VIEW] ^= MASK_COMP;
        } else if (key == GLFW_KEY_5 ) {
            if(map_ptr[INDEX_MODE] != 0) map_ptr[INDEX_MODE] = 0;
            map_ptr[INDEX_VIEW] ^= MASK_FILE;
        } else if (key == GLFW_KEY_6 ) {
            if(map_ptr[INDEX_MODE] != 0) map_ptr[INDEX_MODE] = 0;
            map_ptr[INDEX_VIEW] ^= MASK_ANON;
        } else if (key == GLFW_KEY_7 ) {
            if(map_ptr[INDEX_MODE] != 0) map_ptr[INDEX_MODE] = 0;
            map_ptr[INDEX_VIEW] ^= MASK_USER;
        } else if (key == GLFW_KEY_8 ) {
            if(map_ptr[INDEX_MODE] != 0) map_ptr[INDEX_MODE] = 0;
            map_ptr[INDEX_VIEW] ^= MASK_KERN;
        } else if (key == GLFW_KEY_X ) {
            if(map_ptr[INDEX_MODE] != 0) map_ptr[INDEX_MODE] = 0;
            map_ptr[INDEX_VIEW] ^= MASK_ALL;
        } else if (key == GLFW_KEY_A) {
            if(map_ptr[INDEX_MODE] != 0) map_ptr[INDEX_MODE] = 0;
            if (map_ptr[INDEX_VIEW] ^ MASK_ALL) map_ptr[INDEX_VIEW] = MASK_ALL;
            else map_ptr[INDEX_VIEW] = 0;
        } else if (key == GLFW_KEY_I) {
            show_info = !show_info;
        } else if (key == GLFW_KEY_R) {
            zoom = 1.0f;
            offsetX = 0.0f;
            offsetY = 0.0f;
        }
    }
}

void press_hold_keys(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS &&  zoom > MIN_ZOOM) zoom /= 1.02f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS &&  zoom < MAX_ZOOM) zoom *= 1.02f;

    float panSpeed = 0.02f / zoom;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && offsetY >= MIN_Y) offsetY -= panSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && offsetY <= MAX_Y) offsetY += panSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && offsetX <= MAX_X) offsetX += panSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && offsetX >= MIN_X) offsetX -= panSpeed;

}

void apply_transformation()
{
    glScalef(zoom, zoom, 1.0f);
    glTranslatef(offsetX, offsetY, 0.0f);
}
