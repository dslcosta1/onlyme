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
#include "util.h"
#include "comandosServidor.h"


int in(unsigned char * recvline, int count, int connfd_tcp, 
    char * user_logado_path, char * path, int command_len, int n) 
{
    unsigned char erro[3] = "-1\0", sucesso[3] = "00\0";
    int user_len, pass_len;
    unsigned char * user, * pass;
    user_len = decode_size(recvline, count);
    count += 2;
    user = (unsigned char *)malloc((user_len + 1) * sizeof(unsigned char));
    memcpy(user, &recvline[count], user_len);
    user[user_len] = 0;
    count += user_len;

    pass_len = decode_size(recvline, count);
    count += 2;
    pass = (unsigned char *)malloc((pass_len + 1) * sizeof(unsigned char));
    memcpy(pass, &recvline[count], pass_len);
    pass[pass_len] = 0;
    count += pass_len;
    /*
    open
    iterar pelas linhas
    */

    FILE *file_logados = fopen(user_logado_path, "r");
    int userNotFound = 1;
    unsigned char line[MAXLINE];
    unsigned char newLine[MAXLINE];
    newLine[0] = 0;

    if (file_logados) /*Checando se o usuário não existe em usuarios_logados.txt*/
    {

        while (fgets(line, MAXLINE, file_logados) && userNotFound)
        {
            int count_file = 0;
            int user_file_len = decode_size(line, count_file);
            if (user_file_len != user_len)
            {
                continue;
            }
            count_file += 2;
            unsigned char *user_file = malloc((user_file_len + 1) * sizeof(unsigned char));
            memcpy(user_file, &line[count_file], user_file_len);
            user_file[user_file_len] = 0;
            count_file += user_file_len;
            if (strcmp(user, user_file) == 0)
            {   
                int estado;
                int pontuacao;
                usu_logados(line, &estado, &pontuacao);
                if (estado > 0)
                {
                    userNotFound = 0;
                }else{
                    memcpy(newLine, &line[0], strlen(line));
                    newLine[strlen(line) - 1] = 0;
                }
                break;
            }
            free(user_file);
        }
    }
    if (!userNotFound)
    { /*Usuário ja logado*/
        /*envio de erro*/
        printf("Erro\n");
        write(connfd_tcp, erro, 3);
        /*escrever no arquivo de log*/
        free(user);
        free(pass);
        return 0;
    }
    fclose(file_logados);
    FILE * file = fopen(path, "r");
    int in = 0;
    printf("Teste\n");

    if (file) /*Testando usuário e senha e entrando caso tudo OK*/
    {
        unsigned char line_to_change[MAXLINE];
        int len_header = command_len + 2;
        unsigned char * user_pass = (unsigned char *)malloc((n - len_header + 1) * sizeof(unsigned char));
        memcpy(user_pass, &recvline[len_header], n - len_header);
        user_pass[n - len_header] = 0;
        while (fgets(line, MAXLINE, file))
        {
            int len_str = strlen(line);
            for (int j = 0; j < len_str; j++)
            {
                if (line[j] == '\n')
                    line[j] = '\0';
            }

            if (strcmp(user_pass, line) == 0)
            {
                memcpy(line_to_change, &line, user_len + 2);
                line_to_change[user_len + 2] = 0;
                in = 1;
                break;
            }
        }
        fclose(file);
        /* Usuário existente e ainda não logado*/
        if (in)
        {
            
            write(connfd_tcp, sucesso, 3);
            n = read(connfd_tcp, recvline, MAXLINE); /*Pegar o IP e a porta*/
            recvline[n] = 0;
            /*Logando, falta escrever no log*/
            unsigned char *user_log = malloc(user_len + 10); /*adicionar \n*/
            memcpy(user_log, &user_pass[0], user_len + 2);
            user_log[user_len + 2] = 0;

            /*Atualiza flag de logado*/
            newLine[user_len + 2] = '1';
            strcat(newLine, recvline);
            strcat(newLine, "\n");
            updateFileLine(user_logado_path, user_log, newLine); /*updateFileLine(file_logados, user_log, line_to_change); -> updateFileLine("/tmp/usuarios_logados.txt", user_log, line_to_change);*/

        }
        else
        {
            write(connfd_tcp, erro, 3);
        }
        free(user_pass);
    }
    free(user);
    free(pass);
    return in;
}


