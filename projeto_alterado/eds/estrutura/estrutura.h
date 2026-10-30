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

//desaloca a estrutura e os recursos que ela administra.
void estrutura_apagar(Estrutura **est);

#endif
