#ifndef ITEM_ANCINE_H
#define ITEM_ANCINE_H

#include <stdbool.h>
#include "query.h"

// item_ancine.h - TAD do registro de obra audiovisual (Parte 2).
//
// Guarda os 18 campos do CSV da ANCINE, inclusive valores ausentes.
// duracao_total_minutos e double porque o dataset traz fracoes (12,5 min).
// Declarar int truncaria a parte decimal sem aviso.

typedef struct item_ancine_ ITEM_ANCINE;

// Construtor de 18 parametros. Copia todas as strings - nao retem ponteiro
// do chamador. Retorna NULL se faltar memoria (libera o que ja alocou).
ITEM_ANCINE *item_ancine_criar(
    const char *titulo_original, const char *cpb,
    const char *data_emissao_cpb, const char *situacao_obra,
    const char *tipo_obra, const char *subtipo_obra,
    const char *classificacao_obra, const char *organizacao_temporal,
    double duracao_total_minutos, int quantidade_episodios,
    int ano_producao_inicial, int ano_producao_final,
    const char *segmento_destinacao_inicial, bool coproducao_internacional,
    bool coproducao_valida,
    const char *requerente, const char *cnpj_requerente,
    const char *uf_requerente, const char *municipio_requerente);

ITEM_ANCINE *item_ancine_copiar(ITEM_ANCINE *item);

// Ponte para o modulo generico: mesma assinatura de LeitorCampoQuery.
// Retorna false para campo desconhecido OU valor ausente - os dois casos
// viram "registro rejeitado", nunca erro fatal.
bool item_ancine_ler_campo(const void *item, const char *campo, ValorQuery **saida);

// Validacao antecipada: deixa recusar uma consulta com campo inexistente
// antes de varrer o CSV inteiro e devolver saida vazia sem explicacao.
bool item_ancine_tem_campo(const char *campo);

void item_ancine_apagar(ITEM_ANCINE **item);

// Getters: todos testam NULL e devolvem sentinela coerente com o tipo
// (-1 para int, NAN para double, false para bool, NULL para char*).
const char *get_ancine_titulo_original(ITEM_ANCINE *item);
const char *get_ancine_cpb(ITEM_ANCINE *item);
const char *get_ancine_data_emissao_cpb(ITEM_ANCINE *item);
const char *get_ancine_situacao_obra(ITEM_ANCINE *item);
const char *get_ancine_tipo_obra(ITEM_ANCINE *item);
const char *get_ancine_subtipo_obra(ITEM_ANCINE *item);
const char *get_ancine_classificacao_obra(ITEM_ANCINE *item);
const char *get_ancine_organizacao_temporal(ITEM_ANCINE *item);

double get_ancine_duracao_total_minutos(ITEM_ANCINE *item);
int get_ancine_quantidade_episodios(ITEM_ANCINE *item);
int get_ancine_ano_producao_inicial(ITEM_ANCINE *item);
int get_ancine_ano_producao_final(ITEM_ANCINE *item);

const char *get_ancine_segmento_destinacao_inicial(ITEM_ANCINE *item);
bool get_ancine_coproducao_internacional(ITEM_ANCINE *item);

const char *get_ancine_requerente(ITEM_ANCINE *item);
const char *get_ancine_cnpj_requerente(ITEM_ANCINE *item);
const char *get_ancine_uf_requerente(ITEM_ANCINE *item);
const char *get_ancine_municipio_requerente(ITEM_ANCINE *item);

#endif
