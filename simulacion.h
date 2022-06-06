#include <sys/wait.h>
#include <signal.h>
#include "directorios.h"
#define NUMESCRITURAS 50
#define NUMPROCESOS 100
#define REGMAX 500000
#define DEBUGN12 0
struct REGISTRO {
    time_t fecha;
    pid_t pid;
    int nEscritura;
    int nRegistro;
};
