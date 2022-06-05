
all: server jogador
 
server: server.o util.o comandosServidor.o heartbeat.o
	gcc -pthread -o server server.o util.o comandosServidor.o heartbeat.o
	
server.o: server.c 
	gcc -c server.c 

jogador: jogador.o util.o jogo.o heartbeat.o
	gcc -pthread -o jogador jogador.o util.o jogo.o heartbeat.o

jogador.o: jogador.c 
	gcc -c jogador.c 

util.o: util.c
	gcc -c util.c

comandosServidor.o: comandosServidor.c
	gcc -c comandosServidor.c

jogo.o: jogo.c
	gcc -c jogo.c

heartbeat.o: heartbeat.c 
	gcc -c heartbeat.c

clean:
	rm -rf *.o server jogador 