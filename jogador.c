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
#include <time.h>
#include <sys/stat.h>
#include "util.h"
#include "jogo.h"
#include "heartbeat.h"

typedef struct
{
    int *connfd;
    int *connfd_server;
    char *user;
} request_struct;

typedef struct
{
    int **tabuleiro;
    int *connfd;
    int *connfd_server;
    double *delay;
} partida_struct;

/*
Jogador que começa a partida 
já manda uma jogada antes de entra aqui;
*/

/*
0 - perdeu
1 - empatou
2 - ganhou
*/

/*
Concertar os bugs
Fazer o Pass e o Halloffame
Escrever no log 
Slides
README
Controle de erro
    - colocar um flag socket persistente
    - If para quando der erro
Tentar UDP 
    - Tentar fazer um pouco da comunicação

*/

void *gameOn(void *args)
{
    partida_struct *s = args;
    double diff_time;
    int connfd_request;
    unsigned char recvline[MAXLINE];
    int **tabuleiro = s->tabuleiro;
    char *buffer;
    int n = 0;
    int jogador = 0;
    int adv = 1;
    time_t t1, t0;
    double *delay = s->delay;
    char ping[5];
    printf("Entrou na thread jogo\n");
    
    n = read(*s->connfd, recvline, 15);
    recvline[15] = 0;
    /*
    recvline[0] = 0;
    n = read(*s->connfd, recvline, 2);
    recvline[2] = 0;
    int size_com = decode_size(recvline, 0);

    char *prov = malloc(sizeof(char) * (size_com + 1));
    prov[0] = 0;
    recvline[0] = 0;
    n = read(*s->connfd, recvline, size_com);
    recvline[size_com] = 0;
    memcpy(prov, &recvline[0], size_com);
    prov[size_com] = 0;

    printf("Comando lido foi %s\n", prov);
    printf("Recvline lido foi %s\n", recvline);
    printf("Tamanho do comando %d\n", size_com);
    
    while (strcmp(prov, "play") != 0)
    {
        if (strcmp(prov, "delay") == 0)
            printf("Delay : %.4f %.4f %.4f\n", delay[0], delay[1], delay[2]);
        if (strcmp(prov, "over") == 0)
        {
            //Mandar mensagem para o servidor avisando que acabou
            recvline[0] = 0;
            encodeMessage(recvline, "gameStatus");
            buffer = malloc(sizeof(char));
            buffer[0] = 0;
            sprintf(buffer, "%d", 3);
            encodeMessage(recvline, buffer);
            write(*s->connfd_server, recvline, 15);
            free(buffer);
            return NULL;
        }
        n = read(*s->connfd, recvline, 2);
        recvline[n] = 0;
        size_com = decode_size(recvline, 0);
        free(prov);
        prov = (char *)malloc(sizeof(char) * (size_com + 1));
        memcpy(prov, &recvline[2], size_com);
        prov[size_com] = 0;
    }
    free(prov);
    
    recvline[0] = 0;
    n = read(*s->connfd, recvline, 9);
    */
    printf("Mensagem recebida foi: %s\n", recvline);
    recvline[n] = 0;
    printf("recvline : %s\n", recvline);

    /*Tem que ler um play aqui*/
    /*Recebe mensagem no formato:
        04play01X01Y01J
        X - posição da jogada x
        Y - posição da jogada y
        J - Tipo do jogador 0 ou 1
    */
    char identificador_oponente = recvline[n - 1];
    if (identificador_oponente == '0')
    {
        jogador = 1;
        adv = 0;
    }

    int jogadaX, jogadaY;
    jogadaX = (int)(recvline[8] - '0');
    jogadaY = (int)(recvline[11] - '0');
    printf("O adversário fez a jogada %d %d\n", jogadaX, jogadaY);
    updateTab(tabuleiro, jogadaX, jogadaY, adv);
    printTab(tabuleiro);

    if (jogador == 1)
    {
        n = read(*s->connfd, ping, 4);
        ping[4] = 0;
    }
    else
    {
        write(*s->connfd, "ping", 4);
    }

    while (1)
    {

        char *command = malloc((MAXLINE + 1) * sizeof(char));
        command[0] = 0;
        printf("JogoDaVelha>");
        scanf("%s", command);

        int command_cod = cod_command(command);
        printf("Comando inciado foi: %s\n", command);
        switch (command_cod)
        {
        case PLAY:;
            int jogadaX, jogadoY;

            command[0] = 0;
            scanf("%d", &jogadaX);
            scanf("%d", &jogadoY);

            /* Testa se é uma jogada valida */
            while (!validation(tabuleiro, jogadaX, jogadoY))
            {
                printf("Jogada invalida, tente outra jogada!\n");
                printf("Faça a sua jogado no formato: play X Y\nJogoDaVelha>");
                command[0] = 0;
                scanf("%s", command);
                scanf("%d", &jogadaX);
                scanf("%d", &jogadoY);
            }

            /* Envia jogada para outro jogador */
            recvline[0] = 0;
            encodeMessage(recvline, "play");

            char *buffer = malloc(sizeof(char));
            buffer[0] = 0;
            sprintf(buffer, "%d", jogadaX);
            encodeMessage(recvline, buffer);

            buffer[0] = 0;
            sprintf(buffer, "%d", jogadoY);
            encodeMessage(recvline, buffer);

            buffer[0] = 0;
            sprintf(buffer, "%d", jogador);
            encodeMessage(recvline, buffer);

            /*Tamanho fixo da jogado 04play1X1Y*/
            printf("write message : %s\n", recvline);
            time(&t0);
            write(*s->connfd, recvline, strlen(recvline));
            n = read(*s->connfd, ping, 4);
            ping[4] = 0;
            time(&t1);
            diff_time = difftime(t1, t0);
            delay_insert(delay, diff_time);

            /* Marca jogada e imprime tabuleiro */
            /* Quem começa a jogar fica com o zero */
            updateTab(tabuleiro, jogadaX, jogadoY, jogador);
            printTab(tabuleiro);
            /* Checa se alguem venceu status
            
            int status = checkGameStatus(tabuleiro);
            if (status != 3){
            /* Envia mensagem para server */

            recvline[0] = 0;
            encodeMessage(recvline, "gameStatus");

            int status = checkGameStatus(tabuleiro);
            printf("Status do tabuleiro foi %d \n", status);
            if (status != 3)
            {

                char *buffer = malloc(sizeof(char));
                buffer[0] = 0;

                if (status == jogador)
                {
                    printf("Parabéns você venceu a partida!!!\n");
                    sprintf(buffer, "%d", 2);
                    encodeMessage(recvline, buffer);
                }
                else if (status == adv)
                {
                    sprintf(buffer, "%d", 0);
                    encodeMessage(recvline, buffer);
                }
                else
                {
                    sprintf(buffer, "%d", 1);
                    encodeMessage(recvline, buffer);
                }
                free(buffer);
                write(*s->connfd_server, recvline, 15);
                return NULL;
            }

            n = read(*s->connfd, recvline, MAXLINE);
            recvline[n] = 0;
            write(*s->connfd, "ping", 4);
            int size_com = decode_size(recvline, 0);
            char *prov = malloc(sizeof(char) * (size_com + 1));
            memcpy(prov, &recvline[2], size_com);
            prov[size_com] = 0;
            while (strcmp(prov, "play") != 0)
            {
                if (strcmp(prov, "delay") == 0)
                    printf("Delay : %.4f %.4f %.4f\n", delay[0], delay[1], delay[2]);
                if (strcmp(prov, "over") == 0)
                {
                    /*Mandar mensagem para o servidor avisando que acabou*/
                    recvline[0] = 0;
                    encodeMessage(recvline, "gameStatus");
                    buffer = malloc(sizeof(char));
                    buffer[0] = 0;
                    sprintf(buffer, "%d", 3);
                    encodeMessage(recvline, buffer);
                    write(*s->connfd_server, recvline, 15);
                    free(buffer);
                    return NULL;
                }
                n = read(*s->connfd, recvline, MAXLINE);
                recvline[n] = 0;
                write(*s->connfd, "ping", 4);
                size_com = decode_size(recvline, 0);
                free(prov);
                prov = (char *)malloc(sizeof(char) * (size_com + 1));
                memcpy(prov, &recvline[2], size_com);
                prov[size_com] = 0;
            }

            /*Tem que ler um play aqui*/
            /*Recebe mensagem no formato:
                    04play1X1Y1J
                    X - posição da jogada x
                    Y - posição da jogada y
                    J - Tipo do jogador 0 ou 1
                */

            jogadaX = (int)(recvline[8] - '0');
            jogadaY = (int)(recvline[11] - '0');
            printf("O adversário fez a jogada %d %d\n", jogadaX, jogadaY);
            updateTab(tabuleiro, jogadaX, jogadaY, adv);
            printTab(tabuleiro);

            status = checkGameStatus(tabuleiro);
            printf("Status do tabuleiro foi %d\n", status);
            if (status != 3)
            {
                /* Envia mensagem para server */
                recvline[0] = 0;
                encodeMessage(recvline, "gameStatus");

                buffer = malloc(sizeof(char));
                buffer[0] = 0;

                if (status == jogador)
                {
                    printf("Parabéns você venceu a partida!!!\n");
                    sprintf(buffer, "%d", 2);
                    encodeMessage(recvline, buffer);
                }
                else if (status == adv)
                {
                    sprintf(buffer, "%d", 0);
                    encodeMessage(recvline, buffer);
                }
                else
                {
                    sprintf(buffer, "%d", 1);
                    encodeMessage(recvline, buffer);
                }
                free(buffer);
                write(*s->connfd_server, recvline, 15);
                return NULL;
            }
            break;
        case OVER:
            /*Mandar mensagem para o servidor avisando que acabou*/
            /*gameStatus 0*/
            recvline[0] = 0;
            encodeMessage(recvline, "gameStatus");
            buffer = malloc(sizeof(char));
            buffer[0] = 0;
            sprintf(buffer, "%d", 3);
            encodeMessage(recvline, buffer);
            write(*s->connfd_server, recvline, 15);
            free(buffer);

            write(*s->connfd, "04over", 6);
            return NULL;
        case DELAY:
            write(*s->connfd, "05delay", 7);
            break;
        }
    }
}

