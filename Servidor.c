#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <mysql.h>
#include<pthread.h>

typedef struct{
	char nombre [20];
	int socket;
	int partida;
	int apuesta;
}Tconectado;

typedef struct{
	Tconectado conectados [100];
	int num;
}TlistaConectados;

typedef struct{
	int idpartida;
	Tconectado jugadores[4];
	int num;
}TPartida;
typedef struct{
	TPartida partidas[100];
	int num;
}TListaPartidas;




TlistaConectados lista;
TPartida partida;
TListaPartidas listapartidas;

char invitador [20];
int partidasenjuego;

char conectados[100];
char respuesta[512];
char consulta[500];
char nom[512];


int i, err, contador;
int conectado = 0;
int carta = 0;
int cartas[48];
MYSQL *conn;
MYSQL_RES *resultado;
MYSQL_ROW row;
int sockets[100];
int num_sockets;

//Estructura necesaria para acceso excluyente
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int crearpar (TListaPartidas *lpartidas, int id, char nombre [20], int socket){
	if (lpartidas->num >100){
		printf("No se pueden anadir mas partidas\n");
		return -1;
	}
	
	else{
		lpartidas->partidas[lpartidas->num].idpartida = id;
		lpartidas->partidas[lpartidas->num].jugadores[0].socket = socket;
		strcpy(lpartidas->partidas[lpartidas->num].jugadores[0].nombre, nombre);
		lpartidas->partidas[lpartidas->num].num = 1;
		lpartidas->num++;
		return id;
	}
}
int addjug (TListaPartidas *lpartidas, int id, char nombre[20], int socket){
	int i = 0;
	int enc = 0;
	while (i<lpartidas->num && enc == 0){
		if (lpartidas->partidas[i].idpartida == id)
			enc = 1;
		else
			i++;
	}
	if (enc == 1){
		lpartidas->partidas[i].jugadores[lpartidas->partidas[i].num].socket = socket;
		strcpy(lpartidas->partidas[i].jugadores[lpartidas->partidas[i].num].nombre,nombre);
		lpartidas->partidas[i].num++;
		return 0;
	}
	else
		return -1;
}
void refjug(TListaPartidas *lpartidas, int id, int pet, char username [20]){
	int i = 0;
	int enc = 0;
	while (i<lpartidas->num && enc == 0){
		if (lpartidas->partidas[i].idpartida == id)
			enc = 1;
		else
			i++;
	}
	if (enc == 1){
		int a = 0;
		if (lpartidas->partidas[i].num > 0){
		sprintf(respuesta, "8:%d", lpartidas->partidas[i].num);
		while (a<lpartidas->partidas[i].num){
			strcat(respuesta, "/");
			strcat(respuesta, lpartidas->partidas[i].jugadores[a].nombre);
			a++;
		}
		a = 0;
		if (pet == 0)
			sprintf(respuesta, "%s/%s", respuesta, username);
		printf ("Respuesta: %s\n", respuesta);
		while (a<lpartidas->partidas[i].num){
			write(lpartidas->partidas[i].jugadores[a].socket, respuesta, strlen(respuesta));
			a++;
		}
		memset(respuesta,'\0',strlen(respuesta));
		}
		else{
			lpartidas->num--;
			if (i<lpartidas->num){
			while (a<lpartidas[i+1].num){
				lpartidas->partidas[i].idpartida = lpartidas->partidas[i+1].idpartida;
				strcpy(lpartidas->partidas[i].jugadores[a].nombre, lpartidas->partidas[i+1].jugadores[a].nombre);
				lpartidas->partidas[i].jugadores[a].socket =lpartidas->partidas[i+1].jugadores[a].socket;
				a++;
			}
			lpartidas->partidas[i].num = a;
			}
		}
	}
}
void EnviarApuesta(TListaPartidas *lpartidas, int id, int apuesta, char username[20]){
	int i = 0;
	int enc = 0;
	while (i<lpartidas->num && enc == 0){
		if (lpartidas->partidas[i].idpartida == id)
			enc = 1;
		else
			i++;
	}
	
	if (enc == 1){
		enc = 0;
		int a = 0;
		while(a<lpartidas->partidas[i].num && enc == 0){
			if (strcmp(lpartidas->partidas[i].jugadores[a].nombre, username)==0){
				lpartidas->partidas[i].jugadores[a].apuesta = apuesta;
				enc = 1;
			}
			else
				a++;
		}
		a = 0;
		sprintf(respuesta, "15:%d/%s", apuesta, username);
		printf("Respuesta: %s\n", respuesta);
		while (a<lpartidas->partidas[i].num){
			if (strcmp(lpartidas->partidas[i].jugadores[a].nombre, username)!=0)
				write(lpartidas->partidas[i].jugadores[a].socket, respuesta, strlen(respuesta));
			a++;
		}
		memset(respuesta,'\0',strlen(respuesta));
	}
}
int PosPartida(TListaPartidas *lpartidas, int id){
	int i = 0;
	int enc = 0;
	while (i<lpartidas->num && enc == 0){
		if (lpartidas->partidas[i].idpartida == id)
			enc = 1;
		else
			i++;
	}
	if (enc == 0)
		return -1;
	else
		return i;
	}
