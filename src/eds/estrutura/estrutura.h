#ifndef ESTRUTURA_H
#define ESTRUTURA_H

#include <stdbool.h>

#include "config.h"
#include "item.h"
#include "query.h"

typedef struct estrutura_ Estrutura;

//cria a estrutura escolhida e configura os campos exigidos pela query.
Estrutura *estrutura_criar(OpcaoEstrutura tipo, Query *query);

//insere um item na estrutura selecionada.
bool estrutura_inserir(Estrutura *est, ITEM *item);

// Delega o percurso a TAD escolhida sem expor seus nos ao consumidor
// Cada registro e visitado uma vez, incluindo os itens internos de cada celula
// false no callback interrompe e o retorno conta somente respostas true
// Argumentos invalidos retornam -1, enquanto uma estrutura vazia retorna 0
// O callback nao deve inserir nem remover registros da estrutura percorrida
int estrutura_percorrer(Estrutura *est, ItemVisita visita, void *contexto);

// Conta registros, nao celulas ou grupos, sem percorrer a estrutura
// Devolve -1 para estrutura invalida
int estrutura_tamanho(const Estrutura *est);

//desaloca a estrutura e os recursos que ela administra.
void estrutura_apagar(Estrutura **est);

#endif
