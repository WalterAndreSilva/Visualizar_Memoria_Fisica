#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>        // uintmax_t
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>        // sysconf
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <time.h>

#define PAGE_SIZE 4096
#define PAGES_NUMBER 128   // Igual que en el kernel

// Dimensiones de nuestra "pantalla virtual" en píxeles
#define WIDTH 400
#define HEIGHT 400

// Cada píxel tiene 3 componentes: Rojo, Verde, Azul (RGB)
#define COMPONENTS 3

enum { BUFFER_SIZE = 480000 }; // BUFFER_SIZE > WIDTH * HEIGHT *COMPONENTS

static const char *pathname = "/proc/lkmc_mmap";

unsigned char pixel_buffer[WIDTH * HEIGHT * COMPONENTS];

int main(int argc, char **argv)
{
    int fd;
    long page_size;
    char *buff;

    page_size = sysconf(_SC_PAGE_SIZE)*PAGES_NUMBER;  // varias paginas

    buff = (char*)malloc(BUFFER_SIZE * sizeof(char));
    if (buff == NULL) {
        printf("Error: No se pudo asignar memoria a buff.\n");
        return 1;
    }

    printf("open pathname = %s\n", pathname);
    fd = open(pathname, O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open");
        assert(0);
    }

    // Grafica. Inicializar GLFW y crear ventana
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Pintando Pixeles con OpenGL", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Preparar el dibujo en OpenGL
    // Definimos un rectángulo (quad) que cubra toda la ventana
    float vertices[] = {
        -1.0f, -1.0f, // Inferior Izquierda
        1.0f, -1.0f, // Inferior Derecha
        1.0f,  1.0f, // Superior Derecha
        -1.0f,  1.0f  // Superior Izquierda
    };

    // Coordenadas de textura para mapear el buffer al rectángulo
    float texCoords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    // Crear la textura en la GPU
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Configuración básica de la textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Activamos las características necesarias de OpenGL clásico
    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

    // Solicitar al kernel que mapee el buffer del driver a nuestro espacio
    // NULL: El kernel elige la dirección virtual
    // PAGE_SIZE: Tamaño del buffer (debe coincidir con el del kernel)
    // PROT_READ | PROT_WRITE: Queremos leer y escribir
    // MAP_SHARED: Los cambios se ven en ambos lados (Kernel y Usuario)

    unsigned char *map_ptr = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_ptr == MAP_FAILED) {
        perror("Error en mmap");
        close(fd);
        return -1;
    }

    while (!glfwWindowShouldClose(window)) {
        // Limpiar la pantalla
        glClear(GL_COLOR_BUFFER_BIT);

        // AQUÍ ACTUALIZAMOS LOS PÍXELES
        for (int i = 0; i < WIDTH * HEIGHT * COMPONENTS; i++) {
            unsigned char old =  buff[i];
            unsigned char new = map_ptr[i];
            if(old != new){
                printf("Cambio en posicion: %d, viejo: %d, nuevo:%d \n", i, old, new);
            }
            buff[i] = map_ptr[i];
            pixel_buffer[i] = map_ptr[i];
        }

        // Copiar el buffer de CPU a la textura de la GPU
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel_buffer);

        // Dibujar el rectángulo con la textura aplicada
        glDrawArrays(GL_QUADS, 0, 4);

        // Intercambiar buffers y procesar eventos
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // Limpieza
    free(buff);

    munmap(map_ptr, page_size);
    glDeleteTextures(1, &textureID);
    glfwDestroyWindow(window);
    glfwTerminate();

    puts("close");
    close(fd);
    return EXIT_SUCCESS;
}
