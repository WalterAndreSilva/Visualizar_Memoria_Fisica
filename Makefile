# Compilacion del modulo del kernenl y las aplicaciones del usuario
CFLAGS = -std=c11 -Wall -Wextra -O2 -march=native
LDFLAGS = -lGL -lGLEW -lglfw -lm
ccflags-y := -O2 -march=native
obj-m += mmap_kernel.o
USER_SRCS = mmap_user.c user_app/text.c user_app/shader.c user_app/callback.c

all: mmap_kernel mmap_user full_mem

mmap_kernel:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

mmap_user: $(USER_SRCS)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

full_mem: full_mem.c
	gcc -o full_mem full_mem.c $(CFLAGS)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f mmap_user
	rm -f full_mem
