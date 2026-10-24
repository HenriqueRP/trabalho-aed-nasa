#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "item_nasa.h"

// item_nasa.c - implementacao do registro da Parte 1.

struct item_nasa_ {
    char *asteroid_designation;
    char *asteroid_fullname;
    char *close_approach_date;
    int   year, month;
    double distance_au, velocity_km_s, absolute_magnitude;
    int   days_until_approach;
    // Par _valido: false + valido == false significa "celula vazia no CSV",
    // que e diferente de "nao". Os dois casos precisam ser distinguiveis.
    bool is_past_event,   is_past_event_valido;
    bool is_future_event, is_future_event_valido;
    double risk_score;
    int panic_level;
    char *threat_category, *panic_verdict;
    bool on_sentry_list, on_sentry_list_valido;
    double sentry_impact_prob, sentry_torino_scale,
           sentry_palermo_scale, sentry_diameter_km;
};

ITEM_NASA *item_nasa_criar(
    const char *asteroid_designation, const char *asteroid_fullname,
    const char *close_approach_date, int year, int month,
    double distance_au, double velocity_km_s, double absolute_magnitude,
    int days_until_approach, bool is_past_event, bool is_past_event_valido,
    bool is_future_event, bool is_future_event_valido, double risk_score,
    int panic_level, const char *threat_category, const char *panic_verdict,
    bool on_sentry_list, bool on_sentry_list_valido, double sentry_impact_prob,
    double sentry_torino_scale, double sentry_palermo_scale,
    double sentry_diameter_km
) {
    bool erro = false;
    ITEM_NASA *novo = malloc(sizeof(ITEM_NASA));
    if (novo == NULL) return NULL;

    // Zera os 5 ponteiros ANTES de qualquer malloc. Assim o caminho de erro
    // pode chamar item_nasa_apagar() sem checar o que ainda nao foi alocado.
    novo->asteroid_designation = NULL;
    novo->asteroid_fullname    = NULL;
    novo->close_approach_date  = NULL;
    novo->threat_category      = NULL;
    novo->panic_verdict        = NULL;

    // Texto: strlen+1 e strcpy. NULL no parametro = campo ausente, nao erro.
    if (asteroid_designation != NULL) {
        novo->asteroid_designation = malloc(strlen(asteroid_designation) + 1);
        if (novo->asteroid_designation == NULL) erro = true;
        else strcpy(novo->asteroid_designation, asteroid_designation);
    }
    if (asteroid_fullname != NULL) {
        novo->asteroid_fullname = malloc(strlen(asteroid_fullname) + 1);
        if (novo->asteroid_fullname == NULL) erro = true;
        else strcpy(novo->asteroid_fullname, asteroid_fullname);
    }
    if (close_approach_date != NULL) {
        novo->close_approach_date = malloc(strlen(close_approach_date) + 1);
        if (novo->close_approach_date == NULL) erro = true;
        else strcpy(novo->close_approach_date, close_approach_date);
    }

    // Escalares: copia direta, sem alocacao.
    novo->year                = year;
    novo->month               = month;
    novo->distance_au         = distance_au;
    novo->velocity_km_s       = velocity_km_s;
    novo->absolute_magnitude  = absolute_magnitude;
    novo->days_until_approach = days_until_approach;
    novo->is_past_event       = is_past_event;
    novo->is_past_event_valido = is_past_event_valido;
    novo->is_future_event     = is_future_event;
    novo->is_future_event_valido = is_future_event_valido;
    novo->risk_score          = risk_score;
    novo->panic_level         = panic_level;

    if (threat_category != NULL) {
        novo->threat_category = malloc(strlen(threat_category) + 1);
        if (novo->threat_category == NULL) erro = true;
        else strcpy(novo->threat_category, threat_category);
    }
    if (panic_verdict != NULL) {
        novo->panic_verdict = malloc(strlen(panic_verdict) + 1);
        if (novo->panic_verdict == NULL) erro = true;
        else strcpy(novo->panic_verdict, panic_verdict);
    }

    novo->on_sentry_list          = on_sentry_list;
    novo->on_sentry_list_valido   = on_sentry_list_valido;
    novo->sentry_impact_prob      = sentry_impact_prob;
    novo->sentry_torino_scale     = sentry_torino_scale;
    novo->sentry_palermo_scale    = sentry_palermo_scale;
    novo->sentry_diameter_km      = sentry_diameter_km;

    // Falha parcial: o apagar funciona porque os ponteiros ja foram zerados.
    if (erro) {
        item_nasa_apagar(&novo);
        return NULL;
    }
    return novo;
}

