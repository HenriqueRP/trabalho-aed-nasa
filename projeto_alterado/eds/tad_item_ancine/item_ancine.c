#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include "item_ancine.h"

// item_ancine.c - implementacao do registro da Parte 2.

struct item_ancine_ {
    char *titulo_original;
    char *cpb;
    char *data_emissao_cpb;
    char *situacao_obra;
    char *tipo_obra;
    char *subtipo_obra;
    char *classificacao_obra;
    char *organizacao_temporal;
    double duracao_total_minutos;
    int quantidade_episodios;
    int ano_producao_inicial;
    int ano_producao_final;
    char *segmento_destinacao_inicial;
    bool coproducao_internacional;
    char *requerente;
    char *cnpj_requerente;
    char *uf_requerente;
    char *municipio_requerente;
};

// Copia uma string para o item. NULL no parametro = campo ausente, nao erro.
// Devolve false so quando o malloc falha.
static bool copiar_campo(char **destino, const char *origem) {
    *destino = NULL;
    if (origem == NULL) return true;

    size_t tam = strlen(origem) + 1;
    char *copia = malloc(tam);
    if (copia == NULL) return false;

    memcpy(copia, origem, tam);
    *destino = copia;
    return true;
}

ITEM_ANCINE *item_ancine_criar(
    const char *titulo_original, const char *cpb,
    const char *data_emissao_cpb, const char *situacao_obra,
    const char *tipo_obra, const char *subtipo_obra,
    const char *classificacao_obra, const char *organizacao_temporal,
    double duracao_total_minutos, int quantidade_episodios,
    int ano_producao_inicial, int ano_producao_final,
    const char *segmento_destinacao_inicial, bool coproducao_internacional,
    const char *requerente, const char *cnpj_requerente,
    const char *uf_requerente, const char *municipio_requerente
) {
    ITEM_ANCINE *novo = calloc(1, sizeof(ITEM_ANCINE));
    if (novo == NULL) return NULL;

    // calloc zera os 12 ponteiros. Assim o caminho de erro pode chamar
    // item_ancine_apagar() sem testar o que ainda nao foi alocado.
    bool ok = true;

    ok = ok && copiar_campo(&novo->titulo_original, titulo_original);
    ok = ok && copiar_campo(&novo->cpb, cpb);
    ok = ok && copiar_campo(&novo->data_emissao_cpb, data_emissao_cpb);
    ok = ok && copiar_campo(&novo->situacao_obra, situacao_obra);
    ok = ok && copiar_campo(&novo->tipo_obra, tipo_obra);
    ok = ok && copiar_campo(&novo->subtipo_obra, subtipo_obra);
    ok = ok && copiar_campo(&novo->classificacao_obra, classificacao_obra);
    ok = ok && copiar_campo(&novo->organizacao_temporal, organizacao_temporal);

    // Escalares: copia direta, sem alocacao.
    novo->duracao_total_minutos = duracao_total_minutos;
    novo->quantidade_episodios = quantidade_episodios;
    novo->ano_producao_inicial = ano_producao_inicial;
    novo->ano_producao_final = ano_producao_final;

    ok = ok && copiar_campo(&novo->segmento_destinacao_inicial,
                            segmento_destinacao_inicial);
    novo->coproducao_internacional = coproducao_internacional;

    ok = ok && copiar_campo(&novo->requerente, requerente);
    ok = ok && copiar_campo(&novo->cnpj_requerente, cnpj_requerente);
    ok = ok && copiar_campo(&novo->uf_requerente, uf_requerente);
    ok = ok && copiar_campo(&novo->municipio_requerente, municipio_requerente);

    // Falha parcial: o apagar funciona porque calloc zerou os ponteiros.
    if (!ok) {
        item_ancine_apagar(&novo);
        return NULL;
    }
    return novo;
}

