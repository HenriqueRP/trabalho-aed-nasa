#ifndef LISTA_CRUZADA_H
#define LISTA_CRUZADA_H

#include <stdbool.h>
#include "item.h"

// Antes era LISTA/struct lista_, que colide com o typedef de lista_sequencial.h
typedef struct lista_cruzada LISTA_CRUZ;

// Modo por coordenadas inteiras, mantido por compatibilidade com o TAD original.
// As coordenadas vao de 0 a nlinhas-1 e de 0 a ncolunas-1.
LISTA_CRUZ *lista_criar(int nlinhas, int ncolunas);

// Modo CROSS: os eixos sao descobertos a partir dos valores dos campos
// (por exemplo, "threat_category" e "year"). Nao depende da base NASA.
// Copia os nomes dos campos; nao guarda ponteiros para a Query.
LISTA_CRUZ *lista_cruz_criar_campos(const char *campo_linha,
                                   const char *campo_coluna);
void lista_apagar(LISTA_CRUZ **lista);

// Retorna true se inseriu. Assume a posse de item SOMENTE em caso de sucesso.
// Agora aceita VARIOS ITEMs em uma mesma coordenada (ordem de insercao).
bool lista_inserir(LISTA_CRUZ *lista, ITEM *item, int posX, int posY);

// Insercao automatica: consulta os dois campos via item_ler_campo().
// So pode ser usada numa lista criada por lista_cruz_criar_campos().
bool lista_cruz_inserir_item(LISTA_CRUZ *lista, ITEM *item);

// Obtém o PRIMEIRO ITEM na coordenada; sem transferir propriedade.
ITEM *lista_get(const LISTA_CRUZ *lista, int posX, int posY);

// Remove e devolve o PRIMEIRO ITEM na coordenada (a posse vai ao chamador).
// A celula so e retirada dos dois encadeamentos quando fica vazia.
ITEM *lista_remover(LISTA_CRUZ *lista, int posX, int posY);

// Percurso linha a linha, depois coordenada e depois lista interna.
// Cada ITEM e visitado uma unica vez. Retorno: visitas concluidas.
int lista_cruz_percorrer(LISTA_CRUZ *lista, ItemVisita visita, void *contexto);

// Percursos independentes por indice de linha ou coluna.
int lista_cruz_percorrer_linha(LISTA_CRUZ *lista, int linha,
                              ItemVisita visita, void *contexto);
int lista_cruz_percorrer_coluna(LISTA_CRUZ *lista, int coluna,
                               ItemVisita visita, void *contexto);

// Procura um eixo pelo VALOR (texto, numero ou booleano); apenas modo CROSS.
// Retorna 0 quando a chave nao existe. A string da chave pode ser temporaria.
int lista_cruz_percorrer_valor_linha(LISTA_CRUZ *lista, const ValorQuery *valor,
                                    ItemVisita visita, void *contexto);
int lista_cruz_percorrer_valor_coluna(LISTA_CRUZ *lista, const ValorQuery *valor,
                                     ItemVisita visita, void *contexto);

// Imprime linha por linha. Chame assim: lista_imprimir(lista, item_imprimir);
void lista_imprimir(const LISTA_CRUZ *lista, void (*imprimir_item)(ITEM *));

#endif
