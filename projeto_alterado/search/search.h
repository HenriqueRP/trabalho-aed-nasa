#ifndef SEARCH_H
#define SEARCH_H

#include <stddef.h>
#include "estrutura.h"
#include "query.h"

// Rejeicao pela Query e um resultado normal, enquanto estes erros interrompem a busca
typedef enum {
    SEARCH_OK,
    SEARCH_ARGUMENTOS_INVALIDOS,
    SEARCH_FALHA_PERCURSO,
    SEARCH_FALHA_COPIA,
    SEARCH_FALHA_SIMILARIDADE,
    SEARCH_FALHA_INSERCAO
} ResultadoSearch;

typedef struct {
    size_t n_visitados;   // Registros avaliados, incluindo aquele que causou uma falha
    size_t n_aceitos;     // Registros aprovados pela Query nesta chamada
    size_t n_inseridos;   // Copias que passaram a pertencer a RESULT nesta chamada
} MetricasBusca;

// Percorre toda a BASE e aplica FILTER, SIM e ACCEPT usando query_avaliar
// Filtros consecutivos combinam por AND, conforme o contrato da Query
// Cada aceito recebe uma copia profunda com sua similaridade antes da insercao
// ORDER, GROUP e CROSS sao respeitados pela estrutura de resultado ja configurada
// top-k pertence a exportacao e nao limita a busca nem a quantidade de aceitos
// BASE e RESULT podem ter o mesmo tipo, mas devem ser objetos diferentes
// As duas estruturas e a Query continuam pertencendo ao chamador
// Em falha, BASE permanece intacta e RESULT conserva as insercoes ja concluidas
// Metricas pode ser NULL e, quando fornecida, e reiniciada em cada chamada
// O chamador deve verificar explicitamente se o retorno e SEARCH_OK
ResultadoSearch search_executar(Estrutura *base, Estrutura *resultado,
                                const Query *query, MetricasBusca *metricas);

#endif
