#ifndef QUERY_H
#define QUERY_H

#include <stdbool.h>
#include <stddef.h>
#include "config.h"


//este modulo interpreta os comandos do arquivo de consulta e armazena
//criterios usados pelo modulo de buscas.
//as representacoes internas ficam encapsuladas em query.c.


typedef struct query_ Query;
typedef struct filtro_query_ FiltroQuery;
typedef struct similaridade_query_ SimilaridadeQuery;
typedef struct ordem_query_ OrdemQuery;
typedef struct campos_query_ CamposQuery;
typedef struct valor_query_ ValorQuery;

//operadores de comparacao aceitos por FILTER.
typedef enum {
    QUERY_IGUAL,
    QUERY_DIFERENTE,
    QUERY_MENOR,
    QUERY_MENOR_IGUAL,
    QUERY_MAIOR,
    QUERY_MAIOR_IGUAL
} OperadorFiltro;

//direcao aceita por ORDER.
typedef enum {
    QUERY_ASC,
    QUERY_DESC
} DirecaoQuery;

//tipos de valores que podem ser lidos de um campo.
typedef enum {
    QUERY_TEXTO,
    QUERY_NUMERO,
    QUERY_BOOLEANO
} TipoValorQuery;


//valorQuery e um objeto opaco usado para transferir valores entre leitores,
//query e TADs. Sua struct nao aparece em nenhum cabecalho.
//os construtores de texto mantem uma referencia emprestada ao texto; a copia
//profunda, usada quando um TAD precisa guardar a chave, e feita por separado.

ValorQuery *query_valor_texto(const char *texto);
ValorQuery *query_valor_numero(double numero);
ValorQuery *query_valor_booleano(bool booleano);
ValorQuery *query_valor_copiar(const ValorQuery *valor);
void query_valor_apagar(ValorQuery **valor);
TipoValorQuery query_valor_tipo(const ValorQuery *valor);
const char *query_valor_get_texto(const ValorQuery *valor);
double query_valor_get_numero(const ValorQuery *valor);
bool query_valor_get_booleano(const ValorQuery *valor);
bool query_valor_iguais(const ValorQuery *a, const ValorQuery *b);
const char *query_valor_formatar(const ValorQuery *valor, char *buffer, size_t tamanho);

//o leitor cria um ValorQuery e o devolve por saida quando o campo existe.
typedef bool (*LeitorCampoQuery)(const void *item, const char *campo,
                                 ValorQuery **saida);

//carrega e valida os comandos do arquivo de consulta.
Query *query_parse(const Config *config);
void query_apagar(Query **query);

size_t query_num_filtros(const Query *query);
size_t query_num_similaridades(const Query *query);

//getters de cada propriedade de FILTER, por indice.
const char *query_filtro_campo(const Query *query, size_t indice);
OperadorFiltro query_filtro_operador(const Query *query, size_t indice);
const char *query_filtro_valor(const Query *query, size_t indice);

//getters de cada propriedade de SIM, por indice.
const char *query_similaridade_campo(const Query *query, size_t indice);
double query_similaridade_alvo(const Query *query, size_t indice);
double query_similaridade_tolerancia(const Query *query, size_t indice);
double query_similaridade_peso(const Query *query, size_t indice);

double query_get_accept(const Query *query);
bool query_tem_accept(const Query *query);

//retornam false se o comando nao existir na consulta.
bool query_get_order(const Query *query, const char **campo,
                     DirecaoQuery *direcao);
bool query_get_group(const Query *query, const char **campo1,
                     const char **campo2);
bool query_get_cross(const Query *query, const char **campo1,
                     const char **campo2);

//avalia FILTER e SIM para um registro; similaridade pode ser NULL.
// optamos por usar um ponteiro para função a fim de deixar a função mais geral
// e poder utiliza-la para qualquer item que forneca um leitor para seus campos
// utilizamos como referencia o capitulo 6 do livro "A book on C"
bool query_avaliar(const Query *query, const void *item,
                   LeitorCampoQuery ler_campo, double *similaridade);

#endif
