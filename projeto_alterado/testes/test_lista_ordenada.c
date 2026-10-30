#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lista_ordenada.h"

// Este registro de teste nao depende da NASA nem da ANCINE
typedef struct {
    double numero;
    char texto[32];
    bool booleano;
    bool presente;
} DADO;

static int criados;
static int apagados;

static void apagar_dado(void **dado) {
    assert(dado != NULL && *dado != NULL);
    free(*dado);
    *dado = NULL;
    apagados++;
}

static void *copiar_dado(const void *origem) {
    DADO *copia = malloc(sizeof(*copia));
    if (copia != NULL) {
        *copia = *(const DADO *)origem;
        criados++;
    }
    return copia;
}

static bool ler_campo(const void *registro, const char *campo, ValorQuery **saida) {
    const DADO *dado = registro;
    *saida = NULL;
    if (!dado->presente) return false;
    if (strcmp(campo, "numero") == 0) *saida = query_valor_numero(dado->numero);
    if (strcmp(campo, "texto") == 0) *saida = query_valor_texto(dado->texto);
    if (strcmp(campo, "booleano") == 0) *saida = query_valor_booleano(dado->booleano);
    if (strcmp(campo, "misto") == 0) {
        *saida = dado->booleano ? query_valor_texto(dado->texto) :
                                 query_valor_numero(dado->numero);
    }
    return *saida != NULL;
}

static ITEM *criar_item(int chave, double numero, const char *texto,
                        bool booleano, bool presente) {
    DADO *dado = malloc(sizeof(*dado));
    assert(dado != NULL);
    dado->numero = numero;
    snprintf(dado->texto, sizeof(dado->texto), "%s", texto);
    dado->booleano = booleano;
    dado->presente = presente;
    ITEM *item = item_criar(dado, apagar_dado, copiar_dado, ler_campo, chave);
    assert(item != NULL);
    criados++;
    return item;
}

typedef struct {
    ITEM *itens[16];
    int chamadas;
    int limite;
} VISITAS;

static bool visitar(ITEM *item, void *contexto) {
    VISITAS *v = contexto;
    assert(item != NULL && v->chamadas < 16);
    v->itens[v->chamadas++] = item;
    return v->limite == 0 || v->chamadas < v->limite;
}

static void verificar(LISTA_ORD *lista, const int *chaves, int n) {
    VISITAS visitas = {0};
    assert(lista_ord_tamanho(lista) == n);
    assert(lista_ord_percorrer(lista, visitar, &visitas) == n);
    assert(visitas.chamadas == n);
    for (int i = 0; i < n; i++) assert(item_get_chave(visitas.itens[i]) == chaves[i]);
}

static void testar_numeros(bool desc) {
    char campo[] = "numero";
    LISTA_ORD *lista = lista_ord_criar(campo);
    assert(lista != NULL);
    campo[0] = 'X';

    assert(lista_ord_inserir(lista, criar_item(7, 70, "", false, true), desc));
    verificar(lista, (int[]){7}, 1);
    assert(lista_ord_inserir(lista, criar_item(2, 20, "", false, true), desc));
    verificar(lista, desc ? (int[]){7, 2} : (int[]){2, 7}, 2);
    assert(lista_ord_inserir(lista, criar_item(5, 50, "", false, true), desc));
    verificar(lista, desc ? (int[]){7, 5, 2} : (int[]){2, 5, 7}, 3);
    assert(lista_ord_inserir(lista, criar_item(3, 20, "", false, true), desc));
    verificar(lista, desc ? (int[]){7, 5, 2, 3} : (int[]){2, 3, 5, 7}, 4);

    // Os ausentes permanecem no fim, inclusive no modo decrescente
    assert(lista_ord_inserir(lista, criar_item(9, 0, "", false, false), desc));
    assert(lista_ord_inserir(lista, criar_item(8, NAN, "", false, true), desc));
    assert(lista_ord_inserir(lista, criar_item(10, INFINITY, "", false, true), desc));
    verificar(lista, desc ? (int[]){7, 5, 2, 3, 8, 9, 10} :
                           (int[]){2, 3, 5, 7, 8, 9, 10}, 7);

    ITEM *recusado = criar_item(0, 0, "", false, true);
    int antes = apagados;
    assert(!lista_ord_inserir(lista, recusado, !desc));
    assert(apagados == antes && item_get_chave(recusado) == 0);
    assert(lista_ord_tamanho(lista) == 7);
    item_apagar(&recusado);

    VISITAS corte = {.limite = 2};
    assert(lista_ord_percorrer(lista, visitar, &corte) == 1);
    assert(corte.chamadas == 2);
    assert(lista_ord_tamanho(lista) == 7);

    // Remover devolve o mesmo ITEM observado sem destruir o registro
    ITEM *primeiro = lista_ord_primeiro(lista);
    antes = apagados;
    assert(lista_ord_remover_inicio(lista) == primeiro);
    assert(apagados == antes);
    item_apagar(&primeiro);
    lista_ord_apagar(&lista);
    assert(lista == NULL);
}

