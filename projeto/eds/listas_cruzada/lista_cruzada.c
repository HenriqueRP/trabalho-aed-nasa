#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>

#include "lista_cruzada.h"

// lista_cruzada.c - matriz esparsa representada por duas cadeias de nos.
//
// Cada CELULA pertence a UMA linha e a UMA coluna ao mesmo tempo.
// 'abaixo' percorre a coluna, 'direita' percorre a linha.
// Agora cada celula guarda uma lista de registros da mesma coordenada,
// pois varios asteroides podem ter o mesmo par (categoria, ano).
//
// O ITEM aparece uma vez so na lista cruzada, mas e alcancavel por dois
// caminhos, sem duplicacao interna de registros.

typedef struct no_item_cruz NO_ITEM;
typedef struct no_cruzado NO;

struct no_item_cruz {
    ITEM *item;
    NO_ITEM *proximo;
};

struct no_cruzado {
    NO_ITEM *itens, *fim; // lista de ITEMs na mesma coordenada
    int x, y;            // posicao NESTA matriz, nao no registro
    NO *direita;
    NO *abaixo;
};

struct lista_cruzada {
    NO **colunas;    // vetor de cabecas de coluna
    NO **linhas;     // vetor de cabecas de linha
    int nLinhas;
    int nColunas;

    // CamposQuery define NOMES dos atributos; os valores reais definem eixos.
    // NULL nos nomes = modo original, com coordenadas inteiras explicitas.
    char *campo_linha, *campo_coluna;
    ValorQuery *valores_linha, *valores_coluna; // chaves copiadas, nao emprestadas
};

static char *copiar_string(const char *origem) {
    size_t n = strlen(origem) + 1;
    char *copia = malloc(n);
    if (copia != NULL) memcpy(copia, origem, n);
    return copia;
}

// Copia texto para nao depender do tempo de vida de um ITEM especifico.
static bool copiar_valor(const ValorQuery *origem, ValorQuery *destino) {
    if (origem == NULL || destino == NULL) return false;
    *destino = *origem;
    if (origem->tipo == QUERY_TEXTO) {
        if (origem->valor.texto == NULL) return false;
        destino->valor.texto = copiar_string(origem->valor.texto);
        return destino->valor.texto != NULL;
    }
    if (origem->tipo == QUERY_NUMERO) return isfinite(origem->valor.numero);
    return origem->tipo == QUERY_BOOLEANO;
}

static void apagar_valor(ValorQuery *valor) {
    if (valor->tipo == QUERY_TEXTO) free((void *)valor->valor.texto);
}

static bool valores_iguais(const ValorQuery *a, const ValorQuery *b) {
    if (a->tipo != b->tipo) return false;
    if (a->tipo == QUERY_TEXTO)
        return a->valor.texto != NULL && b->valor.texto != NULL &&
               strcmp(a->valor.texto, b->valor.texto) == 0;
    if (a->tipo == QUERY_NUMERO) return a->valor.numero == b->valor.numero;
    if (a->tipo == QUERY_BOOLEANO) return a->valor.booleano == b->valor.booleano;
    return false;
}

static int buscar_valor(const ValorQuery *chaves, int n, const ValorQuery *valor) {
    if (valor == NULL) return -1;
    for (int i = 0; i < n; i++) {
        if (valores_iguais(&chaves[i], valor)) return i;
    }
    return -1;
}

LISTA_CRUZ *lista_criar(int nlinhas, int ncolunas) {
    if (nlinhas <= 0 || ncolunas <= 0) return NULL;
    if ((size_t)nlinhas > SIZE_MAX / sizeof(NO *) ||
        (size_t)ncolunas > SIZE_MAX / sizeof(NO *)) return NULL;

    LISTA_CRUZ *lista = calloc(1, sizeof(*lista));
    if (lista == NULL) return NULL;

    NO **colunas = calloc((size_t)ncolunas, sizeof(NO *));
    NO **linhas  = calloc((size_t)nlinhas, sizeof(NO *));
    if (colunas == NULL || linhas == NULL) {
        free(colunas);
        free(linhas);
        free(lista);
        return NULL;
    }

    lista->colunas = colunas;
    lista->linhas = linhas;
    lista->nLinhas = nlinhas;
    lista->nColunas = ncolunas;
    return lista;
}

