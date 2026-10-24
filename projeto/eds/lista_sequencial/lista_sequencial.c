#include <stdlib.h>

#include "lista_sequencial.h"

// lista_sequencial.c - vetor dinamico mantido em ordem crescente de chave.
//
// Era um vetor estatico de 100 posicoes. A especificacao exige aceitar
// --base lista_seq com --limit de 10000 ou a base inteira, entao agora a
// capacidade dobra quando enche.
//
// A ordenacao acontece na insercao, nao no fim: a lista esta sempre
// ordenada, o que faz remover_menor ser O(1).

#define CAPACIDADE_INICIAL 16

struct lista_seq {
    ITEM **itens;
    int tamanho;
    int capacidade;
};

LISTA_SEQ *lista_seq_criar(void) {
    LISTA_SEQ *lista = (LISTA_SEQ *)malloc(sizeof(LISTA_SEQ));
    if (lista == NULL) return NULL;

    lista->itens = (ITEM **)malloc(CAPACIDADE_INICIAL * sizeof(ITEM *));
    if (lista->itens == NULL) {
        free(lista);
        return NULL;
    }
    lista->tamanho = 0;
    lista->capacidade = CAPACIDADE_INICIAL;
    return lista;
}

void lista_seq_apagar(LISTA_SEQ **lista) {
    if (lista == NULL || *lista == NULL) return;

    for (int i = 0; i < (*lista)->tamanho; i++) {
        item_apagar(&(*lista)->itens[i]);
    }

    free((*lista)->itens);   // o vetor de ponteiros
    free(*lista);            // a struct
    *lista = NULL;
}

// Dobra o vetor quando enche. realloc em ponteiro temporario: se falhar,
// o vetor original continua intacto e a lista nao fica corrompida.
static bool garantir_espaco(LISTA_SEQ *lista) {
    if (lista->tamanho < lista->capacidade) return true;

    int nova = lista->capacidade * 2;
    ITEM **p = (ITEM **)realloc(lista->itens, nova * sizeof(ITEM *));
    if (p == NULL) return false;

    lista->itens = p;
    lista->capacidade = nova;
    return true;
}

// Insercao ordenada. Acha a posicao, recusa duplicata, abre espaco e insere.
// O deslocamento e o que garante a invariante "sempre ordenada" exigida
// pela especificacao (nao e permitido inserir tudo e ordenar no fim).
bool lista_seq_inserir(LISTA_SEQ *lista, ITEM *item) {
    if (lista == NULL || item == NULL) return false;
    if (!garantir_espaco(lista)) return false;

    int chave = item_get_chave(item);
    int posicao = 0;

    while (posicao < lista->tamanho &&
           item_get_chave(lista->itens[posicao]) < chave) {
        posicao++;
    }

    // chave ja presente: descarta. A estrutura e um conjunto, nao um multiconjunto.
    if (posicao < lista->tamanho &&
        item_get_chave(lista->itens[posicao]) == chave) {
        return false;
    }

    for (int i = lista->tamanho; i > posicao; i--) {
        lista->itens[i] = lista->itens[i - 1];
    }

    lista->itens[posicao] = item;
    lista->tamanho++;
    return true;
}

// O menor esta sempre no indice 0 porque a insercao mantem a ordem.
void lista_seq_remover_menor(LISTA_SEQ *lista) {
    if (lista == NULL || lista->tamanho == 0) return;

    item_apagar(&lista->itens[0]);

    for (int i = 1; i < lista->tamanho; i++) {
        lista->itens[i - 1] = lista->itens[i];
    }

    lista->tamanho--;
    lista->itens[lista->tamanho] = NULL;   // nao deixa ponteiro solto
}

int lista_seq_tam(LISTA_SEQ *lista) { return lista == NULL ? -1 : lista->tamanho; }

bool lista_seq_vazia(LISTA_SEQ *lista) { return lista == NULL || lista->tamanho == 0; }

// Percurso do indice 0 (menor chave) para o ultimo.
int lista_seq_percorrer(LISTA_SEQ *lista, ItemVisita visita, void *contexto) {
    if (lista == NULL || visita == NULL) return 0;

    int n = 0;
    for (int i = 0; i < lista->tamanho; i++) {
        if (!visita(lista->itens[i], contexto)) break;
        n++;
    }
    return n;
}