int borrjug (TListaPartidas *lpartidas, int id, char nombre [20]){
	int i = 0;
	int enc = 0;
	while (i<lpartidas->num && enc == 0){
		if (lpartidas->partidas[i].idpartida == id)
			enc = 1;
		else
			i++;
	}
	if (enc == 1){
		int a = 0;
		enc = 0;
		while (a < lpartidas->partidas[i].num && enc == 0){
			if (strcmp(lpartidas->partidas[i].jugadores[a].nombre, nombre)==0)
				enc = 1;
			else
				a++;
		}
		if (enc == 1){
		lpartidas->partidas[i].num--;
		while (a<lpartidas->partidas[i].num){
			lpartidas->partidas[i].jugadores[a].socket = lpartidas->partidas[i].jugadores[a+1].socket;
			strcpy(lpartidas->partidas[i].jugadores[a].nombre, lpartidas->partidas[i].jugadores[a+1].nombre);
			a++;
		}
		return 0;
		}
	}
	else
		return -1;
}
int addConectados (TlistaConectados *lista, char nombre[20], int socket){
	if (lista->num==100){
		printf("No se ha podido anadir porque la lista esta llena.\n");
		return-1;
	}
	
	else{
		int i = 0;
		int encontrado = 0;
		
		if (lista->num != 0){
			while ((i<lista->num) && (encontrado == 0))
			{
				if (strcmp(lista->conectados[i].nombre,nombre) == 0)
					encontrado = 1;
				else
					i++;
			}
		}
		
		if (encontrado == 0)
		{
			strcpy (lista->conectados[lista->num].nombre, nombre);
			lista->conectados[lista->num].socket=socket;
			lista->conectados[lista->num].partida=0;
			lista->num = lista->num +1;
			conectado = 1;
			return 0;
		}
		else
			return -2;
	}
	
}

int delConectados (TlistaConectados *lista, char nombre[20]){
	int i = 0;
	int encontrado = 0;
	
	if (lista->num == 0){
		printf("No se ha podido eliminar el usuario porque la lista esta vacia.\n");
		return -1;
	}
	
	else{
		while ((i < (lista->num)) && (encontrado == 0)) //primero busca al usuario en la lista y se guarda la posicio en i
		{
			if (strcmp(lista->conectados[i].nombre,nombre)==0)
				encontrado = 1; //usuario encontrado
			else
				i = i + 1;
		}
		if (encontrado == 1)
		{
			while (i < (lista->num - 1)) //mueve todos los usuarios que estan por encima de la posicion i
			{
				strcpy(lista->conectados[i].nombre,lista->conectados[i+1].nombre);
				lista->conectados[i].socket = lista->conectados[i+1].socket;
				i = i + 1;
			}
			
			strcpy ( lista->conectados[lista->num -1].nombre,"");
			lista->conectados[lista->num -1].socket = 0;
			lista->num = (lista->num -1);
			conectado = 1;
			return 0;
		}
		else
			return -2; //si no se ha encontrado al usuario a eliminar de la lista
	}
}


int DamePosicion(TlistaConectados *lista, char nombre[20]){
	
	int encontrado=0;
	int	i=0;
	while((encontrado==0) && (i<lista->num)){
		if(strcmp(lista->conectados[i].nombre, nombre)==0)
			encontrado=1;
		else i++;
	}
	if (encontrado==1)
		return i;
	else
		return -1;
}