// Copia profunda: reconstroi o item pelos getters. Nenhuma string e
// compartilhada entre original e copia.
ITEM_ANCINE *item_ancine_copiar(ITEM_ANCINE *item) {
    if (item == NULL) return NULL;

    return item_ancine_criar(
        get_ancine_titulo_original(item),
        get_ancine_cpb(item),
        get_ancine_data_emissao_cpb(item),
        get_ancine_situacao_obra(item),
        get_ancine_tipo_obra(item),
        get_ancine_subtipo_obra(item),
        get_ancine_classificacao_obra(item),
        get_ancine_organizacao_temporal(item),
        get_ancine_duracao_total_minutos(item),
        get_ancine_quantidade_episodios(item),
        get_ancine_ano_producao_inicial(item),
        get_ancine_ano_producao_final(item),
        get_ancine_segmento_destinacao_inicial(item),
        get_ancine_coproducao_internacional(item),
        get_ancine_requerente(item),
        get_ancine_cnpj_requerente(item),
        get_ancine_uf_requerente(item),
        get_ancine_municipio_requerente(item)
    );
}

// free() puro em cada string. Todas vieram de malloc no criar, entao nao
// ha ponteiro compartilhado nem string literal para proteger.
void item_ancine_apagar(ITEM_ANCINE **item) {
    if (item == NULL || *item == NULL) return;

    free((*item)->titulo_original);
    free((*item)->cpb);
    free((*item)->data_emissao_cpb);
    free((*item)->situacao_obra);
    free((*item)->tipo_obra);
    free((*item)->subtipo_obra);
    free((*item)->classificacao_obra);
    free((*item)->organizacao_temporal);
    free((*item)->segmento_destinacao_inicial);
    free((*item)->requerente);
    free((*item)->cnpj_requerente);
    free((*item)->uf_requerente);
    free((*item)->municipio_requerente);

    free(*item);
    *item = NULL;
}

// Padrao dos 18 getters: testa NULL e devolve sentinela coerente com o tipo.
// O sentinela importa - devolver 0.0 num double seria indistinguivel de uma
// duracao real igual a zero; NAN nunca e um valor valido do dataset.

const char *get_ancine_titulo_original(ITEM_ANCINE *item) {
    if (item == NULL) return NULL;
    return item->titulo_original;
}

const char *get_ancine_cpb(ITEM_ANCINE *item) {
    if (item == NULL) return NULL;
    return item->cpb;
}

const char *get_ancine_data_emissao_cpb(ITEM_ANCINE *item) {
    if (item == NULL) return NULL;
    return item->data_emissao_cpb;
}

const char *get_ancine_situacao_obra(ITEM_ANCINE *item) {
    if (item == NULL) return NULL;
    return item->situacao_obra;
}

const char *get_ancine_tipo_obra(ITEM_ANCINE *item) {
    if (item == NULL) return NULL;
    return item->tipo_obra;
}

const char *get_ancine_subtipo_obra(ITEM_ANCINE *item) {
    if (item == NULL) return NULL;
    return item->subtipo_obra;
}

const char *get_ancine_classificacao_obra(ITEM_ANCINE *item) {
    if (item == NULL) return NULL;
    return item->classificacao_obra;
}

const char *get_ancine_organizacao_temporal(ITEM_ANCINE *item) {
    if (item == NULL) return NULL;
    return item->organizacao_temporal;
}

double get_ancine_duracao_total_minutos(ITEM_ANCINE *item) {
    if (item == NULL) return NAN;
    return item->duracao_total_minutos;
}

int get_ancine_quantidade_episodios(ITEM_ANCINE *item) {
    if (item == NULL) return -1;
    return item->quantidade_episodios;
}

int get_ancine_ano_producao_inicial(ITEM_ANCINE *item) {
    if (item == NULL) return -1;
    return item->ano_producao_inicial;
}

int get_ancine_ano_producao_final(ITEM_ANCINE *item) {
    if (item == NULL) return -1;
    return item->ano_producao_final;
}

