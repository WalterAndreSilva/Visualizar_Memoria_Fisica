#ifndef CALLBACK_H
#define CALLBACK_H

//Limites de la vista
#define MIN_X -1.0f      // Borde izquierdo
#define MAX_X  1.0f      // Borde derecho
#define MIN_Y -1.0f      // Borde inferior
#define MAX_Y  1.0f      // Borde superior
#define MIN_ZOOM 0.5f    // Hacia fuera
#define MAX_ZOOM 200.0f  // Hacia adentro

int get_show_info(void);

void mouse_button_callback_fun(GLFWwindow* window, int button, int action);

void cursor_position_callback_fun(GLFWwindow* window, double xpos, double ypos);

void scroll_callback_fun(double yoffset);

void key_callback_fun(GLFWwindow* window, int key, int action, uint8_t *map_ptr);

void press_hold_keys(GLFWwindow* window);

void apply_transformation(void);

#endif
