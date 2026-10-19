#include "pilha.h"
#include "item.h"

#include <stdlib.h>
#include <stdbool.h>

typedef struct no NO;
struct no{
    ITEM *item;
    NO *ant;
};

// As operações com nós acontecem apenas na TAD então é desnecessário fazê-las públicas, deixei
// elas privadas com static para não terem interferência fora do arquivo .c.
static NO *no_criar(ITEM *item){
    NO *novo = (NO*)malloc(sizeof(NO));
    if(novo == NULL){
        return NULL;
    }

    novo->item = item;
    novo->ant = NULL;
    return novo;
}

static void no_apagar(NO **no){
    if(no == NULL || *no == NULL){
        return;
    }
    free(*no);
    *no = NULL;
}

struct pilha{
    int tam;
    NO *head, *tail;
};

PILHA *criar_pilha(void){
    PILHA *pilha_correta = (PILHA *)malloc(sizeof(PILHA));
    if(pilha_correta == NULL){
        return NULL;
    }

    pilha_correta->tam = 0;
    pilha_correta->tail = NULL;
    pilha_correta->head = no_criar(NULL);
    if(pilha_correta->head == NULL){
        free(pilha_correta);
        pilha_correta = NULL;
    }
    return pilha_correta;
}

bool pilha_push(PILHA *pilha, ITEM *item){
    if(pilha == NULL || item == NULL){
        return false;
    }
    NO *novo = no_criar(item);
    if(novo == NULL){
        return false;
    }
    if(pilha->tail == NULL){
        pilha->tail = novo;
    }
    novo->ant = pilha->head->ant;
    pilha->head->ant = novo;
    (pilha->tam)++;
    return true;
}

// Retorna item, então após a chamada, o item é passado para o usuário, e a liberação de espaço
// na memória fica a cargo deste.
ITEM *pilha_pop(PILHA *pilha){
    if(pilha == NULL){
        return NULL;
    }
    if(!pilha_empty(pilha)){
        NO *x = pilha->head->ant;
        ITEM *removido = x->item;
        pilha->head->ant = pilha->head->ant->ant;
        no_apagar(&x);
        (pilha->tam)--;
        if(pilha->tam == 0){
            pilha->tail = NULL;
            pilha->head->ant = NULL;
        }
        return removido;
    }
    return NULL;
}

bool pilha_empty(PILHA *pilha){
    if(pilha == NULL){
        return true;
    }
    if(pilha->tam == 0){
        return true;
    }
    else{
        return false;
    }
}

int pilha_tamanho(PILHA *pilha){
    if(pilha == NULL){
        return -1;
    }
    return (pilha->tam);
}

// Retorna a referência para o item apenas.
ITEM *pilha_top(PILHA *pilha){
    if(pilha == NULL){
        return NULL;
    }
    if(!pilha_empty(pilha)){
        return pilha->head->ant->item;
    }
    else{
        return NULL;
    }
}

// O pop apaga as estruturas nós retornando item, então é necessário apagar o item também.
void apagar_pilha(PILHA **pilha){
    if(pilha == NULL || *pilha == NULL){
        return;
    }
    ITEM *item;
    while(!pilha_empty(*pilha)){
        item = pilha_pop(*pilha);
        item_apagar(&item);
    }

    no_apagar(&(*pilha)->head);

    free(*pilha);
    *pilha = NULL;
}