#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "lista_generalizado.h"

//lista_generalizada.c - lista generalizada (lista de listas).

//cada elemento e um no com:
//tipo     - ATOMO (guarda ITEM) ou SUBLISTA (guarda lista interna)
//info     - o ITEM, ou o primeiro elemento da sublista
//prox     - proximo irmao no mesmo nivel
//chave    - rotulo, usado so quando e SUBLISTA
//fim      - ultimo elemento da sublista, para append em O(1)

//o item para sempre num ATOMO. Com profundidade 2, a forma e:
//nivel 0: SUBLISTA(cat1) -> SUBLISTA(cat2) -> ATOMO -> ATOMO
//nivel 1: SUBLISTA(ano)  -> ATOMO -> ATOMO

//'fim' existe por desempenho: sem ele, um grupo com milhares de itens
//custaria uma varredura da cadeia a cada insercao, virando O(n^2).

//as chaves sao ValorQuery copiadas, nao char*. Assim year (numerico) e
//threat_category (texto) convivem na mesma lista sem conversao.

struct no_gen {
    TipoNoGen tipo;
    union {
        ITEM *item;
        struct no_gen *sublista;   //primeiro elemento da sublista
    } info;
    struct no_gen *fim;            //sUBLISTA: ultimo elemento
    ValorQuery *chave;             //sUBLISTA: rotulo opaco do grupo
    bool tem_chave;
    struct no_gen *prox;
};

struct lista_gen {
    struct no_gen cabeca;   //sentinela do nivel 0 (tipo = SUBLISTA)
    int total;              //atomos somando todas as sublistas
    int profundidade;
    char **campos;
};
//cria uma copia alocada de uma string.

static char *copiar_string(const char *origem) {
    if (origem == NULL) return NULL;
    size_t n = strlen(origem) + 1;
    char *copia = malloc(n);
    if (copia != NULL) memcpy(copia, origem, n);
    return copia;
}
//cria um no que armazena um item.

static NO_GEN *no_criar_atomo(ITEM *item) {
    NO_GEN *no = calloc(1, sizeof(*no));
    if (no == NULL) return NULL;

    no->tipo = NO_ATOMO;
    no->info.item = item;
    return no;
}
//cria uma sublista e copia sua chave, quando houver.

static NO_GEN *no_criar_sublista(const ValorQuery *chave) {
    NO_GEN *no = calloc(1, sizeof(*no));
    if (no == NULL) return NULL;

    no->tipo = NO_SUBLISTA;
    no->info.sublista = NULL;   //sublista vazia
    no->fim = NULL;

    if (chave != NULL) {
        no->chave = query_valor_copiar(chave);
        if (no->chave == NULL) {
            free(no);
            return NULL;
        }
        no->tem_chave = true;
    }
    return no;
}

//anexa 'novo' ao fim da sublista de 'pai'. O(1) via ponteiro 'fim'.
//'pai' precisa ser SUBLISTA: a uniao e lida como info.sublista.
//adiciona um no ao final da sublista.
static void anexar(NO_GEN *pai, NO_GEN *novo) {
    if (pai->fim == NULL) pai->info.sublista = novo;
    else pai->fim->prox = novo;

    pai->fim = novo;
}

//procura, entre os filhos de 'pai', a sublista com essa chave.
//busca linear: dezenas de grupos por nivel, nao milhares de registros.
//procura uma sublista filha com a chave informada.
static NO_GEN *buscar_filho(const NO_GEN *pai, const ValorQuery *chave) {
    for (NO_GEN *f = pai->info.sublista; f != NULL; f = f->prox) {
        if (f->tipo != NO_SUBLISTA) continue;
        if (!f->tem_chave && chave == NULL) return f;
        if (f->tem_chave && query_valor_iguais(f->chave, chave)) return f;
    }
    return NULL;
}
//cria uma lista generalizada com os campos de agrupamento informados.

LISTA_GEN *lista_gen_criar_n(const char **campos, int n) {
    if (campos == NULL || n <= 0) return NULL;

    LISTA_GEN *lista = calloc(1, sizeof(*lista));
    if (lista == NULL) return NULL;

    lista->campos = calloc((size_t)n, sizeof(char *));
    if (lista->campos == NULL) {
        free(lista);
        return NULL;
    }

    for (int i = 0; i < n; i++) {
        if (campos[i] == NULL || *campos[i] == '\0') {
            for (int j = 0; j < i; j++) free(lista->campos[j]);
            free(lista->campos);
            free(lista);
            return NULL;
        }

        lista->campos[i] = copiar_string(campos[i]);
        if (lista->campos[i] == NULL) {
            for (int j = 0; j < i; j++) free(lista->campos[j]);
            free(lista->campos);
            free(lista);
            return NULL;
        }
    }

    //a sentinela vive dentro da struct: nunca passa por free().
    lista->cabeca.tipo = NO_SUBLISTA;
    lista->cabeca.info.sublista = NULL;
    lista->cabeca.fim = NULL;
    lista->cabeca.tem_chave = false;
    lista->total = 0;
    lista->profundidade = n;
    return lista;
}
//cria uma lista generalizada com dois niveis de agrupamento.

