// Testes das oito estruturas de dados, usadas diretamente (sem a fachada).
// Rode sob valgrind (make valgrind) para conferir posse e liberacao.
#include <stdio.h>

#include "test_util.h"
#include "pilha.h"
#include "fila.h"
#include "lista_sequencial.h"
#include "lista_encadeada.h"
#include "lista_cabeca.h"
#include "lista_ordenada.h"
#include "lista_generalizado.h"
#include "lista_cruzada.h"

// --------------------------------------------------------------- pilha

static void pilha_lifo(void) {
    PILHA *p = criar_pilha();
    CHECK(p != NULL);
    CHECK(pilha_empty(p));
    CHECK_EQ_INT(pilha_tamanho(p), 0);
    CHECK(pilha_pop(p) == NULL);
    CHECK(pilha_top(p) == NULL);

    for (int i = 1; i <= 3; i++) CHECK(pilha_push(p, teste_item(i, 2020, 1, 1, "Low")));
    CHECK_EQ_INT(pilha_tamanho(p), 3);
    CHECK_EQ_INT(item_get_chave(pilha_top(p)), 3);

    ITEM *x = pilha_pop(p);
    CHECK_EQ_INT(item_get_chave(x), 3); // pop transfere a posse
    item_apagar(&x);
    CHECK_EQ_INT(pilha_tamanho(p), 2);
    apagar_pilha(&p); // libera os itens restantes
    CHECK(p == NULL);
}

static void pilha_percurso_e_nulos(void) {
    PILHA *p = criar_pilha();
    for (int i = 1; i <= 4; i++) pilha_push(p, teste_item(i, 2020, 1, 1, "Low"));

    Visitas v = {0};
    CHECK_EQ_INT(pilha_percorrer(p, teste_visitar, &v), 4);
    CHECK_EQ_INT(v.n, 4);
    CHECK_EQ_INT(pilha_tamanho(p), 4); // percorrer nao remove

    Visitas parada = {.parar_apos = 2};
    CHECK_EQ_INT(pilha_percorrer(p, teste_visitar, &parada), 1); // so conta os true
    apagar_pilha(&p);

    CHECK(!pilha_push(NULL, NULL));
    CHECK_EQ_INT(pilha_percorrer(NULL, teste_visitar, &v), 0); // so a fachada devolve -1
    apagar_pilha(NULL);
}

// ---------------------------------------------------------------- fila

static void fila_fifo(void) {
    FILA *f = fila_criar();
    CHECK(f != NULL && fila_vazia(f));
    CHECK(fila_dequeue(f) == NULL);
    for (int i = 1; i <= 3; i++) CHECK(fila_enqueue(f, teste_item(i, 2020, 1, 1, "Low")));
    CHECK_EQ_INT(fila_tamanho(f), 3);
    CHECK_EQ_INT(item_get_chave(fila_frente(f)), 1);

    Visitas v = {0};
    CHECK_EQ_INT(fila_percorrer(f, teste_visitar, &v), 3);
    CHECK_EQ_INT(v.chaves[0], 1);
    CHECK_EQ_INT(v.chaves[2], 3);

    ITEM *x = fila_dequeue(f);
    CHECK_EQ_INT(item_get_chave(x), 1);
    item_apagar(&x);
    CHECK_EQ_INT(fila_tamanho(f), 2);
    CHECK_EQ_INT(item_get_chave(fila_frente(f)), 2);

    // esvaziar e reutilizar: o ponteiro de "fim" nao pode ficar solto.
    while (!fila_vazia(f)) { x = fila_dequeue(f); item_apagar(&x); }
    CHECK(fila_enqueue(f, teste_item(9, 2020, 1, 1, "Low")));
    CHECK_EQ_INT(fila_tamanho(f), 1);
    apagar_fila(&f);
    CHECK(f == NULL);
}

// ------------------------------------------------------ lista sequencial

