#include "config.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct config_ {
    const char *reader;
    const char *input;
    const char *path;
    const char *output;
    OpcaoEstrutura base;
    OpcaoEstrutura resultado;
    int top_k;
    int limit;
};

static OpcaoEstrutura tipo_estrutura(const char *nome) {
    const char *nomes[] = {
        "pilha", "fila", "lista_seq", "lista_enc", "lista_cabeca",
        "lista_ord", "lista_gen", "lista_cruz"
    };
    for (int i = 0; i < 8; i++) {
        if (strcmp(nome, nomes[i]) == 0) return (OpcaoEstrutura)i;
    }
    return invalid;
}

static bool inteiro_nao_negativo(const char *texto, int *valor) {
    char *fim;
    errno = 0;
    long numero = strtol(texto, &fim, 10);
    if (fim == texto || *fim != '\0' || errno == ERANGE ||
        numero < 0 || numero > INT_MAX) return false;
    *valor = (int)numero;
    return true;
}

Config *config_criar(int argc, char **argv) {
    // O enunciado define oito argumentos obrigatorios, cada um com seu valor.
    if (argc != 17 || argv == NULL) return NULL;

    const char *opcoes[8] = {
        "--reader", "--input", "--base", "--result",
        "--query", "--output", "--top-k", "--limit"
    };
    const char *valores[8] = {0};

    for (int i = 1; i < argc; i += 2) {
        if (argv[i] == NULL || argv[i + 1] == NULL) return NULL;
        int indice = -1;
        for (int j = 0; j < 8; j++) {
            if (strcmp(argv[i], opcoes[j]) == 0) {
                indice = j;
                break;
            }
        }
        if (indice < 0 || valores[indice] != NULL) return NULL;
        valores[indice] = argv[i + 1];
    }

    for (int i = 0; i < 8; i++) {
        if (valores[i] == NULL) return NULL;
    }

    Config *config = malloc(sizeof(*config));
    if (config == NULL) return NULL;

    // argv permanece valido durante toda a execucao de main.
    // Nao e necessario copiar/alocar cada caminho de arquivo.
    config->reader = valores[0];
    config->input = valores[1];
    config->base = tipo_estrutura(valores[2]);
    config->resultado = tipo_estrutura(valores[3]);
    config->path = valores[4];
    config->output = valores[5];

    if (config->base == invalid || config->resultado == invalid ||
        !inteiro_nao_negativo(valores[6], &config->top_k) ||
        !inteiro_nao_negativo(valores[7], &config->limit)) {
        free(config);
        return NULL;
    }
    return config;
}

const char *config_get_reader(const Config *config) {
    return config ? config->reader : NULL;
}
const char *config_get_input(const Config *config) {
    return config ? config->input : NULL;
}
const char *config_get_path(const Config *config) {
    return config ? config->path : NULL;
}
const char *config_get_output(const Config *config) {
    return config ? config->output : NULL;
}
OpcaoEstrutura config_get_base(const Config *config) {
    return config ? config->base : invalid;
}
OpcaoEstrutura config_get_resultado(const Config *config) {
    return config ? config->resultado : invalid;
}
int config_get_top_k(const Config *config) {
    return config ? config->top_k : 0;
}
int config_get_limit(const Config *config) {
    return config ? config->limit : 0;
}

void config_apagar(Config **config) {
    if (config == NULL || *config == NULL) return;
    free(*config);
    *config = NULL;
}