// Copia profunda: reconstroi o item pelos getters. Nenhuma string e
// compartilhada entre original e copia.
ITEM_NASA *item_nasa_copiar(ITEM_NASA *item) {
    if (item == NULL) return NULL;
    return item_nasa_criar(
        get_nasa_ast_desig(item),
        get_nasa_ast_fullname(item),
        get_nasa_close_approach(item),
        get_nasa_year(item),
        get_nasa_month(item),
        get_nasa_distance_au(item),
        get_nasa_velocity_km_s(item),
        get_nasa_absolute_magnitude(item),
        get_nasa_days_until_approach(item),
        get_nasa_is_past_event(item),
        get_nasa_is_past_event_valido(item),
        get_nasa_is_future_event(item),
        get_nasa_is_future_event_valido(item),
        get_nasa_risk_score(item),
        get_nasa_panic_level(item),
        get_nasa_threat_category(item),
        get_nasa_panic_verdict(item),
        get_nasa_on_sentry_list(item),
        get_nasa_on_sentry_list_valido(item),
        get_nasa_sentry_impact_prob(item),
        get_nasa_sentry_torino_scale(item),
        get_nasa_sentry_palermo_scale(item),
        get_nasa_sentry_diameter_km(item)
    );
}

// free() puro em cada string. Todas vieram de malloc no criar, entao nao
// ha ponteiro compartilhado nem string literal para proteger.
void item_nasa_apagar(ITEM_NASA **item) {
    if (item == NULL || *item == NULL) return;
    free((*item)->asteroid_designation);
    free((*item)->asteroid_fullname);
    free((*item)->close_approach_date);
    free((*item)->threat_category);
    free((*item)->panic_verdict);
    free(*item);
    *item = NULL;
}

// Padrao dos 24 getters: testa NULL e devolve sentinela coerente com o tipo.
// O sentinela importa - devolver 0.0 num double seria indistinguivel de uma
// medicao real igual a zero; NAN nunca e um valor valido do dataset.

char *get_nasa_ast_desig(ITEM_NASA *item) {
    if (item == NULL) return NULL;
    return item->asteroid_designation;
}

int get_nasa_year(ITEM_NASA *item) {
    if (item == NULL) return -1;
    return item->year;
}

double get_nasa_distance_au(ITEM_NASA *item) {
    if (item == NULL) return NAN;
    return item->distance_au;
}

bool get_nasa_is_past_event(ITEM_NASA *item) {
    if (item == NULL) return false;
    return item->is_past_event;
}

// Os outros 20 getters seguem exatamente este formato, trocando tipo e campo.

// Enumeracao dos campos consultaveis. Serve para o switch do ler_campo
// parar de comparar strings: resolve o nome uma vez e despacha por inteiro.
typedef enum {
    CAMPO_DESIGNACAO, CAMPO_NOME, CAMPO_DATA, CAMPO_ANO, CAMPO_MES,
    CAMPO_DISTANCIA, CAMPO_VELOCIDADE, CAMPO_MAGNITUDE, CAMPO_DIAS,
    CAMPO_PASSADO, CAMPO_PASSADO_VALIDO, CAMPO_FUTURO, CAMPO_FUTURO_VALIDO,
    CAMPO_RISCO, CAMPO_PANICO, CAMPO_CATEGORIA, CAMPO_VEREDITO,
    CAMPO_SENTRY, CAMPO_SENTRY_VALIDO, CAMPO_IMPACTO, CAMPO_TORINO,
    CAMPO_PALERMO, CAMPO_DIAMETRO
} CampoNasa;

// Unica fonte de verdade dos nomes de campo do dataset. Qualquer nome que
// nao esteja aqui e rejeitado - inclusive na validacao da consulta.
static const struct {
    const char *nome;
    CampoNasa id;
} CAMPOS_NASA[] = {
    {"asteroid_designation",    CAMPO_DESIGNACAO},
    {"asteroid_fullname",       CAMPO_NOME},
    {"close_approach_date",     CAMPO_DATA},
    {"year",                    CAMPO_ANO},
    {"month",                   CAMPO_MES},
    {"distance_au",             CAMPO_DISTANCIA},
    {"velocity_km_s",           CAMPO_VELOCIDADE},
    {"absolute_magnitude",      CAMPO_MAGNITUDE},
    {"days_until_approach",     CAMPO_DIAS},
    {"is_past_event",           CAMPO_PASSADO},
    {"is_past_event_valido",    CAMPO_PASSADO_VALIDO},
    {"is_future_event",         CAMPO_FUTURO},
    {"is_future_event_valido",  CAMPO_FUTURO_VALIDO},
    {"risk_score",              CAMPO_RISCO},
    {"panic_level",             CAMPO_PANICO},
    {"threat_category",         CAMPO_CATEGORIA},
    {"panic_verdict",           CAMPO_VEREDITO},
    {"on_sentry_list",          CAMPO_SENTRY},
    {"on_sentry_list_valido",   CAMPO_SENTRY_VALIDO},
    {"sentry_impact_prob",      CAMPO_IMPACTO},
    {"sentry_torino_scale",     CAMPO_TORINO},
    {"sentry_palermo_scale",    CAMPO_PALERMO},
    {"sentry_diameter_km",      CAMPO_DIAMETRO}
};

