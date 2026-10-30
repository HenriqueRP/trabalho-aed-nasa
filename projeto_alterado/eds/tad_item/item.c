#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "item.h"

//item.c - envelope generico.
//nao inclui item_nasa.h de proposito: esta camada precisa ser cega ao dataset.

struct item {
    void *dado;                  //registro real, opaco aqui
    ItemApagarFn apagar;         //como liberar 'dado'
    ItemCopiarFn copiar;         //como duplicar 'dado'
    LeitorCampoQuery ler_campo;  //como consultar um campo de 'dado'
    int chave;                   //ordem de insercao (desempate estavel)
    double similaridade;
    bool tem_similaridade;        // Distingue pontuacao zero de pontuacao ausente
};
//cria um envelope generico para o dado e suas funcoes de manipulacao.

ITEM *item_criar(void *dado, ItemApagarFn apagar, ItemCopiarFn copiar,
                 LeitorCampoQuery ler_campo, int chave) {
    if (dado == NULL) return NULL;

    ITEM *novo = malloc(sizeof(ITEM));
    if (novo == NULL) return NULL;

    novo->dado      = dado;
    novo->apagar    = apagar;
    novo->copiar    = copiar;
    novo->ler_campo = ler_campo;
    novo->chave     = chave;
    novo->similaridade = 0.0;
    novo->tem_similaridade = false;
    return novo;
}
//cria uma copia profunda do item usando a funcao de copia fornecida.

ITEM *item_copiar(const ITEM *item) {
    //sem funcao de copia nao ha como duplicar: falha explicita.
    if (item == NULL || item->copiar == NULL) return NULL;

    void *copia = item->copiar(item->dado);
    if (copia == NULL) return NULL;

    //se o envelope falhar, a copia ja existe e ninguem mais a possui:
    //libera aqui para nao vazar.
    ITEM *novo = item_criar(copia, item->apagar, item->copiar,
                            item->ler_campo, item->chave);
    if (novo == NULL && item->apagar != NULL) item->apagar(&copia);

    // A copia preserva a pontuacao sem compartilhar memoria adicional
    if (novo != NULL) {
        novo->similaridade = item->similaridade;
        novo->tem_similaridade = item->tem_similaridade;
    }

    return novo; //'chave' vai junto: preserva a ordem original do CSV
}
//libera o dado armazenado e o envelope do item.

void item_apagar(ITEM **item) {
    if (item == NULL || *item == NULL) return;

    ITEM *it = *item;
    if (it->apagar != NULL) it->apagar(&it->dado); //libera o registro
    free(it);                                      //libera o envelope
    *item = NULL;
}
//retorna o dado armazenado sem transferir sua posse.

void *item_get_dado(const ITEM *item) {
    if (item == NULL) return NULL;
    return item->dado;
}
//retorna a chave de insercao usada para desempate.
int item_get_chave(const ITEM *item) {
    if (item == NULL) return -1;
    return item->chave;
}
// Define a pontuacao da consulta sem alterar o registro do dataset

bool item_definir_similaridade(ITEM *item, double similaridade) {
    if (item == NULL || !isfinite(similaridade) ||
        similaridade < 0.0 || similaridade > 1.0) return false;
    item->similaridade = similaridade;
    item->tem_similaridade = true;
    return true;
}

// Consulta o metadado reservado ou delega a leitura ao dono do dado
bool item_ler_campo(const void *item, const char *campo, ValorQuery **saida) {
    const ITEM *it = item;
    if (saida != NULL) *saida = NULL;
    if (it == NULL || campo == NULL || saida == NULL) return false;

    // A pontuacao fica no envelope para funcionar com qualquer dataset
    // Sem pontuacao, a lista ordenada trata o campo como ausente
    if (strcmp(campo, "similarity") == 0) {
        if (!it->tem_similaridade) return false;
        *saida = query_valor_numero(it->similaridade);
        return *saida != NULL;
    }
    if (it->ler_campo == NULL) return false;
    return it->ler_campo(it->dado, campo, saida); //delega ao dono do dado
}
