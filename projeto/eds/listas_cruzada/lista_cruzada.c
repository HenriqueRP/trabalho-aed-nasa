#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "lista_cruzada.h"

// lista_cruzada.c - matriz esparsa representada por duas cadeias de nos.
//
// Cada no pertence a UMA linha e a UMA coluna ao mesmo tempo: e o mesmo
// registro participando de duas organizacoes, sem duplicar o item.
// 'abaixo' percorre a coluna, 'direita' percorre a linha.
//
// O item aparece uma vez so na memoria, mas e alcancavel por dois caminhos -
// que e exatamente o que a especificacao pede.

typedef struct no_cruzado NO;

struct no_cruzado {
    ITEM *item;
    int x, y;        // posicao NESTA matriz, nao no registro
    NO *direita;
    NO *abaixo;
};

struct lista_cruzada {
    NO **colunas;    // vetor de cabecas de coluna
    NO **linhas;     // vetor de cabecas de linha
    int nLinhas;
    int nColunas;
};

LISTA_CRUZ *lista_criar(int nlinhas, int ncolunas) {
    if (nlinhas <= 0 || ncolunas <= 0) return NULL;

    LISTA_CRUZ *lista = malloc(sizeof(LISTA_CRUZ));
    if (lista == NULL) return NULL;

    NO **colunas = malloc(ncolunas * sizeof(NO *));
    NO **linhas  = malloc(nlinhas * sizeof(NO *));
    if (colunas == NULL || linhas == NULL) {
        free(colunas);
        free(linhas);
        free(lista);
        return NULL;
    }

    lista->colunas = colunas;
    lista->linhas = linhas;
    lista->nLinhas = nlinhas;
    lista->nColunas = ncolunas;

    for (int i = 0; i < nlinhas; i++)  linhas[i]  = NULL;
    for (int i = 0; i < ncolunas; i++) colunas[i] = NULL;
    return lista;
}

bool lista_inserir(LISTA_CRUZ *lista, ITEM *item, int posX, int posY) {
    if (lista == NULL || item == NULL) return false;
    if (posX < 0 || posY < 0 || posX >= lista->nLinhas || posY >= lista->nColunas)
        return false;

    // celula ocupada: recusa. Sem isso, dois itens na mesma coordenada
    // deixariam o segundo inalcancavel pelo lista_get.
    if (lista_get(lista, posX, posY) != NULL) return false;

    NO *no = malloc(sizeof(NO));
    if (no == NULL) return false;

    no->item = item;
    no->x = posX;
    no->y = posY;
    no->abaixo = NULL;
    no->direita = NULL;

    // Encaixa na coluna posY, mantendo a ordem crescente de x.
    // Comparar no->x (e nao o item) elimina qualquer chance de a coordenada
    // do registro divergir da posicao onde o no foi inserido.
    NO *ant = NULL;
    NO *atual = lista->colunas[posY];
    while (atual != NULL && atual->x < posX) {
        ant = atual;
        atual = atual->abaixo;
    }
    no->abaixo = atual;
    if (ant == NULL) lista->colunas[posY] = no;
    else             ant->abaixo = no;

    // Mesma coisa na linha posX, agora ordenado por y.
    ant = NULL;
    atual = lista->linhas[posX];
    while (atual != NULL && atual->y < posY) {
        ant = atual;
        atual = atual->direita;
    }
    no->direita = atual;
    if (ant == NULL) lista->linhas[posX] = no;
    else             ant->direita = no;

    return true;
}

ITEM *lista_get(const LISTA_CRUZ *lista, int posX, int posY) {
    if (lista == NULL) return NULL;
    if (posX < 0 || posY < 0 || posX >= lista->nLinhas || posY >= lista->nColunas)
        return NULL;

    // desce pela coluna posY procurando x == posX
    NO *no = lista->colunas[posY];
    while (no != NULL && no->x != posX) no = no->abaixo;

    return no ? no->item : NULL;
}

ITEM *lista_remover(LISTA_CRUZ *lista, int posX, int posY) {
    if (lista == NULL) return NULL;
    if (posX < 0 || posY < 0 || posX >= lista->nLinhas || posY >= lista->nColunas)
        return NULL;

    // localiza na coluna guardando o anterior
    NO *ant_col = NULL;
    NO *no = lista->colunas[posY];
    while (no != NULL && no->x != posX) {
        ant_col = no;
        no = no->abaixo;
    }
    if (no == NULL) return NULL;

    // confirma que o no tambem esta na linha ANTES de desligar qualquer coisa.
    // Sem esta checagem, um no so presente na coluna causaria acesso a
    // ponteiro nulo logo abaixo.
    NO *ant_lin = NULL;
    NO *aux = lista->linhas[posX];
    while (aux != NULL && aux != no) {
        ant_lin = aux;
        aux = aux->direita;
    }
    if (aux == NULL) return NULL;   // lista inconsistente: nao mexe em nada

    if (ant_col == NULL) lista->colunas[posY] = no->abaixo;
    else                 ant_col->abaixo = no->abaixo;

    if (ant_lin == NULL) lista->linhas[posX] = no->direita;
    else                 ant_lin->direita = no->direita;

    ITEM *item = no->item;
    free(no);        // so o no: o item sai pelo retorno
    return item;
}

// Percurso linha por linha, coluna por coluna dentro de cada linha.
// E a ordem de leitura da matriz, nao a ordem de insercao.
int lista_cruz_percorrer(LISTA_CRUZ *lista, ItemVisita visita, void *contexto) {
    if (lista == NULL || visita == NULL) return 0;

    int n = 0;
    for (int i = 0; i < lista->nLinhas; i++) {
        for (NO *no = lista->linhas[i]; no != NULL; no = no->direita) {
            if (!visita(no->item, contexto)) return n;
            n++;
        }
    }
    return n;
}

void lista_imprimir(const LISTA_CRUZ *lista, void (*imprimir_item)(ITEM *)) {
    if (lista == NULL || imprimir_item == NULL) return;

    printf("Lista cruzada %d x %d\n", lista->nLinhas, lista->nColunas);

    for (int i = 0; i < lista->nLinhas; i++) {
        bool vazia = true;
        for (NO *no = lista->linhas[i]; no != NULL; no = no->direita) {
            if (vazia) { printf("linha %d:", i); vazia = false; }
            printf(" [col %d] ", no->y);
            imprimir_item(no->item);
        }
        if (vazia) printf("linha %d: (vazia)", i);
        printf("\n");
    }
}

void lista_apagar(LISTA_CRUZ **lista) {
    if (lista == NULL || *lista == NULL) return;

    LISTA_CRUZ *lst = *lista;

    // Varre so pelas linhas. Cada no pertence a exatamente uma linha, entao
    // varrer as colunas tambem daria double free.
    for (int i = 0; i < lst->nLinhas; i++) {
        NO *aux = lst->linhas[i];
        while (aux != NULL) {
            NO *prox = aux->direita;
            item_apagar(&aux->item);   // era free(aux->item): vazava o conteudo
            free(aux);
            aux = prox;
        }
        lst->linhas[i] = NULL;
    }

    free(lst->colunas);
    free(lst->linhas);
    free(lst);
    *lista = NULL;
}