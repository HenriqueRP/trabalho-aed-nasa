#include "lista_encadeada.h"
#include <stdlib.h>
#include <stdbool.h>

//lista_encadeada.c - lista simplesmente encadeada com ponteiros de inicio
//e fim, sem sentinela.

//'fim' existe para que adicionar no fim seja O(1): sem ele cada insercao
//percorreria a lista inteira. Inserir no inicio e O(1) por natureza.
//sem sentinela, inicio == NULL identifica lista vazia.

typedef struct no NO;
struct no {
    ITEM *item;
    NO *proximo;
};
//cria um no para armazenar um item.

static NO *no_criar(ITEM *item) {
    NO *novo = (NO *)malloc(sizeof(NO));
    if (novo == NULL) return NULL;
    novo->item = item;
    novo->proximo = NULL;
    return novo;
}
//libera um no sem liberar o item que ele referencia.

static void no_apagar(NO **no) {
    if (no == NULL || *no == NULL) return;
    free(*no);
    *no = NULL;
}

struct lista_encadeada {
    NO *inicio, *fim;
    int tam;
};
//cria uma lista encadeada vazia.

LISTA_ENC *lista_enc_criar(void) {
    LISTA_ENC *nova = (LISTA_ENC *)malloc(sizeof(LISTA_ENC));
    if (nova == NULL) return NULL;

    nova->inicio = NULL;
    nova->fim = NULL;
    nova->tam = 0;
    return nova;
}

//append no fim. O(1) via 'fim'.
//adiciona um item ao final da lista.
bool lista_enc_adicionar_fim(LISTA_ENC *lista, ITEM *item) {
    if (lista == NULL || item == NULL) return false;

    NO *no = no_criar(item);
    if (no == NULL) return false;

    if (lista->fim != NULL) lista->fim->proximo = no;
    lista->fim = no;

    //primeira insercao: inicio tambem precisa apontar para o novo no
    if (lista->inicio == NULL) lista->inicio = no;

    (lista->tam)++;
    return true;
}

//insercao no inicio. O(1).
//adiciona um item ao inicio da lista.
bool lista_enc_adicionar_inicio(LISTA_ENC *lista, ITEM *item) {
    if (lista == NULL || item == NULL) return false;

    NO *no = no_criar(item);
    if (no == NULL) return false;

    no->proximo = lista->inicio;
    lista->inicio = no;

    //lista estava vazia: 'fim' tambem aponta para o unico no
    if (lista->fim == NULL) lista->fim = no;

    (lista->tam)++;
    return true;
}

//remove do inicio e devolve o ITEM (o chamador passa a ser dono).
//remove e devolve o item do inicio da lista.
ITEM *lista_enc_remover_inicio(LISTA_ENC *lista) {
    if (lista == NULL || lista->tam == 0) return NULL;

    NO *removido = lista->inicio;
    ITEM *item = removido->item;

    lista->inicio = removido->proximo;
    no_apagar(&removido);
    (lista->tam)--;

    //esvaziou: 'fim' precisa voltar a NULL para nao apontar no liberado
    if (lista_enc_vazia(lista)) lista->fim = NULL;

    return item;
}
//retorna a quantidade de itens da lista encadeada.

int lista_enc_tam(LISTA_ENC *lista) {
    if (lista == NULL) return -1;
    return lista->tam;
}
//verifica se a lista encadeada esta vazia.

bool lista_enc_vazia(LISTA_ENC *lista) { return lista == NULL || lista->tam == 0; }

//percurso do inicio para o fim. A insercao durante a leitura usa
//adicionar_fim, entao esta e a ordem de insercao do CSV.
//percorre os itens da lista encadeada.
int lista_enc_percorrer(LISTA_ENC *lista, ItemVisita visita, void *contexto) {
    if (lista == NULL || visita == NULL) return 0;

    int n = 0;
    for (NO *no = lista->inicio; no != NULL; no = no->proximo) {
        if (!visita(no->item, contexto)) break;
        n++;
    }
    return n;
}
//libera a lista encadeada e seus nos.

void lista_enc_apagar(LISTA_ENC **lista) {
    if (lista == NULL || *lista == NULL) return;

    while (!lista_enc_vazia(*lista)) {
        ITEM *removido = lista_enc_remover_inicio(*lista);
        item_apagar(&removido);
    }

    free(*lista);
    *lista = NULL;
}