LISTA_GEN *lista_gen_criar_campos(const char *campo1, const char *campo2) {
    const char *campos[2];
    campos[0] = campo1;
    campos[1] = campo2;
    return lista_gen_criar_n(campos, 2);
}
//insere um item usando as chaves de agrupamento ja calculadas.

bool lista_gen_inserir_chaves(LISTA_GEN *lista, ITEM *item,
                              ValorQuery *const *chaves, int n) {
    if (lista == NULL || item == NULL) return false;
    if (n < 0) return false;
    if (n > 0 && chaves == NULL) return false;

    NO_GEN *atual = &lista->cabeca;

    for (int i = 0; i < n; i++) {
        NO_GEN *filho = buscar_filho(atual, chaves[i]);

        if (filho == NULL) {
            filho = no_criar_sublista(chaves[i]);
            //falha no meio do caminho pode deixar sublista vazia na arvore.
            //ela nao conta em 'total' e nao aparece no percurso dos atomos,
            //entao nao corrompe a estrutura.
            if (filho == NULL) return false;
            anexar(atual, filho);
        }

        atual = filho;
    }

    NO_GEN *atomo = no_criar_atomo(item);
    if (atomo == NULL) return false;

    anexar(atual, atomo);
    lista->total++;
    return true;
}
//calcula as chaves do item e o insere na lista generalizada.

bool lista_gen_inserir_item(LISTA_GEN *lista, ITEM *item) {
    if (lista == NULL || item == NULL) return false;
    if (lista->profundidade <= 0 || lista->campos == NULL) return false;

    //resolve as chaves uma vez: com profundidade 2 sao dois campos lidos
    //por item, em vez de reler o mesmo campo a cada nivel.
    ValorQuery **chaves = calloc((size_t)lista->profundidade, sizeof(*chaves));
    if (chaves == NULL) return false;

    bool ok = true;
    for (int i = 0; i < lista->profundidade; i++) {
        // Campo ausente forma um grupo sem chave (NULL), compartilhado
        // pelos registros que tambem nao possuem esse valor.
        item_ler_campo(item, lista->campos[i], &chaves[i]);
    }

    if (ok) ok = lista_gen_inserir_chaves(lista, item, chaves, lista->profundidade);

    //os objetos temporarios sao nossos; textos continuam emprestados do ITEM.
    for (int i = 0; i < lista->profundidade; i++) query_valor_apagar(&chaves[i]);
    free(chaves);
    return ok;
}
//retorna o numero de itens armazenados.

int lista_gen_tamanho(const LISTA_GEN *lista) {
    if (lista == NULL) return 0;
    return lista->total;
}
//retorna a quantidade de niveis de agrupamento.

int lista_gen_profundidade(const LISTA_GEN *lista) {
    if (lista == NULL) return 0;
    return lista->profundidade;
}
//retorna a quantidade de nos no nivel indicado.

int lista_gen_comprimento(const LISTA_GEN *lista) {
    if (lista == NULL) return 0;

    int n = 0;
    for (const NO_GEN *no = lista->cabeca.info.sublista; no != NULL; no = no->prox) n++;
    return n;
}
//retorna o primeiro no do nivel indicado.

const NO_GEN *lista_gen_primeiro(const LISTA_GEN *lista) {
    if (lista == NULL) return NULL;
    return lista->cabeca.info.sublista;
}
//retorna o proximo no do mesmo nivel.

const NO_GEN *lista_gen_proximo(const NO_GEN *no) {
    if (no == NULL) return NULL;
    return no->prox;
}
//retorna o primeiro filho de uma sublista.

const NO_GEN *lista_gen_filhos(const NO_GEN *no) {
    if (no == NULL) return NULL;
    if (no->tipo != NO_SUBLISTA) return NULL;
    return no->info.sublista;
}
//retorna o tipo do no.

TipoNoGen lista_gen_tipo(const NO_GEN *no) {
    if (no == NULL) return NO_ATOMO;
    return no->tipo;
}
//retorna o item armazenado em um no atomico.

