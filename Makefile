CC=gcc
CFLAGS=-c -g -Wall -std=gnu99
#LDFLAGS=-pthread
 
SOURCES=mi_mkfs.c bloques.c ficheros_basico.c leer_sf.c ficheros.c escribir.c leer.c permitir.c truncar.c directorios.c mi_mkdir.c mi_ls.c mi_chmod.c  mi_stat.c mi_escribir.c mi_cat.c mi_escribir_varios.c mi_link.c  mi_rm.c #semaforo_mutex_posix.c simulacion.c verificacion.c
LIBRARIES=bloques.o ficheros_basico.o ficheros.o directorios.o #semaforo_mutex_posix.o
INCLUDES=bloques.h ficheros_basico.h ficheros.h directorios.h #semaforo_mutex_posix.h simulacion.h
PROGRAMS=mi_mkfs leer_sf escribir leer permitir truncar mi_mkdir mi_ls mi_chmod  mi_stat mi_escribir mi_cat mi_escribir_varios mi_link mi_rm  #simulacion verificacion
OBJS=$(SOURCES:.c=.o)
 
all: $(OBJS) $(PROGRAMS)
 
$(PROGRAMS): $(LIBRARIES) $(INCLUDES)
	$(CC) $(LDFLAGS) $(LIBRARIES) $@.o -o $@
 
%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ -c $<
 
.PHONY: clean
clean:
	rm -rf *.o *~ $(PROGRAMS) disco* ext*  