int DameSocket(TlistaConectados *l, char nombre[20]){
	int posicion = DamePosicion(l, nombre);
	int socket = lista.conectados[posicion].socket;
	return socket;
}

void EnviarNotificacion(TlistaConectados lista, char respueta[512], int sockets[100], int num_sockets){//Envia la notificacion cada conectado/desconectado
	int i=0;
	char num_conectados [100];
	sprintf(num_conectados, "%d", lista.num);
	respuesta[0] = '7';
	respuesta[1] = ':';
	strcat(respuesta, num_conectados);
	while(i<lista.num){
		strcat(respuesta, "/");
		strcat(respuesta, lista.conectados[i].nombre);
		i = i + 1;
	}
	printf("Respuesta: %s\n", respuesta);
	i = 0;
	while (i<num_sockets)
	{
		write (sockets[i],respuesta, strlen(respuesta));
		i = i + 1;
	}
	memset(respuesta,'\0',strlen(respuesta));
}

void mejorJugador (char respuesta[512]){
	strcpy(consulta, "SELECT jugador.username FROM jugador WHERE jugador.ganadas = (SELECT MAX(ganadas) FROM jugador);");
	respuesta[0] = '3';
	respuesta[1] = ':';
	err=mysql_query (conn, consulta);
	
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		respuesta[2] = '2';
	}
	
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	
	if (row == NULL)
	{
		sprintf (respuesta,"No se han obtenido datos en la consulta\n");
		respuesta[2] = '3';
	}
	else{
		char res [512];
		while (row !=NULL) {
			sprintf (res,"%s\n", row[0]);
			row = mysql_fetch_row (resultado);
		}
		strcat(respuesta, res);
	}
}

void peorJugador (char respuesta[512]){
	strcpy(consulta, "SELECT jugador.username FROM jugador WHERE jugador.ganadas = (SELECT MIN(ganadas) FROM jugador);");
	respuesta[0] = '4';
	respuesta[1] = ':';
	err=mysql_query (conn, consulta);
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		respuesta[2] = '2';
	}
	
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	
	if (row == NULL)
	{
		sprintf (respuesta,"No se han obtenido datos en la consulta\n");
		respuesta[2] = '3';
	}
	
	else{
		char res [512];
		while (row !=NULL) {
			sprintf (res,"%s\n", row[0]);
			row = mysql_fetch_row (resultado);
		}
		strcat(respuesta, res);
	}
}

void partidaLarga (char respuesta[512]){
	strcpy(consulta, "SELECT partida.ganador FROM partida WHERE partida.duracion = (SELECT MAX(duracion) FROM partida);");
	respuesta[0] = '5';
	respuesta[1] = ':';
	err=mysql_query (conn, consulta);
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		respuesta[2] = '2';
	}
	
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	
	if (row == NULL)
	{
		sprintf (respuesta,"No se han obtenido datos en la consulta\n");
		respuesta [2] = '3';
	}
	
	else{
		char res [512];
		while (row !=NULL) {
			sprintf (res,"%s\n", row[0]);
			row = mysql_fetch_row (resultado);
		}
		strcat(respuesta, res);
	}
}

void partidaCorta (char respuesta[512]){
	strcpy(consulta, "SELECT partida.ganador FROM partida WHERE partida.duracion = (SELECT MIN(duracion) FROM partida);");
	respuesta[0] = '6';
	respuesta[1] = ':';
	err=mysql_query (conn, consulta);
	
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		respuesta[2] = '2';
	}
	
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	
	if (row == NULL)
	{
		sprintf (respuesta,"No se han obtenido datos en la consulta\n");
		respuesta[2] = '3';
	}
	
	else{
		char res [512];
		while (row !=NULL) {
			sprintf (res,"%s\n", row[0]);
			row = mysql_fetch_row (resultado);
		}
		strcat(respuesta, res);
	}
}


