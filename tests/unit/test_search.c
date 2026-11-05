#include <stdio.h>

#include "test_util.h"
#include "estrutura.h"
#include "search.h"

static Estrutura *base_com(OpcaoEstrutura tipo, Query *q, int n) {
    Estrutura *b = estrutura_criar(tipo, q);
    for (int i = 1; i <= n; i++) {
        // risco = i (1..n), ano alterna 2020/2021
        estrutura_inserir(b, teste_item(i, 2020 + i % 2, (double)i, 10.0 + i,
                                        i % 3 ? "Low" : "High"));
    }
    return b;
}

static void filtro_e_metricas(void) {
    Query *q = teste_query("FILTER|risk_score|>=|6\nORDER|risk_score|ASC\n"
                           "GROUP|threat_category|year\nCROSS|threat_category|year\n");
    Estrutura *base = base_com(fila, q, 10);
    Estrutura *res = estrutura_criar(lista_ord, q);
    MetricasBusca m;

    CHECK_EQ_INT(search_executar(base, res, q, &m), SEARCH_OK);
    CHECK_EQ_INT(m.n_visitados, 10);
    CHECK_EQ_INT(m.n_aceitos, 5); // riscos 6..10
    CHECK_EQ_INT(m.n_inseridos, 5);
    CHECK(m.tempos_validos);
    CHECK(m.search_ms >= 0.0 && m.result_insert_ms >= 0.0);
    CHECK_EQ_INT(estrutura_tamanho(base), 10); // BASE intacta
    CHECK_EQ_INT(estrutura_tamanho(res), 5);

    Visitas v = {0};
    estrutura_percorrer(res, teste_visitar, &v);
    CHECK_EQ_INT(v.chaves[0], 6); // ordenado por risco crescente
    CHECK_EQ_INT(v.chaves[4], 10);

    estrutura_apagar(&base);
    estrutura_apagar(&res);
    query_apagar(&q);
}

static void copias_independentes(void) {
    // se RESULT compartilhasse ITEMs com a BASE, apagar as duas daria free duplo (valgrind)
    Query *q = teste_query("ORDER|risk_score|DESC\nGROUP|threat_category|year\n"
                           "CROSS|threat_category|year\n");
    Estrutura *base = base_com(lista_enc, q, 6);
    Estrutura *res = estrutura_criar(pilha, q);

    CHECK_EQ_INT(search_executar(base, res, q, NULL), SEARCH_OK); // sem FILTER: aceita tudo
    CHECK_EQ_INT(estrutura_tamanho(res), 6);

    estrutura_apagar(&base);
    Visitas v = {0};
    CHECK_EQ_INT(estrutura_percorrer(res, teste_visitar, &v), 6); // RESULT sobrevive a BASE

    estrutura_apagar(&res);
    query_apagar(&q);
}

static void similaridade_ordena_o_resultado(void) {
    Query *q = teste_query("SIM|risk_score|5|5|1\nACCEPT|0.3\nORDER|similarity|DESC\n"
                           "GROUP|threat_category|year\nCROSS|threat_category|year\n");
    Estrutura *base = base_com(fila, q, 10);
    Estrutura *res = estrutura_criar(lista_ord, q);

    CHECK_EQ_INT(search_executar(base, res, q, NULL), SEARCH_OK);

    Visitas v = {0};
    estrutura_percorrer(res, teste_visitar, &v);
    CHECK(v.n >= 1);
    CHECK_EQ_INT(v.chaves[0], 5); // risco 5 = alvo, similaridade 1.0
    CHECK_EQ_INT(v.n, 7); // 1-|r-5|/5 >= 0.3 <=> |r-5| <= 3.5 <=> riscos 2..8

    estrutura_apagar(&base);
    estrutura_apagar(&res);
    query_apagar(&q);
}

static void todas_as_combinacoes_dao_o_mesmo_conjunto(void) {
    Query *q = teste_query("FILTER|year|==|2021\nORDER|risk_score|ASC\n"
                           "GROUP|threat_category|year\nCROSS|threat_category|year\n");
    OpcaoEstrutura t[] = {pilha, fila, lista_seq, lista_enc,
                          lista_cabeca, lista_ord, lista_gen, lista_cruz};

    for (int b = 0; b < 8; b++) {
        for (int r = 0; r < 8; r++) {
            Estrutura *base = base_com(t[b], q, 20);
            Estrutura *res = estrutura_criar(t[r], q);
            MetricasBusca m;

            CHECK_EQ_INT(search_executar(base, res, q, &m), SEARCH_OK);
            CHECK_EQ_INT(m.n_visitados, 20);
            CHECK_EQ_INT(m.n_aceitos, 10); // anos impares -> 2021
            CHECK_EQ_INT(estrutura_tamanho(res), 10);

            Visitas v = {0};
            estrutura_percorrer(res, teste_visitar, &v);
            int soma = 0;
            for (int i = 0; i < v.n; i++) {
                soma += v.chaves[i];
                CHECK(v.chaves[i] % 2 == 1);
            }
            CHECK_EQ_INT(soma, 1 + 3 + 5 + 7 + 9 + 11 + 13 + 15 + 17 + 19);

            estrutura_apagar(&base);
            estrutura_apagar(&res);
        }
    }

    query_apagar(&q);
}

static void argumentos_invalidos(void) {
    Query *q = teste_query("ORDER|risk_score|ASC\n");
    Estrutura *a = estrutura_criar(fila, q);
    Estrutura *b = estrutura_criar(fila, q);
    MetricasBusca m = {.n_aceitos = 99};

    CHECK_EQ_INT(search_executar(NULL, b, q, &m), SEARCH_ARGUMENTOS_INVALIDOS);
    CHECK_EQ_INT(m.n_aceitos, 0); // metricas reiniciadas mesmo na falha
    CHECK_EQ_INT(search_executar(a, NULL, q, NULL), SEARCH_ARGUMENTOS_INVALIDOS);
    CHECK_EQ_INT(search_executar(a, b, NULL, NULL), SEARCH_ARGUMENTOS_INVALIDOS);
    CHECK_EQ_INT(search_executar(a, a, q, NULL), SEARCH_ARGUMENTOS_INVALIDOS); // BASE == RESULT

    estrutura_apagar(&a);
    estrutura_apagar(&b);
    query_apagar(&q);
}

static void base_vazia(void) {
    Query *q = teste_query("ORDER|risk_score|ASC\n");
    Estrutura *a = estrutura_criar(fila, q);
    Estrutura *b = estrutura_criar(lista_ord, q);
    MetricasBusca m;

    CHECK_EQ_INT(search_executar(a, b, q, &m), SEARCH_OK);
    CHECK_EQ_INT(m.n_visitados, 0);
    CHECK_EQ_INT(m.n_inseridos, 0);

    estrutura_apagar(&a);
    estrutura_apagar(&b);
    query_apagar(&q);
}

int main(void) {
    printf("test_search\n");
    RUN(filtro_e_metricas);
    RUN(copias_independentes);
    RUN(similaridade_ordena_o_resultado);
    RUN(todas_as_combinacoes_dao_o_mesmo_conjunto);
    RUN(argumentos_invalidos);
    RUN(base_vazia);
    return testes_resumo("test_search");
}