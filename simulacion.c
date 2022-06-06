#include "simulacion.h"

char dirPrueba[69];
char camino[69];
char camino2[200];
static int acabados;

void my_sleep(unsigned msec) { //recibe tiempo en milisegundos
   struct timespec req, rem;
   int err;
   req.tv_sec = msec / 1000; //conversión a segundos
   req.tv_nsec = (msec % 1000) * 1000000; //conversión a nanosegundos
   while ((req.tv_sec != 0) || (req.tv_nsec != 0)) {
       if (nanosleep(&req, &rem) == 0) 
// rem almacena el tiempo restante si una llamada al sistema
// ha sido interrumpida por una señal
           break;
       err = errno;
       // Interrupted; continue
       if (err == EINTR) {
           req.tv_sec = rem.tv_sec;
           req.tv_nsec = rem.tv_nsec;
       }
   }
}

void reaper(){
  pid_t ended;
  signal(SIGCHLD, reaper);
  while ((ended=waitpid(-1, NULL, WNOHANG))>0) {
    acabados++;
  }
}

void proceso(int pid, char *disco, int nProceso){
  char pidDirectorio[16];
  struct REGISTRO registro;
  if (bmount(disco) == -1) {
    fprintf(stderr, "Error al montar el disco para el proceso con PID %d\n", pid);
    bumount();
    exit(1);
  }
  sprintf(pidDirectorio, "proceso_%d/", pid);
  memset(camino,0,sizeof(camino));
  strcpy(camino, dirPrueba);
  strcat(camino, pidDirectorio);
  //Creamos el directorio del proceso
  if(mi_creat(camino, '7') != 0){
    bumount();
    exit(1);
  }
  memset(camino2,0,sizeof(camino2));
  sprintf(camino2, "%sprueba.dat", camino); 


  //Creamos pureba.dat
  if(mi_creat(camino2, '6') != 0){
    fprintf(stderr, "simulacion.c - Error en la creación del fichero \"prueba.dat\"\n");
    bumount();
    exit(1);
  }
  //Escribimos los registros
  srand(time(NULL) + getpid());
  for(int i = 0; i < NUMESCRITURAS; i++) {
    registro.fecha = time(NULL);
    registro.pid = getpid();
    registro.nEscritura = i + 1;
    registro.nRegistro =  rand() % REGMAX;
    if(mi_write(camino2,&registro,registro.nRegistro*sizeof(struct REGISTRO),sizeof(struct REGISTRO)) < 0){
      fprintf(stderr, "Fallo en la escritura en %s\n", camino2);
      exit(1);
    }
    #if DEBUGN12
        fprintf(stderr, "[simulacion.c --> Escritura %d en %s]\n", (i+1), camino2);
    #endif
    my_sleep(50);
  }
   fprintf(stderr, "[Proceso %d: Completadas %d escrituras en %s]\n", nProceso, NUMESCRITURAS, camino2);
  bumount();
  exit(0);
}

int main (int argc, char **argv) {
    signal(SIGCHLD, reaper);
  char fech[16];
  char f[8] = "/simul_";
  struct tm *ts;
  //Mostramos sintaxis correcta
  if (argc != 2) {
    fprintf(stderr, "Sintaxis correcta: ./simulacion <disco>\n");
    return -1;
  }
  //Montamos el disco
  if (bmount(argv[1]) == -1) {
     fprintf(stderr, "simulacion.c --> Error al montar el disco\n");
    return -1;
  }
    
    
    //Creamos el directorio
    time_t t = time(NULL);
    ts = localtime(&t);
    memset(fech,0,sizeof(fech));
    strftime(fech, sizeof(fech), "%Y%m%d%H%M%S/", ts);
    memset(dirPrueba,0,sizeof(dirPrueba));
    strcpy(dirPrueba, f);
    strcat(dirPrueba, fech);
    if(mi_creat(dirPrueba, '7') < 0) {
        fprintf(stderr, "El directorio '%s' no se ha podido crear\n", dirPrueba);
        bumount();
        return -1;
    } else {
        fprintf(stderr, "Directorio simulación: %s\n", dirPrueba);
  }
  acabados = 0;
  //Asignamos la función enterrador a la señal de finalización de un hijo

  for (int i = 1; i <= NUMPROCESOS; i++) {
    if(fork() == 0) {
      proceso(getpid(), argv[1], i);
    }
    //Esperamos 0.15 s
    my_sleep(150);
  }
  //Esperamos a que todos los procesos acaben
  while (acabados < NUMPROCESOS) {
    pause();
  }
  bumount();
  exit(0);
}