const char *get_ancine_segmento_destinacao_inicial(ITEM_ANCINE *item) {
    if (item == NULL) return NULL;
    return item->segmento_destinacao_inicial;
}

bool get_ancine_coproducao_internacional(ITEM_ANCINE *item) {
    if (item == NULL) return false;
    return item->coproducao_internacional;
}

const char *get_ancine_requerente(ITEM_ANCINE *item) {
    if (item == NULL) return NULL;
    return item->requerente;
}

const char *get_ancine_cnpj_requerente(ITEM_ANCINE *item) {
    if (item == NULL) return NULL;
    return item->cnpj_requerente;
}

const char *get_ancine_uf_requerente(ITEM_ANCINE *item) {
    if (item == NULL) return NULL;
    return item->uf_requerente;
}

const char *get_ancine_municipio_requerente(ITEM_ANCINE *item) {
    if (item == NULL) return NULL;
    return item->municipio_requerente;
}

// Enumeracao dos campos consultaveis. Serve para o switch do ler_campo
// parar de comparar strings: resolve o nome uma vez e despacha por inteiro.
typedef enum {
    CAMPO_TITULO_ORIGINAL, CAMPO_CPB, CAMPO_DATA_EMISSAO_CPB,
    CAMPO_SITUACAO_OBRA, CAMPO_TIPO_OBRA, CAMPO_SUBTIPO_OBRA,
    CAMPO_CLASSIFICACAO_OBRA, CAMPO_ORGANIZACAO_TEMPORAL,
    CAMPO_DURACAO_TOTAL, CAMPO_QUANTIDADE_EPISODIOS,
    CAMPO_ANO_INICIAL, CAMPO_ANO_FINAL,
    CAMPO_SEGMENTO_DESTINACAO, CAMPO_COPRODUCAO_INTERNACIONAL,
    CAMPO_REQUERENTE, CAMPO_CNPJ_REQUERENTE,
    CAMPO_UF_REQUERENTE, CAMPO_MUNICIPIO_REQUERENTE
} CampoAncine;

// Unica fonte de verdade dos nomes de campo do dataset. Qualquer nome que
// nao esteja aqui e rejeitado - inclusive na validacao da consulta.
static const struct {
    const char *nome;
    CampoAncine id;
} CAMPOS_ANCINE[] = {
    {"titulo_original",             CAMPO_TITULO_ORIGINAL},
    {"cpb",                         CAMPO_CPB},
    {"data_emissao_cpb",            CAMPO_DATA_EMISSAO_CPB},
    {"situacao_obra",               CAMPO_SITUACAO_OBRA},
    {"tipo_obra",                   CAMPO_TIPO_OBRA},
    {"subtipo_obra",                CAMPO_SUBTIPO_OBRA},
    {"classificacao_obra",          CAMPO_CLASSIFICACAO_OBRA},
    {"organizacao_temporal",        CAMPO_ORGANIZACAO_TEMPORAL},
    {"duracao_total_minutos",       CAMPO_DURACAO_TOTAL},
    {"quantidade_episodios",        CAMPO_QUANTIDADE_EPISODIOS},
    {"ano_producao_inicial",        CAMPO_ANO_INICIAL},
    {"ano_producao_final",          CAMPO_ANO_FINAL},
    {"segmento_destinacao_inicial", CAMPO_SEGMENTO_DESTINACAO},
    {"coproducao_internacional",    CAMPO_COPRODUCAO_INTERNACIONAL},
    {"requerente",                  CAMPO_REQUERENTE},
    {"cnpj_requerente",             CAMPO_CNPJ_REQUERENTE},
    {"uf_requerente",               CAMPO_UF_REQUERENTE},
    {"municipio_requerente",        CAMPO_MUNICIPIO_REQUERENTE}
};

#define TOTAL_CAMPOS (sizeof(CAMPOS_ANCINE) / sizeof(CAMPOS_ANCINE[0]))

