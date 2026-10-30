#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "estrutura.h"
#include "item_nasa.h"

static int apagados;

static void apagar_nasa(void **dado) {
    // A variavel tipada evita acessar um void * por um ponteiro de outro tipo
    ITEM_NASA *nasa = *dado;
    item_nasa_apagar(&nasa);
    *dado = NULL;
    apagados++;
}

static void *copiar_nasa(const void *dado) {
    return item_nasa_copiar((ITEM_NASA *)dado);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    Config config = {0};
    config.path = argv[1];
    Query *query = query_parse(&config);
    assert(query != NULL);

    Estrutura *base = estrutura_criar(lista_cabeca, NULL);
    Estrutura *resultado = estrutura_criar(lista_ord, query);
    assert(base != NULL && resultado != NULL);
    assert(estrutura_criar(invalid, query) == NULL);
    assert(estrutura_criar(lista_ord, NULL) == NULL);
    assert(!estrutura_inserir(NULL, NULL));

    ITEM_NASA *nasa = item_nasa_criar(
        "2026 TEST", "Asteroide de teste", "2026-10-30", 2026, 10,
        0.05, 15.0, 20.0, 1, false, true, true, true, 0.7, 2,
        "categoria", "veredito", false, true, NAN, NAN, NAN, NAN);
    assert(nasa != NULL);
    ITEM *item = item_criar(nasa, apagar_nasa, copiar_nasa, item_nasa_ler_campo, 1);
    assert(item != NULL);
    assert(estrutura_inserir(base, item));

    // Simula a parte do futuro Search responsavel por uma visita aceita
    // A BASE conserva o original e RESULT recebe uma copia independente
    double similaridade = 0.0;
    assert(query_avaliar(query, item, item_ler_campo, &similaridade));
    assert(fabs(similaridade - 0.8) < 1e-12);
    ITEM *copia = item_copiar(item);
    assert(copia != NULL);
    assert(item_definir_similaridade(copia, similaridade));
    assert(estrutura_inserir(resultado, copia));

    estrutura_apagar(&base);
    query_apagar(&query);
    assert(base == NULL && query == NULL && apagados == 1);

    // A copia e o campo usado pela lista sobrevivem a BASE e a Query
    ValorQuery *valor = NULL;
    assert(item_ler_campo(copia, "risk_score", &valor));
    assert(query_valor_get_numero(valor) == 0.7);
    query_valor_apagar(&valor);
    assert(item_ler_campo(copia, "similarity", &valor));
    assert(fabs(query_valor_get_numero(valor) - 0.8) < 1e-12);
    query_valor_apagar(&valor);

    estrutura_apagar(&resultado);
    estrutura_apagar(&resultado);
    estrutura_apagar(NULL);
    assert(resultado == NULL && apagados == 2);
    puts("integracao_listas: testes passaram");
    return 0;
}
