// A interface POSIX disponibiliza o relogio monotono em sistemas como Linux
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "search.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

static bool marcar_tempo(double *segundos) {
    // Um relogio monotono evita que ajustes na hora do sistema alterem os tempos
#ifdef _WIN32
    LARGE_INTEGER contador, frequencia;
    if (!QueryPerformanceFrequency(&frequencia) || frequencia.QuadPart <= 0 ||
        !QueryPerformanceCounter(&contador)) return false;
    *segundos = (double)contador.QuadPart / (double)frequencia.QuadPart;
#else
    struct timespec instante;
    if (clock_gettime(CLOCK_MONOTONIC, &instante) != 0) return false;
    *segundos = (double)instante.tv_sec + (double)instante.tv_nsec / 1000000000.0;
#endif
    return true;
}

typedef struct {
    Estrutura *resultado;
    const Query *query;
    MetricasBusca metricas;
    ResultadoSearch estado;
} ContextoBusca;

static bool avaliar_registro(ITEM *item, void *contexto) {
    ContextoBusca *busca = contexto;
    busca->metricas.n_visitados++;

    // Search recebe um ITEM por vez, inclusive quando ele veio de uma celula
    // com varios registros ou de um nivel interno da lista generalizada
    double similaridade = 0.0;
    if (!query_avaliar(busca->query, item, item_ler_campo, &similaridade)) {
        return true;
    }
    busca->metricas.n_aceitos++;

    // BASE continua dona do original e RESULT recebera uma copia independente
    // Compartilhar o mesmo ITEM entre as duas causaria dupla liberacao
    ITEM *copia = item_copiar(item);
    if (copia == NULL) {
        busca->estado = SEARCH_FALHA_COPIA;
        return false;
    }

    // A pontuacao deve existir antes da insercao para ORDER por similarity
    // Ela fica no envelope generico e nao acrescenta colunas ao registro
    if (!item_definir_similaridade(copia, similaridade)) {
        item_apagar(&copia);
        busca->estado = SEARCH_FALHA_SIMILARIDADE;
        return false;
    }

    // Somente a insercao pertence a RESULT_INSERT_MS
    // Avaliacao, copia, pontuacao e percurso permanecem em SEARCH_MS
    double inicio = 0.0, fim = 0.0;
    bool medir = busca->metricas.tempos_validos && marcar_tempo(&inicio);
    bool inseriu = estrutura_inserir(busca->resultado, copia);
    if (medir && marcar_tempo(&fim) && fim >= inicio) {
        busca->metricas.result_insert_ms += (fim - inicio) * 1000.0;
    } else {
        busca->metricas.tempos_validos = false;
    }

    if (!inseriu) {
        // A insercao recusada nao transfere a posse, portanto Search libera a copia
        // Interromper evita apresentar como completo um resultado que perdeu itens
        item_apagar(&copia);
        busca->estado = SEARCH_FALHA_INSERCAO;
        return false;
    }
    busca->metricas.n_inseridos++;
    return true;
}

ResultadoSearch search_executar(Estrutura *base, Estrutura *resultado,
                                const Query *query, MetricasBusca *metricas) {
    if (metricas != NULL) *metricas = (MetricasBusca){0};

    // Inserir na propria BASE durante o percurso poderia repetir registros
    // ou impedir o termino, especialmente em estruturas que inserem no fim
    if (base == NULL || resultado == NULL || query == NULL || base == resultado) {
        return SEARCH_ARGUMENTOS_INVALIDOS;
    }
    int esperados = estrutura_tamanho(base);
    if (esperados < 0 || estrutura_tamanho(resultado) < 0) {
        return SEARCH_ARGUMENTOS_INVALIDOS;
    }

    ContextoBusca busca = {
        .resultado = resultado,
        .query = query,
        .metricas = {0},
        .estado = SEARCH_OK
    };
    double inicio = 0.0, fim = 0.0;
    busca.metricas.tempos_validos = marcar_tempo(&inicio);
    int concluidos = estrutura_percorrer(base, avaliar_registro, &busca);

    if (busca.metricas.tempos_validos && marcar_tempo(&fim) && fim >= inicio) {
        double total_ms = (fim - inicio) * 1000.0;
        busca.metricas.search_ms = total_ms - busca.metricas.result_insert_ms;
        if (busca.metricas.search_ms < 0.0) busca.metricas.tempos_validos = false;
    } else {
        busca.metricas.tempos_validos = false;
    }
    // Falha de medicao nao transforma registros corretos em falha de busca
    // O indicador permite ao Report distinguir tempo indisponivel de tempo zero
    if (!busca.metricas.tempos_validos) {
        busca.metricas.search_ms = 0.0;
        busca.metricas.result_insert_ms = 0.0;
    }

    // Os percursos contam somente callbacks que retornaram true
    // As metricas proprias tambem contam o ultimo registro em caso de erro
    // Em sucesso, ambas as contagens devem coincidir com o tamanho da BASE
    if (busca.estado == SEARCH_OK &&
        (concluidos < 0 || busca.metricas.n_visitados != (size_t)esperados ||
         (size_t)concluidos != busca.metricas.n_visitados)) {
        busca.estado = SEARCH_FALHA_PERCURSO;
    }

    if (metricas != NULL) *metricas = busca.metricas;
    return busca.estado;
}
