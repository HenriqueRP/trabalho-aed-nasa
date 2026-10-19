#ifndef LISTA_H
#define LISTA_H

#include <stdbool.h>
#include "item.h"

#define TAM_MAX 100 // Tamanho predefinido pra suportar ate N=100

typedef struct lista_ LISTA;

LISTA *lista_criar(void);
void lista_apagar(LISTA **lista);
bool lista_inserir(LISTA *lista, ITEM *item);
void lista_remover_menor(LISTA *lista);
void lista_imprimir_final(const LISTA *lista);

#endif