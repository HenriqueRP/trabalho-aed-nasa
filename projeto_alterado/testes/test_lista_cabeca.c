#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lista_cabeca.h"

static int criados;
static int apagados;

static void apagar_dado(void **dado) {
    assert(dado != NULL && *dado != NULL);
    free(*dado);
    *dado = NULL;
    apagados++;
}

static ITEM *criar_item(int chave) {
    int *dado = malloc(sizeof(*dado));
    assert(dado != NULL);
    *dado = chave;
    ITEM *item = item_criar(dado, apagar_dado, NULL, NULL, chave);
    assert(item != NULL);
    criados++;
    return item;
}

typedef struct {
    int chaves[8];
    int chamadas;
    int limite;
} VISITAS;

static bool visitar(ITEM *item, void *contexto) {
    VISITAS *v = contexto;
    // Um ITEM NULL indicaria que o percurso incluiu a sentinela
    assert(item != NULL);
    assert(v->chamadas < 8);
    v->chaves[v->chamadas++] = item_get_chave(item);
    return v->limite == 0 || v->chamadas < v->limite;
}

int main(void) {
    LISTA_CAB *lista = lista_cabeca_criar();
    assert(lista != NULL);
    assert(lista_cabeca_vazia(lista));
    assert(lista_cabeca_tamanho(lista) == 0);
    assert(lista_cabeca_primeiro(lista) == NULL);
    assert(lista_cabeca_remover_inicio(lista) == NULL);

    VISITAS vazio = {0};
    assert(lista_cabeca_percorrer(lista, visitar, &vazio) == 0);
    assert(vazio.chamadas == 0);

    ITEM *a = criar_item(2);
    ITEM *b = criar_item(3);
    ITEM *c = criar_item(1);
    assert(lista_cabeca_adicionar(lista, a));
    assert(lista_cabeca_adicionar(lista, b));
    assert(lista_cabeca_adicionar_inicio(lista, c));
    assert(lista_cabeca_tamanho(lista) == 3);
    assert(lista_cabeca_primeiro(lista) == c);

    VISITAS todos = {0};
    assert(lista_cabeca_percorrer(lista, visitar, &todos) == 3);
    assert(todos.chamadas == 3);
    for (int i = 0; i < 3; i++) assert(todos.chaves[i] == i + 1);

    VISITAS corte = {.limite = 2};
    assert(lista_cabeca_percorrer(lista, visitar, &corte) == 1);
    assert(corte.chamadas == 2);
    assert(lista_cabeca_tamanho(lista) == 3);

    // A remocao devolve itens ainda vivos e nao chama o destrutor do dado
    for (int i = 1; i <= 3; i++) {
        ITEM *item = lista_cabeca_remover_inicio(lista);
        assert(item != NULL && item_get_chave(item) == i);
        assert(*(int *)item_get_dado(item) == i);
        assert(apagados == i - 1);
        item_apagar(&item);
    }
    assert(lista_cabeca_vazia(lista));

    // Exercita fim depois de remover o ultimo no e apos inserir no inicio
    assert(lista_cabeca_adicionar_inicio(lista, criar_item(4)));
    assert(lista_cabeca_adicionar(lista, criar_item(5)));
    assert(lista_cabeca_tamanho(lista) == 2);

    ITEM *recusado = criar_item(6);
    assert(!lista_cabeca_adicionar(NULL, recusado));
    assert(!lista_cabeca_adicionar_inicio(NULL, recusado));
    assert(item_get_chave(recusado) == 6);
    item_apagar(&recusado);
    assert(!lista_cabeca_adicionar(lista, NULL));
    assert(!lista_cabeca_adicionar_inicio(lista, NULL));
    assert(lista_cabeca_tamanho(NULL) == -1);
    assert(lista_cabeca_vazia(NULL));
    assert(lista_cabeca_primeiro(NULL) == NULL);
    assert(lista_cabeca_remover_inicio(NULL) == NULL);
    assert(lista_cabeca_percorrer(NULL, visitar, &todos) == 0);
    assert(lista_cabeca_percorrer(lista, NULL, NULL) == 0);

    lista_cabeca_apagar(&lista);
    assert(lista == NULL);
    lista_cabeca_apagar(&lista);
    lista_cabeca_apagar(NULL);
    assert(criados == apagados);
    puts("lista_cabeca: testes passaram");
    return 0;
}