LISTA_CRUZ *lista_cruz_criar_campos(const char *campo_linha,
                                   const char *campo_coluna) {
    if (campo_linha == NULL || campo_coluna == NULL ||
        *campo_linha == '\0' || *campo_coluna == '\0') return NULL;

    // Sem numero fixo de linhas/colunas: cada chave nova cria um eixo.
    LISTA_CRUZ *lista = calloc(1, sizeof(*lista));
    if (lista == NULL) return NULL;
    lista->campo_linha = copiar_string(campo_linha);
    lista->campo_coluna = copiar_string(campo_coluna);
    if (lista->campo_linha == NULL || lista->campo_coluna == NULL) {
        free(lista->campo_linha);
        free(lista->campo_coluna);
        free(lista);
        return NULL;
    }
    return lista;
}

// Acrescenta uma chave no fim de um dos vetores de cabecas. Atribui a cada
// categoria/ano um indice interno usado nos encadeamentos x/y.
// So troca os vetores quando TODAS as alocacoes deram certo.
static bool garantir_eixo(LISTA_CRUZ *lista, bool eh_linha,
                          const ValorQuery *valor, int *indice, bool *novo) {
    int n = eh_linha ? lista->nLinhas : lista->nColunas;
    ValorQuery *chaves = eh_linha ? lista->valores_linha : lista->valores_coluna;
    NO **cabecas = eh_linha ? lista->linhas : lista->colunas;

    *indice = buscar_valor(chaves, n, valor);
    *novo = false;
    if (*indice >= 0) return true;
    if (n == INT_MAX || (size_t)n + 1 > SIZE_MAX / sizeof(NO *) ||
        (size_t)n + 1 > SIZE_MAX / sizeof(ValorQuery)) return false;

    ValorQuery copia;
    if (!copiar_valor(valor, &copia)) return false;
    NO **novas_cabecas = malloc(((size_t)n + 1) * sizeof(*novas_cabecas));
    ValorQuery *novas_chaves = malloc(((size_t)n + 1) * sizeof(*novas_chaves));
    if (novas_cabecas == NULL || novas_chaves == NULL) {
        free(novas_cabecas);
        free(novas_chaves);
        apagar_valor(&copia);
        return false;
    }
    if (n != 0) {
        memcpy(novas_cabecas, cabecas, (size_t)n * sizeof(*novas_cabecas));
        memcpy(novas_chaves, chaves, (size_t)n * sizeof(*novas_chaves));
    }
    novas_cabecas[n] = NULL;
    novas_chaves[n] = copia;
    free(cabecas);
    free(chaves);

    if (eh_linha) {
        lista->linhas = novas_cabecas;
        lista->valores_linha = novas_chaves;
        lista->nLinhas++;
    } else {
        lista->colunas = novas_cabecas;
        lista->valores_coluna = novas_chaves;
        lista->nColunas++;
    }
    *indice = n;
    *novo = true;
    return true;
}

// Em caso de falha posterior, um eixo recem-criado ainda esta vazio.
// A capacidade fisica do vetor pode continuar maior, mas o contador recua.
static void desfazer_ultimo_eixo(LISTA_CRUZ *lista, bool eh_linha) {
    int *n = eh_linha ? &lista->nLinhas : &lista->nColunas;
    ValorQuery *chaves = eh_linha ? lista->valores_linha : lista->valores_coluna;
    (*n)--;
    apagar_valor(&chaves[*n]);
}

// Localiza a celula na linha, onde o encadeamento e ordenado por y.
static NO *buscar_celula(const LISTA_CRUZ *lista, int x, int y) {
    NO *no = lista->linhas[x];
    while (no != NULL && no->y < y) no = no->direita;
    return no != NULL && no->y == y ? no : NULL;
}

