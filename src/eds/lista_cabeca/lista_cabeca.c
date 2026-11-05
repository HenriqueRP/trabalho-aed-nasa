#include "lista_cabeca.h"

#include <limits.h>
#include <stdlib.h>

typedef struct no_cabeca {
    ITEM *item;
    struct no_cabeca *proximo;
} NO_CAB;

struct lista_cabeca {
    NO_CAB *cabeca;
    NO_CAB *fim;
    int tamanho;
};

LISTA_CAB *lista_cabeca_criar(void) {
    LISTA_CAB *lista = malloc(sizeof(*lista));
    if (lista == NULL) return NULL;

    // Este no existe mesmo na lista vazia e nunca representa um registro
    // calloc deixa item e proximo como NULL para marcar essa condicao
    lista->cabeca = calloc(1, sizeof(*lista->cabeca));
    if (lista->cabeca == NULL) {
        free(lista);
        return NULL;
    }

    // Na lista vazia, fim aponta para a propria sentinela
    // Assim toda insercao no fim liga o novo no depois de um no existente
    lista->fim = lista->cabeca;
    lista->tamanho = 0;
    return lista;
}

static NO_CAB *no_criar(ITEM *item) {
    NO_CAB *no = malloc(sizeof(*no));
    if (no == NULL) return NULL;
    no->item = item;
    no->proximo = NULL;
    return no;
}

bool lista_cabeca_adicionar(LISTA_CAB *lista, ITEM *item) {
    if (lista == NULL || item == NULL || lista->tamanho == INT_MAX) return false;

    // Nenhum ponteiro da lista muda antes de a alocacao ter sucesso
    // Em falha, o chamador continua dono do ITEM e pode libera-lo
    NO_CAB *novo = no_criar(item);
    if (novo == NULL) return false;

    lista->fim->proximo = novo;
    lista->fim = novo;
    lista->tamanho++;
    return true;
}

bool lista_cabeca_adicionar_inicio(LISTA_CAB *lista, ITEM *item) {
    if (lista == NULL || item == NULL || lista->tamanho == INT_MAX) return false;

    NO_CAB *novo = no_criar(item);
    if (novo == NULL) return false;

    // O primeiro registro e sempre o sucessor da sentinela
    novo->proximo = lista->cabeca->proximo;
    lista->cabeca->proximo = novo;
    if (lista->tamanho == 0) lista->fim = novo;
    lista->tamanho++;
    return true;
}

ITEM *lista_cabeca_primeiro(const LISTA_CAB *lista) {
    if (lista_cabeca_vazia(lista)) return NULL;
    return lista->cabeca->proximo->item;
}

ITEM *lista_cabeca_remover_inicio(LISTA_CAB *lista) {
    if (lista_cabeca_vazia(lista)) return NULL;

    NO_CAB *removido = lista->cabeca->proximo;
    ITEM *item = removido->item;
    lista->cabeca->proximo = removido->proximo;
    lista->tamanho--;

    // Restaurar fim para a sentinela permite inserir novamente apos esvaziar
    if (lista->tamanho == 0) lista->fim = lista->cabeca;
    free(removido);

    // Somente o no foi liberado, pois o registro passa a ser do chamador
    return item;
}

int lista_cabeca_tamanho(const LISTA_CAB *lista) {
    return lista == NULL ? -1 : lista->tamanho;
}

bool lista_cabeca_vazia(const LISTA_CAB *lista) {
    return lista == NULL || lista->tamanho == 0;
}

int lista_cabeca_percorrer(LISTA_CAB *lista, ItemVisita visita, void *contexto) {
    if (lista == NULL || visita == NULL) return 0;

    int visitados = 0;
    // Comecar depois da cabeca garante que a sentinela nunca chega ao callback
    for (NO_CAB *no = lista->cabeca->proximo; no != NULL; no = no->proximo) {
        if (!visita(no->item, contexto)) break;
        visitados++;
    }
    return visitados;
}

void lista_cabeca_apagar(LISTA_CAB **lista) {
    if (lista == NULL || *lista == NULL) return;

    // A remocao custa O(1), portanto liberar todos os registros custa O(n)
    while (!lista_cabeca_vazia(*lista)) {
        ITEM *item = lista_cabeca_remover_inicio(*lista);
        item_apagar(&item);
    }
    free((*lista)->cabeca);
    free(*lista);
    *lista = NULL;
}
