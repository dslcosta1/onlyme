/*

Biblioteca de funções do tabuleiro/jogo

*/

// 
void updateTab(int ** tab, int i, int j, int s);

// Ver se a posicao é valida
int validation(int ** tab, int i, int j);

/*
-5 | -5 | -5
-5 | -5 | -5
-5 | -5 | -5
*/

int ** createTab();

void cleanTab(int ** tab);

int checkGameStatus(int ** tab);

void printTab(int ** tab);