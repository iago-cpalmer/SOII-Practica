//Autores: Joan Català, Alberto Ruiz y Iago Caldentey Palmer

#include "ficheros_basico.h"

struct superbloque SB;

// Calcula y devuelve el tamaño del mapa de bits (en bloques)
int tamMB(unsigned int nbloques)
{
  int size = (nbloques / 8) / BLOCKSIZE;
  int resto = (nbloques / 8) % BLOCKSIZE;
  if (resto != 0)
  {
    size++;
  }
  return size;
}

// Calcula y devuelve el tamaño del array de inodos (en bloques)
int tamAI(unsigned int ninodos)
{
  // int ninodos = nbloques/4; --> El programa mi_mkfs.c le pasará este dato a esta función como parámetro al llamarla
  int size = (ninodos * INODOSIZE) / BLOCKSIZE;
  int resto = (ninodos * INODOSIZE) % BLOCKSIZE;
  if (resto != 0)
  {
    size++;
  }
  return size;
}

// Inicializa los datos del superbloque y lo escribe en el FS
int initSB(unsigned int nbloques, unsigned int ninodos)
{
  SB.posPrimerBloqueMB = posSB + tamSB;
  SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;
  SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
  SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
  SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
  SB.posUltimoBloqueDatos = nbloques - 1;
  SB.posInodoRaiz = 0;
  SB.posPrimerInodoLibre = 0;      
  SB.cantBloquesLibres = nbloques; 
  SB.cantInodosLibres = ninodos;   
  SB.totBloques = nbloques;
  SB.totInodos = ninodos;
  for (size_t i = 0; i < INODOSIZE - 2 * sizeof(unsigned char) - 3 * sizeof(time_t) - 18 * sizeof(unsigned int) - 6 * sizeof(unsigned char); i++)
  {
    SB.padding[i] = 0;
  }
  return bwrite(posSB, &SB);
}

