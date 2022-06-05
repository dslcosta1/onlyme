/*

Biblioteca de funções do Heartbeat

*/


typedef struct
{
    int *connfd;
    char * ip;
    sem_t *semaphore_log;
} heartbeat_struct;

void *Heartbeat_Servidor(void *args);
void *Heartbeat_Client(void *args);