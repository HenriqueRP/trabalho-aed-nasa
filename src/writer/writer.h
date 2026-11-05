#ifndef WRITER_H
#define WRITER_H

#include <stdbool.h>
#include <stddef.h>

#include "config.h"
#include "estrutura.h"

// exporta o resultado para csv, respeitando o limite top-k.
bool writer_exportar(Estrutura *resultado, const Config *config);
bool writer_exportar_com_contagem(Estrutura *resultado,
                                  const Config *config,
                                  size_t *n_escritos);

#endif