void new(unsigned char * recvline, int count, int connfd_tcp, 
    char * user_logado_path, char * path, int command_len, int n)
{
    unsigned char erro[3] = "-1\0", sucesso[3] = "00\0";
    int user_len, pass_len;
    unsigned char * user, * pass;
    user_len = decode_size(recvline, count);
    count += 2;
    user = (unsigned char *)malloc((user_len + 1) * sizeof(unsigned char));
    user[0] = 0;
    memcpy(user, &recvline[count], user_len);
    user[user_len] = 0;
    count += user_len;
    printf("tamanho: %d\n", user_len);
    printf("Leu usuário: %s\n", user);

    printf("recvline: %s\n", recvline);
    printf("count: %d\n", count);
    pass_len = decode_size(recvline, count);
    count += 2;
    pass = (unsigned char *)malloc((pass_len + 1) * sizeof(unsigned char));
    pass[0] = 0;
    memcpy(pass, &recvline[count], pass_len);
    pass[pass_len] = 0;
    count += pass_len;

    printf("tamanho: %d\n", pass_len);
    printf("Leu senha: %s\n", pass);

    /* Open file */
    FILE * file = fopen(path, "r");
    int userNotFound = 1;

    if (file)
    {
        printf("Entrou no caso em que Arquivo existe\n");
        unsigned char line[MAXLINE];
        line[0] = 0;
        while (fgets(line, MAXLINE, file) && userNotFound)
        {
            int count_file = 0;
            int user_file_len = decode_size(line, count_file);
            if (user_file_len != user_len)
            {
                continue;
            }
            count_file += 2;
            unsigned char *user_file = malloc((user_file_len + 1) * sizeof(unsigned char));
            user_file[0] = 0;
            memcpy(user_file, &line[count_file], user_file_len);
            user_file[user_file_len] = 0;
            printf("%s, %s\n", user_file, user);
            count_file += user_file_len;
            if (strcmp(user, user_file) == 0)
            {
                userNotFound = 0;
                break;
            }
            free(user_file);
        }
        if (fclose(file))
        {
            printf("Erro ao fechar o arquivo\n");
        }
    }

    if (userNotFound)
    {
        printf("Entrou no caso para escrever novo usuario!!\n");
        file = fopen(path, "a");

        int len_header = command_len + 2;
        unsigned char * user_pass = (unsigned char *)malloc((n - len_header + 2) * sizeof(unsigned char));
        user_pass[0] = 0;

        unsigned char * backup = (unsigned char *)malloc((n - len_header + 2) * sizeof(unsigned char));
        backup[0] = 0;
        memcpy(user_pass, &recvline[len_header], n - len_header);
        user_pass[n - len_header] = 0;
        int i;
        for (i = len_header; i < n; i++)
            backup[i - len_header] = recvline[i];
        backup[i - len_header] = 0;
        strcat(user_pass, "\n\0");

        printf("Gerou mensagem: %s\n", user_pass);
        fprintf(file, "%s", user_pass);
        fclose(file);
        printf("Salvou novo usuario no arquivo de usuario!\n");
        /*Colocar usuário no arquivo de logado com estado e pontuação 0*/

        FILE *file_logados = fopen("./tmp/usuarios_logados.txt", "a+");
        printf("Abriu Arquivo de logados\n");
        /*
        Problemas para salvar usuário no arquivo de logados
        Formato errado e não está salvando no arquivo como o de cima
        */

        unsigned char *user_log = malloc((user_len + 20) * sizeof(unsigned char));
        user_log[0] = 0;
        memcpy(user_log, &backup[0], user_len + 2);
        user_log[user_len + 2] = 0;

        //strcat(user_log, backup);

        printf("Copiou isso %s\n", user_log);
        strcat(user_log, "0000\n\0");

        printf("Vai escrever a mensagem %s\n", user_log);
        fprintf(file_logados, "%s", user_log);
        fclose(file_logados);

        free(user_log);
        free(user_pass);
        free(backup);

        write(connfd_tcp, sucesso, 3);
    }
    else
    {
        write(connfd_tcp, erro, 3);
        printf("Usuário já existe\n");
    }

    free(pass);
    free(user);
}