#include "query.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//os comandos de consulta nao dependem do formato interno dos datasets
struct filtro_query_ {
    const char *campo;
    OperadorFiltro operador;
    const char *valor;
};

struct similaridade_query_ {
    const char *campo;
    double alvo;
    double tolerancia;
    double peso;
};

struct ordem_query_ {
    const char *campo;
    DirecaoQuery direcao;
};

struct campos_query_ {
    const char *campo1;
    const char *campo2;
};

struct valor_query_ {
    TipoValorQuery tipo;
    union {
        const char *texto;
        double numero;
        bool booleano;
    } valor;
    bool possui_texto; //true quando a string foi duplicada por este objeto
};

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

//strdup nao faz parte do C padrao: usamos nossa propria copia de string
//cria uma copia alocada de uma string.
static char *copiar_string(const char *origem) {
    size_t tamanho = strlen(origem) + 1;
    char *copia = malloc(tamanho);
    if (copia != NULL) memcpy(copia, origem, tamanho);
    return copia;
}


//construtores de ValorQuery. Texto vindo de um ITEM e emprestado: o item
//continua sendo dono da string. query_valor_copiar, por outro lado, faz
//uma copia independente para chaves que precisam sobreviver ao ITEM.

//cria um valor de consulta textual.
ValorQuery *query_valor_texto(const char *texto) {
    if (texto == NULL) return NULL;
    ValorQuery *valor = calloc(1, sizeof(*valor));
    if (valor == NULL) return NULL;
    valor->tipo = QUERY_TEXTO;
    valor->valor.texto = texto;
    return valor;
}
//cria um valor de consulta numerico.

ValorQuery *query_valor_numero(double numero) {
    ValorQuery *valor = calloc(1, sizeof(*valor));
    if (valor == NULL) return NULL;
    valor->tipo = QUERY_NUMERO;
    valor->valor.numero = numero;
    return valor;
}
//cria um valor de consulta booleano.

ValorQuery *query_valor_booleano(bool booleano) {
    ValorQuery *valor = calloc(1, sizeof(*valor));
    if (valor == NULL) return NULL;
    valor->tipo = QUERY_BOOLEANO;
    valor->valor.booleano = booleano;
    return valor;
}
//cria uma copia independente de um valor de consulta.

ValorQuery *query_valor_copiar(const ValorQuery *origem) {
    if (origem == NULL) return NULL;
    ValorQuery *copia = NULL;
    if (origem->tipo == QUERY_TEXTO) {
        if (origem->valor.texto == NULL) return NULL;
        char *texto = copiar_string(origem->valor.texto);
        if (texto == NULL) return NULL;
        copia = calloc(1, sizeof(*copia));
        if (copia == NULL) {
            free(texto);
            return NULL;
        }
        copia->tipo = QUERY_TEXTO;
        copia->valor.texto = texto;
        copia->possui_texto = true;
        return copia;
    }
    if (origem->tipo == QUERY_NUMERO) return query_valor_numero(origem->valor.numero);
    if (origem->tipo == QUERY_BOOLEANO) return query_valor_booleano(origem->valor.booleano);
    return NULL;
}
//libera um valor de consulta e zera o ponteiro.

void query_valor_apagar(ValorQuery **valor) {
    if (valor == NULL || *valor == NULL) return;
    if ((*valor)->tipo == QUERY_TEXTO && (*valor)->possui_texto)
        free((void *)(*valor)->valor.texto);
    free(*valor);
    *valor = NULL;
}
//retorna o tipo do valor de consulta.

TipoValorQuery query_valor_tipo(const ValorQuery *valor) {
    if (valor == NULL) return QUERY_TEXTO;
    return valor->tipo;
}
//retorna o texto quando o valor e textual.

const char *query_valor_get_texto(const ValorQuery *valor) {
    if (valor == NULL || valor->tipo != QUERY_TEXTO) return NULL;
    return valor->valor.texto;
}
//retorna o numero quando o valor e numerico.

double query_valor_get_numero(const ValorQuery *valor) {
    if (valor == NULL || valor->tipo != QUERY_NUMERO) return 0.0;
    return valor->valor.numero;
}
//retorna o booleano quando o valor e booleano.

bool query_valor_get_booleano(const ValorQuery *valor) {
    if (valor == NULL || valor->tipo != QUERY_BOOLEANO) return false;
    return valor->valor.booleano;
}
//verifica se dois valores de consulta sao iguais.

bool query_valor_iguais(const ValorQuery *a, const ValorQuery *b) {
    if (a == NULL || b == NULL || a->tipo != b->tipo) return false;
    if (a->tipo == QUERY_TEXTO) {
        if (a->valor.texto == NULL || b->valor.texto == NULL) return false;
        return strcmp(a->valor.texto, b->valor.texto) == 0;
    }
    if (a->tipo == QUERY_NUMERO) return a->valor.numero == b->valor.numero;
    if (a->tipo == QUERY_BOOLEANO) return a->valor.booleano == b->valor.booleano;
    return false;
}
//converte um valor de consulta para texto.

