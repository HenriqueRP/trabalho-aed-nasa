#include "test_util.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "item_nasa.h"

int g_testes_checks = 0;
int g_testes_falhas = 0;

int testes_resumo(const char *nome) {
    printf("%s: %d verificacoes, %d falha(s)\n",
           nome, g_testes_checks, g_testes_falhas);
    return g_testes_falhas == 0 ? 0 : 1;
}

static void apagar_nasa(void **dado) {
    item_nasa_apagar((ITEM_NASA **)dado);
}

static void *copiar_nasa(const void *dado) {
    return item_nasa_copiar((ITEM_NASA *)dado);
}

ITEM *teste_item(int chave, int ano, double risco, double velocidade,
                 const char *categoria) {
    char nome[48];
    snprintf(nome, sizeof nome, "%d AB%d", ano, chave);
    ITEM_NASA *nasa = item_nasa_criar(
        nome, nome, "2020-01-15", ano, 1,
        0.1, velocidade, 20.0, 10,
        false, true, true, true, risco, (int)(risco / 2),
        categoria, "veredito", false, true,
        NAN, NAN, NAN, NAN);
    if (nasa == NULL) return NULL;
    ITEM *item = item_criar(nasa, apagar_nasa, copiar_nasa,
                            item_nasa_ler_campo, chave);
    if (item == NULL) item_nasa_apagar(&nasa);
    return item;
}

bool teste_visitar(ITEM *item, void *contexto) {
    Visitas *v = contexto;
    if (v->n < 256) v->chaves[v->n] = item_get_chave(item);
    v->n++;
    return !(v->parar_apos > 0 && v->n >= v->parar_apos);
}

const char *teste_tmp(const char *nome) {
    static char caminho[8][256];
    static int proximo = 0;
    char *dest = caminho[proximo++ % 8];
    snprintf(dest, 256, "build/tmp_%s", nome);
    return dest;
}

const char *teste_escrever(const char *nome, const char *texto) {
    const char *caminho = teste_tmp(nome);
    FILE *f = fopen(caminho, "w");
    if (f == NULL) return NULL;
    fputs(texto, f);
    fclose(f);
    return caminho;
}

Config *teste_config(const char *reader, const char *input, const char *base,
                     const char *resultado, const char *query,
                     const char *saida, int top_k, int limit) {
    char topk[16], lim[16];
    snprintf(topk, sizeof topk, "%d", top_k);
    snprintf(lim, sizeof lim, "%d", limit);
    char *argv[] = {"aed", "--reader", (char *)reader, "--input", (char *)input,
                    "--base", (char *)base, "--result", (char *)resultado,
                    "--query", (char *)query, "--output", (char *)saida,
                    "--top-k", topk, "--limit", lim};
    return config_criar(17, argv);
}

Query *teste_query(const char *texto) {
    const char *caminho = teste_escrever("query_tmp.txt", texto);
    Config *c = teste_config("nasa", "data/nasa_amostra.csv", "fila", "fila",
                             caminho, "build/tmp_query_saida.csv", 0, 0);
    if (c == NULL) return NULL;
    Query *q = query_parse(c);
    config_apagar(&c);
    return q;
}