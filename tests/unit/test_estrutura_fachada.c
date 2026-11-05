// Testes da fachada Estrutura: as mesmas regras valem para as oito opcoes.
#include <stdio.h>

#include "test_util.h"
#include "config.h"
#include "estrutura.h"
#include "query.h"

static const OpcaoEstrutura TIPOS[] = {
    pilha, fila, lista_seq, lista_enc, lista_cabeca, lista_ord, lista_gen, lista_cruz
};
static const char *NOMES[] = {
    "pilha", "fila", "lista_seq", "lista_enc",
    "lista_cabeca", "lista_ord", "lista_gen", "lista_cruz"
};
#define N_TIPOS 8

// A Query so e lida por lista_ord/gen/cruz, que dependem de ORDER/GROUP/CROSS.
static Query *query_completa(void) {
    // query_parse le de um caminho em Config; monta uma Config minima.
    const char *q = teste_escrever("fachada_query.txt",
        "ORDER|risk_score|ASC\nGROUP|threat_category|year\nCROSS|threat_category|year\n");
    char *argv[] = {"aed", "--reader", "nasa", "--input", "data/nasa_amostra.csv",
                    "--base", "pilha", "--result", "fila", "--query", (char *)q,
                    "--output", "build/tmp_fachada_out.csv", "--top-k", "0", "--limit", "0"};
    Config *c = config_criar(17, argv);
    Query *query = query_parse(c);
    config_apagar(&c);
    return query;
}

static void todas_inserem_percorrem_e_contam(void) {
    Query *q = query_completa();
    CHECK(q != NULL);
    for (int t = 0; t < N_TIPOS; t++) {
        Estrutura *e = estrutura_criar(TIPOS[t], q);
        if (e == NULL) {
            fprintf(stderr, " criar falhou: %s\n", NOMES[t]);
            CHECK(false);
            continue;
        }
        CHECK_EQ_INT(estrutura_tamanho(e), 0);

        const char *cats[] = {"Low", "High", "Moderate"};
        for (int i = 1; i <= 12; i++) {
            ITEM *it = teste_item(i, 2019 + i % 3, i * 0.5, 10 + i, cats[i % 3]);
            CHECK(estrutura_inserir(e, it));
        }
        CHECK_EQ_INT(estrutura_tamanho(e), 12);

        Visitas v = {0};
        CHECK_EQ_INT(estrutura_percorrer(e, teste_visitar, &v), 12);
        CHECK_EQ_INT(v.n, 12);

        // cada registro aparece exatamente uma vez no percurso
        int vezes[13] = {0};
        for (int i = 0; i < v.n; i++) vezes[v.chaves[i]]++;
        for (int k = 1; k <= 12; k++) CHECK_EQ_INT(vezes[k], 1);

        // interromper conta so os callbacks que devolveram true
        Visitas parada = {.parar_apos = 5};
        CHECK_EQ_INT(estrutura_percorrer(e, teste_visitar, &parada), 4);

        estrutura_apagar(&e);
        CHECK(e == NULL);
    }
    query_apagar(&q);
}

static void argumentos_invalidos(void) {
    Query *q = query_completa();
    CHECK_EQ_INT(estrutura_tamanho(NULL), -1);
    Visitas v = {0};
    CHECK_EQ_INT(estrutura_percorrer(NULL, teste_visitar, &v), -1);
    CHECK(!estrutura_inserir(NULL, NULL));

    Estrutura *e = estrutura_criar(fila, q);
    CHECK_EQ_INT(estrutura_percorrer(e, NULL, &v), -1);
    CHECK(!estrutura_inserir(e, NULL));
    estrutura_apagar(&e);
    estrutura_apagar(&e); // segundo apagar com NULL e inofensivo
    estrutura_apagar(NULL);

    CHECK(estrutura_criar(invalid, q) == NULL);
    query_apagar(&q);
}

static void estruturas_dependentes_da_query_exigem_o_comando(void) {
    // consulta sem ORDER/GROUP/CROSS: so as estruturas simples podem existir.
    const char *arq = teste_escrever("fachada_vazia.txt", "FILTER|year|>=|2000\n");
    char *argv[] = {"aed", "--reader", "nasa", "--input", "data/nasa_amostra.csv",
                    "--base", "pilha", "--result", "fila", "--query", (char *)arq,
                    "--output", "build/tmp_o.csv", "--top-k", "0", "--limit", "0"};
    Config *c = config_criar(17, argv);
    Query *q = query_parse(c);
    config_apagar(&c);
    CHECK(q != NULL);

    CHECK(estrutura_criar(lista_ord, q) == NULL);
    CHECK(estrutura_criar(lista_gen, q) == NULL);
    CHECK(estrutura_criar(lista_cruz, q) == NULL);
    Estrutura *simples = estrutura_criar(lista_enc, q);
    CHECK(simples != NULL);
    estrutura_apagar(&simples);
    query_apagar(&q);
}

int main(void) {
    printf("test_estrutura_fachada\n");
    RUN(todas_inserem_percorrem_e_contam);
    RUN(argumentos_invalidos);
    RUN(estruturas_dependentes_da_query_exigem_o_comando);
    return testes_resumo("test_estrutura_fachada");
}