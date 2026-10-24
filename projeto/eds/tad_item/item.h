#ifndef ITEM_H
#define ITEM_H

#include <stdbool.h>
#include "query.h"

// item.h - o que as estruturas de dados guardam.
//
// O ITEM nao conhece dataset nenhum: guarda um dado opaco (void*) e o trio
// apagar/copiar/ler_campo que sabe lidar com ele. Trocar de dataset = trocar
// esse trio no reader. Estruturas, busca e escrita nao mudam uma linha.

typedef struct item ITEM;

// Funcoes que o dono do dado fornece. O ITEM nunca as chama por nome.
typedef void  (*ItemApagarFn)(void **dado);
typedef void *(*ItemCopiarFn)(const void *dado);

// Callback de percurso: true continua, false interrompe.
// O corte do top-k e simplesmente um "return false" do consumidor.
typedef bool (*ItemVisita)(ITEM *item, void *contexto);

// Assume a posse de 'dado'. NULL se dado == NULL ou faltar memoria.
ITEM *item_criar(void *dado, ItemApagarFn apagar, ItemCopiarFn copiar,
                 LeitorCampoQuery ler_campo, int chave);

ITEM *item_copiar(const ITEM *item);  // copia profunda, via ItemCopiarFn
void  item_apagar(ITEM **item);       // libera o dado e o envelope

void *item_get_dado(const ITEM *item); // nao transfere posse

// Numero da linha no CSV. Desempate estavel no top-k e chave da lista
// ordenada quando ORDER nao pede similaridade.
int item_get_chave(const ITEM *item);

// Assinatura identica a LeitorCampoQuery: entra direto no query_avaliar.
bool item_ler_campo(const void *item, const char *campo, ValorQuery *saida);

#endif