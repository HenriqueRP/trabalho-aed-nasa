#include "lista_encadeada.h"
#include <stdlib.h>
#include <stdbool.h>

typedef struct no NO;
struct no{
    ITEM *item;
    NO *proximo;
};

static NO *no_criar(ITEM *item){
    NO *novo = (NO *)malloc(sizeof(NO));
    if(novo == NULL){
        return NULL;
    }
    novo->item = item;
    novo->proximo = NULL;
    return novo;
}

static void no_apagar(NO **no){
    if(no == NULL || *no == NULL){
        return;
    }
    free(*no);
    *no = NULL;
}

struct lista_encadeada{
    NO *inicio, *fim;
    int tam;
};

LISTA_ENC *lista_enc_criar(void){
    LISTA_ENC *nova = (LISTA_ENC *)malloc(sizeof(LISTA_ENC));
    if(nova == NULL){
        return NULL;
    }

    nova->inicio = NULL;
    nova->fim = NULL;
    nova->tam = 0;
    return nova;
}

bool lista_enc_adicionar_fim(LISTA_ENC *lista, ITEM *item){
    if(lista == NULL || item == NULL){
        return false;
    }

    NO *no = no_criar(item);
    if(no == NULL){
        return false;
    }
    if(lista->fim != NULL){
        lista->fim->proximo = no;
    }
    lista->fim = no;
    if(lista->inicio == NULL){
        lista->inicio = no;
    }

    (lista->tam)++;
    return true;
}

bool lista_enc_adicionar_inicio(LISTA_ENC *lista, ITEM *item){
    if(lista == NULL || item == NULL){
        return false;
    }

    NO *no = no_criar(item);
    if(no == NULL){
        return false;
    }
    no->proximo =  lista->inicio;
    lista->inicio = no;
    if(lista->fim == NULL){
        lista->fim = no;
    }

    (lista->tam)++;
    return true;
}

ITEM *lista_enc_remover_inicio(LISTA_ENC *lista){
    if(lista == NULL || lista->tam == 0){
        return NULL;
    }
    NO *removido = lista->inicio;
    ITEM *item = removido->item;
    lista->inicio = removido->proximo;
    no_apagar(&removido);
    (lista->tam)--;
    if(lista_enc_vazia(lista)){
        lista->fim = NULL;
    }
    return item;
}

int lista_enc_tam(LISTA_ENC *lista){
    if(lista == NULL){
        return -1;
    }
    return lista->tam;
}

bool lista_enc_vazia(LISTA_ENC *lista){
    if(lista == NULL || lista->tam == 0){
        return true;
    }
    return false;
}

void lista_enc_apagar(LISTA_ENC **lista){
    if(lista == NULL || *lista == NULL){
        return;
    }
    while(!lista_enc_vazia(*lista)){
        ITEM *removido = lista_enc_remover_inicio(*lista);
        item_apagar(&removido);
    }

    free(*lista);
    *lista = NULL;
}