ITEM *lista_gen_atomo(const NO_GEN *no) {
    if (no == NULL) return NULL;
    if (no->tipo != NO_ATOMO) return NULL;
    return no->info.item;
}
//retorna a chave de agrupamento de uma sublista.

const ValorQuery *lista_gen_chave(const NO_GEN *no) {
    if (no == NULL) return NULL;
    if (!no->tem_chave) return NULL;
    return no->chave;
}
//retorna a quantidade de itens da sublista.

int lista_gen_nitens(const NO_GEN *no) {
    if (no == NULL) return 0;
    if (no->tipo == NO_ATOMO) return 0;

    int n = 0;
    for (const NO_GEN *f = no->info.sublista; f != NULL; f = f->prox) {
        if (f->tipo == NO_ATOMO) {
            n++;
            continue;
        }
        n += lista_gen_nitens(f);
    }
    return n;
}

//atomos do proprio nivel primeiro, depois desce nos filhos. Num GROUP de 2
//campos so o nivel mais fundo tem atomos, entao a ordem final e
//cat1 -> ano1 -> itens, cat1 -> ano2 -> itens, cat2 -> ...
//percorre recursivamente os nos e os itens de uma lista generalizada.
static void percorrer_lista(NO_GEN *no, ItemVisita visita, void *contexto,
                            int *visitados, bool *parar) {
    while (no != NULL) {
        if (no->tipo == NO_ATOMO) {
            if (!visita(no->info.item, contexto)) {
                *parar = true;
                return;
            }
            (*visitados)++;
        } else {
            percorrer_lista(no->info.sublista, visita, contexto, visitados, parar);
            if (*parar) return;
        }
        no = no->prox;
    }
}
//percorre todos os itens da lista generalizada.

int lista_gen_percorrer(LISTA_GEN *lista, ItemVisita visita, void *contexto) {
    if (lista == NULL || visita == NULL) return 0;

    int visitados = 0;
    bool parar = false;

    percorrer_lista(lista->cabeca.info.sublista, visita, contexto, &visitados, &parar);
    return visitados;
}
//formata a chave de agrupamento em texto.

const char *lista_gen_texto(const ValorQuery *valor, char *buffer, size_t tam) {
    return query_valor_formatar(valor, buffer, tam);
}

//impressao no formato classico de lista generalizada, com parenteses.
//exemplo: ( (categoria A (2020 (item, item) ) ) )
//imprime recursivamente a lista generalizada para depuracao.
static void imprimir_lista(const NO_GEN *no, void (*imprimir_item)(ITEM *)) {
    char rotulo[64];

    printf("(");
    while (no != NULL) {
        if (no->tipo == NO_ATOMO) {
            if (imprimir_item != NULL) imprimir_item(no->info.item);
            else printf("#");
        } else {
            if (no->tem_chave) {
                lista_gen_texto(no->chave, rotulo, sizeof(rotulo));
            } else {
                lista_gen_texto(NULL, rotulo, sizeof(rotulo));
            }
            printf("%s ", rotulo);
            imprimir_lista(no->info.sublista, imprimir_item);
        }

        if (no->prox != NULL) printf(", ");
        no = no->prox;
    }
    printf(")");
}
//imprime a lista generalizada.

void lista_gen_imprimir(const LISTA_GEN *lista, void (*imprimir_item)(ITEM *)) {
    if (lista == NULL) return;

    printf("Lista generalizada: profundidade %d, %d itens\n",
           lista->profundidade, lista->total);

    imprimir_lista(lista->cabeca.info.sublista, imprimir_item);
    printf("\n");
}

//libera uma cadeia de irmaos e, recursivamente, o que cada sublista guarda.
//libera recursivamente os nos de uma lista generalizada.
static void apagar_lista(NO_GEN *no) {
    while (no != NULL) {
        NO_GEN *prox = no->prox;   //guarda antes de liberar o atual

        if (no->tipo == NO_ATOMO) {
            item_apagar(&no->info.item);
        } else {
            apagar_lista(no->info.sublista);
            query_valor_apagar(&no->chave);
        }

        free(no);
        no = prox;
    }
}
//libera a lista generalizada e seus campos.

void lista_gen_apagar(LISTA_GEN **lista) {
    if (lista == NULL || *lista == NULL) return;

    LISTA_GEN *l = *lista;

    //a sentinela vive dentro da struct: so os elementos sao liberados.
    apagar_lista(l->cabeca.info.sublista);

    for (int i = 0; i < l->profundidade; i++) free(l->campos[i]);
    free(l->campos);
    free(l);
    *lista = NULL;
}
