#ifndef PILHA_H
#define PILHA_H

#include "item.h"
#include <stdbool.h>

typedef struct pilha PILHA;

PILHA *criar_pilha(void);
ITEM *pilha_top(PILHA *pilha);
bool pilha_push(PILHA *pilha, ITEM *item);
ITEM *pilha_pop(PILHA *pilha);
bool pilha_empty(PILHA *pilha);
int pilha_tamanho(PILHA *pilha);
int pilha_percorrer(PILHA *pilha, ItemVisita visita, void *contexto);
void apagar_pilha(PILHA **pilha);

#endif
