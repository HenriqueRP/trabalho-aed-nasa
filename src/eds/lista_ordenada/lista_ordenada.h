#ifndef LISTA_ORDENADA_H
#define LISTA_ORDENADA_H

#include <stdbool.h>
#include "item.h"

typedef struct lista_ordenada LISTA_ORD;

// Copia o nome do campo, sem depender do tempo de vida da Query
// Campo NULL ou vazio faz a criacao falhar
LISTA_ORD *lista_ord_criar(const char *campo);

// Encontra a posicao correta antes de ligar o novo no, em O(n)
// decrescente false corresponde a ASC e true corresponde a DESC
// A primeira insercao fixa a direcao enquanto houver registros na lista
// Trocar a direcao sem esvaziar retorna false e preserva a ordem existente
// Numeros, textos e booleanos usam os getters do ValorQuery opaco
// Valores ausentes, NAN e infinitos ficam no fim em ambas as direcoes
// Valores presentes de tipos diferentes sao recusados para o mesmo campo
// Empates usam a chave original do CSV em ordem crescente
// Chaves e valores iguais preservam a ordem de chegada, sem perder registros
// Somente o sucesso transfere a posse do ITEM para esta lista
// O campo de ordenacao e copiado na insercao e nao muda com edicoes no ITEM
// Para similarity, defina o metadado do ITEM antes de inserir o resultado
bool lista_ord_inserir(LISTA_ORD *lista, ITEM *item, bool decrescente);

// Primeiro empresta o ITEM e remover transfere sua posse ao chamador
// As duas operacoes custam O(1) e retornam NULL para lista vazia
ITEM *lista_ord_primeiro(const LISTA_ORD *lista);
ITEM *lista_ord_remover_inicio(LISTA_ORD *lista);
int lista_ord_tamanho(const LISTA_ORD *lista);
bool lista_ord_vazia(const LISTA_ORD *lista);

// Percorre na ordem mantida pelas insercoes, sem ordenar ao final
// false interrompe e o retorno conta somente callbacks que deram true
// O callback nao deve alterar os nos da lista durante o percurso
int lista_ord_percorrer(LISTA_ORD *lista, ItemVisita visita, void *contexto);

// Apaga itens, chaves copiadas, nos e o nome do campo em O(n)
void lista_ord_apagar(LISTA_ORD **lista);

#endif
