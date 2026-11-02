#ifndef READER_ANCINE_H
#define READER_ANCINE_H

#include <stdbool.h>

#include "estrutura.h"
#include "config.h"

// reader_ancine.h - CSV do ANCINE (Parte 2) -> ITEM generico.
//
// Mesmo contrato do reader_nasa: le, valida e insere na estrutura base.
// A diferenca esta no formato: separador ';' e 18 colunas, com campo vazio
// aparecendo como ";;" no arquivo.

bool reader_ancine_tem_campo(const char *campo);

bool reader_ancine_loader(Estrutura *base, const Config *config,
                          ContadoresLeitura *contadores);

#endif