#define TOTAL_CAMPOS (sizeof(CAMPOS_NASA) / sizeof(CAMPOS_NASA[0]))

// Busca linear em 23 nomes. Chamada uma vez por leitura de campo.
static bool buscar_campo(const char *nome, CampoNasa *saida) {
    if (nome == NULL) return false;
    for (size_t i = 0; i < TOTAL_CAMPOS; i++) {
        if (strcmp(nome, CAMPOS_NASA[i].nome) == 0) {
            *saida = CAMPOS_NASA[i].id;
            return true;
        }
    }
    return false;
}

// Versao publica: usada pelo main para recusar consulta com campo invalido
// antes de comecar a ler o CSV.
bool item_nasa_tem_campo(const char *campo) {
    CampoNasa ignorado;
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

// Booleano so responde se a celula original existia no CSV.
// valido == false conta como valor ausente, entao o filtro rejeita o item
// em vez de comparar contra um false que ninguem digitou.
static bool booleano(bool valor, bool valido, ValorQuery *saida) {
    if (!valido) return false;
    saida->tipo = QUERY_BOOLEANO;
    saida->valor.booleano = valor;
    return true;
}

// Ponto de entrada do modulo generico: recebe void* e devolve o campo pedido.
bool item_nasa_ler_campo(const void *item, const char *campo, ValorQuery *saida) {
    if (item == NULL || campo == NULL || saida == NULL) return false;

    const ITEM_NASA *astro = (const ITEM_NASA *)item;

    CampoNasa id;
    if (!buscar_campo(campo, &id)) return false;

    switch (id) {
        case CAMPO_DESIGNACAO: return texto(astro->asteroid_designation, saida);
        case CAMPO_NOME:       return texto(astro->asteroid_fullname, saida);
        case CAMPO_DATA:       return texto(astro->close_approach_date, saida);
        case CAMPO_CATEGORIA:  return texto(astro->threat_category, saida);
        case CAMPO_VEREDITO:   return texto(astro->panic_verdict, saida);

        case CAMPO_ANO:        return numero((double)astro->year, saida);
        case CAMPO_MES:        return numero((double)astro->month, saida);
        case CAMPO_DIAS:       return numero((double)astro->days_until_approach, saida);
        case CAMPO_PANICO:     return numero((double)astro->panic_level, saida);

        case CAMPO_DISTANCIA:  return numero(astro->distance_au, saida);
        case CAMPO_VELOCIDADE: return numero(astro->velocity_km_s, saida);
        case CAMPO_MAGNITUDE:  return numero(astro->absolute_magnitude, saida);
        case CAMPO_RISCO:      return numero(astro->risk_score, saida);
        case CAMPO_IMPACTO:    return numero(astro->sentry_impact_prob, saida);
        case CAMPO_TORINO:     return numero(astro->sentry_torino_scale, saida);
        case CAMPO_PALERMO:    return numero(astro->sentry_palermo_scale, saida);
        case CAMPO_DIAMETRO:   return numero(astro->sentry_diameter_km, saida);

        // Os _valido sao consultaveis de proposito: permitem filtrar por
        // "campo presente no CSV", que e uma pergunta legitima da consulta.
        case CAMPO_PASSADO:
            return booleano(astro->is_past_event, astro->is_past_event_valido, saida);
        case CAMPO_PASSADO_VALIDO:
            return booleano(astro->is_past_event_valido, true, saida);
        case CAMPO_FUTURO:
            return booleano(astro->is_future_event, astro->is_future_event_valido, saida);
        case CAMPO_FUTURO_VALIDO:
            return booleano(astro->is_future_event_valido, true, saida);
        case CAMPO_SENTRY:
            return booleano(astro->on_sentry_list, astro->on_sentry_list_valido, saida);
        case CAMPO_SENTRY_VALIDO:
            return booleano(astro->on_sentry_list_valido, true, saida);
    }
    return false; // inalcancavel se CAMPOS_NASA e o enum estiverem sincronizados
}