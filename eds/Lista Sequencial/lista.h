#ifndef LISTA_SEQ_H
#define LISTA_SEQ_H

#include <stdbool.h>
#include "item.h"

#define TAM_MAX 100 // Tamanho predefinido pra suportar ate N=100

typedef struct lista_ LISTA;

LISTA *lista_seq_criar(void);
void lista_seq_apagar(LISTA **lista);
bool lista_seq_inserir(LISTA *lista, ITEM *item);
void lista_seq_remover_menor(LISTA *lista);
void lista_seq_imprimir_final(const LISTA *lista);

#endif