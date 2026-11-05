#ifndef FILA_H
#define FILA_H

#include <stdbool.h>
#include "item.h"

typedef struct fila FILA;

FILA *fila_criar(void);
bool fila_enqueue(FILA *fila, ITEM *item);
ITEM *fila_dequeue(FILA *fila);
int fila_tamanho(FILA *fila);
ITEM *fila_frente(FILA *fila);
bool fila_vazia(FILA *fila);
int fila_percorrer(FILA *fila, ItemVisita visita, void *contexto);
void apagar_fila(FILA **fila);

#endif
