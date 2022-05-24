#include "directorios.h"

int main(int argc, char **argv) {

    // Comprobaciòn sintaxis correcta
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr,"Paràmetros no especificados. Uso: mi_mkdir <nombre_dispositivo> </ruta>\n");
        exit(1);
    }
    if(bmount(argv[1]) ==-1) {
        fprintf(stderr,"mi_ls --> Error al montar el dispositivo\n");
        return -1;
    }

    struct STAT stat;

    if(mi_stat(argv[2], &stat)==-1) {
        return -1;
    }

    printf("Tipo: %c\n",stat.tipo);
    printf("Permisos: %d\n",stat.permisos);
    printf("atime: %s",ctime(&stat.atime));
    printf("mtime: %s",ctime(&stat.mtime));
    printf("ctime: %s",ctime(&stat.ctime));
    printf("nlinks: %u\n",stat.nlinks);
    printf("tamEnBytesLog: %u\n",stat.tamEnBytesLog);
    printf("numBloquesOcupados: %u\n",stat.numBloquesOcupados);

    if (bumount() == -1) {
        fprintf(stderr, "mi_stat.c --> Error al desmontar el dispositivo\n");
        return -1;
    }

    return 0;


}