// Busca linear em 18 nomes. Chamada uma vez por leitura de campo.
static bool buscar_campo(const char *nome, CampoAncine *saida) {
    if (nome == NULL) return false;

    for (size_t i = 0; i < TOTAL_CAMPOS; i++) {
        if (strcmp(nome, CAMPOS_ANCINE[i].nome) == 0) {
            *saida = CAMPOS_ANCINE[i].id;
            return true;
        }
    }
    return false;
}

// Versao publica: usada pelo main para recusar consulta com campo invalido
// antes de comecar a ler o CSV.
bool item_ancine_tem_campo(const char *campo) {
    CampoAncine ignorado;
    return buscar_campo(campo, &ignorado);
}

// Helpers de preenchimento: devolvem false quando o valor nao e consultavel.
static bool texto(const char *origem, ValorQuery *saida) {
    if (origem == NULL) return false;   // campo ausente: nao responde
    saida->tipo = QUERY_TEXTO;
    saida->valor.texto = origem;
    return true;
}

static bool numero(double valor, ValorQuery *saida) {
    saida->tipo = QUERY_NUMERO;
    saida->valor.numero = valor;
    return true;
}

static bool booleano(bool valor, ValorQuery *saida) {
    saida->tipo = QUERY_BOOLEANO;
    saida->valor.booleano = valor;
    return true;
}

// Ponto de entrada do modulo generico: recebe void* e devolve o campo pedido.
bool item_ancine_ler_campo(const void *item, const char *campo, ValorQuery **saida) {
    if (item == NULL || campo == NULL || saida == NULL || *saida == NULL) {
        return false;
    }

    const ITEM_ANCINE *obra = (const ITEM_ANCINE *)item;

    CampoAncine id;
    if (!buscar_campo(campo, &id)) return false;

    switch (id) {
        case CAMPO_TITULO_ORIGINAL:
            return texto(obra->titulo_original, *saida);
        case CAMPO_CPB:
            return texto(obra->cpb, *saida);
        case CAMPO_DATA_EMISSAO_CPB:
            return texto(obra->data_emissao_cpb, *saida);
        case CAMPO_SITUACAO_OBRA:
            return texto(obra->situacao_obra, *saida);
        case CAMPO_TIPO_OBRA:
            return texto(obra->tipo_obra, *saida);
        case CAMPO_SUBTIPO_OBRA:
            return texto(obra->subtipo_obra, *saida);
        case CAMPO_CLASSIFICACAO_OBRA:
            return texto(obra->classificacao_obra, *saida);
        case CAMPO_ORGANIZACAO_TEMPORAL:
            return texto(obra->organizacao_temporal, *saida);
        case CAMPO_SEGMENTO_DESTINACAO:
            return texto(obra->segmento_destinacao_inicial, *saida);
        case CAMPO_REQUERENTE:
            return texto(obra->requerente, *saida);
        case CAMPO_CNPJ_REQUERENTE:
            return texto(obra->cnpj_requerente, *saida);
        case CAMPO_UF_REQUERENTE:
            return texto(obra->uf_requerente, *saida);
        case CAMPO_MUNICIPIO_REQUERENTE:
            return texto(obra->municipio_requerente, *saida);

        case CAMPO_DURACAO_TOTAL:
            return numero(obra->duracao_total_minutos, *saida);
        case CAMPO_QUANTIDADE_EPISODIOS:
            return numero((double)obra->quantidade_episodios, *saida);
        case CAMPO_ANO_INICIAL:
            return numero((double)obra->ano_producao_inicial, *saida);
        case CAMPO_ANO_FINAL:
            return numero((double)obra->ano_producao_final, *saida);

        case CAMPO_COPRODUCAO_INTERNACIONAL:
            return booleano(obra->coproducao_internacional, *saida);
    }
    return false;   // inalcancavel se CAMPOS_ANCINE e o enum estao sincronizados
}
