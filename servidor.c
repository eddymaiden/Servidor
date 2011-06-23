#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "message.h"

#define BUF_SIZE 1024

void Escribir_Puerto (int puerto);
int abrir_socket_udp(int *port);
int abrir_port_tcp(int *puerto);
int writen(int dest, const void *buf, int n);


/* FUNCION MAIN DEL PROGRAMA SERVIDOR */
int main(int argc,char* argv[])
{
	/* El esqueleto de la funcion principal muestra, de forma esquematica la secuencia 
	   de operaciones y la correspondiente traza que se sugiere */

	int port, port_tcp, msg_size, loc_file, new_sock, recv, sock_tcp;
	struct sockaddr_in cli_addr;
	ssize_t len;
	socklen_t cli_size;
	UDP_Msg msg;
	char buf[BUF_SIZE];
	int sock_udp = abrir_socket_udp(&port);

	/* Escriubimos el puerto de servicio */
	Escribir_Puerto(port /* Numero de puerto asignado */);
	sock_tcp = abrir_port_tcp(&port_tcp);

	bzero((char *)&cli_addr, sizeof(struct sockaddr_in));
	cli_size = sizeof(cli_addr);
	bzero(&msg, sizeof(UDP_Msg)); 
	msg_size = sizeof(UDP_Msg);






	/* Aceptamos conexiones por el socket */
	fprintf(stdout,"SERVIDOR: Aceptacion de peticiones: %s\n",
			(listen(sock_tcp, 10) < 0) ? "ERROR" : "OK");


	while(1 /* Bucle de procesar peticiones */)
	{
		fprintf(stdout,"SERVIDOR: Esperando mensaje.\n");

		recv = recvfrom(sock_udp, (char *) &msg, msg_size, 0, (struct sockaddr *)&cli_addr, &cli_size);

		/* Recibo mensaje */
		fprintf(stdout,"SERVIDOR: Mensaje del cliente: %s\n", (recv >0)
				? "OK" : "ERROR");
		if (ntohl(msg.op) == REQUEST){
			loc_file = open(msg.remoto, O_RDONLY);
			fprintf(stdout,"SERVIDOR: REQUEST(%s,%s)\n", msg.local, msg.remoto);

			if (loc_file < 0){
				if (errno == ENOENT)
					msg.op = htonl(ERROR);
			}
			else msg.op = htonl(OK);
		}
		else if (ntohl(msg.op) == QUIT){
			msg.op = htonl(OK);
			sendto(sock_udp, (char *)&msg, msg_size, 0, (struct sockaddr *)&cli_addr, cli_size);
			fprintf(stdout,"SERVIDOR: QUIT\n");
			close(sock_udp); close(sock_tcp); break;
		}


		msg.puerto = htons(port_tcp);
		sendto(sock_udp, (char *)&msg, msg_size, 0, (struct sockaddr *)&cli_addr, cli_size);
		if (msg.op == ERROR)
			continue;



		new_sock = accept(sock_tcp, (struct sockaddr *)&cli_addr,
				&cli_size);

		while ((len = read(loc_file, buf, BUF_SIZE)) > 0)
			writen(new_sock, buf, len);

		close(new_sock);
		close(loc_file);
		bzero((char *)&cli_addr, sizeof(struct sockaddr_in));
		cli_size = sizeof(cli_addr);
		bzero(&msg, sizeof(UDP_Msg)); 
		msg_size = sizeof(UDP_Msg);



	}

	fprintf(stdout,"SERVIDOR: Finalizado\n");
	exit(0);
}

/* Funcion auxiliar que escribe un numero de puerto en el fichero */
void Escribir_Puerto (int puerto)
{
	int fd;
	if((fd=creat(FICHERO_PUERTO,0660))>=0)
	{
		write(fd,&puerto,sizeof(int));
		close(fd);
		fprintf(stdout,"SERVIDOR: Puerto guardado en fichero %s: OK\n",FICHERO_PUERTO);
	}
}



/* Abre el socket y le asigna la direccion UDP dinamicamente */
int abrir_socket_udp(int *port)
{
	struct sockaddr_in dir_udp_serv; /* Direccion cliente */
	int sock_udp; /* Socket (de tipo UDP) de trasmision de ordenes */
	socklen_t size = sizeof(dir_udp_serv);

	/* Creacion del socket UDP */
	sock_udp=socket(AF_INET,SOCK_DGRAM,0);

	/* Creacion del socket UDP */
	fprintf(stdout,"SERVIDOR: Creacion del socket UDP: %s\n", 
			(sock_udp < 0) ? "ERROR" : "OK");

	if(sock_udp<0)
	{
		exit(1);
	}

	/* Asignacion de la direccion local (del cliente) */
	bzero((char*)&dir_udp_serv,sizeof(struct sockaddr_in)); /* Pone a 0 
								   la estructura */
	dir_udp_serv.sin_family=AF_INET;
	dir_udp_serv.sin_addr.s_addr=inet_addr(HOST_SERVIDOR); 
	dir_udp_serv.sin_port=htons(0);
	fprintf(stdout,"SERVIDOR:  Asignacion del puerto servidor: ");
	if(bind(sock_udp,
				(struct sockaddr*)&dir_udp_serv,
				sizeof(struct sockaddr_in))<0)
	{
		fprintf(stdout,"ERROR\n");
		close(sock_udp); exit(1);
	}

	fprintf(stdout,"OK\n");
	bzero((char*)&dir_udp_serv,sizeof(struct sockaddr_in)); /* Pone a 0*/

	getsockname(sock_udp, (struct sockaddr *)&dir_udp_serv, &size);
	*port = ntohs((int)dir_udp_serv.sin_port);


	return sock_udp;
}

int abrir_port_tcp(int *puerto)
{
	struct sockaddr_in dir_tcp_serv;
	int sock_tcp;
	socklen_t size = sizeof(dir_tcp_serv);

	sock_tcp = socket(AF_INET, SOCK_STREAM, 0);

	/* Creacion del socket TCP de servicio */
	fprintf(stdout,"SERVIDOR: Creacion del socket TCP: %s\n", (sock_tcp < 0)
			? "ERROR" : "OK");

	bzero((char *)&dir_tcp_serv, sizeof(struct sockaddr_in));

	dir_tcp_serv.sin_family = AF_INET;
	dir_tcp_serv.sin_addr.s_addr = inet_addr(HOST_SERVIDOR);
	dir_tcp_serv.sin_port = htons(0);

	/* Puerto TCP ya disponible */
	fprintf(stdout,"SERVIDOR: Asignacion del puerto servidor: ");
	if (bind(sock_tcp, (struct sockaddr *)&dir_tcp_serv, sizeof(struct
					sockaddr_in)) < 0){
		fprintf(stdout, "ERROR\n");
		close(sock_tcp); exit(1);
	}

	fprintf(stdout, "OK\n");

	bzero((char*)&dir_tcp_serv,sizeof(struct sockaddr_in)); /* Pone a 0*/

	getsockname(sock_tcp, (struct sockaddr *)&dir_tcp_serv, &size);
	*puerto = ntohs((int) dir_tcp_serv.sin_port);

	return sock_tcp;
}
int writen(int dest, const void *buf, int n)
{
	int w, nleft;

	nleft = n;

	while (nleft > 0){
		if((w = write(dest, buf, nleft)) < 0){
			if (nleft == n)
				return -1;
			else
				break;
		} else if (w == 0) {
			break;
		}
		nleft -= w;
		buf += w;
	}
	return(n - nleft);
}