static void testar_textos_e_booleanos(void) {
    for (int desc = 0; desc <= 1; desc++) {
        LISTA_ORD *lista = lista_ord_criar("texto");
        assert(lista_ord_inserir(lista, criar_item(3, 0, "z", false, true), desc));
        assert(lista_ord_inserir(lista, criar_item(1, 0, "a", false, true), desc));
        assert(lista_ord_inserir(lista, criar_item(2, 0, "m", false, true), desc));
        assert(lista_ord_inserir(lista, criar_item(4, 0, "", false, true), desc));
        verificar(lista, desc ? (int[]){3, 2, 1, 4} : (int[]){4, 1, 2, 3}, 4);
        lista_ord_apagar(&lista);

        lista = lista_ord_criar("booleano");
        assert(lista_ord_inserir(lista, criar_item(3, 0, "", true, true), desc));
        assert(lista_ord_inserir(lista, criar_item(1, 0, "", false, true), desc));
        assert(lista_ord_inserir(lista, criar_item(2, 0, "", true, true), desc));
        verificar(lista, desc ? (int[]){2, 3, 1} : (int[]){1, 2, 3}, 3);
        lista_ord_apagar(&lista);
    }

    // A chave textual guardada na lista e uma copia independente
    LISTA_ORD *lista = lista_ord_criar("texto");
    ITEM *a = criar_item(1, 0, "a", false, true);
    assert(lista_ord_inserir(lista, a, false));
    snprintf(((DADO *)item_get_dado(a))->texto, 32, "%s", "z");
    assert(lista_ord_inserir(lista, criar_item(2, 0, "b", false, true), false));
    verificar(lista, (int[]){1, 2}, 2);
    lista_ord_apagar(&lista);
}

static void testar_empates_e_tipos(void) {
    LISTA_ORD *lista = lista_ord_criar("numero");
    ITEM *a = criar_item(INT_MAX, 1, "", false, true);
    ITEM *b = criar_item(INT_MIN, 1, "", false, true);
    ITEM *c = criar_item(INT_MAX, 1, "", false, true);
    assert(lista_ord_inserir(lista, a, true));
    assert(lista_ord_inserir(lista, b, true));
    assert(lista_ord_inserir(lista, c, true));
    VISITAS visitas = {0};
    assert(lista_ord_percorrer(lista, visitar, &visitas) == 3);
    assert(visitas.itens[0] == b && visitas.itens[1] == a && visitas.itens[2] == c);
    lista_ord_apagar(&lista);

    lista = lista_ord_criar("misto");
    assert(lista_ord_inserir(lista, criar_item(1, 10, "", false, true), false));
    ITEM *texto = criar_item(2, 0, "texto", true, true);
    assert(!lista_ord_inserir(lista, texto, false));
    assert(lista_ord_tamanho(lista) == 1);
    ITEM *numero = lista_ord_remover_inicio(lista);
    item_apagar(&numero);
    assert(lista_ord_vazia(lista));
    assert(lista_ord_inserir(lista, texto, true));
    lista_ord_apagar(&lista);

    lista = lista_ord_criar("campo_ausente");
    assert(lista_ord_inserir(lista, criar_item(2, 0, "", false, true), true));
    assert(lista_ord_inserir(lista, criar_item(1, 0, "", false, true), true));
    verificar(lista, (int[]){1, 2}, 2);
    lista_ord_apagar(&lista);
}

static void testar_similaridade(void) {
    ITEM *base = criar_item(3, 42, "original", false, true);
    ValorQuery *valor = NULL;
    assert(!item_ler_campo(base, "similarity", &valor));
    assert(valor == NULL);
    assert(item_definir_similaridade(base, 0.8));
    assert(!item_definir_similaridade(base, -0.1));
    assert(!item_definir_similaridade(base, 1.1));
    assert(!item_definir_similaridade(base, NAN));
    assert(!item_definir_similaridade(base, INFINITY));
    assert(!item_definir_similaridade(NULL, 0.5));

    ITEM *copia = item_copiar(base);
    assert(copia != NULL && item_get_dado(copia) != item_get_dado(base));
    item_apagar(&base);
    assert(item_ler_campo(copia, "similarity", &valor));
    assert(query_valor_get_numero(valor) == 0.8);
    query_valor_apagar(&valor);
    assert(((DADO *)item_get_dado(copia))->numero == 42);

    LISTA_ORD *lista = lista_ord_criar("similarity");
    assert(lista_ord_inserir(lista, copia, true));
    ITEM *melhor = criar_item(1, 0, "", false, true);
    ITEM *zero = criar_item(2, 0, "", false, true);
    assert(item_definir_similaridade(melhor, 1.0));
    assert(item_definir_similaridade(zero, 0.0));
    assert(lista_ord_inserir(lista, melhor, true));
    assert(lista_ord_inserir(lista, zero, true));
    assert(lista_ord_inserir(lista, criar_item(0, 0, "", false, true), true));
    verificar(lista, (int[]){1, 3, 2, 0}, 4);
    lista_ord_apagar(&lista);
}

int main(void) {
    assert(lista_ord_criar(NULL) == NULL);
    assert(lista_ord_criar("") == NULL);
    LISTA_ORD *lista = lista_ord_criar("numero");
    assert(lista != NULL && lista_ord_vazia(lista));
    verificar(lista, NULL, 0);
    assert(lista_ord_primeiro(lista) == NULL);
    assert(lista_ord_remover_inicio(lista) == NULL);
    assert(lista_ord_tamanho(NULL) == -1);
    assert(lista_ord_vazia(NULL));
    assert(lista_ord_primeiro(NULL) == NULL);
    assert(lista_ord_remover_inicio(NULL) == NULL);
    assert(!lista_ord_inserir(lista, NULL, false));
    assert(lista_ord_percorrer(lista, NULL, NULL) == 0);
    assert(lista_ord_percorrer(NULL, visitar, NULL) == 0);
    ITEM *item = criar_item(1, 1, "", false, true);
    assert(!lista_ord_inserir(NULL, item, false));
    assert(!item_ler_campo(item, NULL, NULL));
    item_apagar(&item);
    lista_ord_apagar(&lista);
    lista_ord_apagar(&lista);
    lista_ord_apagar(NULL);

    testar_numeros(false);
    testar_numeros(true);
    testar_textos_e_booleanos();
    testar_empates_e_tipos();
    testar_similaridade();
    assert(criados == apagados);
    puts("lista_ordenada: testes passaram");
    return 0;
}
