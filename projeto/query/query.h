#ifndef QUERY_H
#define QUERY_H

#include <stdbool.h>
#include <stddef.h>
#include "config.h"

/*
De forma geral, esse modulo interpreta as instrucoes de consulta(comandos) no arquivo txt e armazena elas em estruturas que serao utilizadas
pelo modulo de buscas para selecionar os registros que atendem aos requisitos especificados em tais instrucoes
*/

// A representacao interna fica encapsulada em query.c
typedef struct query_ Query;

// Define constantes para os operadores de comparacao entre os registros
typedef enum {
    QUERY_IGUAL,
    QUERY_DIFERENTE,
    QUERY_MENOR,
    QUERY_MENOR_IGUAL,
    QUERY_MAIOR,
    QUERY_MAIOR_IGUAL
} OperadorFiltro;

// Estrutura que armazena a instrucao de filtragem em partes: campo_de_consulta | operador_de_comparacao | valor_de_comparacao
typedef struct {
    const char *campo;
    OperadorFiltro operador;
    const char *valor;
} FiltroQuery;

// "Quebra" da instrucao SIM | campo | valor ideal | tolerancia de variacao | peso no calculo da sim. total
// Estrutura que armazena as informacoes de similaridade minima que um registro precisa ter em relacao a um ideal(definido)
// para ser incluido na estrutura de resultado
typedef struct {
    const char *campo;
    double alvo;
    double tolerancia;
    double peso;
} SimilaridadeQuery;

// Define valores constantes para criterio de ordem crescente e decrescente
typedef enum {
    QUERY_ASC,
    QUERY_DESC
} DirecaoQuery;

// Quebra da instrucao ORDER | campo que sera parametro da ordenacao | crescente ou decrescente
typedef struct {
    const char *campo;
    DirecaoQuery direcao;
} OrdemQuery;

// Estrutura que armazena os campos que serao utilizados para as instrucoes GROUP e CROSS
// ou seja, CROSS | campo1 | campo2 organizara os registros que "passaram" em todos os requisitos
// em uma lista cruzada pelos campos 1 e 2;
// Ja GROUP | campo1 | campo2 organizaraos tais registros eu uma lista generalizada agrupada pela categoria campo1
// e, dentro de tal agrupamento, separara os registros de acordo com a subcategoria campo2
typedef struct {
    const char *campo1;
    const char *campo2;
} CamposQuery;

// Carrega os comandos do arquivo informado em config->path
// Retorna NULL em caso de erro; a mensagem vai para stderr
Query *query_parse(const Config *config);
void query_apagar(Query **query);

// Acesso somente de leitura aos criterios armazenados
size_t query_num_filtros(const Query *query); // getter da quantidade de filtros
bool query_get_filtro(const Query *query, size_t indice, FiltroQuery *saida); // getter do filtro em determinado indice e armazena esse filtro em saida
size_t query_num_similaridades(const Query *query); //getter do numero de similaridades
bool query_get_similaridade(const Query *query, size_t indice, SimilaridadeQuery *saida); //getter da similaridade num determinado indice e armazena essa similaridade em saida

// Se ACCEPT nao apareceu no arquivo, o limiar padrao eh 0.0
double query_get_accept(const Query *query); // getter de ACCEPT
bool query_tem_accept(const Query *query); // verifica se ACCEPT faz parte das instrucoes, retornando true em caso afirmativo, e false caso contrario 

// Retornam false quando o comando correspondente nao aparece na consulta
bool query_get_order(const Query *query, OrdemQuery *saida); //getter das informacoes de ordenacao
bool query_get_group(const Query *query, CamposQuery *saida); //getter das informacoes de agrumapento
bool query_get_cross(const Query *query, CamposQuery *saida); // getter das informacoes para listra cruzada

// Valores genericos: o modulo Query nao conhece os itens das bases
typedef enum {
    QUERY_TEXTO,
    QUERY_NUMERO,
    QUERY_BOOLEANO
} TipoValorQuery;

// estrutura que armazena o valor que a funcao de ler um campo especifico da query retorna 
typedef struct {
    TipoValorQuery tipo;
    union {
        const char *texto;
        double numero;
        bool booleano;
    } valor;
} ValorQuery;

// O Reader/Item fornece esta funcao para consultar um campo pelo nome
// Retornar false significa campo desconhecido ou valor ausente
typedef bool (*LeitorCampoQuery)(const void *item, const char *campo, ValorQuery *saida);

// Aplica todos os FILTER, calcula a media ponderada de SIM e verifica ACCEPT
// Se nao houver SIM, os FILTER bastam e a similaridade e 1.0
// Retorna true se o item for aceito. Similaridade pode ser NULL
bool query_avaliar(const Query *query, const void *item,
                   LeitorCampoQuery ler_campo, double *similaridade);

#endif
