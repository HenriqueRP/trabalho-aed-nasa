#ifndef LISTA_CRUZADA_H
#define LISTA_CRUZADA_H

#include <stdbool.h>
#include "item.h"

// Antes era LISTA/struct lista_, que colide com o typedef de lista_sequencial.h
typedef struct lista_cruzada LISTA_CRUZ;

LISTA_CRUZ *lista_criar(int nlinhas, int ncolunas);
void lista_apagar(LISTA_CRUZ **lista);

bool lista_inserir(LISTA_CRUZ *lista, ITEM *item, int posX, int posY);
ITEM *lista_get(const LISTA_CRUZ *lista, int posX, int posY);
ITEM *lista_remover(LISTA_CRUZ *lista, int posX, int posY);

int lista_cruz_percorrer(LISTA_CRUZ *lista, ItemVisita visita, void *contexto);

// Imprime linha por linha. Chame assim: lista_imprimir(lista, item_imprimir);
void lista_imprimir(const LISTA_CRUZ *lista, void (*imprimir_item)(ITEM *));

#endif