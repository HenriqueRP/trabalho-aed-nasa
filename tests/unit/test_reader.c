#include <stdio.h>

#include "test_util.h"
#include "estrutura.h"
#include "reader.h"

#define NASA_HDR "asteroid_designation,asteroid_fullname,close_approach_date,year,month,distance_au," \
"velocity_km_s,absolute_magnitude,days_until_approach,is_past_event,is_future_event,risk_score," \
"panic_level,threat_category,panic_verdict,on_sentry_list,sentry_impact_prob,sentry_torino_scale," \
"sentry_palermo_scale,sentry_diameter_km"

// Linha NASA valida com 'risco' e 'ano'; sentry ausente (4 celulas vazias).
#define LINHA(nome, ano, risco) nome ",\"" "(" nome ")\",2020-01-15," ano ",1,0.1,10.5,20.1,30,False,True," risco \
",2,Moderate,Verdict,False,,,,"

typedef struct { Config *cfg; Estrutura *base; ContadoresLeitura *c; } Cenario;

static Cenario montar(const char *reader, const char *csv, int limit) {
    Cenario s = {0};
    const char *caminho = teste_escrever("reader_in.csv", csv);
    s.cfg = teste_config(reader, caminho, "lista_enc", "fila",
                         "q.txt", "build/tmp_reader_out.csv", 0, limit);
    s.base = estrutura_criar(lista_enc, NULL);
    s.c = contadores_criar();
    return s;
}

static void desmontar(Cenario *s) {
    estrutura_apagar(&s->base);
    contadores_apagar(&s->c);
    config_apagar(&s->cfg);
}

static void nasa_amostra_completa(void) {
    Config *cfg = teste_config("nasa", "data/nasa_amostra.csv", "lista_enc", "fila", "q", "o", 0, 0);
    Estrutura *base = estrutura_criar(lista_enc, NULL);
    ContadoresLeitura *c = contadores_criar();
    CHECK(reader_load(base, cfg, c));
    CHECK_EQ_INT(contadores_input(c), 42); // 40 validas + 2 defeituosas
    CHECK_EQ_INT(contadores_inseridos(c), 40);
    CHECK_EQ_INT(contadores_invalidos(c), 2);
    CHECK_EQ_INT(estrutura_tamanho(base), 40);
    CHECK(contadores_insert_ms(c) >= 0.0);
    CHECK_EQ_INT(contadores_input(c), contadores_inseridos(c) + contadores_invalidos(c));
    contadores_apagar(&c); estrutura_apagar(&base); config_apagar(&cfg);
}

static void limit_conta_inseridos_nao_linhas(void) {
    Config *cfg = teste_config("nasa", "data/nasa_amostra.csv", "lista_enc", "fila", "q", "o", 0, 15);
    Estrutura *base = estrutura_criar(lista_enc, NULL);
    ContadoresLeitura *c = contadores_criar();
    CHECK(reader_load(base, cfg, c));
    CHECK_EQ_INT(contadores_inseridos(c), 15);
    CHECK_EQ_INT(estrutura_tamanho(base), 15);
    CHECK(contadores_input(c) >= 15); // linhas invalidas antes do corte nao consomem cota
    contadores_apagar(&c); estrutura_apagar(&base); config_apagar(&cfg);
}

static void campos_de_borda_nasa(void) {
    // CRLF, virgula entre aspas, sentry vazio, aspas escapadas
    Cenario s = montar("nasa",
        NASA_HDR "\r\n"
        LINHA("A1", "2020", "5.5") "\r\n"
        "B2,\"(B2), com virgula e \"\"aspas\"\"\",2020-01-15,2021,2,0.1,1,2,3,"
        "True,False,1,1,Low,V,True,0.1,1,-2,0.5\n", 0);
    CHECK(reader_load(s.base, s.cfg, s.c));
    CHECK_EQ_INT(contadores_inseridos(s.c), 2);
    CHECK_EQ_INT(contadores_invalidos(s.c), 0);
    desmontar(&s);
}

static void nasa_linhas_invalidas(void) {
    Cenario s = montar("nasa",
        NASA_HDR "\n"
        "so,tres,campos\n" // layout errado
        LINHA("OK", "2020", "5") "\n"
        LINHA("MESNUM", "2020", "abc") "\n" // risco nao numerico
        LINHA("VAZIO", "", "5") "\n" // ano obrigatorio vazio
        "X,\"aspa aberta,2020-01-15,2020,1,0.1,1,2,3,False,True,5,1,Low,V,False,,,,\n" // aspa nao fecha
        LINHA("OK2", "2021", "1e2") "\n", 0); // notacao cientifica valida
    CHECK(reader_load(s.base, s.cfg, s.c));
    CHECK_EQ_INT(contadores_input(s.c), 6);
    CHECK_EQ_INT(contadores_inseridos(s.c), 2);
    CHECK_EQ_INT(contadores_invalidos(s.c), 4);
    desmontar(&s);
}