const char *query_valor_formatar(const ValorQuery *valor, char *buffer, size_t tamanho) {
    if (buffer == NULL || tamanho == 0) return NULL;
    if (valor == NULL) {
        snprintf(buffer, tamanho, "(nulo)");
        return buffer;
    }
    if (valor->tipo == QUERY_TEXTO) {
        if (valor->valor.texto == NULL) snprintf(buffer, tamanho, "(vazio)");
        else snprintf(buffer, tamanho, "%s", valor->valor.texto);
        return buffer;
    }
    if (valor->tipo == QUERY_NUMERO) {
        snprintf(buffer, tamanho, "%g", valor->valor.numero);
        return buffer;
    }
    if (valor->valor.booleano) {
        snprintf(buffer, tamanho, "%s", "true");
    } else {
        snprintf(buffer, tamanho, "%s", "false");
    }
    return buffer;
}
//remove espacos no inicio e no fim da string.

static char *tirar_espacos(char *str) {
    while (isspace((unsigned char)*str)) str++;
    size_t tamanho = strlen(str);
    while (tamanho > 0 && isspace((unsigned char)str[tamanho - 1])) {
        str[--tamanho] = '\0';
    }
    return str;
}
//valida e converte uma string para double finito.

static bool numero_double(const char *str, double *saida) {
    char *fim;
    errno = 0;
    double x = strtod(str, &fim);
    if (fim == str || *fim != '\0' || errno == ERANGE || !isfinite(x)) return false;
    *saida = x;
    return true;
}
//aumenta um vetor dinamico quando a capacidade acaba.

static bool redimensionar(void **dados, size_t *capacidade,
                          size_t quantidade, size_t tamanho_item) {
    if (quantidade < *capacidade) return true;
    if (*capacidade > SIZE_MAX / 2) return false;
    size_t nova;
    if (*capacidade == 0) nova = 4;
    else nova = *capacidade * 2;
    if (nova > SIZE_MAX / tamanho_item) return false;
    void *p = realloc(*dados, nova * tamanho_item);
    if (p == NULL) return false;
    *dados = p;
    *capacidade = nova;
    return true;
}
//converte o operador textual para a constante correspondente.

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
//armazena um novo criterio FILTER na consulta.

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
//armazena um novo criterio SIM na consulta.

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
//copia os nomes dos campos de agrupamento.

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
//interpreta e valida uma linha do arquivo de consulta.

static bool interpretar_linha(Query *query, char *linha) {
    //dividimos a linha por '|', sem perder campos vazios.
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
    return false; //comando desconhecido (nao "bateu" com nenhum dos comandos)
}
//le e valida os comandos do arquivo de consulta.

Query *query_parse(const Config *config) {
    if (config == NULL || config_get_path(config) == NULL) {
        fprintf(stderr, "Query: caminho da consulta nao informado.\n");
        return NULL;
    }
    FILE *arquivo = fopen(config_get_path(config), "r");
    if (arquivo == NULL) {
        perror("Query: nao foi possivel abrir o arquivo");
        return NULL;
    }

    Query *query = calloc(1, sizeof(*query));
    if (query == NULL) {
        fclose(arquivo);
        return NULL;
    }

    //decidimos que 4096 caracteres por linha eh um limite pratico
    //linhas maiores sao rejeitadas explicitamente
    char linha[4096];
    size_t numero_linha = 0;
    bool sucesso = true;
    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        numero_linha++;
        size_t tamanho = strlen(linha);
        if (tamanho == sizeof(linha) - 1 && linha[tamanho - 1] != '\n') {
            sucesso = false; //possivel linha truncada
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
                config_get_path(config), numero_linha);
        query_apagar(&query);
        return NULL;
    }

    //a media ponderada exige soma de pesos positiva
    if (query->num_sims > 0) {
        double pesos = 0.0;
        for (size_t i = 0; i < query->num_sims; i++) {
            pesos += query->sims[i].peso;
        }
        if (!isfinite(pesos) || pesos <= 0.0) {
            fprintf(stderr, "Query: soma de pesos SIM invalida em '%s'.\n",
                    config_get_path(config));
            query_apagar(&query);
            return NULL;
        }
    }
    return query;
}
//libera os criterios e a consulta.

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
//retorna a quantidade de filtros da consulta.

size_t query_num_filtros(const Query *q) {
    if (q == NULL) return 0;
    return q->num_filtros;
}
//retorna a quantidade de criterios de similaridade.
size_t query_num_similaridades(const Query *q) {
    if (q == NULL) return 0;
    return q->num_sims;
}
//retorna o limiar de aceitacao da consulta.
double query_get_accept(const Query *q) {
    if (q == NULL) return 0.0;
    return q->accept;
}
//informa se a consulta definiu ACCEPT.
bool query_tem_accept(const Query *q) { return q && q->tem_accept; }
//retorna o campo do filtro indicado.

const char *query_filtro_campo(const Query *q, size_t indice) {
    if (q == NULL || indice >= q->num_filtros) return NULL;
    return q->filtros[indice].campo;
}
//retorna o operador do filtro indicado.

