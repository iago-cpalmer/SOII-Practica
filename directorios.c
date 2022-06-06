#include "directorios.h"

static struct UltimaEntrada UltimaEntradaEscritura;
static struct UltimaEntrada UltimaEntradaLectura;

static struct UltimaEntrada ultimaEntradaEscrita;
static struct UltimaEntrada ultimaEntradaLeida;

int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    int  i = 0;
    if(camino[0] != '/') {
        return -1;
    }
    const char *sig_camino = strchr(camino+1, '/');
    memset(inicial, 0, strlen(inicial));
    if(sig_camino==NULL){
        strcpy(inicial, camino+1);
        *final = '\0';
        *tipo = 'f';
    } else {
        i = sig_camino - camino - 1;
        strncpy(inicial, camino+1,i);
        inicial[i]='\0';
        strcpy(final, sig_camino);
        *tipo = 'd';
    }
    return 0;
}


int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, 
unsigned int *p_entrada, bool reservar, unsigned char permisos) {

    struct superbloque SB;
    if (bread(0, &SB) == -1)
    {
        fprintf(stderr, "Error en directorios.c buscar_entrada() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }

    struct entrada entrada;
    struct entrada *lol;
    lol= malloc(sizeof(struct entrada));
    lol->nombre[0] = '\0';
    entrada  = *lol;

    struct inodo inodo_dir;
    char inicial[sizeof(entrada.nombre)];
    inicial[0] = '\0';
    char final[strlen(camino_parcial)];
    final[0]  = '\0';
    char tipo;
    int cant_entradas_inodo, num_entrada_inodo;

    if(strcmp(camino_parcial, "/") == 0) {
        *p_inodo = SB.posInodoRaiz;
        *p_entrada = 0;
        return 0;
    }
    
    int x = extraer_camino(camino_parcial, inicial, final, &tipo);
    if(x==-1) {
        return ERROR_CAMINO_INCORRECTO;
    }
    
    #if DEBUGN7 == 1
        fprintf(stderr, "[buscar_entrada()-->inicial:%s, final: %s, reservar: %d]\n", inicial, final, reservar);
    #endif
    if(leer_inodo(*p_inodo_dir, &inodo_dir)==-1) {
        //error
        fprintf(stderr, "Error en directorios.c buscar_entrada() --> %d: %s\n", errno, strerror(errno));
        return -1;
    }
    if((inodo_dir.permisos & 4) == 0) {
        #if DEBUGN7 == 1
            fprintf(stderr, "[buscar_entrada() --> El inodo %d no tiene permisos de lectura]\n", *p_inodo_dir);
        #endif
        return ERROR_PERMISO_LECTURA;
    }

    struct entrada entradas[BLOCKSIZE / sizeof(struct entrada)];
    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);
    num_entrada_inodo = 0;
    
    if(cant_entradas_inodo>0) {
        //Leer todos los bloques de datos apuntados por el inodo para comprobar las entradas
        //una a una que ese bloque físico contiene.
        int nblogicoActual = 0;
        bool encontrado = false;
        while(num_entrada_inodo<cant_entradas_inodo && !encontrado) {
            mi_read_f(*p_inodo_dir, entradas, num_entrada_inodo*sizeof(struct entrada), BLOCKSIZE);
            for(int i = 0; i < BLOCKSIZE/sizeof(struct entrada) && num_entrada_inodo<cant_entradas_inodo && !encontrado;i++) {
                num_entrada_inodo++;
                if(strcmp(inicial, entradas[i].nombre)==0) {
                    encontrado=true;
                    strcpy(entrada.nombre, entradas[i].nombre);
                    entrada.ninodo=entradas[i].ninodo;
                    num_entrada_inodo--;
                }
                
            }
            nblogicoActual++;
        }
    }
    if(strcmp(inicial, entrada.nombre)!=0) {
        switch(reservar) {
            case 0:
                return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
            case 1:
                if(inodo_dir.tipo=='f') {
                    return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
                }
                if((inodo_dir.permisos & 2) == 0) {
                    return ERROR_PERMISO_ESCRITURA;
                } else{
                    strcpy(entrada.nombre, inicial);
                    if(tipo == 'd') {
                        if(strcmp(final, "/")==0) {
                            entrada.ninodo = reservar_inodo('d', (unsigned char) permisos);

                            #if DEBUGN7 == 1
                                fprintf(stderr, "[buscar_entrada() --> reservado inodo %d tipo %c con permisos %d para %s]\n", entrada.ninodo, tipo, permisos, inicial);
                            #endif
                        } else {
                            return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
                        }
                    } else {
                        entrada.ninodo = reservar_inodo('f', (unsigned char) permisos);
                        #if DEBUGN7 == 1
                                fprintf(stderr, "[buscar_entrada() --> reservado inodo %d tipo %c con permisos %d para %s]\n", entrada.ninodo, tipo, permisos, inicial);
                        #endif
                    }
                    if(mi_write_f(*p_inodo_dir, &entrada, num_entrada_inodo*sizeof(struct entrada), sizeof(struct entrada))<0) {
                        if(entrada.ninodo!=-1) {
                            liberar_inodo(entrada.ninodo);
                        }
                        return EXIT_FAILURE;
                    }
                    #if DEBUGN7 == 1
                       fprintf(stderr, "[buscar_entrada() --> creada entrada: %s, %d]\n", entrada.nombre, entrada.ninodo);
                    #endif         
                }
        }

            
                
    } 
    if((strcmp(final, "/") == 0) || strcmp(final,"")==0) {
        if((num_entrada_inodo<cant_entradas_inodo) && (reservar==1)) {
            return ERROR_ENTRADA_YA_EXISTENTE;
        }
        *p_inodo = entrada.ninodo;
        *p_entrada = num_entrada_inodo;
        return EXIT_SUCCESS;
    } else{
        *p_inodo_dir = entrada.ninodo;
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }

    return EXIT_SUCCESS;

    

}

void mostrar_error_buscar_entrada(int error) {
   // fprintf(stderr, "Error: %d\n", error);
   switch (error) {
   case -1: fprintf(stderr, "Error: Camino incorrecto.\n"); break;
   case -2: fprintf(stderr, "Error: Permiso denegado de lectura.\n"); break;
   case -3: fprintf(stderr, "Error: No existe el archivo o el directorio.\n"); break;
   case -4: fprintf(stderr, "Error: No existe algún directorio intermedio.\n"); break;
   case -5: fprintf(stderr, "Error: Permiso denegado de escritura.\n"); break;
   case -6: fprintf(stderr, "Error: El archivo ya existe.\n"); break;
   case -7: fprintf(stderr, "Error: No es un directorio.\n"); break;
   }
}

int mi_creat(const char *camino, unsigned char permisos) {
    mi_waitSem();
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, 6)) < 0) {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return -1;
    }
    mi_signalSem();
    return 0;
}

