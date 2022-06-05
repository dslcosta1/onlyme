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

void user_logged(unsigned char *user, const int connfd, sem_t *semaphore_user_logados, sem_t * semaphore_log,char *my_ip)
{
    char path[30] = "./tmp/user_info.txt";
    char user_logado_path[40] = "./tmp/usuarios_logados.txt";
    printf("Entrou no logado do servidor, user : %s\n", user);
    unsigned char recvline[MAXLINE + 1], *user_mes, *message, adv[MAXUSERLENGHT], ip_adv[15];
    adv[0] = 0;
    ip_adv[0] = 0;
    user_mes = malloc((strlen(user) + 3) * sizeof(unsigned char));

    int n, userNotFound, user_len = strlen(user), i;
    FILE *file_logados, *file;

    for (i = 0; i < user_len; user_mes[i + 2] = user[i], i++)
        ;
    user_mes[i + 2] = 0;
    user_mes[0] = user_len / 10 + '0';
    user_mes[1] = user_len % 10 + '0';

    while ((n = (read(connfd, recvline, MAXLINE))) > 0)
    {
        recvline[n] = 0;
        printf("%s\n", recvline);
        int count = 0;
        int command_len = decode_size(recvline, count);
        unsigned char *command = malloc(command_len * sizeof(unsigned char));
        unsigned char line[MAXLINE];
        count += 2;
        command[0] = 0;
        memcpy(command, &recvline[count], command_len);
        command[command_len] = 0;
        count += command_len;
        int command_cod = cod_command(command);

        switch (command_cod)
        {
        case PASS:; // To avoid the erro: "a label can only be part of a statement and a declaration is not a statement"
            int pass_len = decode_size(recvline, count) + 2;
            unsigned char *pass = malloc((pass_len) * sizeof(unsigned char));
            pass[0] = 0;
            memcpy(pass, &recvline[count], pass_len);
            pass[pass_len] = 0;
            count += pass_len;

            int new_pass_len = decode_size(recvline, count) + 2;
            unsigned char *new_pass = malloc((new_pass_len) * sizeof(unsigned char));
            new_pass[0] = 0;
            memcpy(new_pass, &recvline[count], new_pass_len);
            pass[new_pass_len] = 0;
            count += new_pass_len;

            unsigned char *oldLine = malloc((user_len + pass_len + 2) * sizeof(unsigned char));
            oldLine[0] = 0;
            strcat(oldLine, user_mes);
            strcat(oldLine, pass);

            int correct_pass = 0;
            FILE *file = fopen(path, "r");
            while (fgets(line, MAXLINE, file))
            {
                int len_str = strlen(line);
                for (int j = 0; j < len_str; j++)
                {
                    if (line[j] == '\n')
                        line[j] = '\0';
                }

                if (strcmp(oldLine, line) == 0)
                {
                    correct_pass = 1;
                    break;
                }
            }
            fclose(file);

            if (correct_pass)
            {
                unsigned char *newLine = malloc((user_len + pass_len + 2) * sizeof(unsigned char));
                newLine[0] = 0;
                strcat(newLine, user_mes);
                strcat(newLine, new_pass);

                updateFileLine(path, oldLine, newLine);

                free(newLine);

                write(connfd, "00\0", 3);
            }
            else
            {
                write(connfd, "-1\0", 3);
            }
            free(pass);
            free(new_pass);
            free(oldLine);
            /*Enviar mensagem para o jogador se deu errado*/

        case CALL:;
            printf("Entrou no CALL do servidor\n");
            int adv_len = decode_size(recvline, count);
            unsigned char *adv_mes = malloc((3 + adv_len) * sizeof(char));
            memcpy(adv_mes, &recvline[count], adv_len + 2);
            adv_mes[adv_len + 2] = 0;
            count += 2;
            memcpy(adv, &recvline[count], adv_len);
            count += adv_len;
            adv[adv_len] = 0;
            file_logados = fopen("./tmp/usuarios_logados.txt", "r");
            userNotFound = 1;
            if (file_logados)
            {
                printf("Entrou no caso em que Arquivo existe\n");
                unsigned char line[MAXLINE];
                line[0] = 0;
                while (fgets(line, MAXLINE, file_logados) && userNotFound)
                {
                    int count_file = 0;
                    int user_file_len = decode_size(line, count_file);
                    /*
                    if (user_file_len != adv_len)
                    {
                        continue;
                    }
                    */
                    count_file += 2;
                    unsigned char *user_file = malloc((user_file_len + 1) * sizeof(unsigned char));
                    user_file[0] = 0;
                    memcpy(user_file, &line[count_file], user_file_len);
                    user_file[user_file_len] = 0;
                    count_file += user_file_len;
                    if (strcmp(adv, user_file) == 0)
                    {
                        //Checa se o oponente não está disponível
                        if (line[2 + adv_len] == '1')
                            userNotFound = 0;
                        //break;
                    }
                    if (strcmp(adv, user) == 0)
                    {
                        //Checa se o usuário não está disponível
                        if (line[2 + strlen(user)] == '1')
                            userNotFound = 0;
                        //break;
                    }
                    free(user_file);
                }
                if (fclose(file_logados))
                {
                    printf("Erro ao fechar o arquivo\n");
                }
            }
            /* Se ambos os usuários estão logados e não jogando*/
            if (!userNotFound)
            {
                printf("Entrou aqui então as condicoes para os requestes foram atendidas\n");
                int modificado = 0;
                file_logados = fopen("./tmp/usuarios_logados.txt", "r");
                unsigned char line[MAXLINE];
                int ip_len, port_len, mes_tam;
                line[0] = 0;
                while (fgets(line, MAXLINE, file_logados))
                {
                    int user_file_len = decode_size(line, 0);
                    if ((user_file_len != user_len) && (user_file_len != adv_len))
                    {
                        continue;
                    }
                    unsigned char *user_file = malloc((user_file_len + 3) * sizeof(unsigned char));
                    user_file[0] = 0;
                    memcpy(user_file, &line[0], user_file_len + 2);
                    user_file[user_file_len + 2] = 0;
                    if (strcmp(user_mes, user_file) == 0)
                    {
                        line[user_file_len + 2] = '2';
                        updateFileLine("./tmp/usuarios_logados.txt", user_mes, line);
                        modificado++;
                    }
                    if (strcmp(adv_mes, user_file) == 0)
                    {
                        mes_tam = strlen(line) - (6 + user_file_len);
                        message = (char *)malloc(sizeof(char) * (mes_tam));
                        memcpy(message, &line[6 + user_file_len], mes_tam);
                        line[user_file_len + 2] = '2';
                        updateFileLine("./tmp/usuarios_logados.txt", adv_mes, line);
                        modificado++;
                    }
                    free(user_file);
                    if (modificado > 1)
                        break;
                }
                message[mes_tam - 1] = 0;
                fclose(file_logados);
                printf("A mensagem que vai ser enviada é: %s", &message[0]);
                int ip_prov = (((int)(message[0] - '0')) * 10) + ((int)( message[1] - '0'));
                memcpy(ip_adv, &message[2], ip_prov);
                ip_adv[ip_prov] = 0;
                write(connfd, (const char *)&message[0], mes_tam);
                free(message);
                recvline[0] = 0;
                strcat(recvline, "O usuário ");
                strcat(recvline, &user_mes[2]);
                strcat(recvline, " com ip ");
                strcat(recvline, my_ip);
                strcat(recvline, " iniciou uma partida com o usuário ");
                strcat(recvline, adv);
                strcat(recvline, " com IP ");
                strcat(recvline, ip_adv);
                strcat(recvline, "\n");
                write_log(recvline, semaphore_log);
            }
            else
            {
                write(connfd, "-1", 2);
            }

            free(adv_mes);

            /*
            Olha se o jogador existe está logado e não está jogando

            Muda flag de ambos para 2

            envia ip e porta do jogador2

            se jogadore recusou muda as flags de volta para 1

            */

            break;

        case REQUEST:;
            int resp_len = decode_size(recvline, count);
            count += 2;
            unsigned char *resp = malloc((resp_len + 1) * sizeof(unsigned char));
            memcpy(resp, &recvline[count], resp_len);
            resp[resp_len] = 0;
            int pont_add = 0;
            if (strcmp(resp, "accept") == 0)
            {
                n = (read(connfd, recvline, MAXLINE));
                recvline[n] = 0;

                pont_add = recvline[n - 1] - '0';
                printf("O valor do revline depois que acaba a partida %s", recvline);
                recvline[0] = 0;
                if(adv[0] != 0){
                    strcat(recvline, "A partida entre o usuário ");
                    strcat(recvline, user);
                    strcat(recvline, " com ip: ");
                    strcat(recvline, my_ip);
                    strcat(recvline, " e o usuário ");
                    strcat(recvline, adv);
                    strcat(recvline, " com ip: ");
                    strcat(recvline, ip_adv);
                    strcat(recvline, " resultou em ");
                }
                switch (pont_add)
                {
                case 0:
                    if (adv[0] != 0){
                        strcat(recvline, "vitória para o usuário ");
                        strcat(recvline, adv);
                        strcat(recvline, "\n");
                        write_log(recvline, semaphore_log);
                    }
                    break;
                case 1:
                    if(adv[0] != 0){
                        strcat(recvline, "empate\n");
                        write_log(recvline, semaphore_log);
                    }
                    break;
                case 2:
                    if(adv[0] != 0){
                        strcat(recvline, "vitória para o usuário ");
                        strcat(recvline, user);
                        strcat(recvline, "\n");
                        write_log(recvline, semaphore_log);
                    }
                    break;
                case 3:
                    if(adv[0] != 0){
                        strcat(recvline, "nada, um usuário pediu o termino da partida antes do fim\n");
                        write_log(recvline, semaphore_log);
                    }
                    pont_add = 0;
                    break;
                default:
                    pont_add = 0;
                    break;
                }
                adv[0] = 0;
                ip_adv[0] = 0;
                /*Casos para vitória, derrota, empate*/
            }
            sem_wait(semaphore_user_logados);
            file_logados = fopen("./tmp/usuarios_logados.txt", "r");
            line[0] = 0;
            while (fgets(line, MAXLINE, file_logados))
            {
                int user_file_len = decode_size(line, 0);
                if (user_file_len != user_len)
                {
                    continue;
                }
                unsigned char *user_file = malloc((user_file_len + 3) * sizeof(unsigned char));
                user_file[0] = 0;
                memcpy(user_file, &line[0], user_file_len + 2);
                user_file[user_file_len + 2] = 0;
                if (strcmp(user_mes, user_file) == 0)
                {
                    printf("Line : %s\n", line);
                    line[user_file_len + 2] = '1';
                    int pont = 0, aux;
                    for (i = 0, aux = 100; i < 3; pont += (line[user_file_len + 3 + i] - '0') * aux, i++, aux /= 10)
                        ;
                    pont += pont_add;
                    for (i = 2, aux = pont; i >= 0; line[user_file_len + 3 + i] = pont % 10 + '0', i--, pont /= 10)
                        ;

                    updateFileLine("./tmp/usuarios_logados.txt", user_mes, line);
                    free(user_file);
                    break;
                }
                free(user_file);
            }
            fclose(file_logados);
            sem_post(semaphore_user_logados);
            break;
        case HALLOFFAME:
            file_logados = fopen("./tmp/usuarios_logados.txt", "r");
            line[0] = 0;
            while (fgets(line, MAXLINE, file_logados))
            {
                int user_file_len = decode_size(line, 0);
                unsigned char *user_file = malloc((user_file_len + 3) * sizeof(unsigned char));
                user_file[0] = 0;
                memcpy(user_file, &line[0], user_file_len + 6);
                user_file[user_file_len + 2] = 0;
                char tam[2];
                tam[0] = user_file[0];
                tam[1] = user_file[1];
                write(connfd, tam, 2);
                write(connfd, &user_file[2], user_file_len + 4);
                printf("%s\n", user_file);
                free(user_file);
            }
            fclose(file_logados);
            write(connfd, "00", 2);
            break;
        case L:
            file_logados = fopen("./tmp/usuarios_logados.txt", "r");
            line[0] = 0;
            while (fgets(line, MAXLINE, file_logados))
            {
                int user_file_len = decode_size(line, 0);
                unsigned char *user_file = malloc((user_file_len + 3) * sizeof(unsigned char));
                user_file[0] = 0;
                memcpy(user_file, &line[0], user_file_len + 2);
                user_file[user_file_len + 2] = 0;
                if (((int)(line[user_file_len + 2] - '0')) > 0)
                {
                    char tam[2];
                    tam[0] = user_file[0];
                    tam[1] = user_file[1];
                    write(connfd, tam, 2);
                    write(connfd, &user_file[2], user_file_len);
                    printf("%s\n", user_file);
                }
                free(user_file);
            }
            fclose(file_logados);
            write(connfd, "00", 2);
            break;
        case OUT:
            sem_wait(semaphore_user_logados);
            file_logados = fopen("./tmp/usuarios_logados.txt", "r");
            line[0] = 0;
            while (fgets(line, MAXLINE, file_logados))
            {
                int user_file_len = decode_size(line, 0);
                if (user_file_len != user_len)
                {
                    continue;
                }
                unsigned char *user_file = malloc((user_file_len + 3) * sizeof(unsigned char));
                user_file[0] = 0;
                memcpy(user_file, &line[0], user_file_len + 2);
                user_file[user_file_len + 2] = 0;
                if (strcmp(user_mes, user_file) == 0)
                {
                    printf("Line : %s\n", line);
                    line[user_file_len + 2] = '0';
                    line[user_file_len + 6] = '\n';
                    line[user_file_len + 7] = '\0';

                    updateFileLine("./tmp/usuarios_logados.txt", user_mes, line);
                    free(user_file);
                    break;
                }
                free(user_file);
            }
            fclose(file_logados);
            sem_post(semaphore_user_logados);
            return;
        default:
            break;
        }
    }
}

