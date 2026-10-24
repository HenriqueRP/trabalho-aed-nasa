#include "pilha.h"
#include "item.h"
#include <stdlib.h>
#include <stdbool.h>

// pilha.c - pilha encadeada com sentinela na cabeca.
//
// O topo e head->ant, entao push e pop so mexem nesse ponteiro: ambos O(1).
// A cadeia cresce no campo 'ant' (para tras), e e nesse mesmo sentido que
// corre o percurso natural da pilha: do topo para a base.

typedef struct no NO;
struct no {
    ITEM *item;
    NO *ant;
};

static NO *no_criar(ITEM *item) {
    NO *novo = (NO*)malloc(sizeof(NO));
    if (novo == NULL) return NULL;
    novo->item = item;
    novo->ant = NULL;
    return novo;
}

static void no_apagar(NO **no) {
    if (no == NULL || *no == NULL) return;
    free(*no);
    *no = NULL;
}

struct pilha {
    int tam;
    NO *head;   // sentinela: head->ant e o topo
};

PILHA *criar_pilha(void) {
    PILHA *p = (PILHA *)malloc(sizeof(PILHA));
    if (p == NULL) return NULL;

    p->tam = 0;
    p->head = no_criar(NULL);
    if (p->head == NULL) {
        free(p);   // libera a struct antes de desistir
        return NULL;
    }
    return p;
}

// Empilha no topo. O(1).
bool pilha_push(PILHA *pilha, ITEM *item) {
    if (pilha == NULL || item == NULL) return false;

    NO *novo = no_criar(item);
    if (novo == NULL) return false;

    novo->ant = pilha->head->ant;   // novo aponta para o antigo topo
    pilha->head->ant = novo;        // e passa a ser o topo
    (pilha->tam)++;
    return true;
}

// Desempilha e devolve o ITEM. O chamador passa a ser dono dele e deve
// liberar com item_apagar().
ITEM *pilha_pop(PILHA *pilha) {
    if (pilha == NULL || pilha_empty(pilha)) return NULL;

    NO *x = pilha->head->ant;
    ITEM *removido = x->item;

    pilha->head->ant = x->ant;   // desliga o no do topo
    no_apagar(&x);               // libera so o no; o item sai pelo retorno
    (pilha->tam)--;
    return removido;
}

bool pilha_empty(PILHA *pilha) { return pilha == NULL || pilha->tam == 0; }

int pilha_tamanho(PILHA *pilha) { return pilha == NULL ? -1 : pilha->tam; }

// Espia o topo sem remover. Nao transfere posse.
ITEM *pilha_top(PILHA *pilha) {
    if (pilha == NULL || pilha_empty(pilha)) return NULL;
    return pilha->head->ant->item;
}

// Percurso do topo para a base - a ordem em que os itens sairiam por pop.
// E isso que faz --result pilha --top-k N devolver os N ultimos inseridos.
int pilha_percorrer(PILHA *pilha, ItemVisita visita, void *contexto) {
    if (pilha == NULL || visita == NULL) return 0;

    int n = 0;
    for (NO *no = pilha->head->ant; no != NULL; no = no->ant) {
        if (!visita(no->item, contexto)) break;
        n++;
    }
    return n;
}

// pop ja libera o no e devolve o item; aqui so falta apagar o item.
void apagar_pilha(PILHA **pilha) {
    if (pilha == NULL || *pilha == NULL) return;

    ITEM *item;
    while (!pilha_empty(*pilha)) {
        item = pilha_pop(*pilha);
        item_apagar(&item);
    }

    no_apagar(&(*pilha)->head);
    free(*pilha);
    *pilha = NULL;
}