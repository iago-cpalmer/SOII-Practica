#include "verificacion.h"

int main (int argc, char **argv) {
	// Comprobaciòn de sintaxis
	if (argc != 3) {
		fprintf(stderr, "Sintaxis correcta: ./verificacion <disco> <directorio_simulacion>\n");
		return -1;
	}
	char *ruta = argv[1];
	if(bmount(ruta)==-1){
		fprintf(stderr,"Error al montar el disco");
		return -1;
	}	// Montamos el disco
	char *camino = argv[2];
	fprintf(stderr, "dir_sim: %s\n", camino);

	// Obtenimos la información del fichero
	struct STAT stat;
	if(mi_stat(camino, &stat) < 0){
		fprintf(stderr, "verificacion.c --> No se ha podido obtener STAT\n");
		return -1;
	}

	// Calculamos el número  de entradas a paritr del tamEnbytesLog
	int numentradas=stat.tamEnBytesLog/sizeof(struct entrada);
	if(numentradas != NUMPROCESOS){
		fprintf(stderr, "verificacion.c --> No aparecen 100 entradas\n");
		return -1;
	}

	fprintf(stderr, "numentradas: %d NUMPROCESOS: %d\n", numentradas, NUMPROCESOS);
	
	
	//Creamos el fichero informe.txt dentro del diretorio de simulación
	char informe[100];
	char aux[15];//para guardar informe.txt
	strcpy(informe,camino);
	sprintf(aux,"informe.txt");
	strcat(informe,aux);

	if(mi_creat(informe, '7') < 0){
		fprintf(stderr, "verificacion.c --> EL Informe no se ha podido crear\n");
		return -1;
	}
	//leemos los directorios correspondientes a los procesos
	unsigned char buffer_ent[NUMPROCESOS*sizeof(struct entrada)];
	//inicializamos el buffer
	memset(buffer_ent, 0, NUMPROCESOS*sizeof(struct entrada));
	struct entrada *entrada;
	entrada=malloc(sizeof(struct entrada));

	//Leemos las 100 entradas que tiene que tener el fichero y las guardamos en un buffer de tamaño
	// NUMPROCESOS*sizeof(strcut entrada) así no tenemos que acceer para cada una al dispositivo, ya que vamos a leer del buffer directamente
	if(mi_read(camino, &buffer_ent, 0, sizeof(struct entrada)*numentradas) < 0){
		fprintf(stderr, "verificacion.c --> Lectura de entradas incorrecta\n");
		return -1;
	}
	char *str;
	unsigned int pid;

	char prueba[100];
	unsigned int bytesleidos = 1;

	char date [80];
	struct tm *ts;

	struct INFORMACION info;
	int cant_registros_buffer_escrituras = 256;
	struct REGISTRO buffer_escrituras [cant_registros_buffer_escrituras];
	char buffer_esc[BLOCKSIZE];
	int off_info=0;

	for(int i=0;i<numentradas;i++){
		//leemos una entrada del buffer y lo guardamos en enteada
		memcpy(entrada,i*sizeof(struct entrada)+buffer_ent,sizeof(struct entrada));
		//extraemos el PID
		str=strchr(entrada->nombre,'_')+1; //nos interesa lo que esta inmediatamente después del _
		pid=atoi(str); //lo pasamos a entero
		strcpy(prueba,camino);
		strcat(prueba,entrada->nombre);
		sprintf(aux,"/prueba.dat");//enviamos la cadena formateada a aux
		strcat(prueba,aux);

		int j=0;
		int contador=0;
		while(bytesleidos>0 && contador<50){
			memset(&buffer_escrituras,0,sizeof(buffer_escrituras));
			//leemos las entradas del fichero
			if((bytesleidos=mi_read(prueba, &buffer_escrituras, j*sizeof(struct REGISTRO), sizeof(buffer_escrituras))) < 0){
				fprintf(stderr, "verificacion.c --> Lectura de entrada incorrecta.\n");
				return -1;
			}
			//leemos los registros del buffer de escritura
			for (int n = 0; n < cant_registros_buffer_escrituras; n++) {
				if(buffer_escrituras[n].pid==pid){
					if (contador==0){//Primera escritura
						info.PrimeraEscritura=buffer_escrituras[n];
						info.UltimaEscritura=buffer_escrituras[n];
						info.MayorPosicion=buffer_escrituras[n];
						info.MenorPosicion=buffer_escrituras[n];
					}
					if (buffer_escrituras[n].nEscritura<info.PrimeraEscritura.nEscritura){
						info.PrimeraEscritura=buffer_escrituras[n];
					}
					if (info.UltimaEscritura.nEscritura<buffer_escrituras[n].nEscritura){
						info.UltimaEscritura=buffer_escrituras[n];
					}
					if (buffer_escrituras[n].nRegistro<info.MenorPosicion.nRegistro){
						info.MenorPosicion=buffer_escrituras[n];
					}
					if (info.MayorPosicion.nRegistro<buffer_escrituras[n].nRegistro){
						info.MayorPosicion=buffer_escrituras[n];
					}
					contador++;//incrementamos escrituras
				}
			}
			j+=cant_registros_buffer_escrituras;
		}
		memset(buffer_esc, 0, BLOCKSIZE);  //limpiamos buffer de escrituras

		//Calculamos número de escrituras
		sprintf(buffer_esc, "\nPID: %u\n", pid);
		sprintf(buffer_esc + strlen(buffer_esc), "Numero escrituras: %d\n", contador);

		//Primera escritura
		ts = localtime(&info.PrimeraEscritura.fecha);
		strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", ts);
		sprintf(buffer_esc + strlen(buffer_esc), "Primera Escritura\t%u\t%u\t%s\n", info.PrimeraEscritura.nEscritura, info.PrimeraEscritura.nRegistro, date);

		//ultima escritura
		ts = localtime(&info.UltimaEscritura.fecha);
		strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", ts);
		sprintf(buffer_esc + strlen(buffer_esc), "Última Escritura\t%u\t%u\t%s\n", info.UltimaEscritura.nEscritura, info.UltimaEscritura.nRegistro, date);

		//menor posición
		ts = localtime(&info.MenorPosicion.fecha);
		strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", ts);
		sprintf(buffer_esc + strlen(buffer_esc), "Menor Posición\t\t%u\t%u\t%s\n", info.MenorPosicion.nEscritura, info.MenorPosicion.nRegistro, date);

		//mayor posición
		ts = localtime(&info.MayorPosicion.fecha);
		strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S", ts);
		sprintf(buffer_esc + strlen(buffer_esc), "Mayor Posición\t\t%u\t%u\t%s\n", info.MayorPosicion.nEscritura, info.MayorPosicion.nRegistro, date);

		//escribimimos informe.txt
		if(mi_write(informe, buffer_esc, off_info, strlen(buffer_esc))< 0){
			fprintf(stderr, "Error escribiendo informe.txt");
			return 0;
		}
		//redireccionamos salida estandar
		write(1, buffer_esc, strlen(buffer_esc));
		off_info+=strlen(buffer_esc);
	}
	//desmontamos fichero
	bumount();
}
