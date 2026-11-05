#include "report.h"

#include <stdio.h>

// O relatorio usa stdout; mensagens de diagnostico continuam em stderr.
void report_imprimir(bool sucesso, const ContadoresLeitura *contadores, const MetricasBusca *metricas, size_t output_records, double load_ms, double write_ms) {
    size_t input_records = 0;
    size_t matched_records = 0;
    double search_ms = 0.0;
    double result_insert_ms = 0.0;

    if (contadores != NULL) {
        input_records = contadores_input(contadores);
    }

    if (metricas != NULL) {
        matched_records = metricas->n_aceitos;

        if (metricas->tempos_validos) {
            search_ms = metricas->search_ms;
            result_insert_ms = metricas->result_insert_ms;
        }
    }

    // Mantem a saida previsivel mesmo quando uma etapa falha.
    if (printf("STATUS=%s\n", sucesso ? "OK" : "ERROR") < 0) return;
    if (printf("INPUT_RECORDS=%zu\n", input_records) < 0) return;
    if (printf("MATCHED_RECORDS=%zu\n", matched_records) < 0) return;
    if (printf("OUTPUT_RECORDS=%zu\n", output_records) < 0) return;
    if (printf("LOAD_MS=%.3f\n", load_ms) < 0) return;
    if (printf("SEARCH_MS=%.3f\n", search_ms) < 0) return;
    if (printf("RESULT_INSERT_MS=%.3f\n", result_insert_ms) < 0) return;
    printf("WRITE_MS=%.3f\n", write_ms);
}
