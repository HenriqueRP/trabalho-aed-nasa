#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "reader.h"
#include "reader_nasa.h"
#include "reader_ancine.h"

// reader.c - fachada opaca dos readers de CSV.
//
// Duas responsabilidades:
//   1. dono da definicao de ContadoresLeitura, que nao aparece em header
//   2. escolha do loader (nasa ou ancine) a partir da config
//
// C nao permite switch sobre string: os case precisam ser inteiros. Por isso
// o nome vem da config, vira um enum privado e o switch opera sobre ele.

// A definicao vive AQUI e em nenhum outro lugar. E o que torna o tipo opaco:
// fora deste arquivo, ContadoresLeitura e um nome sem campos.
struct contadores_leitura {
    size_t n_input;       // linhas de dados lidas (sem o cabecalho)
    size_t n_inseridos;   // itens aceitos pela estrutura base
    size_t n_invalidos;   // linhas descartadas
};

typedef enum {
    TIPO_NASA = 0,
    TIPO_ANCINE
} TipoReader;

// ------------------------------------------------------------- contadores

ContadoresLeitura *contadores_criar(void) {
    // calloc zera os tres campos: o relatorio pode ser impresso mesmo que a
    // carga falhe antes do primeiro contador ser tocado.
    return calloc(1, sizeof(ContadoresLeitura));
}

void contadores_apagar(ContadoresLeitura **contadores) {
    if (contadores == NULL) return;
    if (*contadores == NULL) return;

    free(*contadores);
    *contadores = NULL;
}

void contadores_resetar(ContadoresLeitura *contadores) {
    if (contadores == NULL) return;

    contadores->n_input = 0;
    contadores->n_inseridos = 0;
    contadores->n_invalidos = 0;
}

size_t contadores_input(const ContadoresLeitura *contadores) {
    if (contadores == NULL) return 0;
    return contadores->n_input;
}

size_t contadores_inseridos(const ContadoresLeitura *contadores) {
    if (contadores == NULL) return 0;
    return contadores->n_inseridos;
}

size_t contadores_invalidos(const ContadoresLeitura *contadores) {
    if (contadores == NULL) return 0;
    return contadores->n_invalidos;
}

void contadores_inc_input(ContadoresLeitura *contadores) {
    if (contadores == NULL) return;
    contadores->n_input++;
}

void contadores_inc_inseridos(ContadoresLeitura *contadores) {
    if (contadores == NULL) return;
    contadores->n_inseridos++;
}

void contadores_inc_invalidos(ContadoresLeitura *contadores) {
    if (contadores == NULL) return;
    contadores->n_invalidos++;
}

// ---------------------------------------------------------------- leitura

// Resolve o nome do dataset da config para o enum interno.
// false quando o nome nao foi informado ou nao e conhecido.
static bool resolver(const Config *config, TipoReader *saida) {
    if (config == NULL || saida == NULL) return false;

    const char *nome = config_get_reader(config);
    if (nome == NULL) return false;

    if (strcmp(nome, "nasa") == 0) { *saida = TIPO_NASA; return true; }
    if (strcmp(nome, "ancine") == 0) { *saida = TIPO_ANCINE; return true; }

    return false;
}

bool reader_tem_campo(const Config *config, const char *campo) {
    if (campo == NULL) return false;

    TipoReader tipo;
    if (!resolver(config, &tipo)) return false;

    switch (tipo) {
        case TIPO_NASA: return reader_nasa_tem_campo(campo);
        case TIPO_ANCINE: return reader_ancine_tem_campo(campo);
    }
    return false;
}

bool reader_load(Estrutura *base, const Config *config,
                 ContadoresLeitura *contadores) {
    if (base == NULL || config == NULL || contadores == NULL) return false;

    // Zera antes de tudo: se a resolucao ou a abertura do arquivo falhar, o
    // relatorio ja encontra os contadores consistentes em vez de herdar
    // numeros de uma chamada anterior.
    contadores_resetar(contadores);

    TipoReader tipo;
    if (!resolver(config, &tipo)) {
        // O nome vem da config e pode estar nulo: nao da para imprimir com
        // %s sem testar antes.
        const char *nome = config_get_reader(config);
        if (nome == NULL) nome = "(nao informado)";

        fprintf(stderr,
                "Reader desconhecido: '%s'. Use 'nasa' ou 'ancine'.\n", nome);
        return false;
    }

    // Cada loader abre o proprio CSV a partir de config_get_input. Nao ha
    // FILE* para passar: o formato mora no loader, nao na fachada.
    //
    // Sem default de proposito: se um dataset novo entrar no enum e ficar
    // sem loader, o -Wswitch avisa. O return depois do switch existe so para
    // satisfazer o -Wreturn-type.
    switch (tipo) {
        case TIPO_NASA: return reader_nasa_loader(base, config, contadores);
        case TIPO_ANCINE: return reader_ancine_loader(base, config, contadores);
    }
    return false;
}