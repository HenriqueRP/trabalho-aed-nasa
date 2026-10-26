#include <stdio.h>
#include <stdbool.h>

#include "config.h"
#include "query.h"

int main(int argc, char *argv[]){
    // config vive na pilha, então vai por endereço
    // se o parser falhar, ele já liberou o que alocou, mas chamo o apagar de novo
    // porque é seguro (testa NULL antes de cada free)
    Config config;
    if(!config_init(&config, argc, argv)){
        config_apagar(&config);
        return 1;
    }

    Query *query = query_parse(&config);

    Estrutura *base = estrutura_criar(config.base, query); // está com erro porque o modulo estrutura ainda nao foi implementado
    Estrutura *resultado = estrutura_criar(config.resultado, query); // quando for implementado: retirar esse comentario

    reader_load(base, config);

    search_reader(base, resultado, config, query);

    write_resultado(resultado, config);

    estrutura_apagar(&base);
    estrutura_apagar(&resultado);

    report();

    config_apagar(&config);

    return 0;
}
