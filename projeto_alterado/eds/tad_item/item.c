#include <stdlib.h>
#include "item.h"

//item.c - envelope generico.
//nao inclui item_nasa.h de proposito: esta camada precisa ser cega ao dataset.

struct item {
    void *dado;                  //registro real, opaco aqui
    ItemApagarFn apagar;         //como liberar 'dado'
    ItemCopiarFn copiar;         //como duplicar 'dado'
    LeitorCampoQuery ler_campo;  //como consultar um campo de 'dado'
    int chave;                   //ordem de insercao (desempate estavel)
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
//consulta um campo do dado por meio da funcao fornecida.

bool item_ler_campo(const void *item, const char *campo, ValorQuery **saida) {
    const ITEM *it = item;
    if (it == NULL || it->ler_campo == NULL) return false;
    return it->ler_campo(it->dado, campo, saida); //delega ao dono do dado
}
