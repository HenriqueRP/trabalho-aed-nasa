#ifndef TEST_UTIL_H
#define TEST_UTIL_H

// test_util.h - mini framework de testes (sem dependencias externas).
//
// CHECK* registram a falha com arquivo:linha e deixam o teste seguir, para
// que uma execucao mostre todas as falhas de uma vez. TEST_MAIN roda a lista
// de funcoes e devolve 0 somente se nenhuma verificacao falhou.

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "item.h"
#include "query.h"

extern int g_testes_checks;
extern int g_testes_falhas;

#define CHECK(cond) do { \
    g_testes_checks++; \
    if (!(cond)) { g_testes_falhas++; \
        fprintf(stderr, " FALHA %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define CHECK_EQ_INT(obtido, esperado) do { \
    long long o_ = (long long)(obtido), e_ = (long long)(esperado); \
    g_testes_checks++; \
    if (o_ != e_) { g_testes_falhas++; \
        fprintf(stderr, " FALHA %s:%d: %s = %lld, esperado %lld\n", \
                __FILE__, __LINE__, #obtido, o_, e_); } \
} while (0)

#define CHECK_NEAR(obtido, esperado, tol) do { \
    double o_ = (obtido), e_ = (esperado); \
    g_testes_checks++; \
    if (!(fabs(o_ - e_) <= (tol))) { g_testes_falhas++; \
        fprintf(stderr, " FALHA %s:%d: %s = %g, esperado %g\n", \
                __FILE__, __LINE__, #obtido, o_, e_); } \
} while (0)

#define CHECK_STR(obtido, esperado) do { \
    const char *o_ = (obtido), *e_ = (esperado); \
    g_testes_checks++; \
    if (o_ == NULL || strcmp(o_, e_) != 0) { g_testes_falhas++; \
        fprintf(stderr, " FALHA %s:%d: %s = \"%s\", esperado \"%s\"\n", \
                __FILE__, __LINE__, #obtido, o_ ? o_ : "(null)", e_); } \
} while (0)

// Executa uma funcao de teste e imprime o nome e o resultado dela.
#define RUN(fn) do { \
    int antes_ = g_testes_falhas; \
    fn(); \
    printf(" [%s] %s\n", g_testes_falhas == antes_ ? "ok" : "FALHOU", #fn); \
} while (0)

// Resumo final; use como "return testes_resumo(argv[0]);".
int testes_resumo(const char *nome);

// Item NASA completo para testes. 'categoria' pode ser NULL (campo ausente).
// A chave do envelope e a usada como ordem de chegada/desempate.
ITEM *teste_item(int chave, int ano, double risco, double velocidade,
                 const char *categoria);

// Visitante que guarda a chave de cada item visitado, na ordem do percurso.
typedef struct {
    int chaves[256];
    int n;
    int parar_apos; // > 0: devolve false depois de visitar esta quantidade
} Visitas;

bool teste_visitar(ITEM *item, void *contexto);

// Caminho de um arquivo temporario unico dentro de build/ (para writer/reader).
const char *teste_tmp(const char *nome);

// Escreve 'texto' em um arquivo temporario e devolve o caminho (ou NULL).
const char *teste_escrever(const char *nome, const char *texto);

// Config valida montada a partir de valores (passa pelo config_criar real).
Config *teste_config(const char *reader, const char *input, const char *base,
                     const char *resultado, const char *query,
                     const char *saida, int top_k, int limit);

// Query lida de um texto (via arquivo temporario). NULL se o parse recusar.
Query *teste_query(const char *texto);

#endif