static void arquivo_vazio_cabecalho_e_inexistente(void) {
    Cenario vazio = montar("nasa", "", 0);
    CHECK(!reader_load(vazio.base, vazio.cfg, vazio.c));
    desmontar(&vazio);

    Cenario so_cab = montar("nasa", NASA_HDR "\n", 0);
    CHECK(reader_load(so_cab.base, so_cab.cfg, so_cab.c));
    CHECK_EQ_INT(contadores_input(so_cab.c), 0);
    CHECK_EQ_INT(estrutura_tamanho(so_cab.base), 0);
    desmontar(&so_cab);

    Config *cfg = teste_config("nasa", "build/nao_existe.csv", "lista_enc", "fila", "q", "o", 0, 0);
    Estrutura *base = estrutura_criar(lista_enc, NULL);
    ContadoresLeitura *c = contadores_criar();
    CHECK(!reader_load(base, cfg, c));
    contadores_apagar(&c); estrutura_apagar(&base); config_apagar(&cfg);
}

static void ancine_amostra(void) {
    Config *cfg = teste_config("ancine", "data/ancine_amostra.csv", "fila", "fila", "q", "o", 0, 0);
    Estrutura *base = estrutura_criar(fila, NULL);
    ContadoresLeitura *c = contadores_criar();
    CHECK(reader_load(base, cfg, c));
    CHECK_EQ_INT(contadores_input(c), 10);
    CHECK_EQ_INT(contadores_inseridos(c), 8);
    CHECK_EQ_INT(contadores_invalidos(c), 2); // campos a menos + duracao "abc"
    CHECK_EQ_INT(estrutura_tamanho(base), 8);
    contadores_apagar(&c); estrutura_apagar(&base); config_apagar(&cfg);
}

static void reader_desconhecido_e_argumentos_nulos(void) {
    Config *cfg = teste_config("outro", "data/nasa_amostra.csv", "fila", "fila", "q", "o", 0, 0);
    Estrutura *base = estrutura_criar(fila, NULL);
    ContadoresLeitura *c = contadores_criar();
    CHECK(!reader_load(base, cfg, c));
    CHECK(!reader_load(NULL, cfg, c));
    CHECK(!reader_load(base, NULL, c));
    CHECK(!reader_load(base, cfg, NULL));
    CHECK(!reader_tem_campo(cfg, "year")); // dataset desconhecido
    contadores_apagar(&c); estrutura_apagar(&base); config_apagar(&cfg);
}

static void tem_campo_por_dataset(void) {
    Config *n = teste_config("nasa", "i", "fila", "fila", "q", "o", 0, 0);
    Config *a = teste_config("ancine", "i", "fila", "fila", "q", "o", 0, 0);
    CHECK(reader_tem_campo(n, "risk_score"));
    CHECK(!reader_tem_campo(n, "duracao_total_minutos"));
    CHECK(reader_tem_campo(a, "duracao_total_minutos"));
    CHECK(!reader_tem_campo(a, "risk_score"));
    CHECK(!reader_tem_campo(n, NULL));
    config_apagar(&n); config_apagar(&a);
}

static void contadores_api(void) {
    ContadoresLeitura *c = contadores_criar();
    CHECK(c != NULL);
    CHECK_EQ_INT(contadores_input(c), 0);
    contadores_inc_input(c); contadores_inc_input(c);
    contadores_inc_inseridos(c); contadores_inc_invalidos(c);
    CHECK_EQ_INT(contadores_input(c), 2);
    CHECK_EQ_INT(contadores_inseridos(c), 1);
    CHECK_EQ_INT(contadores_invalidos(c), 1);

    Estrutura *e = estrutura_criar(fila, NULL);
    ITEM *it = teste_item(1, 2020, 1, 1, "Low");
    CHECK(reader_inserir(e, it, c));
    CHECK(contadores_insert_ms(c) >= 0.0);
    CHECK_EQ_INT(estrutura_tamanho(e), 1);
    estrutura_apagar(&e);

    contadores_resetar(c);
    CHECK_EQ_INT(contadores_input(c), 0);
    CHECK_NEAR(contadores_insert_ms(c), 0.0, 0.0);
    contadores_apagar(&c);
    CHECK(c == NULL);

    // ponteiro nulo nunca derruba
    CHECK_EQ_INT(contadores_input(NULL), 0);
    CHECK_NEAR(contadores_insert_ms(NULL), 0.0, 0.0);
    contadores_inc_input(NULL); contadores_resetar(NULL); contadores_apagar(NULL);
}

int main(void) {
    printf("test_reader\n");
    RUN(nasa_amostra_completa);
    RUN(limit_conta_inseridos_nao_linhas);
    RUN(campos_de_borda_nasa);
    RUN(nasa_linhas_invalidas);
    RUN(arquivo_vazio_cabecalho_e_inexistente);
    RUN(ancine_amostra);
    RUN(reader_desconhecido_e_argumentos_nulos);
    RUN(tem_campo_por_dataset);
    RUN(contadores_api);
    return testes_resumo("test_reader");
}