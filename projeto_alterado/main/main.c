#include <stdio.h>
#include <stdbool.h>

#include "config.h"
#include "query.h"
#include "estrutura.h"
#include "reader.h"
#include "search.h"
#include "writer.h"


// coordena a inicializacao, a leitura, a consulta e a liberacao dos recursos.
int main(int argc, char *argv[]) {

    // a configuracao e opaca e gerenciada pelo modulo config.
    Config *config = config_criar(argc, argv);

    if (config == NULL) {
        return 1;
    }

    // interpreta a consulta definida na configuracao.
    Query *query = query_parse(config);

    if (query == NULL) {
        config_apagar(&config);
        return 1;
    }

    // cria as estruturas de dados escolhidas na configuracao.
    Estrutura *base = estrutura_criar(config_get_base(config), query);
    Estrutura *resultado = estrutura_criar(config_get_resultado(config), query);

    if (base == NULL || resultado == NULL) {
        estrutura_apagar(&base);
        estrutura_apagar(&resultado);
        query_apagar(&query);
        config_apagar(&config);
        return 1;
    }

    // cria os contadores utilizados pelo reader e pelo futuro report.
    ContadoresLeitura *contadores = contadores_criar();

    if (contadores == NULL) {
        estrutura_apagar(&base);
        estrutura_apagar(&resultado);
        query_apagar(&query);
        config_apagar(&config);
        return 1;
    }

    // carrega os dados do arquivo de entrada para a estrutura base.
    if (!reader_load(base, config, contadores)) {
        fprintf(stderr, "Reader: falha ao carregar os dados.\n");

        contadores_apagar(&contadores);
        estrutura_apagar(&base);
        estrutura_apagar(&resultado);
        query_apagar(&query);
        config_apagar(&config);

        return 1;
    }

    // executa a busca e armazena as metricas da operacao.
    MetricasBusca metricas_busca;

    ResultadoSearch estado = search_executar(
        base,
        resultado,
        query,
        &metricas_busca
    );

    if (estado != SEARCH_OK) {
        fprintf(stderr,
                "Search: busca interrompida com codigo %d\n",
                (int)estado);

        contadores_apagar(&contadores);
        estrutura_apagar(&base);
        estrutura_apagar(&resultado);
        query_apagar(&query);
        config_apagar(&config);

        return 1;
    }

    // exporta os resultados para o arquivo csv configurado.
    if (!writer_exportar(resultado, config)) {
        fprintf(stderr, "Writer: falha ao exportar os resultados.\n");

        contadores_apagar(&contadores);
        estrutura_apagar(&base);
        estrutura_apagar(&resultado);
        query_apagar(&query);
        config_apagar(&config);

        return 1;
    }

    // libera os recursos utilizados pela execucao.
    contadores_apagar(&contadores);
    estrutura_apagar(&base);
    estrutura_apagar(&resultado);
    query_apagar(&query);
    config_apagar(&config);

    return 0;
}