int mi_dir(const char *camino, char *buffer, char tipo) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 6)) < 0) {
        mostrar_error_buscar_entrada(error);
        return -1;
    }
    

    struct inodo inodo;
    if(leer_inodo(p_inodo, &inodo) == -1) {
        fprintf(stderr, "Error en directorios.c mi_dir--> %d: %s\n", errno, strerror(errno));
        return -1;
     }
    if(inodo.tipo != tipo) {
        fprintf(stderr, "Error en directorios.c mi_dir--> la sintaxis no concuerda con el tipo.");
        return -1;
    }    
    strcat(buffer, "Tipo  Modo  mTime                   Tamaño          Nombre");
    strcat(buffer, "\n------------------------------------------------------------\n");
    if(tipo == 'd'){
        struct entrada entradas[BLOCKSIZE/sizeof(struct entrada)];
        int nentradasTotal = inodo.tamEnBytesLog / sizeof(struct entrada);
        fprintf(stderr, "Total: %d\n", nentradasTotal);
        int nentradas = 0;
        int offset = 0;
        while(nentradas < nentradasTotal) {
        
            mi_read_f(p_inodo, entradas, offset, BLOCKSIZE);
            for(int i = 0; i < BLOCKSIZE/sizeof(struct entrada) && nentradas < nentradasTotal; i++) {
                if(leer_inodo(entradas[i].ninodo, &inodo) == -1) {
                    fprintf(stderr, "Error en directorios.c mi_dir() --> %d: %s\n", errno, strerror(errno));
                    return -1;
                }
                //Tipo
                char tipod[2];
                tipod[0] = inodo.tipo;
                tipod[1] = '\0';
                strcat(buffer, tipod);
                strcat(buffer, "    ");
                //Permisos
                if(inodo.permisos & 4) {
                    strcat(buffer, "r");
                } else {
                    strcat(buffer, "-");
                }
                if(inodo.permisos & 2) {
                    strcat(buffer, "w");
                } else {
                    strcat(buffer, "-");
                }
            
                if(inodo.permisos & 1) {
                    strcat(buffer, "x");
                } else {
                    strcat(buffer, "-");
                }

                strcat(buffer, "    ");
                //MTime
                struct tm *tm;
                tm = localtime(&inodo.mtime);
                char tmp[1024];
                sprintf(tmp, "%d-%02d-%02d:%02d:%02d:%02d", tm-> tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
                strcat(buffer, tmp);
                strcat(buffer, "        ");
                char sizeEntrada[8];
                sprintf(sizeEntrada, "%d", inodo.tamEnBytesLog);
                strcat(buffer, sizeEntrada);
                strcat(buffer, "            ");
                strcat(buffer, entradas[i].nombre);

                strcat(buffer, "\n");   
                nentradas++;
                
            }
            offset+=BLOCKSIZE;
        }
    } else if(tipo=='f') {
                //Tipo
                char tipod[2];
                tipod[0] = inodo.tipo;
                tipod[1] = '\0';
                strcat(buffer, tipod);
                strcat(buffer, "    ");
                //Permisos
                if(inodo.permisos & 4) {
                    strcat(buffer, "r");
                } else {
                    strcat(buffer, "-");
                }
                if(inodo.permisos & 2) {
                    strcat(buffer, "w");
                } else {
                    strcat(buffer, "-");
                }
            
                if(inodo.permisos & 1) {
                    strcat(buffer, "x");
                } else {
                    strcat(buffer, "-");
                }

                strcat(buffer, "    ");
                //MTime
                struct tm *tm;
                tm = localtime(&inodo.mtime);
                char tmp[1024];
                sprintf(tmp, "%d-%02d-%02d:%02d:%02d:%02d", tm-> tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
                strcat(buffer, tmp);
                strcat(buffer, "        ");
                char sizeEntrada[8];
                sprintf(sizeEntrada, "%d", inodo.tamEnBytesLog);
                strcat(buffer, sizeEntrada);
                strcat(buffer, "            ");
                struct entrada entradaFichero;
                mi_read_f(p_inodo_dir, &entradaFichero, p_entrada*sizeof(struct entrada), sizeof(struct entrada));
                strcat(buffer, entradaFichero.nombre);
                strcat(buffer, "\n"); 
    }
                
    
    
    
    return 0;
}

int mi_chmod(const char *camino, unsigned char permisos) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 6)) < 0) {
        mostrar_error_buscar_entrada(error);
        return -1;
    }
    return mi_chmod_f(p_inodo, permisos);
}

