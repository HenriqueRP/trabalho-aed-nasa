#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "config.h"
#include "query.h"
#include "estrutura.h"
#include "reader.h"
#include "search.h"
#include "writer.h"
#include "report.h"

// Retorna o instante monotono em segundos.
static bool marcar_tempo(double *segundos) {
    struct timespec instante;

    if (segundos == NULL) return false;
    if (clock_gettime(CLOCK_MONOTONIC, &instante) != 0) return false;

    *segundos = (double)instante.tv_sec +
                (double)instante.tv_nsec / 1000000000.0;
    return true;
}

// Converte dois instantes monotono em milissegundos.
static double calcular_ms(double inicio, double fim) {
    if (fim < inicio) return 0.0;
    return (fim - inicio) * 1000.0;
}

int main(int argc, char *argv[]) {
    Config *config = config_criar(argc, argv);
    if (config == NULL) return 1;

    Query *query = query_parse(config);
    if (query == NULL) {
        config_apagar(&config);
        return 1;
    }

    Estrutura *base = estrutura_criar(config_get_base(config), query);
    Estrutura *resultado = estrutura_criar(config_get_resultado(config), query);
    ContadoresLeitura *contadores = contadores_criar();

    if (base == NULL || resultado == NULL || contadores == NULL) {
        estrutura_apagar(&base);
        estrutura_apagar(&resultado);
        contadores_apagar(&contadores);
        query_apagar(&query);
        config_apagar(&config);
        return 1;
    }

    bool sucesso = true;
    bool tempo_inicio_valido;
    bool tempo_fim_valido;
    double inicio = 0.0;
    double fim = 0.0;
    double load_ms = 0.0;
    double write_ms = 0.0;
    size_t output_records = 0;
    MetricasBusca metricas_busca = {0};

    // A leitura recebe os contadores opacos e os reinicia internamente.
    tempo_inicio_valido = marcar_tempo(&inicio);
    if (!reader_load(base, config, contadores)) {
        fprintf(stderr, "Reader: falha ao carregar os dados.\n");
        sucesso = false;
    }
    tempo_fim_valido = marcar_tempo(&fim);

    if (tempo_inicio_valido && tempo_fim_valido) {
        load_ms = calcular_ms(inicio, fim);
    }

    if (sucesso) {
        ResultadoSearch estado = search_executar(
            base, resultado, query, &metricas_busca
        );

        if (estado != SEARCH_OK) {
            fprintf(stderr,
                    "Search: busca interrompida com codigo %d\n",
                    (int)estado);
            sucesso = false;
        }
    }

    if (sucesso) {
        tempo_inicio_valido = marcar_tempo(&inicio);
        if (!writer_exportar_com_contagem(
                resultado, config, &output_records)) {
            fprintf(stderr, "Writer: falha ao exportar os resultados.\n");
            sucesso = false;
        }
        tempo_fim_valido = marcar_tempo(&fim);

        if (tempo_inicio_valido && tempo_fim_valido) {
            write_ms = calcular_ms(inicio, fim);
        }
    }

    // O report precisa ser gerado antes de liberar contadores e metricas.
    report_imprimir(
        sucesso, contadores, &metricas_busca,
        output_records, load_ms, write_ms
    );

    contadores_apagar(&contadores);
    estrutura_apagar(&base);
    estrutura_apagar(&resultado);
    query_apagar(&query);
    config_apagar(&config);

    return sucesso ? 0 : 1;
}
