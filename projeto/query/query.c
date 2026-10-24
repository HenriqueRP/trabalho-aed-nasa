#include "query.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Os comandos de consulta nao dependem do formato interno dos datasets
struct query_ {
    FiltroQuery *filtros;
    size_t num_filtros, cap_filtros;
    SimilaridadeQuery *sims;
    size_t num_sims, cap_sims;

    double accept;
    bool tem_accept;

    OrdemQuery order;
    bool tem_order;
    CamposQuery group;
    bool tem_group;
    CamposQuery cross;
    bool tem_cross;
};

// strdup nao faz parte do C padrao: usamos nossa propria copia de string
static char *copiar_string(const char *origem) {
    size_t tamanho = strlen(origem) + 1;
    char *copia = malloc(tamanho);
    if (copia != NULL) memcpy(copia, origem, tamanho);
    return copia;
}

static char *tirar_espacos(char *str) {
    while (isspace((unsigned char)*str)) str++;
    size_t tamanho = strlen(str);
    while (tamanho > 0 && isspace((unsigned char)str[tamanho - 1])) {
        str[--tamanho] = '\0';
    }
    return str;
}

static bool numero_double(const char *str, double *saida) {
    char *fim;
    errno = 0;
    double x = strtod(str, &fim);
    if (fim == str || *fim != '\0' || errno == ERANGE || !isfinite(x)) return false;
    *saida = x;
    return true;
}

static bool redimensionar(void **dados, size_t *capacidade,
                          size_t quantidade, size_t tamanho_item) {
    if (quantidade < *capacidade) return true;
    if (*capacidade > SIZE_MAX / 2) return false;
    size_t nova = (*capacidade == 0) ? 4 : (*capacidade * 2);
    if (nova > SIZE_MAX / tamanho_item) return false;
    void *p = realloc(*dados, nova * tamanho_item);
    if (p == NULL) return false;
    *dados = p;
    *capacidade = nova;
    return true;
}

static bool operador_parse(const char *texto, OperadorFiltro *saida) {
    if (!strcmp(texto, "==")) *saida = QUERY_IGUAL;
    else if (!strcmp(texto, "!=")) *saida = QUERY_DIFERENTE;
    else if (!strcmp(texto, "<")) *saida = QUERY_MENOR;
    else if (!strcmp(texto, "<=")) *saida = QUERY_MENOR_IGUAL;
    else if (!strcmp(texto, ">")) *saida = QUERY_MAIOR;
    else if (!strcmp(texto, ">=")) *saida = QUERY_MAIOR_IGUAL;
    else return false;
    return true;
}

static bool adicionar_filtro(Query *query, const char *campo,
                            OperadorFiltro op, const char *valor) {
    char *novo_campo = copiar_string(campo);
    char *novo_valor = copiar_string(valor);
    if (novo_campo == NULL || novo_valor == NULL) {
        free(novo_campo);
        free(novo_valor);
        return false;
    }
    if (!redimensionar((void **)&query->filtros, &query->cap_filtros,
                       query->num_filtros, sizeof(*query->filtros))) {
        free(novo_campo);
        free(novo_valor);
        return false;
    }
    query->filtros[query->num_filtros++] = (FiltroQuery){novo_campo, op, novo_valor};
    return true;
}

static bool adicionar_sim(Query *query, const char *campo,
                          double alvo, double tolerancia, double peso) {
    char *novo_campo = copiar_string(campo);
    if (novo_campo == NULL) return false;
    if (!redimensionar((void **)&query->sims, &query->cap_sims,
                       query->num_sims, sizeof(*query->sims))) {
        free(novo_campo);
        return false;
    }
    query->sims[query->num_sims++] =
        (SimilaridadeQuery){novo_campo, alvo, tolerancia, peso};
    return true;
}

static bool definir_campos(CamposQuery *destino, const char *campo1,
                           const char *campo2) {
    char *primeiro = copiar_string(campo1);
    char *segundo = copiar_string(campo2);
    if (primeiro == NULL || segundo == NULL) {
        free(primeiro);
        free(segundo);
        return false;
    }
    destino->campo1 = primeiro;
    destino->campo2 = segundo;
    return true;
}

