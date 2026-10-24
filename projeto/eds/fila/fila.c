#include "fila.h"
#include "item.h"
#include <stdlib.h>
#include <stdbool.h>

// fila.c - fila simplesmente encadeada com no sentinela na cabeca.
//
// O sentinela (head) nao guarda registro: existe so para que enfileirar e
// desenfileirar nunca precisem tratar a fila vazia como caso especial.
// 'tail' aponta para o ultimo no real, o que deixa o enqueue em O(1).

typedef struct no NO;
struct no {
    ITEM *item;
    NO *prox;
};

// Criacao e destruicao de no ficam static: outro arquivo nao deve depender
// do formato interno do no.
static NO *no_criar(ITEM *item) {
    NO *novo = (NO*)malloc(sizeof(NO));
    if (novo == NULL) return NULL;
    novo->item = item;
    novo->prox = NULL;
    return novo;
}

// Recebe NO** para tambem zerar o ponteiro do chamador, evitando dangling.
static void no_apagar(NO **no) {
    if (no == NULL || *no == NULL) return;
    free(*no);
    *no = NULL;
}

struct fila {
    NO *head;   // sentinela: head->prox e o primeiro item real
    NO *tail;   // ultimo no real; NULL quando a fila esta vazia
    int tam;
};

FILA *fila_criar(void) {
    FILA *nova = (FILA *)malloc(sizeof(FILA));
    if (nova == NULL) return NULL;

    // O sentinela recebe item NULL de proposito: nunca sera visitado.
    nova->head = no_criar(NULL);
    if (nova->head == NULL) {
        free(nova);   // sem o sentinela a struct nao serve para nada
        return NULL;
    }
    nova->tail = NULL;
    nova->tam = 0;
    return nova;
}

// Enfileira no fim. O(1) porque usa 'tail' em vez de percorrer.
bool fila_enqueue(FILA *fila, ITEM *item) {
    if (fila == NULL || item == NULL) return false;

    NO *novo = no_criar(item);
    if (novo == NULL) return false;

    // fila vazia: head->prox nao aponta para ninguem, entao o novo se liga
    // direto ao sentinela. Com fila cheia, encaixa depois do ultimo.
    if (fila->tail != NULL) fila->tail->prox = novo;
    else                    fila->head->prox = novo;

    fila->tail = novo;
    (fila->tam)++;
    return true;
}

// Desenfileira do inicio. Devolve o ITEM, que passa a ser de quem chamou:
// liberar com item_apagar() e responsabilidade do chamador.
ITEM *fila_dequeue(FILA *fila) {
    if (fila == NULL || fila_vazia(fila)) return NULL;

    NO *removido = fila->head->prox;
    fila->head->prox = removido->prox;

    ITEM *item = removido->item;
    no_apagar(&removido);   // libera o no; o item sai pelo retorno
    (fila->tam)--;

    // Fila ficou vazia: tail tem que voltar a NULL, senao o proximo enqueue
    // encaixaria depois de um no ja liberado.
    if (fila->tam == 0) fila->tail = NULL;

    return item;
}

int fila_tamanho(FILA *fila) { return fila == NULL ? -1 : fila->tam; }

// Espia o proximo a sair, sem remover. Nao transfere posse.
ITEM *fila_frente(FILA *fila) {
    if (fila == NULL || fila_vazia(fila)) return NULL;
    return fila->head->prox->item;
}

bool fila_vazia(FILA *fila) { return fila == NULL || fila->tam == 0; }

// Percurso da frente para o fim - a mesma ordem de acesso do dequeue.
// 'visita' devolve false para interromper, o que permite ao writer cortar
// no top-k sem visitar o resto da fila.
int fila_percorrer(FILA *fila, ItemVisita visita, void *contexto) {
    if (fila == NULL || visita == NULL) return 0;

    int n = 0;
    for (NO *no = fila->head->prox; no != NULL; no = no->prox) {
        if (!visita(no->item, contexto)) break;
        n++;
    }
    return n;
}

// Esvazia liberando itens e nos. O sentinela sai por ultimo, porque o laco
// precisa dele para saber onde comeca a fila.
void apagar_fila(FILA **fila) {
    if (fila == NULL || *fila == NULL) return;

    while (!fila_vazia(*fila)) {
        ITEM *removido = fila_dequeue(*fila);
        item_apagar(&removido);
    }

    no_apagar(&(*fila)->head);
    free(*fila);
    *fila = NULL;
}