static void lista_seq_ordena_por_chave(void) {
    LISTA_SEQ *l = lista_seq_criar();
    CHECK(l != NULL && lista_seq_vazia(l));
    int chaves[] = {5, 2, 8, 1, 9, 3};
    for (int i = 0; i < 6; i++)
        CHECK(lista_seq_inserir(l, teste_item(chaves[i], 2020, 1, 1, "Low")));
    CHECK_EQ_INT(lista_seq_tam(l), 6);

    Visitas v = {0};
    lista_seq_percorrer(l, teste_visitar, &v);
    int esperado[] = {1, 2, 3, 5, 8, 9};
    for (int i = 0; i < 6; i++) CHECK_EQ_INT(v.chaves[i], esperado[i]);

    // chave duplicada e rejeitada e o chamador continua dono do item
    ITEM *dup = teste_item(5, 2020, 1, 1, "Low");
    CHECK(!lista_seq_inserir(l, dup));
    item_apagar(&dup);
    CHECK_EQ_INT(lista_seq_tam(l), 6);

    lista_seq_remover_menor(l);
    CHECK_EQ_INT(lista_seq_tam(l), 5);
    Visitas v2 = {0};
    lista_seq_percorrer(l, teste_visitar, &v2);
    CHECK_EQ_INT(v2.chaves[0], 2);
    lista_seq_apagar(&l);
    CHECK(l == NULL);
}

static void lista_seq_cresce_alem_da_capacidade_inicial(void) {
    LISTA_SEQ *l = lista_seq_criar();
    for (int i = 0; i < 500; i++)
        CHECK(lista_seq_inserir(l, teste_item(500 - i, 2020, 1, 1, "Low")));
    CHECK_EQ_INT(lista_seq_tam(l), 500);
    Visitas v = {.parar_apos = 1};
    lista_seq_percorrer(l, teste_visitar, &v);
    CHECK_EQ_INT(v.chaves[0], 1);
    lista_seq_apagar(&l);
}

// ------------------------------------------------------ lista encadeada

static void lista_enc_inicio_e_fim(void) {
    LISTA_ENC *l = lista_enc_criar();
    CHECK(l != NULL && lista_enc_vazia(l));
    CHECK(lista_enc_remover_inicio(l) == NULL);
    CHECK(lista_enc_adicionar_fim(l, teste_item(2, 2020, 1, 1, "Low")));
    CHECK(lista_enc_adicionar_fim(l, teste_item(3, 2020, 1, 1, "Low")));
    CHECK(lista_enc_adicionar_inicio(l, teste_item(1, 2020, 1, 1, "Low")));
    CHECK_EQ_INT(lista_enc_tam(l), 3);

    Visitas v = {0};
    CHECK_EQ_INT(lista_enc_percorrer(l, teste_visitar, &v), 3);
    for (int i = 0; i < 3; i++) CHECK_EQ_INT(v.chaves[i], i + 1);

    ITEM *x = lista_enc_remover_inicio(l);
    CHECK_EQ_INT(item_get_chave(x), 1);
    item_apagar(&x);
    CHECK_EQ_INT(lista_enc_tam(l), 2);
    lista_enc_apagar(&l);
    CHECK(l == NULL);
    CHECK_EQ_INT(lista_enc_tam(NULL), -1);
}

// -------------------------------------------------------- lista cabeca

static void lista_cabeca_sentinela(void) {
    LISTA_CAB *l = lista_cabeca_criar();
    CHECK(l != NULL && lista_cabeca_vazia(l));
    CHECK_EQ_INT(lista_cabeca_tamanho(l), 0); // a sentinela nao conta
    CHECK(lista_cabeca_primeiro(l) == NULL);
    CHECK(lista_cabeca_remover_inicio(l) == NULL);

    CHECK(lista_cabeca_adicionar(l, teste_item(2, 2020, 1, 1, "Low")));
    CHECK(lista_cabeca_adicionar(l, teste_item(3, 2020, 1, 1, "Low")));
    CHECK(lista_cabeca_adicionar_inicio(l, teste_item(1, 2020, 1, 1, "Low")));
    CHECK_EQ_INT(lista_cabeca_tamanho(l), 3);
    CHECK_EQ_INT(item_get_chave(lista_cabeca_primeiro(l)), 1);

    Visitas v = {0};
    CHECK_EQ_INT(lista_cabeca_percorrer(l, teste_visitar, &v), 3);
    CHECK_EQ_INT(v.chaves[0], 1);
    CHECK_EQ_INT(v.chaves[2], 3);

    ITEM *x = lista_cabeca_remover_inicio(l);
    CHECK_EQ_INT(item_get_chave(x), 1);
    item_apagar(&x);
    CHECK_EQ_INT(lista_cabeca_tamanho(l), 2);
    lista_cabeca_apagar(&l);
    CHECK(l == NULL);
    CHECK_EQ_INT(lista_cabeca_tamanho(NULL), -1);
}

// ------------------------------------------------------ lista ordenada

static void ord_chaves(LISTA_ORD *l, int *saida, int *n) {
    Visitas v = {0};
    lista_ord_percorrer(l, teste_visitar, &v);
    *n = v.n;
    for (int i = 0; i < v.n && i < 256; i++) saida[i] = v.chaves[i];
}

