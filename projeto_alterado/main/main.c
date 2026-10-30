#include <stdio.h>
#include <stdbool.h>

#include "config.h"
#include "query.h"
#include "estrutura.h"
#include "search.h"
//coordena a inicializacao, a leitura, a consulta e a liberacao dos recursos.

int main(int argc, char *argv[]){
    //config vive na pilha, então vai por endereço
    //se o parser falhar, ele já liberou o que alocou, mas chamo o apagar de novo
    //porque é seguro (testa NULL antes de cada free)
    Config config;
    if(!config_init(&config, argc, argv)){
        config_apagar(&config);
        return 1;
    }

    Query *query = query_parse(&config);
    if (query == NULL) {
        config_apagar(&config);
        return 1;
    }

    Estrutura *base = estrutura_criar(config.base, query);
    Estrutura *resultado = estrutura_criar(config.resultado, query);
    if (base == NULL || resultado == NULL) {
        estrutura_apagar(&base);
        estrutura_apagar(&resultado);
        query_apagar(&query);
        config_apagar(&config);
        return 1;
    }

    reader_load(base, config);

    // Search avalia toda a BASE e prepara RESULT antes da exportacao
    // Os contadores e tempos ficam disponiveis para o futuro modulo Report
    MetricasBusca metricas_busca;
    ResultadoSearch estado = search_executar(base, resultado, query, &metricas_busca);
    if (estado != SEARCH_OK) {
        fprintf(stderr, "Search: busca interrompida com codigo %d\n", (int)estado);
        estrutura_apagar(&base);
        estrutura_apagar(&resultado);
        query_apagar(&query);
        config_apagar(&config);
        return 1;
    }

    write_resultado(resultado, config);

    estrutura_apagar(&base);
    estrutura_apagar(&resultado);

    report();

    query_apagar(&query);
    config_apagar(&config);

    return 0;
}