bool lista_inserir(LISTA_CRUZ *lista, ITEM *item, int posX, int posY) {
    if (lista == NULL || item == NULL) return false;
    if (posX < 0 || posY < 0 || posX >= lista->nLinhas || posY >= lista->nColunas)
        return false;

    // Cada celula pode ter varios registros, sem criar varios nos da matriz.
    NO_ITEM *entrada = malloc(sizeof(*entrada));
    if (entrada == NULL) return false;
    entrada->item = item;
    entrada->proximo = NULL;

    NO *celula = buscar_celula(lista, posX, posY);
    if (celula != NULL) {
        // Mesma coordenada: so insere na lista interna (O(1), ponteiro fim).
        celula->fim->proximo = entrada;
        celula->fim = entrada;
        return true;
    }

    NO *no = malloc(sizeof(*no));
    if (no == NULL) {
        free(entrada);
        return false;
    }
    no->itens = no->fim = entrada;
    no->x = posX;
    no->y = posY;
    no->abaixo = NULL;
    no->direita = NULL;

    // Encaixa na coluna posY, mantendo a ordem crescente de x.
    NO *ant = NULL;
    NO *atual = lista->colunas[posY];
    while (atual != NULL && atual->x < posX) {
        ant = atual;
        atual = atual->abaixo;
    }
    no->abaixo = atual;
    if (ant == NULL) lista->colunas[posY] = no;
    else             ant->abaixo = no;

    // Mesma coisa na linha posX, agora ordenado por y.
    ant = NULL;
    atual = lista->linhas[posX];
    while (atual != NULL && atual->y < posY) {
        ant = atual;
        atual = atual->direita;
    }
    no->direita = atual;
    if (ant == NULL) lista->linhas[posX] = no;
    else             ant->direita = no;

    return true;
}

bool lista_cruz_inserir_item(LISTA_CRUZ *lista, ITEM *item) {
    if (lista == NULL || item == NULL || lista->campo_linha == NULL) return false;
    ValorQuery valor_linha, valor_coluna;
    if (!item_ler_campo(item, lista->campo_linha, &valor_linha) ||
        !item_ler_campo(item, lista->campo_coluna, &valor_coluna)) return false;

    int x, y;
    bool nova_linha, nova_coluna;
    if (!garantir_eixo(lista, true, &valor_linha, &x, &nova_linha)) return false;
    if (!garantir_eixo(lista, false, &valor_coluna, &y, &nova_coluna)) {
        if (nova_linha) desfazer_ultimo_eixo(lista, true);
        return false;
    }
    if (!lista_inserir(lista, item, x, y)) {
        if (nova_linha) desfazer_ultimo_eixo(lista, true);
        if (nova_coluna) desfazer_ultimo_eixo(lista, false);
        return false;
    }
    return true;
}

ITEM *lista_get(const LISTA_CRUZ *lista, int posX, int posY) {
    if (lista == NULL) return NULL;
    if (posX < 0 || posY < 0 || posX >= lista->nLinhas || posY >= lista->nColunas)
        return NULL;
    // desce pela coluna posY procurando x == posX
    NO *no = lista->colunas[posY];
    while (no != NULL && no->x < posX) no = no->abaixo;
    return no != NULL && no->x == posX ? no->itens->item : NULL;
}

ITEM *lista_remover(LISTA_CRUZ *lista, int posX, int posY) {
    if (lista == NULL) return NULL;
    if (posX < 0 || posY < 0 || posX >= lista->nLinhas || posY >= lista->nColunas)
        return NULL;

    // localiza na coluna guardando o anterior
    NO *ant_col = NULL;
    NO *no = lista->colunas[posY];
    while (no != NULL && no->x < posX) {
        ant_col = no;
        no = no->abaixo;
    }
    if (no == NULL || no->x != posX) return NULL;

    // Mais de um registro na coordenada: retira o primeiro, sem mexer na malha.
    if (no->itens->proximo != NULL) {
        NO_ITEM *entrada = no->itens;
        ITEM *item = entrada->item;
        no->itens = entrada->proximo;
        free(entrada);
        return item;
    }

    // Ultimo ITEM: confirma presenca na linha ANTES de desligar a celula.
    NO *ant_lin = NULL;
    NO *aux = lista->linhas[posX];
    while (aux != NULL && aux != no) {
        ant_lin = aux;
        aux = aux->direita;
    }
    if (aux == NULL) return NULL;

    if (ant_col == NULL) lista->colunas[posY] = no->abaixo;
    else                 ant_col->abaixo = no->abaixo;
    if (ant_lin == NULL) lista->linhas[posX] = no->direita;
    else                 ant_lin->direita = no->direita;

    ITEM *item = no->itens->item;
    free(no->itens);
    free(no); // so os nos: a posse do ITEM sai pelo retorno
    return item;
}