void registro (char username[20], char password[20], char respuesta[512], int socket){
	strcpy(consulta, "SELECT COUNT(username) FROM jugador WHERE jugador.username='");
	strcat(consulta, username);
	strcat(consulta, "' AND jugador.password= '");
	strcat(consulta, password);
	respuesta[0] = '1';
	respuesta[1] = ':';
	
	strcat(consulta, "';");
	err=mysql_query (conn, consulta);
	
	if (err!=0) {
		printf ("Error al consultar datos de la base de datos %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		respuesta[2] = '3';
	}
	else{
		resultado = mysql_store_result (conn);
		row = mysql_fetch_row (resultado);
		if (row == NULL)
		{
			printf ("No se han obtenido datos en la consulta\n");
			respuesta[2] = '3';
		}
		else{
			char numero1 [100];
			while (row !=NULL) {
				strcpy(numero1, row[0]);
				row = mysql_fetch_row (resultado);
			}
			int numero;
			numero = atoi(numero1);
			if(numero == 0){
				printf("Usuario logueado correctamente\n");
				char agregar[80];
				strcpy(agregar, "INSERT INTO jugador VALUES ('");
				strcat(agregar, username);
				strcat(agregar, "','");
				strcat(agregar, password);
				strcat(agregar, "',0,0,0);");
				err=mysql_query (conn, agregar);
				if (err!=0) {
					printf ("Error al consultar datos de la base de datos %u %s\n",
							mysql_errno(conn), mysql_error(conn));
					respuesta[2] = '2';
				}
				else{
					printf("%s\n", agregar);
					respuesta[2] = '0';
					strcat(respuesta, "/");
					strcat(respuesta, username);
					addConectados(&lista, username,socket);
				}
			}
			else{
				printf("Los datos introducidos no son correctos \n");
				respuesta[2] = '1';
			}
		}
	}
}

int DatedeBaja(char username [20]){
	//Sirve para darse de baja en la base de datos usando la consulta DELETE
	sprintf(consulta,"DELETE FROM jugador WHERE jugador.username = '%s'", username);
	
	int err;
	err=mysql_query (conn, consulta);
	
	if (err!=0)
	{
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
		return -1;
	}
	
	else{
		printf("Usuario eliminado correctamente\n");
		return 0;
	}
}
void *AtenderCliente (void *socket){
	
	setbuf(stdout, NULL);
	int sock_conn;
	int *s;
	s= (int *) socket;
	sock_conn= *s;
	
	char peticion[512];
	int ret;
	int ID;
	
	conn = mysql_init(NULL);
	if (conn==NULL) {
		printf ("Error al crear la conexion: %u %s\n", 
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	
	conn = mysql_real_connect (conn, "localhost","root", "mysql", "M8",0, NULL, 0);
	if (conn==NULL) {
		printf ("Error al inicializar la conexion: %u %s\n", 
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	
	int terminar = 0;
	while(terminar==0){
		printf("Esperando peticion\n");
		// Ahora recibimos la peticion
		ret=read(sock_conn,peticion, sizeof(peticion));
		printf ("Recibido\n");
		
		// Tenemos que anadirle la marca de fin de string 
		// para que no escriba lo que hay despues en el buffer
		peticion[ret]='\0';
		
		char username[20];
		char password[20];
		// vamos a ver que quieren
		char *p = strtok( peticion, "/");
		int codigo =  atoi (p);
		// Ya tenemos el codigo de peticion
		p = strtok( NULL, "/");
		
		
		if (codigo==0){	//Desconexion 	
			pthread_mutex_lock( &mutex );
			printf("Peticion 0: Desconectar usuario del servidor\n");
			strcpy (username, p);
			int del = delConectados(&lista,username);
			int i = 0;
			int enc = 0;
			while (i<num_sockets && enc == 0){
				if (sockets[i] == sock_conn)
					enc = 1;
				else
					i++;
			}
			if (enc == 1){
				while (i<num_sockets -1){
					sockets[i] == sockets[i+1];
					i++;
				}
				num_sockets--;
				conectado = 1;
				terminar = 1;
			}
			
			pthread_mutex_unlock(&mutex);
		}
		
		else if (codigo==1){	//Registro
			pthread_mutex_lock( &mutex );
			printf("Peticion 1: Registro\n");
			strcpy (username, p);
			p = strtok(NULL, "/");
			strcpy (password, p);
			printf("Usuario: %s Contrasena: %s\n", username, password);
			registro(username,password,respuesta, sock_conn);
			pthread_mutex_unlock( &mutex);
		}
		
		else if (codigo==2){	//Iniciar sesion
			pthread_mutex_lock( &mutex );
			printf("Peticion 2: Iniciar Sesion\n");
			strcpy (username, p);
			p = strtok(NULL, "/");
			strcpy (password, p);
			strcpy(consulta, "SELECT COUNT(username) FROM jugador WHERE jugador.username='");
			strcat(consulta, username);
			strcat(consulta, "' AND jugador.password= '");
			strcat(consulta, password);
			strcat(consulta, "';");
			respuesta[0] = '2';
			respuesta[1] = ':';
			
			err=mysql_query (conn, consulta);
			if (err!=0) {
				printf ("Error al consultar datos de la base %u %s\n",
						mysql_errno(conn), mysql_error(conn));
				respuesta[2] = '3';
			}
			else{
				resultado = mysql_store_result (conn);
				row = mysql_fetch_row (resultado);
				if (row == NULL){
					printf ("No se han obtenido datos en la consulta\n");
					respuesta[2] = '3';
				}
				else{
					char numero1 [100];
					while (row != NULL) {
						strcpy(numero1, row[0]);
						row = mysql_fetch_row (resultado);
					}
					int numero;
					numero = atoi(numero1);
					if(numero == 1){
						printf("Usuario iniciado correctamente \n");
						int add;
						add = addConectados (&lista, username, sock_conn);
						if (add == 0)
							respuesta[2] = '0';
						
						else if (add == -1)
							respuesta[2] = '1';
						else{
							respuesta[2] = '2';
							printf("El usuario %s ya esta conectado\n", username);
						}
					}
					else{
						printf("Los datos introducidos no son correctos \n");
						respuesta[2] = '3';
					}
				}
			}
			pthread_mutex_unlock(&mutex);
		}
		
		else if (codigo==3){ //Mejor jugador
			pthread_mutex_lock( &mutex );
			printf ("Peticion 3: Mejor Jugador\n");
			mejorJugador(respuesta);
			pthread_mutex_unlock( &mutex);	
		}
		
		else if (codigo==4){ //Peor jugador
			pthread_mutex_lock( &mutex );
			printf ("Peticion 4: Peor Jugador\n");
			peorJugador(respuesta);
			pthread_mutex_unlock( &mutex);	
		}
		
		else if (codigo==5){ //Partida mas larga
			printf ("Peticion 5: Partida mas larga\n");
			pthread_mutex_lock( &mutex);
			partidaLarga(respuesta);
			pthread_mutex_unlock( &mutex);	
		}
		
		else if (codigo==6){ //Partida mas corta
			pthread_mutex_lock( &mutex );
			printf ("Peticion 6: Partida mas corta\n");
			partidaCorta(respuesta);
			pthread_mutex_unlock( &mutex);	
		}
		
		else if (codigo==7){ //Mostrar lista de conectados
			pthread_mutex_lock( &mutex );
			printf ("Peticion 7: Lista conectados\n");
			conectado = 1;
			printf("%s\n",respuesta);
			pthread_mutex_unlock(&mutex);
		}
		
		else if (codigo==8){
			pthread_mutex_lock( &mutex );
			printf("Petición 8: Abandonar Partida y Servidor\n");
			int idpartida = atoi(p);
			p = strtok(NULL, "/");
			strcpy(username, p);
			if (borrjug(&listapartidas, idpartida, username) == 0){
				refjug(&listapartidas, idpartida, 0, username);
				int del = delConectados(&lista,username);
				int i = 0;
				int enc = 0;
				while (i<num_sockets && enc == 0){
					if (sockets[i] == sock_conn)
						enc = 1;
					else
						i++;
				}
				if (enc == 1){
					while (i<num_sockets -1){
						sockets[i] == sockets[i+1];
						i++;
					}
					num_sockets--;
					conectado = 1;
					terminar = 1;
				}
			}
			pthread_mutex_unlock(&mutex);
		}
		
		else if (codigo==9){
			pthread_mutex_lock( &mutex );
			printf("Peticion 9: Desconectar del servidor sin usuario\n");
			int i = 0;
			int enc = 0;
			while (i<num_sockets && enc == 0){
				if (sockets[i] == sock_conn)
					enc = 1;
				else
					i++;
			}
			if (enc == 1){
				
				while (i<num_sockets -1){
					sockets[i] == sockets[i+1];
					i++;
				}
				num_sockets--;
				terminar = 1;
			}
			pthread_mutex_unlock(&mutex);
		}
		
		else if (codigo==10){
			pthread_mutex_lock( &mutex );
			printf("Peticion 10: Enviar Apuesta\n");
			int apuesta = atoi (p);
			p = strtok (NULL, "/");
			int idpartida = atoi (p);
			EnviarApuesta(&listapartidas, idpartida, apuesta, username);
			sprintf(respuesta, "15:0/%d", apuesta);
			pthread_mutex_unlock(&mutex);
		}
		
		else if (codigo==11){
			pthread_mutex_lock( &mutex );
			printf("Petición 11: Abandonar Partida y Servidor mientras se juega.\n");
			int idpartida = atoi(p);
			p = strtok(NULL, "/");
			strcpy(username, p);
			if (borrjug(&listapartidas, idpartida, username) == 0){
				refjug(&listapartidas, idpartida, 0, username);
				int del = delConectados(&lista,username);
				int i = 0;
				int enc = 0;
				while (i<num_sockets && enc == 0){
					if (sockets[i] == sock_conn)
						enc = 1;
					else
						i++;
				}
				if (enc == 1){
					while (i<num_sockets -1){
						sockets[i] == sockets[i+1];
						i++;
					}
					num_sockets--;
					conectado = 1;
					terminar = 1;
				}
			}
			pthread_mutex_unlock(&mutex);
		}
		else if (codigo==12){
			pthread_mutex_lock( &mutex );
			printf("Peticion 12: Invitacion\n");
			strcpy(respuesta, "12:");
			int idpartida= atoi (p);
			p = strtok(NULL, "/");
			int num_inv = atoi (p);
			p = strtok (NULL, "/");
			strcat (respuesta, p);
			strcat (respuesta, "/");
			sprintf(respuesta, "%s%d", respuesta, idpartida);
			if (lista.conectados[DamePosicion(&lista, p)].partida == 0){
				lista.conectados[DamePosicion(&lista, p)].partida = idpartida;
				crearpar (&listapartidas, idpartida, p, sock_conn);
			}
			
			for (int i =0; i<num_inv; i++){
				p = strtok(NULL, "/");
				int socki = DameSocket(&lista, p);
				printf ("Respuesta: %s\n", respuesta);
				write (socki,respuesta, strlen(respuesta));
			}
			memset(respuesta,'\0',strlen(respuesta));
			pthread_mutex_unlock(&mutex);
		}
		
		else if (codigo==13){
			pthread_mutex_lock( &mutex );
			printf("Peticion 13: Chat\n");
			char chat[100];
			sprintf(chat,"13:%s",p);
			int i;
			printf("Chat: %s\n", chat);
			for(i = 0; i < lista.num ; i++)
			{
				int socket = lista.conectados[i].socket;
				write(socket,chat,strlen(chat));
			}
			pthread_mutex_unlock(&mutex);
		}
		
		else if (codigo==14){
			pthread_mutex_lock( &mutex );
			printf("Peticion 14: Abandonar Partida\n");
			int idpartida = atoi (p);
			if (borrjug(&listapartidas, idpartida, username) == 0){
				refjug(&listapartidas, idpartida, 0, username);
				lista.conectados[DamePosicion(&lista, username)].partida = 0;
				sprintf (respuesta, "14:0");
			}
			pthread_mutex_unlock(&mutex);
		}
		else if (codigo==15){
			pthread_mutex_lock( &mutex );
			int idpartida = atoi (p);
			p = strtok(NULL, "/");
			char in [20];
			strcpy(in, p);
			p = strtok(NULL, "/");
			int a = atoi (p);
			if (a == 1){
				printf("Peticion 15: Aceptar invitacion\n");
				if (addjug(&listapartidas, idpartida, username, sock_conn)==0){
					lista.conectados[DamePosicion(&lista, username)].partida = idpartida;
					sprintf(respuesta,"10:1/%s", username);
					printf("Respuesta: %s\n", respuesta);
					write(DameSocket(&lista, in), respuesta, strlen(respuesta));
					memset(respuesta,'\0',strlen(respuesta));
					refjug(&listapartidas, idpartida, 1, username);
					sprintf(respuesta, "10:0/%d", idpartida);
				}
				
				else{
					sprintf(respuesta,"9:1/%s", username); 
					printf("Respuesta: %s\n", respuesta);
					write(DameSocket(&lista, in), respuesta, strlen(respuesta));
					memset(respuesta,'\0',strlen(respuesta));
					sprintf(respuesta, "9:0");
				}
			}
			else{
				printf("Peticion 15: Rechazar invitacion\n");
				sprintf(respuesta,"11:1/%s", username);
				write(DameSocket(&lista, in), respuesta, strlen(respuesta));
				memset(respuesta,'\0',strlen(respuesta));
				sprintf(respuesta, "11:0");
			}
			pthread_mutex_unlock(&mutex);
		}
		
		else if (codigo==16){
			pthread_mutex_lock( &mutex );
			printf("Petición 16: Abandonar Partida mientras se juega\n");
			int idpartida = atoi(p);
			p = strtok(NULL, "/");
			strcpy(username, p);
			if (borrjug(&listapartidas, idpartida, username) == 0){
				refjug(&listapartidas, idpartida, 0, username);
				lista.conectados[DamePosicion(&lista, username)].partida = 0;
				sprintf (respuesta, "14:0");
			}
			pthread_mutex_unlock(&mutex);
		}
		else if (codigo==17){
			pthread_mutex_lock( &mutex );
			printf ("peticion 17: Repartir cartas\n");
			if (carta == 0){
				int i = 0;
				int a = 1;
				while (i<48){
					if (i % 4 == 0 && i != 0)
						a++;
					cartas[i] = a;
					i++;
				}
				carta = 1;
			}
			srand(time(NULL));
			int idpartida = atoi(p);
			int carta1 = rand()%48;
			int carta2 = rand()%48;
			int carta3 = rand()%48;
			int carta4 = rand()%48;
			int carta5 = rand()%48;
			int carta6 = rand()%48;
			int carta7 = rand()%48;
			int carta8 = rand()%48;
			int carta9 = rand()%48;
			int carta10 = rand()%48;
			int carta11 = rand()%48;
			int carta12 = rand()%48;
			int punt01 = cartas[carta1] + cartas[carta2] + cartas[carta3];
			int punt1;
			if (listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[0].apuesta < punt01)
				punt1 = punt01 - listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[0].apuesta;
			else
				punt1 = listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[0].apuesta - punt01;
			int punt02 = cartas[carta4] + cartas[carta5] + cartas[carta6];
			int punt2;
			if (listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[1].apuesta < punt02)
				punt2 = punt02 - listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[1].apuesta;
			else
				punt2 = listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[1].apuesta - punt02;
			
			int punt03 = cartas[carta7] + cartas[carta8] + cartas[carta9];
			int punt3;
			if (listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[2].apuesta < punt03)
				punt3 = punt03 - listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[2].apuesta;
			else
				punt3 = listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[2].apuesta - punt03;
			int punt04 = cartas[carta10] + cartas[carta11] + cartas[carta12];
			int punt4;
			if (listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[3].apuesta < punt04)
				punt4 = punt04 - listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[3].apuesta;
			else
				punt4 = listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[3].apuesta - punt04;
			int a = punt1;
			if (a > punt2)
				a = punt2;
			if (a > punt3)
				a = punt3;
			if (a > punt4)
				a = punt4;
				
			sprintf(respuesta, "16:%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d", carta1, carta2, carta3, carta4, carta5, carta6, carta7, carta8, carta9, carta10, carta11, carta12, punt1, punt2, punt3, punt4, a);
			printf ("Respuesta: %s\n", respuesta);
			char respuesta1 [100];
			char respuesta2 [100];
			char respuesta3 [100];
			char respuesta4 [100];
			sprintf(respuesta1, "16:%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d", 0, carta1, carta2, carta3, carta4, carta5, carta6, carta7, carta8, carta9, carta10, carta11, carta12, punt1, punt2, punt3, punt4, a);
			sprintf(respuesta2, "16:%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d", 1, carta1, carta2, carta3, carta4, carta5, carta6, carta7, carta8, carta9, carta10, carta11, carta12, punt1, punt2, punt3, punt4, a);
			sprintf(respuesta3, "16:%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d", 2, carta1, carta2, carta3, carta4, carta5, carta6, carta7, carta8, carta9, carta10, carta11, carta12, punt1, punt2, punt3, punt4, a);
			sprintf(respuesta4, "16:%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d", 3, carta1, carta2, carta3, carta4, carta5, carta6, carta7, carta8, carta9, carta10, carta11, carta12, punt1, punt2, punt3, punt4, a);
			write(listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[0].socket, respuesta1, strlen(respuesta1));
			write(listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[1].socket, respuesta2, strlen(respuesta2));
			write(listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[2].socket, respuesta3, strlen(respuesta3));
			write(listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[3].socket, respuesta4, strlen(respuesta4));
			memset(respuesta,'\0',strlen(respuesta));
			memset(respuesta1,'\0',strlen(respuesta1));
			memset(respuesta2,'\0',strlen(respuesta2));
			memset(respuesta3,'\0',strlen(respuesta3));
			memset(respuesta4,'\0',strlen(respuesta4));
			pthread_mutex_unlock(&mutex);
		}
		else if (codigo==18){
			//Codigo para darse de baja
			pthread_mutex_lock( &mutex );
			printf("Peticion 18: Dar de baja un usuario\n");
			int o = DatedeBaja(p);
			if(o == 0)
			{
				strcpy (username, p);
				int del = delConectados(&lista,username);
				conectado = 1;
				strcpy(respuesta, "18:0");
			}
			
			else{
				strcpy (respuesta, "18:1");
			}
			pthread_mutex_unlock(&mutex);
		}
		
		else if (codigo==19){
			pthread_mutex_lock( &mutex );
			printf("Peticion 19: Reiniciar Partida\n");
			sprintf(respuesta, "19:%s", username);
			int idpartida = atoi(p);
			write(listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[0].socket, respuesta, strlen(respuesta));
			write(listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[1].socket, respuesta, strlen(respuesta));
			write(listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[2].socket, respuesta, strlen(respuesta));
			write(listapartidas.partidas[PosPartida(&listapartidas, idpartida)].jugadores[3].socket, respuesta, strlen(respuesta));
			pthread_mutex_unlock(&mutex);
		}
		
		// Enviamos la respuesta
		if (codigo != 9 && codigo != 0 && codigo != 12 && codigo != 17 && codigo != 19){
			printf ("Respuesta: %s\n", respuesta);
			write (sock_conn,respuesta, strlen(respuesta));
		}
		
		memset(respuesta,'\0',strlen(respuesta));
		memset(peticion,'\0',strlen(peticion));
		
		if (conectado == 1){
			EnviarNotificacion(lista, respuesta, sockets, num_sockets);
			conectado = 0;
		}
	}
	// Se acabo el servicio para este cliente
	close(sock_conn); 
}

int main(int argc, char *argv[]) {
	
	int sock_conn, sock_listen;
	struct sockaddr_in serv_adr;
	
	pthread_t thread[100];
	
	
	// Obrim el socket
	if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("Error creant socket\n");
	// Fem el bind al port
	
	memset(&serv_adr, 0, sizeof(serv_adr));// inicialitza a zero serv_addr
	serv_adr.sin_family = AF_INET;
	
	// asocia el socket a cualquiera de las IP de la maquina. 
	//htonl formatea el numero que recibe al formato necesario
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	// establecemos el puerto de escucha
	serv_adr.sin_port = htons(9215);
	
	if (bind(sock_listen, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0)
		printf ("Error al bind\n");
	
	if (listen(sock_listen, 3) < 0)
		printf("Error en el Listen\n");
	
	// bucle infinito
	for (;;){
		printf ("Escuchando\n");
		sock_conn = accept(sock_listen, NULL, NULL);
		printf ("He recibido conexion\n");
		//sock_conn es el socket que usaremos para este cliente
		
		sockets[num_sockets]=sock_conn;
		pthread_create( &thread[num_sockets], NULL, AtenderCliente, &sockets[num_sockets]);		
		num_sockets = num_sockets + 1;
	}			
}


