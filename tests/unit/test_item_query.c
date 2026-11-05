#include <stdio.h>

#include "test_util.h"
#include "item.h"
#include "query.h"

static void item_posse_e_copia(void) {
    ITEM *a = teste_item(4, 2020, 6.5, 12.0, "High");
    CHECK(a != NULL);
    CHECK_EQ_INT(item_get_chave(a), 4);
    CHECK(item_get_dado(a) != NULL);

    ITEM *b = item_copiar(a);
    CHECK(b != NULL && b != a);
    CHECK(item_get_dado(b) != item_get_dado(a)); // copia profunda
    CHECK_EQ_INT(item_get_chave(b), 4);

    item_apagar(&a);
    CHECK(a == NULL);
    ValorQuery *v = NULL;
    CHECK(item_ler_campo(b, "risk_score", &v)); // a copia sobrevive ao original
    CHECK_NEAR(query_valor_get_numero(v), 6.5, 1e-9);
    query_valor_apagar(&v);
    item_apagar(&b);
    item_apagar(&b); // idempotente

    CHECK(item_criar(NULL, NULL, NULL, NULL, 0) == NULL);
    CHECK(item_copiar(NULL) == NULL);
}

static void item_similaridade_e_campos(void) {
    ITEM *it = teste_item(1, 2021, 3.0, 9.0, "Low");
    ValorQuery *v = NULL;

    CHECK(!item_ler_campo(it, "similarity", &v)); // ainda nao definida
    CHECK(!item_definir_similaridade(it, 1.5)); // fora de [0,1]
    CHECK(!item_definir_similaridade(it, -0.1));
    CHECK(!item_definir_similaridade(it, NAN));
    CHECK(item_definir_similaridade(it, 0.75));
    CHECK(item_ler_campo(it, "similarity", &v));
    CHECK_NEAR(query_valor_get_numero(v), 0.75, 1e-12);
    query_valor_apagar(&v);

    CHECK(item_ler_campo(it, "threat_category", &v));
    CHECK_EQ_INT(query_valor_tipo(v), QUERY_TEXTO);
    CHECK_STR(query_valor_get_texto(v), "Low");
    query_valor_apagar(&v);

    CHECK(item_ler_campo(it, "year", &v));
    CHECK_EQ_INT(query_valor_tipo(v), QUERY_NUMERO);
    query_valor_apagar(&v);

    CHECK(!item_ler_campo(it, "campo_inexistente", &v));
    item_apagar(&it);

    ITEM *sem = teste_item(2, 2021, 3.0, 9.0, NULL);
    CHECK(!item_ler_campo(sem, "threat_category", &v)); // ausente != ""
    item_apagar(&sem);
}

static void valores_query(void) {
    ValorQuery *t = query_valor_texto("abc");
    ValorQuery *n = query_valor_numero(2.5);
    ValorQuery *b = query_valor_booleano(true);
    CHECK(t && n && b);
    CHECK_EQ_INT(query_valor_tipo(t), QUERY_TEXTO);
    CHECK_EQ_INT(query_valor_tipo(n), QUERY_NUMERO);
    CHECK_EQ_INT(query_valor_tipo(b), QUERY_BOOLEANO);
    CHECK(query_valor_get_booleano(b));

    ValorQuery *c = query_valor_copiar(t);
    CHECK(query_valor_iguais(t, c));
    CHECK(!query_valor_iguais(t, n));
    CHECK(query_valor_texto(NULL) == NULL);
    query_valor_apagar(&t); query_valor_apagar(&n);
    query_valor_apagar(&b); query_valor_apagar(&c);
    CHECK(t == NULL);
}

static bool avalia(Query *q, ITEM *it, double *sim) {
    return query_avaliar(q, it, item_ler_campo, sim);
}

static void filtros_combinam_por_and(void) {
    Query *q = teste_query("FILTER|risk_score|>=|3\nFILTER|year|<|2022\n");
    CHECK(q != NULL);
    CHECK_EQ_INT(query_num_filtros(q), 2);
    CHECK_STR(query_filtro_campo(q, 0), "risk_score");
    CHECK_EQ_INT(query_filtro_operador(q, 1), QUERY_MENOR);
    CHECK_STR(query_filtro_valor(q, 1), "2022");

    ITEM *ok = teste_item(1, 2021, 5.0, 1, "Low");
    ITEM *baixo = teste_item(2, 2021, 2.0, 1, "Low");
    ITEM *tarde = teste_item(3, 2022, 9.0, 1, "Low");
    CHECK(avalia(q, ok, NULL));
    CHECK(!avalia(q, baixo, NULL));
    CHECK(!avalia(q, tarde, NULL));
    item_apagar(&ok); item_apagar(&baixo); item_apagar(&tarde);
    query_apagar(&q);
    CHECK(q == NULL);
}

