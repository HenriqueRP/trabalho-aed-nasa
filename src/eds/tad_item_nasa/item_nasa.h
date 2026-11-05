#ifndef ITEM_NASA_H
#define ITEM_NASA_H

#include <stdbool.h>
#include "query.h"

//item_nasa.h - TAD do registro de asteroide (Parte 1).

//guarda os 23 campos do CSV. Cada booleano tem um par "_valido" porque
//celula vazia no CSV nao e false: e ausencia. Sem esse par, um filtro
//"on_sentry_list == false" acabaria aceitando registro sem dado.

typedef struct item_nasa_ ITEM_NASA;

//construtor de 23 parametros. Copia todas as strings - nao retem ponteiro
//do chamador. Retorna NULL se faltar memoria (libera o que ja alocou).
ITEM_NASA *item_nasa_criar(
    const char *asteroid_designation, const char *asteroid_fullname,
    const char *close_approach_date, int year, int month,
    double distance_au, double velocity_km_s, double absolute_magnitude,
    int days_until_approach, bool is_past_event, bool is_past_event_valido,
    bool is_future_event, bool is_future_event_valido, double risk_score,
    int panic_level, const char *threat_category, const char *panic_verdict,
    bool on_sentry_list, bool on_sentry_list_valido, double sentry_impact_prob,
    double sentry_torino_scale, double sentry_palermo_scale,
    double sentry_diameter_km);

ITEM_NASA *item_nasa_copiar(ITEM_NASA *item);

//ponte para o modulo generico: mesma assinatura de LeitorCampoQuery.
//retorna false para campo desconhecido OU valor ausente - os dois casos
//viram "registro rejeitado", nunca erro fatal.
bool item_nasa_ler_campo(const void *item, const char *campo, ValorQuery **saida);

//validacao antecipada: deixa recusar uma consulta com campo inexistente
//antes de varrer 89 mil linhas e devolver um CSV vazio sem explicacao.
bool item_nasa_tem_campo(const char *campo);

void item_nasa_apagar(ITEM_NASA **item);

//getters: todos testam NULL e devolvem sentinela coerente com o tipo
//(-1 para int, NAN para double, false para bool, NULL para char*).
char *get_nasa_ast_desig(ITEM_NASA *item);
char *get_nasa_ast_fullname(ITEM_NASA *item);
char *get_nasa_close_approach(ITEM_NASA *item);

int get_nasa_year(ITEM_NASA *item);
int get_nasa_month(ITEM_NASA *item);

double get_nasa_distance_au(ITEM_NASA *item);
double get_nasa_velocity_km_s(ITEM_NASA *item);
double get_nasa_absolute_magnitude(ITEM_NASA *item);
int get_nasa_days_until_approach(ITEM_NASA *item);

bool get_nasa_is_past_event(ITEM_NASA *item);
bool get_nasa_is_past_event_valido(ITEM_NASA *item);
bool get_nasa_is_future_event(ITEM_NASA *item);
bool get_nasa_is_future_event_valido(ITEM_NASA *item);

double get_nasa_risk_score(ITEM_NASA *item);
int get_nasa_panic_level(ITEM_NASA *item);

char *get_nasa_threat_category(ITEM_NASA *item);
char *get_nasa_panic_verdict(ITEM_NASA *item);

bool get_nasa_on_sentry_list(ITEM_NASA *item);
bool get_nasa_on_sentry_list_valido(ITEM_NASA *item);

double get_nasa_sentry_impact_prob(ITEM_NASA *item);
double get_nasa_sentry_torino_scale(ITEM_NASA *item);
double get_nasa_sentry_palermo_scale(ITEM_NASA *item);
double get_nasa_sentry_diameter_km(ITEM_NASA *item);

#endif
