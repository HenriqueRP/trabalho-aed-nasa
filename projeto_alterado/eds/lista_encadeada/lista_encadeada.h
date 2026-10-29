#ifndef LISTA_ENCADEADA_H
#define LISTA_ENCADEADA_H

#include <stdbool.h>
#include "item.h"

typedef struct lista_encadeada LISTA_ENC;

LISTA_ENC *lista_enc_criar(void);

bool lista_enc_adicionar_fim(LISTA_ENC *lista, ITEM *item);
bool lista_enc_adicionar_inicio(LISTA_ENC *lista, ITEM *item);

ITEM *lista_enc_remover_inicio(LISTA_ENC *lista);

int lista_enc_tam(LISTA_ENC *lista);
bool lista_enc_vazia(LISTA_ENC *lista);

int lista_enc_percorrer(LISTA_ENC *lista, ItemVisita visita, void *contexto);

void lista_enc_apagar(LISTA_ENC **lista);

#endif