void *Listen_for_request(void *args)
{
    /*
    Adicionar socket do server 
    */

    request_struct *s = args;
    int connfd_request;
    unsigned char recvline[MAXLINE];
    int n = 0;
    double delay[3];

    if (listen(*s->connfd, LISTENQ) == -1)
    {
        perror("listen :(\n");
        exit(4);
    }
    while (1)
    {

        if ((connfd_request = accept(*s->connfd, (struct sockaddr *)NULL, NULL)) != -1)
        {
            n = read(connfd_request, recvline, MAXLINE);
            recvline[n] = 0;
            int querJogar = 0, adv_len = decode_size(recvline, 0), i;
            char *adv = malloc(sizeof(char) * (adv_len + 1));
            for (i = 0; i < adv_len; adv[i] = recvline[i + 2], i++)
                ;
            adv[i] = 0;
            printf("\nO usuário %s está solicitando uma partida. Para aceitar digite 'request accept' e para recusar digite 'request reject'\nJogoDaVelha>", adv);
            scanf("%s", &recvline[0]);
            printf("%s\n", recvline);
            if (strcmp(recvline, "accept") == 0)
            {
                write(*s->connfd_server, "07request06accept", 17);
                write(connfd_request, "accept", 6);
                querJogar = 1;
            }
            else
            {
                write(*s->connfd_server, "07request06reject", 17);
                write(connfd_request, "reject", 6);
            }
            if (querJogar)
            {
                /*
                Cálculo do delay
                */
                printf("Quer jogar\n");
                time_t t0, t1;
                double diff_time;
                n = read(connfd_request, recvline, MAXLINE);
                for (int k = 0; k < 3; k++)
                {
                    time(&t0);
                    /*    Manda e recebe a mensagem : "ping"     */

                    write(connfd_request, "ping", 4);
                    n = read(connfd_request, recvline, MAXLINE);
                    recvline[n] = 0;
                    time(&t1);

                    diff_time = difftime(t1, t0);
                    delay[k] = diff_time;
                }

                int **tabuleiro = createTab();

                /*
                Vai para outra thread com as funções da partida já com o tabuleiro
                e o vetor de delay
                */
                partida_struct *novoJogo = malloc(sizeof *novoJogo);
                novoJogo->tabuleiro = tabuleiro;
                novoJogo->connfd = &connfd_request;
                novoJogo->delay = delay;
                novoJogo->connfd_server = s->connfd_server;

                pthread_t gameOnThread;
                pthread_create(&gameOnThread, NULL, gameOn, novoJogo);
                pthread_join(gameOnThread, NULL);
            }
            free(adv);
        }
        else
        {
            printf("Conxeão para jogar :(\n");
        }

        close(connfd_request);
        char path_fifo[MAXUSERLENGHT];
        path_fifo[0] = 0;
        strcat(path_fifo, "./tmp/");
        strcat(path_fifo, s->user);
        strcat(path_fifo, ".XXXXXX");
        int fd = open(path_fifo, O_WRONLY);
        write(fd, " ", 1);
        close(fd);
        remove(path_fifo);
    }
}

