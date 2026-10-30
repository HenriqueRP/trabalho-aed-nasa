#include "lista_ordenada.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct no_ordenado {
    ITEM *item;
    ValorQuery *valor;
    struct no_ordenado *proximo;
} NO_ORD;

struct lista_ordenada {
    NO_ORD *inicio;
    char *campo;
    int tamanho;
    bool decrescente;
    bool tem_tipo;
    TipoValorQuery tipo;
};

LISTA_ORD *lista_ord_criar(const char *campo) {
    if (campo == NULL || *campo == '\0') return NULL;

    LISTA_ORD *lista = calloc(1, sizeof(*lista));
    if (lista == NULL) return NULL;

    // A Query pode ser apagada depois da criacao sem invalidar este nome
    size_t tamanho = strlen(campo) + 1;
    lista->campo = malloc(tamanho);
    if (lista->campo == NULL) {
        free(lista);
        return NULL;
    }
    memcpy(lista->campo, campo, tamanho);
    return lista;
}

static bool copiar_campo(const LISTA_ORD *lista, ITEM *item, ValorQuery **copia) {
    ValorQuery *valor = NULL;
    *copia = NULL;

    // Pela API atual, false significa campo desconhecido ou ausente
    // Manter esse registro no fim evita perder linhas ao carregar a BASE
    if (!item_ler_campo(item, lista->campo, &valor) || valor == NULL) {
        query_valor_apagar(&valor);
        return true;
    }

    TipoValorQuery tipo = query_valor_tipo(valor);
    if (tipo == QUERY_NUMERO && !isfinite(query_valor_get_numero(valor))) {
        query_valor_apagar(&valor);
        return true;
    }
    if (tipo == QUERY_TEXTO && query_valor_get_texto(valor) == NULL) {
        query_valor_apagar(&valor);
        return true;
    }

    if ((tipo != QUERY_TEXTO && tipo != QUERY_NUMERO && tipo != QUERY_BOOLEANO) ||
        (lista->tem_tipo && tipo != lista->tipo)) {
        query_valor_apagar(&valor);
        return false;
    }

    // O valor retornado pelo leitor pode emprestar texto do registro
    // A copia profunda fixa a chave e evita reler campos durante a busca
    *copia = query_valor_copiar(valor);
    query_valor_apagar(&valor);
    return *copia != NULL;
}

static int comparar_nos(const NO_ORD *a, const NO_ORD *b, bool decrescente) {
    // A regra dos ausentes nao e invertida por DESC, pois eles ficam no fim
    if (a->valor == NULL && b->valor != NULL) return 1;
    if (a->valor != NULL && b->valor == NULL) return -1;

    int ordem = 0;
    if (a->valor != NULL) {
        switch (query_valor_tipo(a->valor)) {
            case QUERY_TEXTO: {
                int resultado = strcmp(query_valor_get_texto(a->valor),
                                       query_valor_get_texto(b->valor));
                // Normalizar antes de inverter evita negar um possivel INT_MIN
                ordem = (resultado > 0) - (resultado < 0);
                break;
            }
            case QUERY_NUMERO: {
                double x = query_valor_get_numero(a->valor);
                double y = query_valor_get_numero(b->valor);
                ordem = (x > y) - (x < y);
                break;
            }
            case QUERY_BOOLEANO: {
                bool x = query_valor_get_booleano(a->valor);
                bool y = query_valor_get_booleano(b->valor);
                ordem = (x > y) - (x < y);
                break;
            }
        }
    }

    if (ordem != 0) return decrescente ? -ordem : ordem;

    // O desempate sempre segue a ordem original, inclusive no modo DESC
    // Comparar sem subtrair tambem evita overflow com chaves extremas
    int x = item_get_chave(a->item);
    int y = item_get_chave(b->item);
    return (x > y) - (x < y);
}

bool lista_ord_inserir(LISTA_ORD *lista, ITEM *item, bool decrescente) {
    if (lista == NULL || item == NULL || lista->tamanho == INT_MAX) return false;
    if (lista->tamanho > 0 && lista->decrescente != decrescente) return false;

    ValorQuery *valor = NULL;
    if (!copiar_campo(lista, item, &valor)) return false;

    NO_ORD *novo = malloc(sizeof(*novo));
    if (novo == NULL) {
        query_valor_apagar(&valor);
        return false;
    }
    novo->item = item;
    novo->valor = valor;

    // elo aponta para o ponteiro que deve passar a referenciar o novo no
    // Pode ser inicio ou o proximo de um no, sem tratar esses casos a parte
    NO_ORD **elo = &lista->inicio;
    while (*elo != NULL && comparar_nos(*elo, novo, decrescente) <= 0) {
        elo = &(*elo)->proximo;
    }
    novo->proximo = *elo;
    *elo = novo;

    // As propriedades so mudam depois de todas as alocacoes terem sucesso
    lista->decrescente = decrescente;
    if (valor != NULL) {
        lista->tipo = query_valor_tipo(valor);
        lista->tem_tipo = true;
    }
    lista->tamanho++;
    return true;
}

ITEM *lista_ord_primeiro(const LISTA_ORD *lista) {
    return lista_ord_vazia(lista) ? NULL : lista->inicio->item;
}

ITEM *lista_ord_remover_inicio(LISTA_ORD *lista) {
    if (lista_ord_vazia(lista)) return NULL;

    NO_ORD *removido = lista->inicio;
    ITEM *item = removido->item;
    lista->inicio = removido->proximo;
    query_valor_apagar(&removido->valor);
    free(removido);
    lista->tamanho--;

    // Depois de esvaziar, uma nova sequencia pode escolher outra direcao
    if (lista->tamanho == 0) lista->tem_tipo = false;
    return item;
}

int lista_ord_tamanho(const LISTA_ORD *lista) {
    return lista == NULL ? -1 : lista->tamanho;
}

bool lista_ord_vazia(const LISTA_ORD *lista) {
    return lista == NULL || lista->tamanho == 0;
}

int lista_ord_percorrer(LISTA_ORD *lista, ItemVisita visita, void *contexto) {
    if (lista == NULL || visita == NULL) return 0;

    int visitados = 0;
    for (NO_ORD *no = lista->inicio; no != NULL; no = no->proximo) {
        if (!visita(no->item, contexto)) break;
        visitados++;
    }
    return visitados;
}

void lista_ord_apagar(LISTA_ORD **lista) {
    if (lista == NULL || *lista == NULL) return;

    while (!lista_ord_vazia(*lista)) {
        ITEM *item = lista_ord_remover_inicio(*lista);
        item_apagar(&item);
    }
    free((*lista)->campo);
    free(*lista);
    *lista = NULL;
}