int mi_stat(const char *camino, struct STAT *p_stat) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 6)) < 0) {
        mostrar_error_buscar_entrada(error);
        return -1;
    }
    fprintf(stderr, "Nº de inodo: %d\n", p_inodo);
    return mi_stat_f(p_inodo, p_stat);
}

/*
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    mi_waitSem();
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;

    

    if(strcmp(UltimaEntradaEscritura.camino, camino)==0) {
        #if DEBUGN9
            fprintf(stderr, "[mi_write) --> Utilizamos la caché de escritura en vez de llamar a buscar_entrada()]\n");
        #endif
        p_inodo = UltimaEntradaEscritura.p_inodo;
    } else {
        if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 6)) < 0) {
            mostrar_error_buscar_entrada(error);
            mi_signalSem();
            return -1;
        }

        //Guardar en ultima entrada
        #if DEBUGN9
            fprintf(stderr, "[mi_write() --> Actualizamos la caché de escritura]\n");
        #endif
        strcpy(UltimaEntradaEscritura.camino ,camino);
        UltimaEntradaEscritura.p_inodo = p_inodo;
    }
    mi_signalSem();
    return mi_write_f(p_inodo, buf, offset, nbytes);

}

int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes) {
    mi_waitSem();
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;


    if(strcmp(UltimaEntradaLectura.camino, camino)==0) {
        #if DEBUGN9
            fprintf(stderr, "\n[mi_read() --> Utilizamos la caché de lectura en vez de llamar a buscar_entrada()]\n");
        #endif
        p_inodo = UltimaEntradaLectura.p_inodo;
    } else {
        if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4)) < 0) {
            mostrar_error_buscar_entrada(error);
            mi_signalSem();
            return -1;
        }
        //Guardar en ultima entrada
        #if DEBUGN9
            fprintf(stderr, "[mi_read() --> Actualizamos la caché de lectura]\n");
        #endif
        strcpy(UltimaEntradaLectura.camino ,camino);
        UltimaEntradaLectura.p_inodo = p_inodo;
    }
    mi_signalSem();
    return mi_read_f(p_inodo, buf, offset, nbytes);

}*/
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes){
  mi_waitSem();
	unsigned int p_inodo_dir, p_inodo, p_entrada;
	p_inodo_dir = 0;
	if(strcmp(camino, ultimaEntradaLeida.camino) == 0) {
		p_inodo = ultimaEntradaLeida.p_inodo;
	} else {
		if(buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, '6') < 0) {
      mi_signalSem();
      //printf("DEBUG - mi_write | buscar_entrada ha ido mal\n");
      return -1;
    }
		strcpy(ultimaEntradaLeida.camino, camino);
		ultimaEntradaLeida.p_inodo = p_inodo;
    //printf("DEBUG - mi_write | buscar_entrada ha ido bien\n");
	}
	int bytesLeidos = mi_write_f(p_inodo, buf, offset, nbytes);
  //printf("DEBUG - mi_write | mi_write_f ha acabado con resultado %d\n",bytesLeidos);
	if(bytesLeidos < 0) {
    mi_signalSem();
    return -1;
  }
  mi_signalSem();
	return bytesLeidos;
}
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes){
  mi_waitSem();
	unsigned int p_inodo_dir, p_inodo, p_entrada;
  p_inodo_dir = 0;
	if(strcmp (camino, ultimaEntradaLeida.camino) == 0) {
		p_inodo = ultimaEntradaLeida.p_inodo;
	} else {
		if(buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, '4') < 0) {
      mi_signalSem();
      return -1;
    }
		strcpy(ultimaEntradaLeida.camino, camino);
		ultimaEntradaLeida.p_inodo = p_inodo;
	}
  mi_signalSem();
	return mi_read_f(p_inodo, buf, offset, nbytes);
}

