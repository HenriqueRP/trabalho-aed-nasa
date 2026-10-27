#ifndef LISTA_CRUZADA_H
#define LISTA_CRUZADA_H

#include <stdbool.h>
#include "item.h"
#include "query.h"

typedef struct lista_cruzada LISTA_CRUZ;

// Modo 1: coordenadas inteiras explicitas, dimensoes fixas na criacao.
LISTA_CRUZ *lista_cruz_criar(int nlinhas, int ncolunas);

// Modo 2: eixos por valor de campo. Os indices nascem conforme os valores
// aparecem, entao nao ha dimensao a informar. Campos do CROSS.
LISTA_CRUZ *lista_cruz_criar_campos(const char *campo_linha,
                                   const char *campo_coluna);

void lista_apagar(LISTA_CRUZ **lista);

bool lista_inserir(LISTA_CRUZ *lista, ITEM *item, int posX, int posY);
bool lista_cruz_inserir_item(LISTA_CRUZ *lista, ITEM *item);

ITEM *lista_get(const LISTA_CRUZ *lista, int posX, int posY);
ITEM *lista_remover(LISTA_CRUZ *lista, int posX, int posY);

// Contadores e acessores. estrutura_tamanho usa lista_cruz_tamanho; sem ele
// a unica saida seria percorrer a matriz inteira so para contar.
int lista_cruz_tamanho(const LISTA_CRUZ *lista);
int lista_cruz_linhas(const LISTA_CRUZ *lista);
int lista_cruz_colunas(const LISTA_CRUZ *lista);

// Rotulo do eixo. Devolve NULL quando o valor nao e texto (caso de year
// no CROSS) ou quando o modo e coordenada explicita.
const char *lista_cruz_rotulo_linha(const LISTA_CRUZ *lista, int indice);
const char *lista_cruz_rotulo_coluna(const LISTA_CRUZ *lista, int indice);

int lista_cruz_percorrer(LISTA_CRUZ *lista, ItemVisita visita, void *contexto);
int lista_cruz_percorrer_linha(LISTA_CRUZ *lista, int linha,
                              ItemVisita visita, void *contexto);
int lista_cruz_percorrer_coluna(LISTA_CRUZ *lista, int coluna,
                               ItemVisita visita, void *contexto);
int lista_cruz_percorrer_valor_linha(LISTA_CRUZ *lista, const ValorQuery *valor,
                                    ItemVisita visita, void *contexto);
int lista_cruz_percorrer_valor_coluna(LISTA_CRUZ *lista, const ValorQuery *valor,
                                     ItemVisita visita, void *contexto);

void lista_imprimir(const LISTA_CRUZ *lista, void (*imprimir_item)(ITEM *));

#endif