// Percurso linha por linha, coluna por coluna dentro de cada linha.
// E a ordem de leitura da matriz, nao a ordem de insercao.
int lista_cruz_percorrer(LISTA_CRUZ *lista, ItemVisita visita, void *contexto) {
    if (lista == NULL || visita == NULL) return 0;
    int n = 0;
    for (int i = 0; i < lista->nLinhas; i++) {
        for (NO *no = lista->linhas[i]; no != NULL; no = no->direita) {
            for (NO_ITEM *entrada = no->itens; entrada != NULL; entrada = entrada->proximo) {
                if (!visita(entrada->item, contexto)) return n;
                n++;
            }
        }
    }
    return n;
}

int lista_cruz_percorrer_linha(LISTA_CRUZ *lista, int linha,
                              ItemVisita visita, void *contexto) {
    if (lista == NULL || visita == NULL || linha < 0 || linha >= lista->nLinhas) return 0;
    int n = 0;
    for (NO *no = lista->linhas[linha]; no != NULL; no = no->direita) {
        for (NO_ITEM *entrada = no->itens; entrada != NULL; entrada = entrada->proximo) {
            if (!visita(entrada->item, contexto)) return n;
            n++;
        }
    }
    return n;
}

int lista_cruz_percorrer_coluna(LISTA_CRUZ *lista, int coluna,
                               ItemVisita visita, void *contexto) {
    if (lista == NULL || visita == NULL || coluna < 0 || coluna >= lista->nColunas) return 0;
    int n = 0;
    for (NO *no = lista->colunas[coluna]; no != NULL; no = no->abaixo) {
        for (NO_ITEM *entrada = no->itens; entrada != NULL; entrada = entrada->proximo) {
            if (!visita(entrada->item, contexto)) return n;
            n++;
        }
    }
    return n;
}

int lista_cruz_percorrer_valor_linha(LISTA_CRUZ *lista, const ValorQuery *valor,
                                    ItemVisita visita, void *contexto) {
    if (lista == NULL || lista->campo_linha == NULL) return 0;
    int indice = buscar_valor(lista->valores_linha, lista->nLinhas, valor);
    return lista_cruz_percorrer_linha(lista, indice, visita, contexto);
}

int lista_cruz_percorrer_valor_coluna(LISTA_CRUZ *lista, const ValorQuery *valor,
                                     ItemVisita visita, void *contexto) {
    if (lista == NULL || lista->campo_coluna == NULL) return 0;
    int indice = buscar_valor(lista->valores_coluna, lista->nColunas, valor);
    return lista_cruz_percorrer_coluna(lista, indice, visita, contexto);
}

void lista_imprimir(const LISTA_CRUZ *lista, void (*imprimir_item)(ITEM *)) {
    if (lista == NULL || imprimir_item == NULL) return;
    printf("Lista cruzada %d x %d\n", lista->nLinhas, lista->nColunas);
    for (int i = 0; i < lista->nLinhas; i++) {
        bool vazia = true;
        for (NO *no = lista->linhas[i]; no != NULL; no = no->direita) {
            if (vazia) { printf("linha %d:", i); vazia = false; }
            printf(" [col %d] ", no->y);
            for (NO_ITEM *entrada = no->itens; entrada != NULL; entrada = entrada->proximo)
                imprimir_item(entrada->item);
        }
        if (vazia) printf("linha %d: (vazia)", i);
        printf("\n");
    }
}

void lista_apagar(LISTA_CRUZ **lista) {
    if (lista == NULL || *lista == NULL) return;
    LISTA_CRUZ *lst = *lista;

    // Varre so pelas linhas. Cada celula pertence a exatamente uma linha,
    // entao varrer as colunas tambem daria double free.
    for (int i = 0; i < lst->nLinhas; i++) {
        NO *aux = lst->linhas[i];
        while (aux != NULL) {
            NO *prox = aux->direita;
            NO_ITEM *entrada = aux->itens;
            while (entrada != NULL) {
                NO_ITEM *proxima = entrada->proximo;
                item_apagar(&entrada->item);
                free(entrada);
                entrada = proxima;
            }
            free(aux);
            aux = prox;
        }
        lst->linhas[i] = NULL;
    }
    for (int i = 0; i < lst->nLinhas && lst->valores_linha != NULL; i++)
        apagar_valor(&lst->valores_linha[i]);
    for (int i = 0; i < lst->nColunas && lst->valores_coluna != NULL; i++)
        apagar_valor(&lst->valores_coluna[i]);
    free(lst->valores_linha);
    free(lst->valores_coluna);
    free(lst->campo_linha);
    free(lst->campo_coluna);
    free(lst->colunas);
    free(lst->linhas);
    free(lst);
    *lista = NULL;
}