static void filtro_de_texto_e_campo_ausente(void) {
    Query *q = teste_query("FILTER|threat_category|==|High\n");
    CHECK(q != NULL);
    ITEM *h = teste_item(1, 2020, 1, 1, "High");
    ITEM *l = teste_item(2, 2020, 1, 1, "Low");
    ITEM *n = teste_item(3, 2020, 1, 1, NULL);
    CHECK(avalia(q, h, NULL));
    CHECK(!avalia(q, l, NULL));
    CHECK(!avalia(q, n, NULL)); // ausente nunca satisfaz o filtro
    item_apagar(&h); item_apagar(&l); item_apagar(&n);
    query_apagar(&q);
}

static void similaridade_e_accept(void) {
    // alvo 6, tolerancia 4: risco 6 -> 1.0; risco 4 -> 0.5; risco 20 -> 0
    Query *q = teste_query("SIM|risk_score|6|4|1\nACCEPT|0.5\n");
    CHECK(q != NULL);
    CHECK(query_tem_accept(q));
    CHECK_NEAR(query_get_accept(q), 0.5, 1e-12);
    CHECK_EQ_INT(query_num_similaridades(q), 1);
    CHECK_NEAR(query_similaridade_alvo(q, 0), 6, 1e-12);

    double s = -1;
    ITEM *exato = teste_item(1, 2020, 6.0, 1, "Low");
    ITEM *meio = teste_item(2, 2020, 4.0, 1, "Low");
    ITEM *longe = teste_item(3, 2020, 20.0, 1, "Low");
    CHECK(avalia(q, exato, &s)); CHECK_NEAR(s, 1.0, 1e-12);
    CHECK(avalia(q, meio, &s)); CHECK_NEAR(s, 0.5, 1e-12); // no limiar: aceita
    CHECK(!avalia(q, longe, &s)); CHECK_NEAR(s, 0.0, 1e-12);
    item_apagar(&exato); item_apagar(&meio); item_apagar(&longe);
    query_apagar(&q);
}

static void media_ponderada(void) {
    // risco: parcial 1.0 (peso 3); velocidade: parcial 0.0 (peso 1) -> 0.75
    Query *q = teste_query("SIM|risk_score|5|2|3\nSIM|velocity_km_s|100|10|1\n");
    ITEM *it = teste_item(1, 2020, 5.0, 10.0, "Low");
    double s = 0;
    CHECK(avalia(q, it, &s));
    CHECK_NEAR(s, 0.75, 1e-12);
    item_apagar(&it);
    query_apagar(&q);
}

static void order_group_cross_e_comentarios(void) {
    Query *q = teste_query("# comentario\n\nORDER|risk_score|DESC\n"
                           "GROUP|threat_category|year\nCROSS|year|threat_category\n");
    CHECK(q != NULL);
    const char *c1, *c2; DirecaoQuery d;
    CHECK(query_get_order(q, &c1, &d)); CHECK_STR(c1, "risk_score"); CHECK_EQ_INT(d, QUERY_DESC);
    CHECK(query_get_group(q, &c1, &c2)); CHECK_STR(c1, "threat_category"); CHECK_STR(c2, "year");
    CHECK(query_get_cross(q, &c1, &c2)); CHECK_STR(c1, "year");
    query_apagar(&q);

    Query *vazio = teste_query("FILTER|year|>|0\n");
    CHECK(!query_get_order(vazio, &c1, &d));
    CHECK(!query_get_group(vazio, &c1, &c2));
    CHECK(!query_tem_accept(vazio));
    query_apagar(&vazio);
}

static void consultas_invalidas_sao_recusadas(void) {
    const char *ruins[] = {
        "FILTER|year|>=\n", // faltou o valor
        "FILTER|year|=>|3\n", // operador desconhecido
        "SIM|risk_score|5|0|1\n", // tolerancia zero
        "SIM|risk_score|5|2|-1\n", // peso negativo
        "SIM|risk_score|5|2|0\n", // soma de pesos nula
        "ACCEPT|1.5\n", // fora de [0,1]
        "ACCEPT|0.2\nACCEPT|0.3\n", // duplicado
        "ORDER|year|SIDEWAYS\n",
        "ORDER|a|ASC\nORDER|b|DESC\n",
        "TOP|3\n", // comando desconhecido
        "FILTER||>|3\n", // campo vazio
    };
    for (size_t i = 0; i < sizeof ruins / sizeof *ruins; i++) {
        Query *q = teste_query(ruins[i]);
        if (q != NULL) fprintf(stderr, " aceitou indevidamente: %s", ruins[i]);
        CHECK(q == NULL);
        query_apagar(&q);
    }
}

int main(void) {
    printf("test_item_query\n");
    RUN(item_posse_e_copia);
    RUN(item_similaridade_e_campos);
    RUN(valores_query);
    RUN(filtros_combinam_por_and);
    RUN(filtro_de_texto_e_campo_ausente);
    RUN(similaridade_e_accept);
    RUN(media_ponderada);
    RUN(order_group_cross_e_comentarios);
    RUN(consultas_invalidas_sao_recusadas);
    return testes_resumo("test_item_query");
}