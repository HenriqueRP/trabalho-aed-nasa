#include <stdlib.h>
#include "item.h"

// item.c - envelope generico.
// Nao inclui item_nasa.h de proposito: esta camada precisa ser cega ao dataset.

struct item {
    void *dado;                  // registro real, opaco aqui
    ItemApagarFn apagar;         // como liberar 'dado'
    ItemCopiarFn copiar;         // como duplicar 'dado'
    LeitorCampoQuery ler_campo;  // como consultar um campo de 'dado'
    int chave;                   // ordem de insercao (desempate estavel)
};

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

ITEM *item_copiar(const ITEM *item) {
    // Sem funcao de copia nao ha como duplicar: falha explicita.
    if (item == NULL || item->copiar == NULL) return NULL;

    void *copia = item->copiar(item->dado);
    if (copia == NULL) return NULL;

    // Se o envelope falhar, a copia ja existe e ninguem mais a possui:
    // libera aqui para nao vazar.
    ITEM *novo = item_criar(copia, item->apagar, item->copiar,
                            item->ler_campo, item->chave);
    if (novo == NULL && item->apagar != NULL) item->apagar(&copia);

    return novo; // 'chave' vai junto: preserva a ordem original do CSV
}

void item_apagar(ITEM **item) {
    if (item == NULL || *item == NULL) return;

    ITEM *it = *item;
    if (it->apagar != NULL) it->apagar(&it->dado); // libera o registro
    free(it);                                      // libera o envelope
    *item = NULL;
}

void *item_get_dado(const ITEM *item) { return item ? item->dado : NULL; }
int   item_get_chave(const ITEM *item) { return item ? item->chave : -1; }

bool item_ler_campo(const void *item, const char *campo, ValorQuery *saida) {
    const ITEM *it = item;
    if (it == NULL || it->ler_campo == NULL) return false;
    return it->ler_campo(it->dado, campo, saida); // delega ao dono do dado
}