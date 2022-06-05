#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>
#include "util.h"
#include "comandosServidor.h"
#include "jogo.h"
#include "heartbeat.h"



void *Heartbeat_Servidor(void *args)
{
    heartbeat_struct *s = args;
    int n;
    unsigned char recvline[21];
    int contador = 0;
    for (;;)
    {
        recvline[0] = '\0';
        strcat(recvline, "Heartbeat Request");
        write(*s->connfd, recvline, strlen(recvline));
        n = read(*s->connfd, recvline, 20);
        recvline[n] = 0;
        if ((strcmp(recvline, "Heartbeat Response") != 0) && contador < 3)
        {
            printf("Heartbeat response malformed. Attempt %d :%s\n", contador, recvline);
            contador++;
            continue;
        }
        else if ((strcmp(recvline, "Heartbeat Response") != 0) && contador >= 3)
        {
            char prov[MAXLINE];
            prov[0] = 0;
            strcat(prov, "Desconexão inesperada do cliente de IP ");
            strcat(prov, s->ip);
            strcat(prov, "\n");
            write_log(prov, s->semaphore_log);
        }
        else
        {
            contador = 0;
        }
        sleep(10);
    }
}


void *Heartbeat_Client(void *args)
{
    heartbeat_struct *s = args;
    unsigned char recvline[21];
    for (;;)
    {
        recvline[0] = '\0';
        strcat(recvline, "Heartbeat Request");
        read(*s->connfd, recvline, 20);
        if (strcmp(recvline, "Heartbeat Request") != 0)
        {
            printf("Heartbeat request malformed.\n");
            continue;
        }
        else
        {
            recvline[0] = '\0';
            strcat(recvline, "Heartbeat Response");
            write(*s->connfd, recvline, strlen(recvline));
        }
    }
    return NULL;
}