#include "directorios.h"

int main(int argc, char **argv) {

    // Comprobaciòn sintaxis correcta
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr,"Paràmetros no especificados. Uso: mi_ls <nombre_dispositivo> </ruta>\n");
        exit(1);
    }
    if(bmount(argv[1]) ==-1) {
        fprintf(stderr,"mi_ls --> Error al montar el dispositivo\n");
        return -1;
    }

    char buffer[100000];
    buffer[0] = '\0';
    char *camino = argv[2];
    char tipo;
    if(camino[strlen(camino)-1]=='/') {
        tipo = 'd';
    } else {
        tipo = 'f';
    }
    if(mi_dir(camino, buffer, tipo)==-1) {
        return -1;
    }
    printf("%s\n", buffer);

    if (bumount() == -1) {
        fprintf(stderr, "mi_stat.c --> Error al desmontar el dispositivo\n");
        return -1;
    }
    return 0;


}