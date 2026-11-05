#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_util.h"
#include "estrutura.h"
#include "reader.h"
#include "report.h"
#include "search.h"
#include "writer.h"

#define HDR "asteroid_designation,asteroid_fullname,close_approach_date,year,month,distance_au," \
"velocity_km_s,absolute_magnitude,days_until_approach,is_past_event,is_future_event,risk_score," \
"panic_level,threat_category,panic_verdict,on_sentry_list,sentry_impact_prob,sentry_torino_scale," \
"sentry_palermo_scale,sentry_diameter_km"

static int contar_linhas(const char *caminho) {
    FILE *f = fopen(caminho, "r");
    if (f == NULL) return -1;
    int n = 0, c;
    while ((c = fgetc(f)) != EOF) if (c == '\n') n++;
    fclose(f);
    return n;
}

static char *ler_tudo(const char *caminho) {
    FILE *f = fopen(caminho, "r");
    if (f == NULL) return NULL;
    char *buf = calloc(1, 1 << 16);
    size_t n = fread(buf, 1, (1 << 16) - 1, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

static void writer_top_k_e_cabecalho(void) {
    const char *in = teste_escrever("w_in.csv", HDR "\n");
    const char *out = teste_tmp("w_out.csv");
    Estrutura *res = estrutura_criar(fila, NULL);
    for (int i = 1; i <= 5; i++)
        estrutura_inserir(res, teste_item(i, 2020, i, 10, "Low"));

    Config *c = teste_config("nasa", in, "fila", "fila", "q.txt", out, 3, 0);
    size_t n = 99;
    CHECK(writer_exportar_com_contagem(res, c, &n));
    CHECK_EQ_INT(n, 3); // top-k corta a escrita
    CHECK_EQ_INT(contar_linhas(out), 4); // cabecalho + 3
    char *txt = ler_tudo(out);
    CHECK(txt != NULL && strncmp(txt, HDR "\n", strlen(HDR) + 1) == 0); // cabecalho identico ao input
    free(txt);
    config_apagar(&c);

    // top-k 0 = sem limite
    c = teste_config("nasa", in, "fila", "fila", "q.txt", out, 0, 0);
    CHECK(writer_exportar_com_contagem(res, c, &n));
    CHECK_EQ_INT(n, 5);
    config_apagar(&c);
    estrutura_apagar(&res);
}

static void writer_escapa_virgulas_e_aspas_e_ausentes(void) {
    const char *in = teste_escrever("w_in2.csv", HDR "\n");
    const char *out = teste_tmp("w_out2.csv");
    Estrutura *res = estrutura_criar(fila, NULL);
    estrutura_inserir(res, teste_item(1, 2020, 2.0, 10, "Low"));
    Config *c = teste_config("nasa", in, "fila", "fila", "q.txt", out, 0, 0);
    CHECK(writer_exportar(res, c));
    char *txt = ler_tudo(out);
    CHECK(txt != NULL);
    // 20 colunas => 19 separadores fora de aspas; campo sentry ausente sai vazio
    char *linha2 = txt ? strchr(txt, '\n') + 1 : NULL;
    int virgulas = 0; bool dentro = false;
    for (char *p = linha2; p && *p && *p != '\n'; p++) {
        if (*p == '"') dentro = !dentro;
        else if (*p == ',' && !dentro) virgulas++;
    }
    CHECK_EQ_INT(virgulas, 19);
    free(txt);
    config_apagar(&c);
    estrutura_apagar(&res);
}

static void writer_nao_sobrescreve_fontes(void) {
    const char *in = teste_escrever("w_in3.csv", HDR "\n");
    const char *qry = teste_escrever("w_q3.txt", "ORDER|year|ASC\n");
    Estrutura *res = estrutura_criar(fila, NULL);
    Config *c1 = teste_config("nasa", in, "fila", "fila", qry, in, 0, 0); // output == input
    CHECK(!writer_exportar(res, c1));
    char *depois = ler_tudo(in);
    CHECK(depois != NULL && strlen(depois) > 0); // input intacto
    free(depois);
    Config *c2 = teste_config("nasa", in, "fila", "fila", qry, qry, 0, 0); // output == query
    CHECK(!writer_exportar(res, c2));
    Config *c3 = teste_config("nasa", "build/nao_existe.csv", "fila", "fila",
                              qry, "build/tmp_x.csv", 0, 0);
    CHECK(!writer_exportar(res, c3));
    CHECK(!writer_exportar(NULL, c3));
    CHECK(!writer_exportar(res, NULL));
    config_apagar(&c1); config_apagar(&c2); config_apagar(&c3);
    estrutura_apagar(&res);
}

// ---------------------------------------------------------------- report

static char *capturar(bool detalhado, bool ok, const ContadoresLeitura *c,
                      const MetricasBusca *m, size_t out, double load, double wr) {
    const char *caminho = teste_tmp("report.txt");
    FILE *f = fopen(caminho, "w");
    CHECK(report_escrever(f, detalhado, ok, c, m, out, load, wr));
    fclose(f);
    return ler_tudo(caminho);
}

static void report_formato_padrao(void) {
    ContadoresLeitura *c = contadores_criar();
    for (int i = 0; i < 10; i++) contadores_inc_input(c);
    MetricasBusca m = {.n_aceitos = 4, .n_visitados = 8, .n_inseridos = 4,
                       .search_ms = 1.5, .result_insert_ms = 0.25, .tempos_validos = true};
    char *txt = capturar(false, true, c, &m, 3, 12.3456, 0.5);
    CHECK_STR(txt,
        "STATUS=OK\nINPUT_RECORDS=10\nMATCHED_RECORDS=4\nOUTPUT_RECORDS=3\n"
        "LOAD_MS=12.346\nSEARCH_MS=1.500\nRESULT_INSERT_MS=0.250\nWRITE_MS=0.500\n");
    free(txt);
    contadores_apagar(&c);
}

static void report_com_etapas_ausentes_e_erro(void) {
    char *txt = capturar(false, false, NULL, NULL, 0, 0.0, 0.0);
    CHECK_STR(txt,
        "STATUS=ERROR\nINPUT_RECORDS=0\nMATCHED_RECORDS=0\nOUTPUT_RECORDS=0\n"
        "LOAD_MS=0.000\nSEARCH_MS=0.000\nRESULT_INSERT_MS=0.000\nWRITE_MS=0.000\n");
    free(txt);

    // relogio sem medicao valida: tempos de busca saem 0, nunca valores inventados
    MetricasBusca m = {.n_aceitos = 2, .search_ms = 7.0, .result_insert_ms = 3.0,
                       .tempos_validos = false};
    txt = capturar(false, true, NULL, &m, 0, 0.0, 0.0);
    CHECK(strstr(txt, "MATCHED_RECORDS=2\n") != NULL);
    CHECK(strstr(txt, "SEARCH_MS=0.000\n") != NULL);
    CHECK(strstr(txt, "RESULT_INSERT_MS=0.000\n") != NULL);
    free(txt);
    CHECK(!report_escrever(NULL, false, true, NULL, NULL, 0, 0, 0));
}

static void report_detalhado_separa_leitura_de_insercao(void) {
    ContadoresLeitura *c = contadores_criar();
    Estrutura *e = estrutura_criar(fila, NULL);
    for (int i = 1; i <= 3; i++) {
        contadores_inc_input(c);
        CHECK(reader_inserir(e, teste_item(i, 2020, 1, 1, "Low"), c));
        contadores_inc_inseridos(c);
    }
    contadores_inc_input(c); contadores_inc_invalidos(c);
    MetricasBusca m = {.n_visitados = 3, .n_aceitos = 2, .n_inseridos = 2, .tempos_validos = true};
    double bi = contadores_insert_ms(c);
    CHECK(bi >= 0.0);

    char *txt = capturar(true, true, c, &m, 2, 10.0, 1.0);
    CHECK(strstr(txt, "INVALID_RECORDS=1\n") != NULL);
    CHECK(strstr(txt, "BASE_INSERTED_RECORDS=3\n") != NULL);
    CHECK(strstr(txt, "VISITED_RECORDS=3\n") != NULL);
    CHECK(strstr(txt, "RESULT_INSERTED_RECORDS=2\n") != NULL);
    CHECK(strstr(txt, "BASE_INSERT_MS=") != NULL);
    CHECK(strstr(txt, "PARSE_MS=") != NULL);
    // as 8 chaves originais continuam primeiro e na mesma ordem
    CHECK(strncmp(txt, "STATUS=OK\nINPUT_RECORDS=4\n", 26) == 0);
    free(txt);

    // sem o modo detalhado nada extra aparece
    txt = capturar(false, true, c, &m, 2, 10.0, 1.0);
    CHECK(strstr(txt, "PARSE_MS") == NULL);
    CHECK_EQ_INT(strlen(txt) > 0, 1);
    free(txt);
    estrutura_apagar(&e); contadores_apagar(&c);
}

static void report_parse_menos_insercao_nunca_negativo(void) {
    ContadoresLeitura *c = contadores_criar();
    Estrutura *e = estrutura_criar(fila, NULL);
    reader_inserir(e, teste_item(1, 2020, 1, 1, "Low"), c);
    char *txt = capturar(true, true, c, NULL, 0, 0.0, 0.0); // LOAD_MS = 0 < tempo de insercao
    CHECK(strstr(txt, "PARSE_MS=0.000\n") != NULL);
    free(txt);
    estrutura_apagar(&e); contadores_apagar(&c);
}

int main(void) {
    printf("test_writer_report\n");
    RUN(writer_top_k_e_cabecalho);
    RUN(writer_escapa_virgulas_e_aspas_e_ausentes);
    RUN(writer_nao_sobrescreve_fontes);
    RUN(report_formato_padrao);
    RUN(report_com_etapas_ausentes_e_erro);
    RUN(report_detalhado_separa_leitura_de_insercao);
    RUN(report_parse_menos_insercao_nunca_negativo);
    return testes_resumo("test_writer_report");
}