static bool interpretar_linha(Query *query, char *linha) {
    // Dividimos a linha por '|', sem perder campos vazios.
    char *partes[6];
    size_t quantidade = 1;
    partes[0] = linha;
    for (char *p = linha; *p != '\0'; p++) {
        if (*p == '|') {
            if (quantidade == 6) return false;
            *p = '\0';
            partes[quantidade++] = p + 1;
        }
    }
    if (quantidade > 5) return false;
    for (size_t i = 0; i < quantidade; i++) {
        partes[i] = tirar_espacos(partes[i]);
        if (*partes[i] == '\0') return false;
    }

    if (!strcmp(partes[0], "FILTER")) {
        OperadorFiltro op;
        return quantidade == 4 && operador_parse(partes[2], &op) &&
               adicionar_filtro(query, partes[1], op, partes[3]);
    }
    if (!strcmp(partes[0], "SIM")) {
        double alvo, tolerancia, peso;
        return quantidade == 5 &&
               numero_double(partes[2], &alvo) &&
               numero_double(partes[3], &tolerancia) &&
               numero_double(partes[4], &peso) &&
               tolerancia > 0.0 && peso >= 0.0 &&
               adicionar_sim(query, partes[1], alvo, tolerancia, peso);
    }
    if (!strcmp(partes[0], "ACCEPT")) {
        double limiar;
        if (quantidade != 2 || query->tem_accept ||
            !numero_double(partes[1], &limiar) || limiar < 0.0 || limiar > 1.0)
            return false;
        query->accept = limiar;
        query->tem_accept = true;
        return true;
    }
    if (!strcmp(partes[0], "ORDER")) {
        if (quantidade != 3 || query->tem_order) return false;
        DirecaoQuery direcao;
        if (!strcmp(partes[2], "ASC")) direcao = QUERY_ASC;
        else if (!strcmp(partes[2], "DESC")) direcao = QUERY_DESC;
        else return false;
        char *campo = copiar_string(partes[1]);
        if (campo == NULL) return false;
        query->order = (OrdemQuery){campo, direcao};
        query->tem_order = true;
        return true;
    }
    if (!strcmp(partes[0], "GROUP")) {
        if (quantidade != 3 || query->tem_group ||
            !definir_campos(&query->group, partes[1], partes[2])) return false;
        query->tem_group = true;
        return true;
    }
    if (!strcmp(partes[0], "CROSS")) {
        if (quantidade != 3 || query->tem_cross ||
            !definir_campos(&query->cross, partes[1], partes[2])) return false;
        query->tem_cross = true;
        return true;
    }
    return false; // Comando desconhecido (nao "bateu" com nenhum dos comandos)
}

Query *query_parse(const Config *config) {
    if (config == NULL || config->path == NULL) {
        fprintf(stderr, "Query: caminho da consulta nao informado.\n");
        return NULL;
    }
    FILE *arquivo = fopen(config->path, "r");
    if (arquivo == NULL) {
        perror("Query: nao foi possivel abrir o arquivo");
        return NULL;
    }

    Query *query = calloc(1, sizeof(*query));
    if (query == NULL) {
        fclose(arquivo);
        return NULL;
    }

    // Decidimos que 4096 caracteres por linha eh um limite pratico
    // linhas maiores sao rejeitadas explicitamente
    char linha[4096];
    size_t numero_linha = 0;
    bool sucesso = true;
    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        numero_linha++;
        size_t tamanho = strlen(linha);
        if (tamanho == sizeof(linha) - 1 && linha[tamanho - 1] != '\n') {
            sucesso = false; // Possivel linha truncada
            break;
        }
        char *texto = tirar_espacos(linha);
        if (*texto == '\0' || *texto == '#') continue;
        if (!interpretar_linha(query, texto)) {
            sucesso = false;
            break;
        }
    }
    if (ferror(arquivo)) sucesso = false;
    fclose(arquivo);

    if (!sucesso) {
        fprintf(stderr, "Query: comando ou leitura invalida em '%s', linha %zu.\n",
                config->path, numero_linha);
        query_apagar(&query);
        return NULL;
    }

    // A media ponderada exige soma de pesos positiva
    if (query->num_sims > 0) {
        double pesos = 0.0;
        for (size_t i = 0; i < query->num_sims; i++) {
            pesos += query->sims[i].peso;
        }
        if (!isfinite(pesos) || pesos <= 0.0) {
            fprintf(stderr, "Query: soma de pesos SIM invalida em '%s'.\n",
                    config->path);
            query_apagar(&query);
            return NULL;
        }
    }
    return query;
}

void query_apagar(Query **query) {
    if (query == NULL || *query == NULL) return;
    Query *q = *query;
    for (size_t i = 0; i < q->num_filtros; i++) {
        free((void *)q->filtros[i].campo);
        free((void *)q->filtros[i].valor);
    }
    for (size_t i = 0; i < q->num_sims; i++) free((void *)q->sims[i].campo);
    free(q->filtros);
    free(q->sims);
    free((void *)q->order.campo);
    free((void *)q->group.campo1);
    free((void *)q->group.campo2);
    free((void *)q->cross.campo1);
    free((void *)q->cross.campo2);
    free(q);
    *query = NULL;
}