int mi_link(const char *camino, const char *camino2) {
    mi_waitSem();
    unsigned int p_inodo_dir1 = 0;
    unsigned int p_entrada1 = 0;
    unsigned int p_inodo1 = 0;
    unsigned int p_inodo2 = 0;
    unsigned int p_inodo_dir2 =0 ;
    unsigned int p_entrada2 = 0;
    
    struct inodo inodo1;
    
    int error;
    if ((error = buscar_entrada(camino, &p_inodo_dir1, &p_inodo1, &p_entrada1, 0, 4)) < 0) {
            mostrar_error_buscar_entrada(error);
            mi_signalSem();
            return -1;
    }    
    if ((error = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, 1, 6)) < 0) {
            mostrar_error_buscar_entrada(error);
            mi_signalSem();
            return -1;
    }  

    if(leer_inodo(p_inodo1, &inodo1)==-1) {
        fprintf(stderr, "Error en directorios.c mi_link() --> %d: %s\n", errno, strerror(errno));
        mi_signalSem();
        return -1;
    }
    struct entrada entrada;
    if(mi_read_f(p_inodo_dir2, &entrada, p_entrada2*sizeof(struct entrada), sizeof(struct entrada))==-1) {
        fprintf(stderr, "Error en directorios.c mi_link() --> %d: %s\n", errno, strerror(errno));
        mi_signalSem();
        return -1;
    }
    liberar_inodo(p_inodo2);
    entrada.ninodo=p_inodo1;
    
    if(mi_write_f(p_inodo_dir2, &entrada, p_entrada2*sizeof(struct entrada), sizeof(struct entrada))==-1) {
        fprintf(stderr, "Error en directorios.c mi_link() --> %d: %s\n", errno, strerror(errno));
        mi_signalSem();
        return -1;
    }
    inodo1.nlinks++;
    inodo1.ctime=time(NULL);
    if(escribir_inodo(p_inodo1, inodo1)==-1) {
        fprintf(stderr, "Error en directorios.c mi_link() --> %d: %s\n", errno, strerror(errno));
        mi_signalSem();
        return -1;
    }
    return 0;
}


