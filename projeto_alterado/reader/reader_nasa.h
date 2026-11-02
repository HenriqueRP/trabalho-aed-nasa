// eds/reader_nasa/reader_nasa.h
#ifndef READER_NASA_H
#define READER_NASA_H

#include <stdbool.h>

#include "reader.h"   // ContadoresLeitura (tipo incompleto) + Estrutura
#include "config.h"

bool reader_nasa_tem_campo(const char *campo);

bool reader_nasa_loader(Estrutura *base, const Config *config,
                        ContadoresLeitura *contadores);

#endif