static void lista_ord_asc_desc(void) {
    double riscos[] = {5.0, 1.0, 9.0, 3.0};
    int ch[256], n;
    LISTA_ORD *asc = lista_ord_criar("risk_score");
    LISTA_ORD *desc = lista_ord_criar("risk_score");
    CHECK(asc != NULL && desc != NULL);
    for (int i = 0; i < 4; i++) {
        CHECK(lista_ord_inserir(asc, teste_item(i + 1, 2020, riscos[i], 1, "Low"), false));
        CHECK(lista_ord_inserir(desc, teste_item(i + 1, 2020, riscos[i], 1, "Low"), true));
    }

    ord_chaves(asc, ch, &n);
    CHECK_EQ_INT(n, 4);
    CHECK_EQ_INT(ch[0], 2); CHECK_EQ_INT(ch[1], 4);
    CHECK_EQ_INT(ch[2], 1); CHECK_EQ_INT(ch[3], 3);
    ord_chaves(desc, ch, &n);
    CHECK_EQ_INT(ch[0], 3); CHECK_EQ_INT(ch[1], 1);
    CHECK_EQ_INT(ch[2], 4); CHECK_EQ_INT(ch[3], 2);

    CHECK_EQ_INT(item_get_chave(lista_ord_primeiro(desc)), 3);
    ITEM *x = lista_ord_remover_inicio(desc);
    CHECK_EQ_INT(item_get_chave(x), 3);
    item_apagar(&x);
    CHECK_EQ_INT(lista_ord_tamanho(desc), 3);

    // direcao fixada pela primeira insercao enquanto a lista nao esvazia
    ITEM *outro = teste_item(10, 2020, 4.0, 1, "Low");
    CHECK(!lista_ord_inserir(desc, outro, false));
    item_apagar(&outro);

    lista_ord_apagar(&asc);
    lista_ord_apagar(&desc);
    CHECK(asc == NULL && desc == NULL);
}

static void lista_ord_empates_e_ausentes(void) {
    LISTA_ORD *l = lista_ord_criar("risk_score");
    // dois empates (risco 5) e um item sem o campo? categoria ausente nao afeta risco;
    // usa "threat_category" em outra lista para testar ausencia abaixo.
    CHECK(lista_ord_inserir(l, teste_item(7, 2020, 5.0, 1, "Low"), false));
    CHECK(lista_ord_inserir(l, teste_item(3, 2020, 5.0, 1, "Low"), false));
    CHECK(lista_ord_inserir(l, teste_item(5, 2020, 5.0, 1, "Low"), false));
    int ch[256], n;
    ord_chaves(l, ch, &n);
    CHECK_EQ_INT(n, 3);
    // empate: chave original do CSV em ordem crescente
    CHECK_EQ_INT(ch[0], 3); CHECK_EQ_INT(ch[1], 5); CHECK_EQ_INT(ch[2], 7);
    lista_ord_apagar(&l);

    LISTA_ORD *c = lista_ord_criar("threat_category");
    CHECK(lista_ord_inserir(c, teste_item(1, 2020, 1, 1, NULL), false)); // ausente
    CHECK(lista_ord_inserir(c, teste_item(2, 2020, 1, 1, "Zeta"), false));
    CHECK(lista_ord_inserir(c, teste_item(3, 2020, 1, 1, "Alfa"), false));
    ord_chaves(c, ch, &n);
    CHECK_EQ_INT(ch[0], 3); CHECK_EQ_INT(ch[1], 2); CHECK_EQ_INT(ch[2], 1); // ausente no fim
    lista_ord_apagar(&c);

    CHECK(lista_ord_criar(NULL) == NULL);
    CHECK(lista_ord_criar("") == NULL);
}

// ------------------------------------------------------ lista generalizada

