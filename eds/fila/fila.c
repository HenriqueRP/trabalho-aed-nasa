#include "fila.h"
#include "item.h"
#include <stdlib.h>
#include <stdbool.h>

typedef struct no NO;
struct no{
    ITEM *item;
    NO *prox;
};

static NO *no_criar(ITEM *item){
    NO *novo = (NO*)malloc(sizeof(NO));
    if(novo == NULL){
        return NULL;
    }
    novo->item = item;
    novo->prox = NULL;
    return novo;
}

static void no_apagar(NO **no){
    if(no == NULL || *no == NULL){
        return;
    }
    free(*no);
    *no = NULL;
}

struct fila{
    NO *head, *tail;
    int tam;
};

FILA *fila_criar(void){
    FILA *nova = (FILA *)malloc(sizeof(FILA));
    if(nova == NULL){
        return NULL;
    }
    nova->head = no_criar(NULL);
    if(nova->head == NULL){
        free(nova);
        nova = NULL;
        return NULL;
    }
    nova->tail = NULL;
    nova->tam = 0;
    return nova;
}

bool fila_enqueue(FILA *fila, ITEM *item){
    if(fila == NULL){
        return false;
    }
    if(item == NULL){
        return false;
    }
    NO *novo = no_criar(item);
    if(novo == NULL){
        return false;
    }

    if(fila->tail != NULL){
        fila->tail->prox = novo;
    }
    fila->tail = novo;
    if(fila->tam == 0){
        fila->head->prox = novo;
    }
    (fila->tam)++;

    return true;
}

ITEM *fila_dequeue(FILA *fila){
    if(fila == NULL){
        return NULL;
    }
    if(fila_vazia(fila)){
        return NULL;
    }

    NO *removido = fila->head->prox;
    fila->head->prox = fila->head->prox->prox;
    ITEM *item = removido->item;
    no_apagar(&removido);
    (fila->tam)--;
    if(fila->tam == 0){
        fila->tail = NULL;
    }
    return item;
}

int fila_tamanho(FILA *fila){
    if(fila == NULL){
        return -1;
    }
    return fila->tam;
}

ITEM *fila_frente(FILA *fila){
    if(fila == NULL){
        return NULL;
    }
    if(fila_vazia(fila)){
        return NULL;
    }
    return fila->head->prox->item;
}

bool fila_vazia(FILA *fila){
    if(fila == NULL){
        return true;
    }
    if(fila->tam == 0){
        return true;
    }
    else{
        return false;
    }
}

void apagar_fila(FILA **fila){
    if(fila == NULL || *fila == NULL){
        return;
    }
    while(!fila_vazia(*fila)){
        ITEM *removido = fila_dequeue(*fila);
        item_apagar(&removido);
    }

    no_apagar(&(*fila)->head);

    free(*fila);
    *fila = NULL;
}