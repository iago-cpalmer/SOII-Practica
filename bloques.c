//Autores: Joan Català, Alberto Ruiz y Iago Caldentey Palmer

#include "bloques.h"
#include "semaforo_mutex_posix.h"
static int descriptor = 0;  // Descriptor del fichero del file system
static sem_t *mutex;
static unsigned int inside_sc = 0;

// bmount: Abre el file system para la lectura/escritura
int bmount(const char *camino) {
   if (descriptor > 0) {
       close(descriptor);
   }
   if ((descriptor = open(camino, O_RDWR | O_CREAT, 0666)) == -1) {
      fprintf(stderr, "Error: bloques.c → bmount() → open()\n");
   }
   if (!mutex) {
       mutex = initSem();
       if (mutex == SEM_FAILED) {
           return -1;
       }
   }
   return descriptor;
}

// bumount: "Desmonta" el file system que hemos abierto para la lectura/escritura
int bumount() {
   descriptor = close(descriptor);
   if (descriptor == -1) {
       fprintf(stderr, "Error: bloques.c → bumount() → close(): %d: %s\n", errno, strerror(errno));
       return -1;
   }
   deleteSem();
   return 0;
}

// escriure un bloc, el indicat per nbloque, s'sescriu *buf
// bwrite retorna el nº de bytes que s'ha escrit o -1 si error
int bwrite(unsigned int nbloque, const void *buf) {

  // Con 'lseek' nos desplazamos en el gestor de ficheros para coger
  // el bloque que nos interesa
  off_t desplazamiento = lseek(descriptor, nbloque * BLOCKSIZE, SEEK_SET);

  if (desplazamiento == -1) {
    //Error
    fprintf(stderr, "ERROR: fallo al buscar bloque. \n");
    return -1;
  }
  else {
    int bescritos = write(descriptor, buf, BLOCKSIZE);  // N. de bytes escritos
    if (bescritos == -1) {
      fprintf(stderr, "ERROR: fallo al escribir el bloque. \n");
      return -1;
    }
    return bescritos;
  }
}

// bread: lee un bloque indicado por nbloque, devuelve el
//        numero de bytes leidos
int bread(unsigned int nbloque, void *buf) {

  off_t desplazamiento = lseek(descriptor, nbloque * BLOCKSIZE, SEEK_SET);
  if (desplazamiento == -1) {
    //Error
    fprintf(stderr, "ERROR: fallo al buscar bloque. \n");
    return -1;
  }
  else {
    int bleidos = read(descriptor, buf, BLOCKSIZE); // N. de bytes leidos
    if (bleidos == -1) {
      fprintf(stderr, "ERROR: fallo al leer el bloque. \n");
      return -1;
    }
    return bleidos;
  }
}


// Funciones de semáforos
void mi_waitSem() {
  if (!inside_sc) waitSem(mutex);
  inside_sc++;
}

void mi_signalSem() {
  inside_sc--;
  if (!inside_sc) signalSem(mutex);
}