void logado(int connfd, char *user)
{
    printf("Entrou no logado do jogador\n");
    char recvline[MAXLINE];
    char *message, *adv, *user_mes;
    struct sockaddr_in play_addr;
    int listenfd, playfd, n, user_len;
    unsigned char erro[3] = "-1\0", sucesso[3] = "00\0";
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket :(\n");
        exit(2);
    }
    bzero(&play_addr, sizeof(play_addr));
    play_addr.sin_family = AF_INET;
    play_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    play_addr.sin_port = htons(0);

    if (bind(listenfd, (struct sockaddr *)&play_addr, sizeof(play_addr)) < 0)
        fprintf(stderr, "bind error :(\n");
    socklen_t prov = sizeof(play_addr);
    if (getsockname(listenfd, (struct sockaddr *)&play_addr, &prov) < 0)
    {
        perror("hostname :( \n");
        exit(1);
    }

    int port = (int)ntohs(play_addr.sin_port), count, i, aux;
    ;
    printf("%d\n", port);
    char *ip = inet_ntoa(play_addr.sin_addr), ip_len[3];
    ip_len[0] = strlen(ip) / 10 + '0';
    ip_len[1] = strlen(ip) % 10 + '0';
    ip_len[2] = 0;
    for (count = 0, aux = port; aux > 0; count++, aux /= 10)
        ;
    char port_char[count + 1], port_len[3];
    port_len[0] = count / 10 + '0';
    port_len[1] = count % 10 + '0';
    port_len[2] = 0;
    port_char[count] = 0;
    for (aux = port, i = count; i > 0; port_char[i - 1] = (aux % 10) + '0', aux /= 10, i--)
        ;
    message = (char *)malloc((strlen(ip) + count + 5) * sizeof(char));
    message[0] = 0;
    strcat(message, ip_len);
    strcat(message, ip);
    strcat(message, port_len);
    strcat(message, port_char);
    printf("Len porta: %s \n porta: %s\n", port_len, port_char);
    write(connfd, message, strlen(message));
    request_struct *args = malloc(sizeof *args);
    args->connfd = &listenfd;
    args->connfd_server = &connfd;
    args->user = user;
    pthread_t listen_for_request;
    pthread_create(&listen_for_request, NULL, Listen_for_request, args);
    user_len = strlen(user);
    user_mes = (char *)malloc(sizeof(char) * (user_len + 3));
    user_mes[0] = user_len / 10 + '0';
    user_mes[1] = user_len % 10 + '0';
    for (i = 0; i < user_len; user_mes[i + 2] = user[i], i++)
        ;
    user_mes[i + 2] = 0;
    while (1)
    {
        unsigned char *command = malloc((MAXLINE + 1) * sizeof(char));
        command[0] = 0;
        printf("JogoDaVelha>");
        scanf("%s", command);
        printf("Scanf do logado\n");
        int command_cod = cod_command(command);
        printf("Comando inciado foi: %s\n", command);
        printf("O código do comando é: %d\n", command_cod);
        switch (command_cod)
        {
        case HALLOFFAME:
            printf("Entro aqui - no halloffame\n");
            write(connfd, "10halloffame", 12);
            n = read(connfd, recvline, 2);
            recvline[n] = 0;
            while (strcmp(recvline, "00"))
            {
                int tam = (recvline[0] - '0') * 10 + (recvline[1] - '0');
                n = read(connfd, recvline, tam + 4);
                recvline[n] = 0;
                char *user_rcv = malloc(sizeof(char) * (tam + 1));
                memcpy(user_rcv, &recvline[0], tam);
                user_rcv[tam] = 0;

                char user_points[3];
                memcpy(user_points, &recvline[tam + 1], 3);
                user_points[3] = 0;

                printf("Pontuação de %s : %s\n", user_rcv, user_points);
                free(user_rcv);
                n = read(connfd, recvline, 2);
                recvline[n] = 0;
            }
            /*Itera no arquivo de logados, ver pontuação e coloca o nome*/
            break;

        case L:
            write(connfd, "01l", 3);
            n = read(connfd, recvline, 2);
            recvline[n] = 0;
            while (strcmp(recvline, "00"))
            {
                int tam = (recvline[0] - '0') * 10 + (recvline[1] - '0');
                n = read(connfd, recvline, tam);
                recvline[n] = 0;
                printf("%s\n", &recvline[0]);
                n = read(connfd, recvline, 2);
                recvline[n] = 0;
            }
            /*Itera no arquivo de logados, ver pontuação e coloca o nome*/
            break;

        case CALL:

            message = (char *)malloc(MAXLINE * sizeof(char));
            message[0] = 0;
            adv = (char *)malloc(MAXUSERLENGHT * sizeof(char));
            adv[0] = 0;
            scanf("%s", adv);
            printf("Nome do adversário: %s\n", adv);
            printf("Mensagem vazia: %s\n", message);

            encodeMessage(message, command);
            printf("Parte incial da mensagem é: %s\n", message);
            encodeMessage(message, adv);
            printf("Enviou CALL a seguinte mensagem para o servidor: %s\n", message);
            write(connfd, message, strlen(message));

            recvline[0] = 0;
            n = read(connfd, &recvline[0], MAXLINE);
            recvline[n] = 0;
            printf("Leu CALL a seguinte a mensagem do servidor: %s\n", &recvline[0]);

            if (recvline[0] == '-' && recvline[1] == '1')
            {
                printf("Usuário não existente ou não disponível\n");
                break;
            }
            recvline[n] = 0;
            int tam_ip, tam_port, sock_req;
            struct sockaddr_in req_addr;
            tam_ip = decode_size(recvline, 0);
            tam_port = decode_size(recvline, 2 + tam_ip);
            char *ip = malloc(sizeof(char) * (tam_ip + 1));
            char *port_char = malloc(sizeof(char) * (tam_port + 1));
            for (i = 0; i < tam_ip; ip[i] = recvline[2 + i], i++)
                ;
            ip[i] = 0;
            for (i = 0; i < tam_port; port_char[i] = recvline[4 + tam_ip + i], i++)
                ;
            port_char[i] = 0;
            printf("port : %s, ip : %s\n", port_char, ip);
            if ((sock_req = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                fprintf(stderr, "socket error :( \n");
            bzero(&req_addr, sizeof(req_addr));
            req_addr.sin_family = AF_INET;
            req_addr.sin_port = htons(atoi(port_char));
            if (inet_pton(AF_INET, ip, &req_addr.sin_addr) <= 0)
                fprintf(stderr, "inet_pton error for %s :(\n", ip);

            if (connect(sock_req, (struct sockaddr *)&req_addr, sizeof(req_addr)) < 0)
                fprintf(stderr, "connect error :(\n");
            write(sock_req, user_mes, user_len + 2);
            n = read(sock_req, recvline, MAXLINE);
            recvline[n] = 0;

            if (strcmp(recvline, "accept") == 0)
            {
                /*Cálculo do delay*/
                printf("Aceitou !!!!!!!!!!!!!!!!!!!!!!!!\n");
                write(connfd, "07request06accept", 17);
                double delay[3];
                time_t t0, t1;
                double diff_time;
                for (int k = 0; k < 3; k++)
                {
                    time(&t0);
                    /*    Manda e recebe a mensagem : "ping"     */
                    printf("Delay\n");
                    write(sock_req, "ping", 4);
                    n = read(sock_req, recvline, MAXLINE);
                    recvline[n] = 0;
                    time(&t1);

                    diff_time = difftime(t1, t0);
                    delay[k] = diff_time;
                }

                /*Mais um envio para completar o delay do adversário*/
                write(sock_req, "ping", 4);
                printf("OK\n");

                /*Faz a primeira jogada antes de entrar na thread*/
                int jogadaX, jogadoY;
                printf("Faça a sua jogado no formato: play X Y\nJogoDaVelha>");
                command[0] = 0;
                scanf("%s", command);
                scanf("%d", &jogadaX);
                scanf("%d", &jogadoY);

                /*Cria tabuleiro*/
                int **tabuleiro = createTab();

                /* Testa se é uma jogada valida */
                while (!validation(tabuleiro, jogadaX, jogadoY))
                {
                    printf("Jogada invalida, tente outra jogada!");
                    command[0] = 0;
                    scanf("%s", command);
                    scanf("%d", &jogadaX);
                    scanf("%d", &jogadoY);
                }

                /* Envia jogada para outro jogador */
                recvline[0] = 0;
                encodeMessage(recvline, command);

                char *buffer = malloc(sizeof(char));
                buffer[0] = 0;
                sprintf(buffer, "%d", jogadaX);
                encodeMessage(recvline, buffer);

                buffer[0] = 0;
                sprintf(buffer, "%d", jogadoY);
                encodeMessage(recvline, buffer);

                /*O jogador incial é o Zero*/
                buffer[0] = 0;
                sprintf(buffer, "%d", 0);
                encodeMessage(recvline, buffer);

                write(sock_req, recvline, strlen(recvline));
                write(sock_req, "ping", 4);

                /* Marca jogada e imprime tabuleiro */
                /* Quem começa a jogar fica com o zero */
                updateTab(tabuleiro, jogadaX, jogadoY, 0);
                printTab(tabuleiro);

                /*Entra na thread já passando o tabuleiro*/
                partida_struct *novoJogo = malloc(sizeof *novoJogo);
                novoJogo->tabuleiro = tabuleiro;
                novoJogo->connfd = &sock_req;
                novoJogo->delay = delay;
                novoJogo->connfd_server = &connfd;

                pthread_t gameOnThread;
                pthread_create(&gameOnThread, NULL, gameOn, novoJogo);
                pthread_join(gameOnThread, NULL);
            }
            else
            {
                close(sock_req);
                write(connfd, "07request06reject", 17);
            }
            break;

        case REQUEST:; //Espera pela resposta do server
            char path_fifo[MAXUSERLENGHT];
            path_fifo[0] = 0;
            strcat(path_fifo, "./tmp/");
            strcat(path_fifo, user);
            strcat(path_fifo, ".XXXXXX");
            mkfifo((const char *)path_fifo, 0644);
            int fd = open(path_fifo, O_RDONLY);
            unlink((const char *)path_fifo);
            n = read(fd, recvline, MAXLINE);
            close(fd);
            printf("Destravei\n");
            break;

        case OUT:
            message = (char *)malloc(MAXLINE * sizeof(char));
            message[0] = 0;
            encodeMessage(message, command);
            write(connfd, message, strlen(message));
            close(listenfd);
            pthread_cancel(listen_for_request);
            return;

        case PASS:;
            unsigned char *message = (char *)malloc(MAXLINE * sizeof(char));
            message[0] = 0;

            unsigned char *pass = (char *)malloc(MAXUSERLENGHT * sizeof(char));
            pass[0] = 0;
            unsigned char *newpass = (char *)malloc(MAXUSERLENGHT * sizeof(char));
            newpass[0] = 0;
            scanf("%s", pass);
            scanf("%s", newpass);

            encodeMessage(message, command);
            printf("Parte incial da mensagem é: %s\n", message);
            encodeMessage(message, pass);
            encodeMessage(message, newpass);

            printf("Comando Enviado foi: %s\n", message);
            write(connfd, message, strlen(message));
            unsigned char recive[3];
            recive[0] = 0;
            n = read(connfd, recive, 3);
            recvline[n] = '\0';
            printf("Resposta recebida foi: %s\n", recive);
            printf("Mensagem de sucesso é: %s\n", sucesso);

            if (strcmp(recive, sucesso) == 0)
            {
                printf("Senha Atualizada com sucesso\n");
            }
            if (strcmp(recive, erro) == 0)
                printf("Erro ao realizar ao alterar a senha\n");
            free(pass);
            free(newpass);
            free(message);

        default:
            break;
        }
        free(command);
    }
}

int main(int argc, char **argv)
{
    int sockfd, n, heartbeatfd, listenheartbeat;
    unsigned char erro[3] = "-1\0", sucesso[3] = "00\0";
    ;
    char recvline[MAXLINE + 1];
    /*;*/

    struct sockaddr_in servaddr, heartbeat_addr, heartbeat_addr_act;

    if (argc != 3)
        fprintf(stderr, "usage: %s <IPaddress>\n", argv[0]);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        fprintf(stderr, "socket error :( \n");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        fprintf(stderr, "inet_pton error for %s :(\n", argv[1]);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        fprintf(stderr, "connect error :(\n");

    listenheartbeat = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&heartbeat_addr, sizeof(heartbeat_addr));
    heartbeat_addr.sin_family = AF_INET;
    heartbeat_addr.sin_addr.s_addr = INADDR_ANY; /*Fazendo bind para que o cliente só aceite conexão do servidor*/
    heartbeat_addr.sin_port = htons(0);

    if (bind(listenheartbeat, (struct sockaddr *)&heartbeat_addr, sizeof(heartbeat_addr)) < 0)
        fprintf(stderr, "bind error :(\n");

    socklen_t prov = sizeof(heartbeat_addr_act);
    if (getsockname(listenheartbeat, (struct sockaddr *)&heartbeat_addr_act, &prov) < 0)
    {
        perror("hostname :( \n");
        exit(1);
    }

    int port = (int)ntohs(heartbeat_addr_act.sin_port), count, aux, i; /*Pegando o valor da porta escolhida*/
    printf("%d\n", port);
    for (count = 0, aux = port; aux > 0; count++, aux /= 10)
        ;
    char port_char[count];
    for (aux = port, i = count; i > 0; port_char[i - 1] = (aux % 10) + '0', aux /= 10, i--)
        ;                            /*Trasformando o valor da porta em char*/
    write(sockfd, port_char, count); /*Mandando a porta me que estará escutando para o servidor*/
    printf("%s\n", port_char);

    if (listen(listenheartbeat, LISTENQ) == -1)
    {
        perror("listen :(\n");
        exit(4);
    }
    if ((heartbeatfd = accept(listenheartbeat, (struct sockaddr *)NULL, NULL)) == -1)
    {
        perror("accept :(\n");
        exit(5);
    }
    close(listenheartbeat);
    heartbeat_struct *args = malloc(sizeof *args);
    args->connfd = &heartbeatfd;
    pthread_t heartbeat;
    pthread_create(&heartbeat, NULL, Heartbeat_Client, args);

    /*if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        fprintf(stderr,"connect error :(\n");*/

    char recive[3];
    char *message, *user, *pass;
    char *command = malloc((MAXLINE + 1) * sizeof(char));
    while (1)
    {
        command[0] = 0;
        printf("JogoDaVelha>");
        scanf("%s", command);
        printf("Scanf do main\n");
        int command_cod = cod_command(command);
        printf("Comando inciado foi: %s\n", command);
        printf("Código inciado foi: %d\n", command_cod);
        switch (command_cod)
        {
        /*
        IN e NEW tem mesma esturtura do lado do cliente
        Portanto podemos rodar os dois com a seguinte estrutura;
        */
        case IN:
            message = (char *)malloc(MAXLINE * sizeof(char));
            message[0] = 0;
            user = (char *)malloc(MAXUSERLENGHT * sizeof(char));
            user[0] = 0;
            pass = (char *)malloc(MAXUSERLENGHT * sizeof(char));
            pass[0] = 0;
            scanf("%s", user);
            scanf("%s", pass);
            printf("Mensagem vazia: %s\n", message);

            encodeMessage(message, command);
            printf("Parte incial da mensagem é: %s\n", message);
            encodeMessage(message, user);
            encodeMessage(message, pass);

            printf("Comando Enviado foi: %s\n", message);
            write(sockfd, message, strlen(message));
            recive[3];
            recive[0] = 0;
            n = read(sockfd, recive, 3);
            recvline[n] = '\0';
            printf("Resposta recebida foi: %s\n", recive);
            printf("Mensagem de sucesso é: %s\n", sucesso);

            if (strcmp(recive, sucesso) == 0)
            {
                printf("Login realizado com sucesso\n");
                logado(sockfd, user);
            }
            if (strcmp(recive, erro) == 0)
                printf("Erro ao realizar o login\n");
            free(user);
            free(pass);
            free(message);
            break;
        case NEW:
            /*Heartbeat Morrendo por isso incapaz de testar criar e logar usuario*/
            message = (char *)malloc(MAXLINE * sizeof(char));
            message[0] = 0;
            user = (char *)malloc(MAXUSERLENGHT * sizeof(char));
            user[0] = 0;
            pass = (char *)malloc(MAXUSERLENGHT * sizeof(char));
            pass[0] = 0;
            scanf("%s", user);
            scanf("%s", pass);
            printf("Mensagem vazia: %s\n", message);
            /*
            Primeiro usuário está sendo criado com sucesso      
            Mas segundo não     
            Por algum motivo o message já tem algum conteúdo e por isso está dando erro
            */

            encodeMessage(message, command);
            printf("Parte incial da mensagem é: %s\n", message);
            encodeMessage(message, user);
            encodeMessage(message, pass);

            printf("Comando Enviado foi: %s\n", message);
            write(sockfd, message, strlen(message));
            recive[3];
            recive[0] = 0;
            n = read(sockfd, recive, 3);
            recvline[n] = '\0';
            printf("Resposta recebida foi: %s\n", recive);
            printf("Mensagem de sucesso é: %s\n", sucesso);

            if (strcmp(recive, sucesso) == 0)
                printf("Operacao realizada com sucesso\n");
            if (strcmp(recive, erro) == 0)
                printf("Erro para realizar a operacao, tente novamente\n");

            free(message);
            free(user);
            free(pass);

            break;

        case BYE:
            write(sockfd, "03bye", 5);
            close(heartbeatfd);
            pthread_cancel(heartbeat);
            close(sockfd);
            exit(0);
        default:
            break;
        }
        //recvline[n] = 0;
    }
    free(command);
    if (n < 0)
        fprintf(stderr, "read error :(\n");

    exit(0);
}
