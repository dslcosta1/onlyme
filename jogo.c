#include <stdio.h>
#include <stdlib.h>
#include "jogo.h"

int **createTab()
{
    int i, j, **tab = malloc(3 * sizeof(int *));
    for (i = 0; i < 3; tab[i] = malloc(3 * sizeof(int)), i++)
        ;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; tab[i][j] = -5, j++)
            ;
    return tab;
}

void cleanTab(int **tab)
{
    for (int i = 0; i < 3; free(tab[i]), i++)
        ;
    free(tab);
    return;
}

void updateTab(int **tab, int i, int j, int s)
{
    tab[i][j] = s;
    return;
}
int validation(int **tab, int i, int j)
{
    if (i < 0 || i > 2)
        return 0;
    if (j < 0 || j > 2)
        return 0;
    if (tab[i][j] != -5)
        return 0;
    return 1;
}

int checkGameStatus(int **tab)
{ /*0 - player 0 ganhou; 1 - player 1 ganhou; 2 - empate; 3 -não acabou*/
    int i, j, aux = 0;

    for (i = 0; i < 3; i++)
    { /*Linhas*/
        int val_line = tab[i][0] + tab[i][1] + tab[i][2];
        printf("Valor da linha %d\n", val_line);
        if (val_line >= 0)
        { /*Linha 'cheia'*/
            if (val_line == 0)
                return 0;
            else if (val_line == 3)
                return 1;
        }
        else
            aux++;
        printf("O valor do aux foi: %d\n", aux);
    }

    for (i = 0; i < 3; i++)
    { /*Colunas*/
        int val_coluna = tab[0][i] + tab[1][i] + tab[2][i];
        printf("Valor da coluna %d\n", val_coluna);
        if (val_coluna >= 0)
        { /*coluna 'cheia'*/
            if (val_coluna == 0)
                return 0;
            else if (val_coluna == 3)
                return 1;
        }
    }
    int val_diagonal_pri = tab[0][0] + tab[1][1] + tab[2][2];
    printf("Valor da diagonal 1 %d\n", val_diagonal_pri);
    if (val_diagonal_pri >= 0)
    { /*Diagonal 'cheia'*/
        if (val_diagonal_pri == 0)
            return 0;
        else if (val_diagonal_pri == 3)
            return 1;
    }

    int val_diagonal_seg = tab[0][2] + tab[1][1] + tab[2][0];
    printf("Valor da diagonal 2 %d\n", val_diagonal_seg);
    if (val_diagonal_seg >= 0)
    { /*Diagonal 'cheia'*/
        if (val_diagonal_seg == 0)
            return 0;
        else if (val_diagonal_seg == 3)
            return 1;
    }
    if (aux > 0)
        return 3;
    else
        return 2;
}

void printTab(int **tab)
{
    int i, j;
    printf("\n    0   1   2 \n\n0  ");

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            if (tab[i][j] == -5)
                printf("   ");
            else if (tab[i][j] == 0)
                printf(" X ");
            else
                printf(" O ");
            if (j != 2)
                printf("|");
            else
                printf("\n");
        }
        if (i != 2)
            printf("   -----------\n%d  ", i + 1);
        else
            printf("\n\n");
    }
}