OperadorFiltro query_filtro_operador(const Query *q, size_t indice) {
    if (q == NULL || indice >= q->num_filtros) return QUERY_IGUAL;
    return q->filtros[indice].operador;
}
//retorna o valor do filtro indicado.

const char *query_filtro_valor(const Query *q, size_t indice) {
    if (q == NULL || indice >= q->num_filtros) return NULL;
    return q->filtros[indice].valor;
}
//retorna o campo do criterio SIM indicado.

const char *query_similaridade_campo(const Query *q, size_t indice) {
    if (q == NULL || indice >= q->num_sims) return NULL;
    return q->sims[indice].campo;
}
//retorna o alvo do criterio SIM indicado.

double query_similaridade_alvo(const Query *q, size_t indice) {
    if (q == NULL || indice >= q->num_sims) return NAN;
    return q->sims[indice].alvo;
}
//retorna a tolerancia do criterio SIM indicado.

double query_similaridade_tolerancia(const Query *q, size_t indice) {
    if (q == NULL || indice >= q->num_sims) return NAN;
    return q->sims[indice].tolerancia;
}
//retorna o peso do criterio SIM indicado.

double query_similaridade_peso(const Query *q, size_t indice) {
    if (q == NULL || indice >= q->num_sims) return NAN;
    return q->sims[indice].peso;
}
//retorna o criterio ORDER, se existir.

bool query_get_order(const Query *q, const char **campo, DirecaoQuery *direcao) {
    if (q == NULL || !q->tem_order || campo == NULL || direcao == NULL) return false;
    *campo = q->order.campo;
    *direcao = q->order.direcao;
    return true;
}
//retorna os campos GROUP, se existirem.

bool query_get_group(const Query *q, const char **campo1, const char **campo2) {
    if (q == NULL || !q->tem_group || campo1 == NULL || campo2 == NULL) return false;
    *campo1 = q->group.campo1;
    *campo2 = q->group.campo2;
    return true;
}
//retorna os campos CROSS, se existirem.

bool query_get_cross(const Query *q, const char **campo1, const char **campo2) {
    if (q == NULL || !q->tem_cross || campo1 == NULL || campo2 == NULL) return false;
    *campo1 = q->cross.campo1;
    *campo2 = q->cross.campo2;
    return true;
}
//compara dois valores numericos segundo o operador informado.

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
//verifica se um item satisfaz um criterio FILTER.

static bool avaliar_filtro(const FiltroQuery *filtro, const void *item,
                          LeitorCampoQuery ler_campo) {
    ValorQuery *campo = NULL;
    if (!ler_campo(item, filtro->campo, &campo) || campo == NULL) {
        query_valor_apagar(&campo);
        return false;
    }

    bool resultado = false;
    TipoValorQuery tipo = query_valor_tipo(campo);
    if (tipo == QUERY_NUMERO) {
        double numero;
        double atual = query_valor_get_numero(campo);
        resultado = isfinite(atual) && numero_double(filtro->valor, &numero) &&
                    comparar(atual, numero, filtro->operador);
    } else if (tipo == QUERY_BOOLEANO) {
        bool valor;
        if (!strcmp(filtro->valor, "true")) valor = true;
        else if (!strcmp(filtro->valor, "false")) valor = false;
        else {
            query_valor_apagar(&campo);
            return false;
        }
        if (filtro->operador != QUERY_IGUAL && filtro->operador != QUERY_DIFERENTE) {
            query_valor_apagar(&campo);
            return false;
        }
        resultado = (query_valor_get_booleano(campo) == valor) ==
                    (filtro->operador == QUERY_IGUAL);
    } else if (tipo == QUERY_TEXTO && query_valor_get_texto(campo) != NULL) {
        int ordem = strcmp(query_valor_get_texto(campo), filtro->valor);
        resultado = comparar((double)(ordem > 0) - (ordem < 0), 0, filtro->operador);
    }
    query_valor_apagar(&campo);
    return resultado;
}
//avalia os filtros e a similaridade de um item.

bool query_avaliar(const Query *q, const void *item,
                   LeitorCampoQuery ler_campo, double *similaridade) {
    if (similaridade != NULL) *similaridade = 0.0;
    if (q == NULL || item == NULL || ler_campo == NULL) return false;

    //todos os FILTER devem passar: basta um falhar para rejeitar
    for (size_t i = 0; i < q->num_filtros; i++) {
        if (!avaliar_filtro(&q->filtros[i], item, ler_campo)) return false;
    }

    double nota = 1.0;
    if (q->num_sims != 0) {
        double soma = 0.0, soma_pesos = 0.0;
        for (size_t i = 0; i < q->num_sims; i++) {
            const SimilaridadeQuery *sim = &q->sims[i];
            ValorQuery *campo = NULL;
            if (!ler_campo(item, sim->campo, &campo) || campo == NULL ||
                query_valor_tipo(campo) != QUERY_NUMERO ||
                !isfinite(query_valor_get_numero(campo))) {
                query_valor_apagar(&campo);
                return false;
            }
            double parcial = 1.0 - fabs(query_valor_get_numero(campo) - sim->alvo) /
                                     sim->tolerancia;
            query_valor_apagar(&campo);
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