// Inicializa el mapa de bits (todos a 0)
int initMB()
{
  unsigned char buffer[BLOCKSIZE];
  memset(buffer, '\0', BLOCKSIZE);
  // Leemos superbloque para obtener las posiciones de los datos
  if (bread(0, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c initMB() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
  for (size_t i = SB.posPrimerBloqueMB; i <= SB.posUltimoBloqueMB; i++)
  {
    if (bwrite(i, buffer) == -1)
    {
      fprintf(stderr, "Error en ficheros_basico.c initMB() --> %d: %s\n", errno, strerror(errno));
      return -1;
    }
  }

  // Actualizamos mapa de bits
  int bloquesLibres = SB.cantBloquesLibres;
  escribir_bit(posSB, 1);
  bloquesLibres--;

  for (int i = SB.posPrimerBloqueMB; i <= SB.posUltimoBloqueMB; i++)
  {
    escribir_bit(i, 1);
    bloquesLibres--;
  }
  for (int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++)
  {
    escribir_bit(i, 1);
    bloquesLibres--;
  }

  SB.cantBloquesLibres = bloquesLibres;

  // Actualizamos el superbloque
  if (bwrite(0, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c initMB() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }

  return 0;
}

// Inicializa la lista enlazada de inodos
int initAI()
{
  struct inodo inodos[BLOCKSIZE / INODOSIZE];
  // Leemos superbloque para obtener las posiciones de los datos
  if (bread(0, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c initAI() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
  unsigned int contInodos = SB.posPrimerInodoLibre + 1;
  for (size_t i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++)
  {
    for (size_t j = 0; j < BLOCKSIZE / INODOSIZE; j++)
    {
      inodos[j].tipo = 'l'; // libre
      if (contInodos < SB.totInodos)
      {
        inodos[j].punterosDirectos[0] = contInodos;
        contInodos++;
      }
      else
      {
        inodos[j].punterosDirectos[0] = UINT_MAX;
      }
    }
    // Escribimos el bloque de inodos
    if (bwrite(i, inodos) == -1)
    {
      fprintf(stderr, "Error en ficheros_basico.c initAI() --> %d: %s\n", errno, strerror(errno));
      return -1;
    }
  }
  if (bwrite(0, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c initAI() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
  return 0;
}

// Permite cambiar el valor de un bit en el mapa de bits
int escribir_bit(unsigned int nbloque, unsigned int bit)
{
  unsigned char mascara = 128; // 10000000
  if (bread(0, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c escribir_bit() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }

  // Calculamos la posición del byte en el MB, posbyte, y la posición del bit dentro de ese byte, posbit
  unsigned int posbyte = nbloque / 8;
  unsigned int posbit = nbloque % 8;

  // Hemos de determinar luego en qué bloque del MB, nbloqueMB, se halla ese bit para leerlo
  unsigned int nbloqueMB = posbyte / BLOCKSIZE;

  // Y finalmente hemos de obtener en qué posición absoluta del dispositivo virtual
  // se encuentra ese bloque, nbloqueabs, donde leer/escribir el bit
  unsigned int nbloqueabs = nbloqueMB + SB.posPrimerBloqueMB;

  posbyte = posbyte % BLOCKSIZE;
  unsigned char bufferMB[BLOCKSIZE];
  memset(bufferMB, '\0', BLOCKSIZE);
  bread(nbloqueabs, &bufferMB);
  mascara >>= posbit; // desplazamiento de bits a la derecha
  if (bit == 1)
  {
    bufferMB[posbyte] |= mascara; // operador OR para bits
  }
  else
  {
    bufferMB[posbyte] &= ~mascara; // operadores AND y NOT para bits
  }
  return bwrite(nbloqueabs, bufferMB);
}

// Permite leer el valor de un bit en el mapa de bits
unsigned char leer_bit(unsigned int nbloque)
{
  unsigned char mascara = 128; // 10000000
  if (bread(0, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c leer_bit() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
  // Calculamos la posición del byte en el MB, posbyte, y la posición del bit dentro de ese byte, posbit
  unsigned int posbyte = nbloque / 8;
  unsigned int posbit = nbloque % 8;

  // Hemos de determinar luego en qué bloque del MB, nbloqueMB, se halla ese bit para leerlo
  unsigned int nbloqueMB = posbyte / BLOCKSIZE;

  // Y finalmente hemos de obtener en qué posición absoluta del dispositivo virtual
  // se encuentra ese bloque, nbloqueabs, donde leer/escribir el bit
  unsigned int nbloqueabs = nbloqueMB + SB.posPrimerBloqueMB;

  posbyte = posbyte % BLOCKSIZE;
  unsigned char bufferMB[BLOCKSIZE];
  memset(bufferMB, '\0', BLOCKSIZE);
  if (bread(nbloqueabs, &bufferMB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c leer_bit() --> %d: %s\nImposible leer el bloque %d", errno, strerror(errno), nbloqueabs);
    return -1;
  }

  mascara >>= posbit;           // Desplazamiento de bits a la derecha
  mascara &= bufferMB[posbyte]; // AND para bits
  mascara >>= (7 - posbit);     // Desplazamiento de bits a la derecha, ahora el bit leido està
  // al final de la mascara
  return mascara; // Devolvemos el bit leido
}

// Reserva un bloque dentro del sistema de ficheros
int reservar_bloque()
{
  // Leemos el Super Bloque
  if (bread(posSB, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c reservar_bloque() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
  // Si no hay bloques libres, acaba con -1
  if (SB.cantBloquesLibres == 0)
  {
    fprintf(stderr, "Error en ficheros_basico.c reservar_bloque() --> %d: %s\nImposible reservar bloque, no quedan libres!\n", errno, strerror(errno));
    return -1;
  }
  unsigned int posBloqueMB = SB.posPrimerBloqueMB;
  unsigned char bufferMB[BLOCKSIZE]; // Buffer de lectura MB
  memset(bufferMB, '\0', BLOCKSIZE);
  unsigned char bufferAUX[BLOCKSIZE]; // Buffer auxiliario (todos 1)
  memset(bufferAUX, 255, BLOCKSIZE);
  int verificar = 1;
  int bit = 0;
  unsigned char mascara = 128;

  // Recorremos los bloques del MB hasta encontrar uno que esté a 0
  for (; posBloqueMB <= SB.posUltimoBloqueMB && verificar > 0 && bit == 0; posBloqueMB++)
  {
    verificar = bread(posBloqueMB, bufferMB);
    bit = memcmp(bufferMB, bufferAUX, BLOCKSIZE);
  }
  // Una vez encontrado, vamos atràs de 1
  posBloqueMB--;

  if (verificar > 0)
  {
    for (int posbyte = 0; posbyte < BLOCKSIZE; posbyte++)
    {
      if (bufferMB[posbyte] < 255)
      {
        int posbit = 0;
        while (bufferMB[posbyte] & mascara)
        {
          posbit++;
          bufferMB[posbyte] <<= 1; // Desplazamos bits a la izquierda
        }
        // Ahora posbit contiene el bit = 0
        int nbloque = ((posBloqueMB - SB.posPrimerBloqueMB) * BLOCKSIZE + posbyte) * 8 + posbit;
        escribir_bit(nbloque, 1);                        // Marcamos el bloque como reservado
        SB.cantBloquesLibres = SB.cantBloquesLibres - 1; // Actualizamos cantidad de bloques libres en el SB
        // Guardamos el SB
        if (bwrite(posSB, &SB) == -1)
        {
          fprintf(stderr, "Error en ficheros_basico.c reservar_bloque() --> %d: %s\n", errno, strerror(errno));
          return -1;
        }
        // Grabamos un buffer de 0s en la pos. nbloque
        unsigned char bufferVacio[BLOCKSIZE]; // Buffer de ceros
        memset(bufferVacio, '\0', BLOCKSIZE);
        if (bwrite(nbloque, &bufferVacio) == -1)
        {
          fprintf(stderr, "Error en ficheros_basico.c reservar_bloque() --> %d: %s\nImposible escribir buffer vacio", errno, strerror(errno));
          return -1;
        }
        return nbloque;
      }
    }
  }
  fprintf(stderr, "Error en ficheros_basico.c reservar_bloque() --> %d: %s\nNo es valido", errno, strerror(errno));
  return -1;
}

// Libera el bloque pasado por paràmetro
int liberar_bloque(unsigned int nbloque)
{
  // Primero escribimos un 0 en el bloque pasado como parámetro
  if (escribir_bit(nbloque, 0) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c liberar_bloque() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
  // Después leemos el Super Bloque
  if (bread(0, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c liberar_bloque() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
  // Sumamos una unidad a la cantidad de bloques libres
  SB.cantBloquesLibres = SB.cantBloquesLibres + 1;
  // Guardamos el SB
  if (bwrite(0, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c liberar_bloque() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
  // Devolvemos el nº de bloque liberado
  return nbloque;
}

int escribir_inodo(unsigned int ninodo, struct inodo inodo)
{
  // Escribe el contenido de una variable del tipo struct inodo en un determinado inodo del array de inodos
  if (bread(0, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c escribir_inodo() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
  unsigned int posInodo = SB.posPrimerBloqueAI + ((ninodo * INODOSIZE) / BLOCKSIZE);
  struct inodo inodos[BLOCKSIZE / INODOSIZE];
  bread(posInodo, inodos); // Leemos el bloque del inodo
  memcpy(&inodos[ninodo % (BLOCKSIZE / INODOSIZE)], &inodo, INODOSIZE);

  return bwrite(posInodo, inodos);
}

// Lee y devuelve el inodo especificado
int leer_inodo(unsigned int ninodo, struct inodo *inodo)
{
  if (bread(0, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c escribir_inodo() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
  unsigned int posInodo = SB.posPrimerBloqueAI + (ninodo * INODOSIZE) / BLOCKSIZE;
  struct inodo inodos[BLOCKSIZE / INODOSIZE];
  int comprobar = bread(posInodo, inodos);
  *inodo = inodos[ninodo % (BLOCKSIZE / INODOSIZE)];
  return (comprobar < 0) ? -1 : 0;
}

// Permite reservar un nuevo inodo y asignarle tipo y permisos
int reservar_inodo(unsigned char tipo, unsigned char permisos)
{
  // encuentra el primer idondo libre (dato alamcenado en el SB), lo reserva (con
  // la ayuda de la funcuion escribir_inodo(), devuelve su numero y actualiza la
  // lista de inodos libres)
  if (bread(0, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c reservar_inodo() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
  // Si no hay inodos libres, error
  if (SB.cantInodosLibres == 0)
  {
    fprintf(stderr, "Error en ficheros_basico.c reservar_inodo() --> %d: %s\nNingún inodo libre!", errno, strerror(errno));
    return -1;
  }

  int posInodoReservado = SB.posPrimerInodoLibre;
  struct inodo in;
  leer_inodo(posInodoReservado, &in);

  // Actualización superbloque
  SB.posPrimerInodoLibre = in.punterosDirectos[0];
  SB.cantInodosLibres = SB.cantInodosLibres - 1;
  if (bwrite(posSB, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c reservar_inodo() --> %d: %s\nNo se ha podido actualizar el SB!", errno, strerror(errno));
    return -1;
  }

  in.tipo = tipo;
  in.permisos = permisos;
  in.nlinks = 1;
  in.tamEnBytesLog = 0;
  in.atime = time(NULL);
  in.ctime = time(NULL);
  in.mtime = time(NULL);
  in.numBloquesOcupados = 0;
  memset(in.punterosDirectos, 0, sizeof(in.punterosDirectos));
  memset(in.punterosIndirectos, 0, sizeof(in.punterosIndirectos));
  escribir_inodo(posInodoReservado, in);

  return posInodoReservado;
}

// Devuelve el rango de bloques lògicos
int obtener_nrangoBL(struct inodo *inodo, unsigned int nblogico, unsigned int *ptr)
{
  if (nblogico < DIRECTOS)
  {
    *ptr = inodo->punterosDirectos[nblogico];
    return 0;
  }
  else if (nblogico < INDIRECTOS0)
  {
    *ptr = inodo->punterosIndirectos[0];
    return 1;
  }
  else if (nblogico < INDIRECTOS1)
  {
    *ptr = inodo->punterosIndirectos[1];
    return 2;
  }
  else if (nblogico < INDIRECTOS2)
  {
    *ptr = inodo->punterosIndirectos[2];
    return 3;
  }
  else
  {
    *ptr = 0;
    fprintf(stderr, "Error en ficheros_basico.c obtener_nrangoBL()\nBloque lógico fuera de rango --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
}

// Permite obtener el indice de un bloque en funciòn de su nivel
int obtener_indice(unsigned int nblogico, unsigned int nivel_punteros)
{
  if (nblogico < DIRECTOS)
  {
    return nblogico;
  }
  else if (nblogico < INDIRECTOS0)
  {
    return nblogico - DIRECTOS;
  }
  else if (nblogico < INDIRECTOS1)
  {
    if (nivel_punteros == 2)
    {
      return (nblogico - INDIRECTOS0) / NPUNTEROS;
    }
    else if (nivel_punteros == 1)
    {
      return (nblogico - INDIRECTOS0) % NPUNTEROS;
    }
  }
  else if (nblogico < INDIRECTOS2)
  {
    if (nivel_punteros == 3)
    {
      return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
    }
    else if (nivel_punteros == 2)
    {
      return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;
    }
    else if (nivel_punteros == 1)
    {
      return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS;
    }
  }
  fprintf(stderr, "Error en ficheros_basico.c obtener_indice() --> %d: %s\n", errno, strerror(errno));
  return -1;
}

int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, char reservar)
{
  // Esta función se encarga de obtener el nº de bloque físico correspondiente a
  // un bloque lógico determinado del inodo indicado. Enmascara la gestión de los
  // diferentes rangos de punteros directos e indirectos del inodo, de manera que
  // funciones externas no tienen que preocuparse de cómo acceder a los bloques
  // físicos apuntados desde el inodo.
  struct inodo inodo;
  unsigned int ptr, ptr_ant, salvar_inodo, nRangoBL, nivel_punteros, indice;
  int buffer[NPUNTEROS];
  if (leer_inodo(ninodo, &inodo) == -1)
    return -1;
  ptr = 0, ptr_ant = 0, salvar_inodo = 0;
  nRangoBL = obtener_nrangoBL(&inodo, nblogico, &ptr);
  nivel_punteros = nRangoBL;
  while (nivel_punteros > 0)
  {
    if (ptr == 0)
    {
      if (reservar == 0)
      {
        return -1;
      }
      else
      {
        salvar_inodo = 1;

        if ((ptr = reservar_bloque()) == -1)
        {
          fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nptr ==-1", errno, strerror(errno));
          return -1;
        }
        inodo.numBloquesOcupados++;
        inodo.ctime = time(NULL);
        if (nivel_punteros == nRangoBL)
        {
          inodo.punterosIndirectos[nRangoBL - 1] = ptr;
          #if DEBUGN1 == 1
            fprintf(stderr, "[traducir_bloque_inodo()--> inodo.punterosIndirectos[%d] = %d (reservado BF %d para punteros_nivel%d)]\n", nRangoBL - 1, ptr, ptr, nivel_punteros);
          #endif
        }
        else
        {

          buffer[indice] = ptr;
          if (bwrite(ptr_ant, buffer) == -1)
          {
            fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nerror en bwrite", errno, strerror(errno));
            return -1;
          }
#if DEBUGN1 == 1
          fprintf(stderr, "[traducir_bloque_inodo()--> punteros_nivel%d[%d] = %d (reservado BF %d para punteros_nivel%d)]\n",
                 nivel_punteros + 1, indice, ptr, ptr, nivel_punteros);
#endif
        }
      }
      //
    }
    if (bread(ptr, buffer) == -1)
    {
      fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nerror en bread", errno, strerror(errno));
      return -1;
    }
    if ((indice = obtener_indice(nblogico, nivel_punteros)) == -1)
    {
      fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nobtenerindice=-1", errno, strerror(errno));
      return -1;
    }
    ptr_ant = ptr;
    ptr = buffer[indice];
    nivel_punteros--;
  } // al salir de este bucle ya estamos al nivel de datos
  if (ptr == 0)
  {
    if (reservar == 0)
    {
      // fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> reservar = 0 (error de lectura bloque)\n");
      return -1; // error lectura ∄ bloque
    }
    else
    {
      salvar_inodo = 1;
      if ((ptr = reservar_bloque()) == -1)
      {
        fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nptr=reservarbloque", errno, strerror(errno));
        return -1;
      } // de datos
      inodo.numBloquesOcupados++;
      inodo.ctime = time(NULL);
      if (nRangoBL == 0)
      {
        inodo.punterosDirectos[nblogico] = ptr; //
#if DEBUGN1 == 1
        printf("[traducir_bloque_inodo()→ inodo.punterosDirectos[%d] = %d (reservado BF %d para BL %d)]\n", nblogico, ptr, ptr, nblogico);
#endif
      }
      else
      {
        buffer[indice] = ptr; // (imprimirlo)
#if DEBUGN1 == 1
        printf("[traducir_bloque_inodo()→ inodo.punteros_nivel1[%d] = %d (reservado BF %d para BL %d)\n]", indice, ptr, ptr, nblogico);
#endif
        if (bwrite(ptr_ant, buffer) == -1)
        {
          fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nbwrite ptr_ant", errno, strerror(errno));
          return -1;
        }
      }
    }
  }
  if (salvar_inodo == 1)
  {
    if (escribir_inodo(ninodo, inodo) == -1)
    {
      fprintf(stderr, "Error en ficheros_basico.c traducir_bloque_inodo--> %d: %s\nescribirinodo", errno, strerror(errno));
      return -1;
    } // sólo si lo hemos actualizado
  }

  return ptr; // nbfisico
}
// Permite liberar un inodo reservado
int liberar_inodo(unsigned int ninodo)
{
  struct superbloque SB;
  struct inodo inodo;
  // Llamar a la función auxiliar liberar_bloques_inodo() para liberar todos los bloques del inodo
  int bLiberados = liberar_bloques_inodo(ninodo, 0);
  // Leer el inodo actualizado
  leer_inodo(ninodo, &inodo);

  // A la cantidad de bloques ocupados del inodo se le restará
  // la cantidad de bloques liberados por esta función y debería ser 0
  //fprintf(stderr, "NumBlOcupados es: %d\n", inodo.numBloquesOcupados);
  //fprintf(stderr, "bLiberados es: %d\n", bLiberados);
  if (inodo.numBloquesOcupados - bLiberados == 0)
  { 
    // Marcar el inodo como tipo libre
    inodo.tipo = 'l';
    inodo.numBloquesOcupados = 0;
    inodo.tamEnBytesLog = 0;
  }
  else
  {
    fprintf(stderr, "Error en ficheros_basico.c liberar_inodo()\n El nùmero de bloques ocupados por el inodo liberado y los bloques liberados no son los mismos! Error --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
  if (bread(posSB, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c liberar_inodo() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
  inodo.punterosDirectos[0] = SB.posPrimerInodoLibre;
  inodo.tipo = 'l';
  SB.posPrimerInodoLibre = ninodo;
  SB.cantInodosLibres = SB.cantInodosLibres + 1;
  escribir_inodo(ninodo, inodo);
  if (bwrite(posSB, &SB) == -1)
  {
    fprintf(stderr, "Error en ficheros_basico.c liberar_inodo() --> %d: %s\n", errno, strerror(errno));
    return -1;
  }
  return ninodo;
}

// Libera los bloques que ocupa un inodo
int liberar_bloques_inodo(unsigned int ninodo, unsigned int nblogico)
{
  // Variables
  struct inodo inodo;
  unsigned int nRangoBL, nivel_punteros, indice, ptr, nblog, ultimoBL, tamInodo;
  int bloques_punteros[3][NPUNTEROS];
  int indices[3];
  int ptr_nivel[3];
  int liberados, salvar_inodo;
  int nr = 0, nw=0;
  unsigned char bufferAUX[BLOCKSIZE];
  memset(bufferAUX, 0, BLOCKSIZE);

  // Procedimiento
  liberados = 0;
  salvar_inodo = 0;
  leer_inodo(ninodo, &inodo);
  tamInodo = inodo.tamEnBytesLog;
  if (tamInodo == 0)
  {
    return 0; // Fichero vacìo
  }
  // Obtenemos el ùltimo bloque lògico del inodo
  if (tamInodo % BLOCKSIZE == 0)
  {
    ultimoBL = tamInodo / BLOCKSIZE - 1;
  }
  else
  {
    ultimoBL = tamInodo / BLOCKSIZE;
  }
  ptr = 0;
#if DEBUGN1 == 1
  fprintf(stderr, "[liberar_bloques_inodo()→ primer BL: %d, útlimo BL: %d\n", nblogico, ultimoBL);
#endif

  // Recorrido BLs
  for (nblog = nblogico; nblog <= ultimoBL; nblog++)
  {
    nRangoBL = obtener_nrangoBL(&inodo, nblog, &ptr);
    if (nRangoBL < 0)
    {
      fprintf(stderr, "Error en ficheros_basico.c liberar_bloques_inodo() --> %d: %s\n", errno, strerror(errno));
      return -1;
    }
    nivel_punteros = nRangoBL; // El nivel_punteros +alto cuelga del inodo
    while (ptr > 0 && nivel_punteros > 0)
    {
      bread(ptr, bloques_punteros[nivel_punteros - 1]);
      nr++;
      indice = obtener_indice(nblog, nivel_punteros);
      ptr_nivel[nivel_punteros - 1] = ptr;
      indices[nivel_punteros - 1] = indice;
      ptr = bloques_punteros[nivel_punteros - 1][indice];
      nivel_punteros--;
    }
    if (ptr > 0)
    {
      liberar_bloque(ptr);
    #if DEBUGN1 == 1
      fprintf(stderr, "[liberar_bloques_inodo()→ liberado BF %d de datos para BL: %d\n", ptr, nblog);
    #endif
      liberados++;
      if (nRangoBL == 0)
      {
        inodo.punterosDirectos[nblog] = 0;
        salvar_inodo = 1;
      }
      else
      {
        while (nivel_punteros < nRangoBL)
        {
          indice = indices[nivel_punteros];
          bloques_punteros[nivel_punteros][indice] = 0;
          ptr = ptr_nivel[nivel_punteros];
          if (memcmp(bloques_punteros[nivel_punteros], bufferAUX, BLOCKSIZE) == 0)
          {
            // No cuelgan bloques ocupados, hay que liberar el bloque de punteros
            liberar_bloque(ptr);
            liberados++;
            nivel_punteros++;
             #if DEBUGN1 == 1
              fprintf(stderr, "[liberar_bloques_inodo()→ liberado BF %d de punteros_nivel%d correspondiente al BL: %d\n", 
               ptr, nivel_punteros, nblog);
            #endif

            if (nivel_punteros == nRangoBL)
            {
              inodo.punterosIndirectos[nRangoBL - 1] = 0;
              salvar_inodo = 1;
            }
          }
          else
          {
            // Escribimos en el dispositivo el bloque de punteros modificado
            bwrite(ptr, bloques_punteros[nivel_punteros]);
            nw++;
            #if DEBUGN1 == 1
              fprintf(stderr, "[liberar_bloques_inodo()→ salvado bloque punteros_nivel%d correspondiente al BL %d\n", 
              nivel_punteros + 1, nblog);
            #endif
            nivel_punteros = nRangoBL; // Salimos del bucle

          }
        }
      }
    } 
  
  }
  if (salvar_inodo == 1)
  {
    escribir_inodo(ninodo, inodo);
  }
  #if DEBUGN1 == 1
  fprintf(stderr, "[liberar_bloques_inodo()→ total bloques liberados: %d, total_breads: %d, total_bwrites: %d", liberados, nr, nw);
  #endif
  return liberados;
}
