# Compilacion del modulo del kernenl y las aplicaciones del usuario
obj-m += mmap_kernel.o

all: mmap_kernel mmap_user full_mem

mmap_kernel:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

mmap_user: mmap_user.c
	gcc -o mmap_user mmap_user.c -lGL -lGLEW -lglfw -lm

full_men: full_mem.c
	gcc -o full_mem full_mem.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f mmap_user
	rm -f full_mem
