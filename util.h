#include <semaphore.h>
#define LISTENQ 1
#define MAXDATASIZE 100
#define MAXLINE 4096
#define MAXCOMMANDLENTGH 20
#define MAXNCOMANDS 15
#define BUFFER_SIZE 10000
#define MAXUSERLENGHT 99

#define IN 0
#define NEW 1
#define BYE 2
#define HALLOFFAME 3
#define PASS 4
#define L 5
#define CALL 6
#define PLAY 7
#define DELAY 8
#define OVER 9
#define OUT 10
#define REQUEST 11

int cod_command(unsigned char *command);
void encodeMessage(char *message, char *add);
void updateFileLine(char *path, char *startWith, char *newline);
int decode_size(unsigned char *recvline, int count);
void usu_logados(char *line, int *estado, int *pontuacao);
void delay_insert(double * delay, double  diff_time);
void write_log(char * line, sem_t * semaphore_log);