#include "directorios.h"

int main(int argc, char **argv) {

    // Comprobaciòn sintaxis correcta
    if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL) {
        fprintf(stderr,"Paràmetros no especificados. Uso: mi_mkdir <nombre_dispositivo> <permisos> </ruta>\n");
        exit(1);
    }
    if(bmount(argv[1]) ==-1) {
        fprintf(stderr,"mi_mkdir --> Error al montar el dispositivo\n");
        return -1;
    }
    //COmprobación permisos válidos
    int permisos = atoi(argv[2]);
    if(permisos < 0 || permisos > 7) {
        fprintf(stderr,"Permisos especificados no válidos. Debe ser un número positivo menor o igual a 7.\n");
        exit(1);
    }

    
    int x = mi_creat(argv[3], permisos);
    if (bumount() == -1) {
        fprintf(stderr, "mi_mkdir.c --> Error al desmontar el dispositivo\n");
        return -1;
    }
    return x;

}