size_t query_num_filtros(const Query *q) { return q ? q->num_filtros : 0; }
size_t query_num_similaridades(const Query *q) { return q ? q->num_sims : 0; }
double query_get_accept(const Query *q) { return q ? q->accept : 0.0; }
bool query_tem_accept(const Query *q) { return q && q->tem_accept; }

bool query_get_filtro(const Query *q, size_t indice, FiltroQuery *saida) {
    if (!q || !saida || indice >= q->num_filtros) return false;
    *saida = q->filtros[indice];
    return true;
}
bool query_get_similaridade(const Query *q, size_t indice, SimilaridadeQuery *saida) {
    if (!q || !saida || indice >= q->num_sims) return false;
    *saida = q->sims[indice];
    return true;
}
bool query_get_order(const Query *q, OrdemQuery *saida) {
    if (!q || !saida || !q->tem_order) return false;
    *saida = q->order;
    return true;
}
bool query_get_group(const Query *q, CamposQuery *saida) {
    if (!q || !saida || !q->tem_group) return false;
    *saida = q->group;
    return true;
}
bool query_get_cross(const Query *q, CamposQuery *saida) {
    if (!q || !saida || !q->tem_cross) return false;
    *saida = q->cross;
    return true;
}

static bool comparar(double x, double y, OperadorFiltro op) {
    switch (op) {
        case QUERY_IGUAL: return x == y;
        case QUERY_DIFERENTE: return x != y;
        case QUERY_MENOR: return x < y;
        case QUERY_MENOR_IGUAL: return x <= y;
        case QUERY_MAIOR: return x > y;
        case QUERY_MAIOR_IGUAL: return x >= y;
    }
    return false;
}

static bool avaliar_filtro(const FiltroQuery *filtro, const void *item,
                          LeitorCampoQuery ler_campo) {
    ValorQuery campo;
    if (!ler_campo(item, filtro->campo, &campo)) return false;
    if (campo.tipo == QUERY_NUMERO) {
        double numero;
        return isfinite(campo.valor.numero) && numero_double(filtro->valor, &numero) &&
               comparar(campo.valor.numero, numero, filtro->operador);
    }
    if (campo.tipo == QUERY_BOOLEANO) {
        bool valor;
        if (!strcmp(filtro->valor, "true")) valor = true;
        else if (!strcmp(filtro->valor, "false")) valor = false;
        else return false;
        if (filtro->operador != QUERY_IGUAL && filtro->operador != QUERY_DIFERENTE)
            return false;
        return (campo.valor.booleano == valor) == (filtro->operador == QUERY_IGUAL);
    }
    if (campo.tipo == QUERY_TEXTO && campo.valor.texto != NULL) {
        int ordem = strcmp(campo.valor.texto, filtro->valor);
        return comparar((double)(ordem > 0) - (ordem < 0), 0, filtro->operador);
    }
    return false;
}

bool query_avaliar(const Query *q, const void *item,
                   LeitorCampoQuery ler_campo, double *similaridade) {
    if (similaridade != NULL) *similaridade = 0.0;
    if (q == NULL || item == NULL || ler_campo == NULL) return false;

    // Todos os FILTER devem passar: basta um falhar para rejeitar
    for (size_t i = 0; i < q->num_filtros; i++) {
        if (!avaliar_filtro(&q->filtros[i], item, ler_campo)) return false;
    }

    double nota = 1.0;
    if (q->num_sims != 0) {
        double soma = 0.0, soma_pesos = 0.0;
        for (size_t i = 0; i < q->num_sims; i++) {
            const SimilaridadeQuery *sim = &q->sims[i];
            ValorQuery campo;
            if (!ler_campo(item, sim->campo, &campo) || campo.tipo != QUERY_NUMERO ||
                !isfinite(campo.valor.numero)) return false;
            double parcial = 1.0 - fabs(campo.valor.numero - sim->alvo) / sim->tolerancia;
            if (parcial < 0.0) parcial = 0.0;
            soma += parcial * sim->peso;
            soma_pesos += sim->peso;
        }
        if (soma_pesos <= 0.0 || !isfinite(soma)) return false;
        nota = soma / soma_pesos;
    }
    if (similaridade != NULL) *similaridade = nota;
    return nota >= q->accept;
}
