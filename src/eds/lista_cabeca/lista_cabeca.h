#ifndef LISTA_CABECA_H
#define LISTA_CABECA_H

#include <stdbool.h>
#include "item.h"

// A representacao dos nos fica privada para preservar a sentinela
typedef struct lista_cabeca LISTA_CAB;

LISTA_CAB *lista_cabeca_criar(void);

// Adicionar insere no fim em O(1), preservando a ordem de chegada
// A lista assume a posse do ITEM somente quando a insercao retorna true
// Cada ITEM deve pertencer a uma unica estrutura dona
bool lista_cabeca_adicionar(LISTA_CAB *lista, ITEM *item);
bool lista_cabeca_adicionar_inicio(LISTA_CAB *lista, ITEM *item);

// Consultar empresta o ponteiro, enquanto remover transfere a posse
// Ambas devolvem NULL quando nao existe um primeiro registro
ITEM *lista_cabeca_primeiro(const LISTA_CAB *lista);
ITEM *lista_cabeca_remover_inicio(LISTA_CAB *lista);

// A sentinela nao conta no tamanho nem aparece no percurso
// Tamanho devolve -1 para lista NULL, como a lista encadeada existente
int lista_cabeca_tamanho(const LISTA_CAB *lista);
bool lista_cabeca_vazia(const LISTA_CAB *lista);

// Visita do primeiro ao ultimo registro sem remover itens
// false interrompe e o retorno conta somente callbacks que deram true
// O callback pode consultar itens, mas nao deve modificar os nos da lista
int lista_cabeca_percorrer(LISTA_CAB *lista, ItemVisita visita, void *contexto);

// Libera os registros ainda pertencentes a lista, os nos e a sentinela
// Tambem zera o ponteiro do chamador e aceita ponteiros NULL
void lista_cabeca_apagar(LISTA_CAB **lista);

#endif