int mi_unlink(const char *camino) {
    mi_waitSem();
    unsigned int p_inodo_dir1 = 0;
    unsigned int p_entrada1 = 0;
    unsigned int p_inodo1 = 0;
    int error;
    if ((error = buscar_entrada(camino, &p_inodo_dir1, &p_inodo1, &p_entrada1, 0, 4)) < 0) {
            mostrar_error_buscar_entrada(error);
            mi_signalSem();
            return -1;
    } 
    struct inodo inodo1;
    struct inodo inodo2;
    if(leer_inodo(p_inodo1, &inodo1)==-1) {
        fprintf(stderr, "Error en directorios.c mi_unlink() --> %d: %s\n", errno, strerror(errno));
        mi_signalSem();
        return -1;
    }
    if((inodo1.tipo=='d') && inodo1.tamEnBytesLog>0) {
        fprintf(stderr, "Error: El directorio %s no está vacío\n", camino);
        mi_signalSem();
        return -1;
    }
    if(leer_inodo(p_inodo_dir1, &inodo2)==-1) {
        fprintf(stderr, "Error en directorios.c mi_unlink() --> %d: %s\n", errno, strerror(errno));
        mi_signalSem();
        return -1;
    }
    int nentradas = inodo2.tamEnBytesLog/sizeof(struct entrada);
    struct entrada entrada;
    if(p_entrada1!=nentradas-1) {
        if(mi_read_f(p_inodo_dir1, &entrada, (nentradas-1)*sizeof(struct entrada), sizeof(struct entrada))==-1) {
            fprintf(stderr, "Error en directorios.c mi_unlink() --> %d: %s\n", errno, strerror(errno));
            mi_signalSem();
            return -1;
        }

        if(mi_write_f(p_inodo_dir1, &entrada, p_entrada1*sizeof(struct entrada), sizeof(struct entrada))==-1) {
            fprintf(stderr, "Error en directorios.c mi_unlink() --> %d: %s\n", errno, strerror(errno));
            mi_signalSem();
            return -1;
        }
        
    }
        
    mi_truncar_f(p_inodo_dir1, (nentradas-1)*sizeof(struct entrada));
    if(leer_inodo(p_inodo1, &inodo1)) {
        fprintf(stderr, "Error en directorios.c mi_unlink() --> %d: %s\n", errno, strerror(errno));
        mi_signalSem();
        return -1;
    }
    inodo1.nlinks--;
    
    if(inodo1.nlinks==0) {
        liberar_inodo(p_inodo1);
        mi_signalSem();
        return 0;
    }
    inodo1.ctime=time(NULL);

    if(escribir_inodo(p_inodo1, inodo1)==-1) {
        fprintf(stderr, "Error en directorios.c mi_unlink() --> %d: %s\n", errno, strerror(errno));
        mi_signalSem();
        return -1;
    }
    mi_signalSem();
    return 0;
}