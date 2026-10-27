#ifndef LISTA_GENERALIZADA_H
#define LISTA_GENERALIZADA_H

#include <stdbool.h>
#include <stddef.h>

#include "item.h"
#include "query.h"

typedef struct lista_gen LISTA_GEN;
typedef struct no_gen NO_GEN;

// Lista generalizada: cada elemento e ou um ATOMO (guarda um ITEM) ou uma
// SUBLISTA (guarda uma lista interna, rotulada por uma chave).
//
// E a estrutura classica - tipo + uniao + proximo - usada para representar
// agrupamentos com profundidade variavel:
//     GROUP|threat_category        -> ( (cat1 i1 i2) (cat2 i3) )
//     GROUP|threat_category|year   -> ( (cat1 (2020 i1 i2) (2021 i3)) )
//     GROUP|a|b|c                  -> um nivel de sublista a mais
//
// O rotulo fica no proprio no da sublista ('chave' + 'tem_chave'), nao num
// atomo separado: mantem o trio tipo/uniao/prox e evita um terceiro tipo.
typedef enum { NO_ATOMO, NO_SUBLISTA } TipoNoGen;

// 'campos' traz um nome de campo por nivel de agrupamento.
LISTA_GEN *lista_gen_criar_n(const char **campos, int n);

// Atalho do caso que o GROUP produz hoje.
LISTA_GEN *lista_gen_criar_campos(const char *campo1, const char *campo2);

void lista_gen_apagar(LISTA_GEN **lista);

// Le do proprio item os campos configurados e desce a hierarquia, criando
// os niveis que ainda nao existem.
bool lista_gen_inserir_item(LISTA_GEN *lista, ITEM *item);

// Insercao com chaves ja resolvidas. 'n' pode ser menor que a profundidade
// configurada: nesse caso o item para num nivel intermediario.
bool lista_gen_inserir_chaves(LISTA_GEN *lista, ITEM *item,
                              const ValorQuery *chaves, int n);

int lista_gen_tamanho(const LISTA_GEN *lista);       // total de atomos
int lista_gen_profundidade(const LISTA_GEN *lista);  // niveis configurados
int lista_gen_comprimento(const LISTA_GEN *lista);   // elementos no nivel 0

// Navegacao. Um NO_GEN e ATOMO ou SUBLISTA; use lista_gen_tipo antes de
// pedir lista_gen_atomo ou lista_gen_filhos.
const NO_GEN *lista_gen_primeiro(const LISTA_GEN *lista);
const NO_GEN *lista_gen_proximo(const NO_GEN *no);
const NO_GEN *lista_gen_filhos(const NO_GEN *no);   // 1o elemento da sublista
TipoNoGen lista_gen_tipo(const NO_GEN *no);
ITEM *lista_gen_atomo(const NO_GEN *no);
const ValorQuery *lista_gen_chave(const NO_GEN *no);

// Quantos atomos existem na subarvore desse no (0 para ATOMO).
int lista_gen_nitens(const NO_GEN *no);

// Percorre os atomos na ordem de insercao: grupo -> subgrupo -> itens.
int lista_gen_percorrer(LISTA_GEN *lista, ItemVisita visita, void *contexto);

// Formata uma chave para exibicao. 'buffer' precisa de pelo menos 64 bytes.
const char *lista_gen_texto(const ValorQuery *valor, char *buffer, size_t tam);

void lista_gen_imprimir(const LISTA_GEN *lista, void (*imprimir_item)(ITEM *));

#endif