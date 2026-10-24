#ifndef LISTA_SEQ_H
#define LISTA_SEQ_H

#include <stdbool.h>
#include "item.h"

// Antes era LISTA, que colidia com o typedef de lista_cruzada.h quando os
// dois headers entravam no mesmo arquivo (era o caso do estrutura.c).
typedef struct lista_seq LISTA_SEQ;

LISTA_SEQ *lista_seq_criar(void);
void lista_seq_apagar(LISTA_SEQ **lista);

// Insercao ordenada por item_get_chave. Rejeita chave duplicada.
bool lista_seq_inserir(LISTA_SEQ *lista, ITEM *item);

// Remove o item de menor chave, que esta sempre na posicao 0.
void lista_seq_remover_menor(LISTA_SEQ *lista);

int lista_seq_tam(LISTA_SEQ *lista);
bool lista_seq_vazia(LISTA_SEQ *lista);

// Do indice 0 (menor chave) para o ultimo.
int lista_seq_percorrer(LISTA_SEQ *lista, ItemVisita visita, void *contexto);

#endif