int main(int arg, char **argv)
{
    sem_t *semaphore_log = sem_open("semaphore_log", O_CREAT | O_EXCL, 0, 1);
    sem_unlink("semaphore_log");
    write_log("Servidor iniciado\n", semaphore_log);
    int listenfd_tcp, listenfd_udp, connfd_tcp; /*, heartbeat_tcp*/

    struct sockaddr_in servaddr_tcp; /*, heartbeat_tcp_s*/

    pid_t childpid;

    unsigned char recvline[MAXLINE + 1], erro[3] = "-1\0", sucesso[] = "00\0";

    ssize_t n;
    if ((listenfd_tcp = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket :(\n");
        exit(2);
    }

    char path[30] = "./tmp/user_info.txt";
    char user_logado_path[40] = "./tmp/usuarios_logados.txt";

    bzero(&servaddr_tcp, sizeof(servaddr_tcp));
    servaddr_tcp.sin_family = AF_INET;
    servaddr_tcp.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr_tcp.sin_port = htons(atoi(argv[1]));
    if (bind(listenfd_tcp, (struct sockaddr *)&servaddr_tcp, sizeof(servaddr_tcp)) == -1)
    {
        perror("bind :(\n");
        exit(3);
    }
    /*
    bzero(&heartbeat_tcp, sizeof(heartbeat_tcp));
    heartbeat_tcp_s.sin_family = AF_INET;
    heartbeat_tcp_s.sin_addr.s_addr = htonl(INADDR_ANY);
    heartbeat_tcp_s.sin_port = 0;
    if (bind(heartbeat_tcp, (struct sockaddr *)&heartbeat_tcp_s, sizeof(heartbeat_tcp)) == -1)
    {
        exit(3);
    }
        perror("bind :(\n");
    */

    if (listen(listenfd_tcp, LISTENQ) == -1)
    {
        perror("listen :(\n");
        exit(4);
    }
    /*
    if (listen(listenheartbeat_tcp, LISTENQ) == -1)
    {
        perror("listen :(\n");
        exit(4);
    }
    */
    sem_t *semaphore_user_logados = sem_open("semaphore_user_logados", O_CREAT | O_EXCL, 0, 1);
    sem_unlink("semaphore_user_logados");
    for (;;)
    {
        struct sockaddr_in cliente_addr;
        socklen_t prov = sizeof(cliente_addr);
        if ((connfd_tcp = accept(listenfd_tcp, (struct sockaddr *)&cliente_addr, &prov)) == -1)
        {
            perror("accept :(\n");
            exit(5);
        }
        else
        {
            printf("Conxeão\n");
        }
        /*
        if ((heartbeat_tcp = accept(listenheartbeat_tcp, (struct sockaddr *)NULL, NULL)) == -1)
        {
            perror("accept :(\n");
            exit(5);
        }
        */

        if ((childpid == fork()) == 0)
        {
            unsigned char *user, *pass, *user_pass, *backup;
            FILE *file;
            int i, j, user_len, pass_len, userNotFound;
            int port, heartbeat_tcp;
            struct sockaddr_in heartbeat_addr;
            struct sockaddr_in *pV4Addr = (struct sockaddr_in *)&cliente_addr; /*Pegando o valor do IP do cliente*/
            struct in_addr ipAddr = pV4Addr->sin_addr;
            char str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &ipAddr, str, INET_ADDRSTRLEN); /*Trasformando o IP em string*/
            if ((heartbeat_tcp = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                fprintf(stderr, "socket error :( \n");
            n = read(connfd_tcp, recvline, MAXLINE); /*Recebendo a porta em que o cliente estará escutando*/
            recvline[n] = 0;
            printf("%s - ", recvline);
            for (port = 0, i = strlen(recvline) - 1, j = 1; i >= 0; port += ((int)recvline[i] - '0') * j, i--, j *= 10)
                ; /*Decodificando o número da porta que sera escutada pelo cliente*/
            printf("%d \n", port);
            bzero(&heartbeat_addr, sizeof(heartbeat_addr));
            heartbeat_addr.sin_family = AF_INET;
            heartbeat_addr.sin_port = htons(port);
            if (inet_pton(AF_INET, str, &heartbeat_addr.sin_addr) <= 0)
                fprintf(stderr, "inet_pton error for %s :(\n", str);

            if (connect(heartbeat_tcp, (struct sockaddr *)&heartbeat_addr, sizeof(heartbeat_addr)) < 0)
                fprintf(stderr, "connect error :(\n");
            printf("[Uma conexão aberta]\n");

            close(listenfd_tcp);
            /*close(listenheartbeat_tcp);*/
            heartbeat_struct *args = malloc(sizeof *args);
            args->connfd = &heartbeat_tcp;
            args->ip = str;
            args->semaphore_log = semaphore_log;
            pthread_t heartbeat;
            pthread_create(&heartbeat, NULL, Heartbeat_Servidor, args);
            recvline[0] = 0;
            strcat(recvline, "O cliente com IP ");
            strcat(recvline, str);
            strcat(recvline, " se conectou\n");
            write_log(recvline, semaphore_log);
            while ((n = (read(connfd_tcp, recvline, MAXLINE))) > 0)
            {
                recvline[n] = 0;
                int count = 0;
                int command_len = decode_size(recvline, count);
                unsigned char *command = malloc((command_len + 1) * sizeof(unsigned char));
                command[0] = 0;
                count += 2;

                memcpy(command, &recvline[count], command_len);
                count += command_len;
                command[command_len] = 0;
                printf("%s\n", command);
                printf("%s\n", recvline);
                switch (cod_command(command))
                {
                case IN:;
                    user_len = decode_size(recvline, 2 + command_len);
                    user = (unsigned char *)malloc((user_len + 1) * sizeof(unsigned char));
                    user[0] = 0;
                    for (i = 0; i < user_len; user[i] = recvline[i + 4 + command_len], i++)
                        ;
                    user[user_len] = 0;
                    int logou = in(recvline, count, connfd_tcp,
                                   user_logado_path, path, command_len, n);

                    recvline[0] = 0;
                    strcat(recvline, "O usuário ");
                    strcat(recvline, user);
                    strcat(recvline, " com IP ");
                    strcat(recvline, str);
                    strcat(recvline, " logou");
                    if (logou == 1)
                    {    
                        strcat(recvline, " com sucesso\n");
                        write_log(recvline, semaphore_log);
                        printf("-------------%s-------------\n", user);
                        user_logged(user, connfd_tcp, semaphore_user_logados, semaphore_log, str);
                    }
                    else
                    {
                        strcat(recvline, " sem sucesso\n");
                        write_log(recvline, semaphore_log);
                        printf("Erro ao logar\n");
                    }
                    free(user);
                    break;

                case NEW:
                    printf("Entrou no new do servidor!!\n");
                    printf("count: %d\n", count);
                    new (recvline, count, connfd_tcp, user_logado_path,
                         path, command_len, n);
                    break;

                case BYE:
                    /*Atenção com heartbeat*/
                    close(heartbeat_tcp);
                    pthread_cancel(heartbeat);
                    close(connfd_tcp);
                    recvline[0] = 0;
                    strcat(recvline, "O cliente com IP ");
                    strcat(recvline, str);
                    strcat(recvline, " se desconectou\n");
                    write_log(recvline, semaphore_log);
                    return (0);

                default:
                    break;
                }
                free(command);
            }
            close(connfd_tcp);
        }
        else
            close(connfd_tcp);
    }
    write_log("O servidor se desconectou\n", semaphore_log);
}