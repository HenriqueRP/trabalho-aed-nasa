// eds/reader_ancine/reader_ancine.h
#ifndef READER_ANCINE_H
#define READER_ANCINE_H

#include <stdbool.h>

#include "reader.h"   // ContadoresLeitura (tipo incompleto) + Estrutura
#include "config.h"

bool reader_ancine_tem_campo(const char *campo);

bool reader_ancine_loader(Estrutura *base, const Config *config,
                          ContadoresLeitura *contadores);

#endif