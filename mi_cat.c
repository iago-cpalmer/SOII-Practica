#include "directorios.h"

const int tambuffer = BLOCKSIZE*4;

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso correcto: ./mi_cat <disco> </ruta_fichero>\n");
        return -1;
    }
    if (bmount(argv[1]) == -1) {
        fprintf(stderr, "mi_cat.c --> Error al montar el disco\n");
        return -1;
    }
    char *camino = argv[2];
    unsigned int offset = 0, bytesLeidos = 0;
    char buf[tambuffer];
    if (camino[strlen(argv[2]) - 1] == '/') {
        fprintf(stderr, "mi_cat.c --> La entrada %s no es un fichero\n", camino);
        return -1;
    }

    memset(buf, 0, tambuffer);

    int errores = mi_read(camino, buf, offset, tambuffer);
    while (errores > 0) {
        write(1, buf, errores);
        bytesLeidos += errores;
        offset += tambuffer;
        memset(buf, 0, tambuffer);
        errores = mi_read(camino, buf, offset, tambuffer);
    }
    char bufInfo[100];
    sprintf(bufInfo, "\nTotal_leidos: %d\n", bytesLeidos);
    write(2, bufInfo, strlen(bufInfo));
    if (bumount() == -1) {
        fprintf(stderr, "mi_cat.c --> Error al desmontar el disco\n");
        return -1;
    }
    return 0;
}