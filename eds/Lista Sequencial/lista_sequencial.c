#include <stdio.h>
#include <stdlib.h>

#include "lista.h"

struct lista_ {
    ITEM *itens[TAM_MAX];
    int tamanho;
};

LISTA *lista_seq_criar(void) {
    LISTA *lista = (LISTA *) malloc(sizeof(LISTA));

    if (lista != NULL) {
        lista->tamanho = 0;
    }

    return lista;
}

void lista_seq_apagar(LISTA **lista) {
    int i;

    if (lista == NULL || *lista == NULL) {
        return;
    }

    for (i = 0; i < (*lista)->tamanho; i++) {
        item_apagar(&(*lista)->itens[i]);
    }

    free(*lista);
    *lista = NULL;
}

bool lista_seq_inserir(LISTA *lista, ITEM *item) {
    int chave;
    int posicao;
    int i;

    if (lista == NULL || item == NULL || lista->tamanho >= TAM_MAX) {
        return false;
    }

    chave = item_get_chave(item);
    posicao = 0;

    while (posicao < lista->tamanho &&
           item_get_chave(lista->itens[posicao]) < chave) {
        posicao++;
    }

    if (posicao < lista->tamanho &&
        item_get_chave(lista->itens[posicao]) == chave) {
        printf("Insercao de [%08d] nao realizada: codigo previamente registrado!\n",
               chave);
        return false;
    }

    for (i = lista->tamanho; i > posicao; i--) {
        lista->itens[i] = lista->itens[i - 1];
    }

    lista->itens[posicao] = item;
    lista->tamanho++;

    printf("Codigo [%08d] registrado na posicao [%d].\n", chave, posicao);
    return true;
}

void lista_seq_remover_menor(LISTA *lista) {
    int i;

    if (lista == NULL || lista->tamanho == 0) {
        return;
    }

    item_apagar(&lista->itens[0]);

    for (i = 1; i < lista->tamanho; i++) {
        lista->itens[i - 1] = lista->itens[i];
    }

    lista->tamanho--;
    lista->itens[lista->tamanho] = NULL;
}

void lista_seq_imprimir_final(const LISTA *lista) {
    int i;

    if (lista == NULL) {
        return;
    }

    printf("\nLista final:\n");
    for (i = 0; i < lista->tamanho; i++) {
        printf("%08d\n", item_get_chave(lista->itens[i]));
    }
}