static void lista_gen_agrupa(void) {
    LISTA_GEN *l = lista_gen_criar_campos("threat_category", "year");
    CHECK(l != NULL);
    CHECK_EQ_INT(lista_gen_profundidade(l), 2);

    CHECK(lista_gen_inserir_item(l, teste_item(1, 2020, 1, 1, "Low")));
    CHECK(lista_gen_inserir_item(l, teste_item(2, 2021, 1, 1, "High")));
    CHECK(lista_gen_inserir_item(l, teste_item(3, 2020, 1, 1, "Low")));
    CHECK(lista_gen_inserir_item(l, teste_item(4, 2021, 1, 1, "Low")));
    CHECK_EQ_INT(lista_gen_tamanho(l), 4);
    CHECK_EQ_INT(lista_gen_comprimento(l), 2); // Low e High

    // grupo -> subgrupo -> itens, na ordem de criacao dos grupos
    Visitas v = {0};
    CHECK_EQ_INT(lista_gen_percorrer(l, teste_visitar, &v), 4);
    CHECK_EQ_INT(v.chaves[0], 1);
    CHECK_EQ_INT(v.chaves[1], 3); // Low/2020 agrupou 1 e 3
    CHECK_EQ_INT(v.chaves[2], 4); // Low/2021
    CHECK_EQ_INT(v.chaves[3], 2); // High/2021

    const NO_GEN *g = lista_gen_primeiro(l);
    CHECK(g != NULL && lista_gen_tipo(g) == NO_SUBLISTA);
    CHECK_EQ_INT(lista_gen_nitens(g), 3);
    lista_gen_apagar(&l);
    CHECK(l == NULL);
}

static void lista_gen_campo_ausente(void) {
    LISTA_GEN *l = lista_gen_criar_campos("threat_category", "year");
    ITEM *sem = teste_item(1, 2020, 1, 1, NULL); // categoria ausente
    bool ok = lista_gen_inserir_item(l, sem);
    if (!ok) item_apagar(&sem); // recusa nao transfere posse
    CHECK_EQ_INT(lista_gen_tamanho(l), ok ? 1 : 0);
    lista_gen_apagar(&l);
    CHECK(lista_gen_criar_campos(NULL, "year") == NULL);
}

// ------------------------------------------------------- lista cruzada

static void lista_cruz_por_campos(void) {
    LISTA_CRUZ *l = lista_cruz_criar_campos("threat_category", "year");
    CHECK(l != NULL);
    CHECK(lista_cruz_inserir_item(l, teste_item(1, 2020, 1, 1, "Low")));
    CHECK(lista_cruz_inserir_item(l, teste_item(2, 2021, 1, 1, "High")));
    CHECK(lista_cruz_inserir_item(l, teste_item(3, 2020, 1, 1, "Low"))); // mesma celula do 1
    CHECK_EQ_INT(lista_cruz_tamanho(l), 3);
    CHECK_EQ_INT(lista_cruz_linhas(l), 2);
    CHECK_EQ_INT(lista_cruz_colunas(l), 2);
    CHECK_STR(lista_cruz_rotulo_linha(l, 0), "Low");

    Visitas v = {0};
    CHECK_EQ_INT(lista_cruz_percorrer(l, teste_visitar, &v), 3); // cada registro uma vez
    CHECK_EQ_INT(v.n, 3);
    lista_apagar(&l);
    CHECK(l == NULL);
}

static void lista_cruz_por_coordenada(void) {
    LISTA_CRUZ *l = lista_cruz_criar(2, 3);
    CHECK(l != NULL);
    CHECK(lista_inserir(l, teste_item(1, 2020, 1, 1, "Low"), 0, 2));
    CHECK(lista_inserir(l, teste_item(2, 2020, 1, 1, "Low"), 1, 0));
    CHECK_EQ_INT(item_get_chave(lista_get(l, 0, 2)), 1);
    CHECK(lista_get(l, 0, 0) == NULL);

    ITEM *fora = teste_item(3, 2020, 1, 1, "Low");
    CHECK(!lista_inserir(l, fora, 5, 5)); // fora das dimensoes
    item_apagar(&fora);

    ITEM *x = lista_remover(l, 1, 0);
    CHECK_EQ_INT(item_get_chave(x), 2);
    item_apagar(&x);
    CHECK_EQ_INT(lista_cruz_tamanho(l), 1);
    lista_apagar(&l);
}

int main(void) {
    printf("test_estruturas\n");
    RUN(pilha_lifo);
    RUN(pilha_percurso_e_nulos);
    RUN(fila_fifo);
    RUN(lista_seq_ordena_por_chave);
    RUN(lista_seq_cresce_alem_da_capacidade_inicial);
    RUN(lista_enc_inicio_e_fim);
    RUN(lista_cabeca_sentinela);
    RUN(lista_ord_asc_desc);
    RUN(lista_ord_empates_e_ausentes);
    RUN(lista_gen_agrupa);
    RUN(lista_gen_campo_ausente);
    RUN(lista_cruz_por_campos);
    RUN(lista_cruz_por_coordenada);
    return testes_resumo("test_estruturas");
}