#ifndef REPORT_H
#define REPORT_H

#include <stdbool.h>
#include <stddef.h>

#include "reader.h"
#include "search.h"

// imprime o resumo da execucao no formato exigido pelo projeto.
void report_imprimir(
    bool sucesso,
    const ContadoresLeitura *contadores,
    const MetricasBusca *metricas,
    size_t output_records,
    double load_ms,
    double write_ms
);

#endif
