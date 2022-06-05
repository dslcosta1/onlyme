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

int cod_command(unsigned char *command)
{
    printf("Entrou aqui para decodificar o commando\n");
    unsigned char listOfCommands[MAXNCOMANDS][MAXCOMMANDLENTGH] = {"in", "new", "bye", "halloffame", "pass", "l", "call",
                                                                   "play", "delay", "over", "out", "request"};

    for (int i = 0; i < 12; i++)
    {
        if (strcmp(listOfCommands[i], command) == 0)
        {
            return i;
        }
    }
    return -1;
}

/*

void encodeMessage(char * message, char * add)

Retorna a mensagem inicial, concatenada com a string adicional com o tamanho dela junto

Ex:

Entrada:
message = 03new
add = daniel


Saída:
message = 03new05daniel

*/
void encodeMessage(char *message, char *add)
{
    printf("A mensagem recebida é %s e vai ser adcionado %s\n", message, add);
    char *buffer = malloc(sizeof(char));
    sprintf(buffer, "%ld", strlen(add));
    if (strlen(add) < 10)
    {
        strcat(message, "0");
    }

    strcat(message, buffer);
    strcat(message, add);

    free(buffer);
    return;
}

void updateFileLine(char *path, char *startWith, char *newline)
{
    /* File pointer to hold reference of input file */
    FILE *fPtr;
    FILE *fTemp;
    //char path[100];

    char buffer[BUFFER_SIZE];

    /* Remove extra new line character from stdin */
    fflush(stdin);

    /*  Open all required files */
    fPtr = fopen(path, "r");
    fTemp = fopen("replace.tmp", "w");

    /* fopen() return NULL if unable to open file in given mode. */
    if (fPtr == NULL || fTemp == NULL)
    {
        /* Unable to open file hence exit */
        printf("\nUnable to open file.\n");
        printf("Please check whether file exists and you have read/write privilege.\n");
        exit(EXIT_SUCCESS);
    }

    /*
     * Read line from source file and write to destination 
     * file after replacing given line.
     */
    int pattern_len = strlen(startWith);
    unsigned char line[MAXLINE];
    while ((fgets(line, MAXLINE, fPtr)) != NULL) /*Dando Warning, talvez mudar para BUFFER_SIZE para MAXLINE*/
    {
        unsigned char *start_file_line = malloc(pattern_len * sizeof(unsigned char));
        start_file_line[0] = 0;
        memcpy(start_file_line, &line[0], pattern_len);
        start_file_line[pattern_len] = 0;
        /* If current line is line to replace */
        if (strcmp(start_file_line, startWith) == 0)
            fputs(newline, fTemp);
        else
            fputs(line, fTemp);
        free(start_file_line);
    }

    /* Close all files to release resource */
    fclose(fPtr);
    fclose(fTemp);

    /* Delete original source file */
    remove(path);

    /* Rename temporary file as original file */
    rename("replace.tmp", path);

    printf("\nSuccessfully replaced '%s' line with '%s'.", line, newline);

    return;
}

void write_log(char *line, sem_t *semaphore_log)
{ /*Considerando que */
    sem_wait(semaphore_log);
    FILE *fd = fopen("./tmp/log.txt", "a+");
    /*
    char buff[20];
    time_t now = time(NULL);
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));

    printf("Vai imprimir o tempo: %s", buff);
    fputs(buff, fd);
    fputs(' ', fd);*/
    fputs(line, fd);
    fclose(fd);
    sem_post(semaphore_log);
}

int decode_size(unsigned char *recvline, int count)
{
    unsigned char command_len_char[2];
    command_len_char[0] = recvline[count];
    command_len_char[1] = recvline[count + 1];
    printf("Comando string: %s\n", command_len_char);
    int command_len = atoi(command_len_char); //((int)(command_len_char[0] - '0')) * 10 + ((int)(command_len_char[1] - '0'));
    return command_len;
}

void usu_logados(char *line, int *estado, int *pontuacao)
{
    printf("Entrou no usu_logados - 1\n");
    int usu_len = decode_size(line, 0), line_len = strlen(line), i, j, pont;
    *estado = (int)(line[2 + usu_len] - '0');
    printf("Entrou no usu_logados: Estado é %d\n", *estado);
    pont = ((int)(line[3 + usu_len] - '0')) * 100 + ((int)(line[4 + usu_len] - '0')) * 10 + (int)(line[5 + usu_len - '0']);
    printf("Entrou no usu_logados- 3\n");
    *pontuacao = pont;
    return;
}

void delay_insert(double *delay, double time)
{
    delay[0] = delay[1];
    delay[1] = delay[2];
    delay[2] = time;
}
