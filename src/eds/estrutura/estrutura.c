#include "estrutura.h"

#include <stdlib.h>
#include <string.h>

#include "pilha.h"
#include "fila.h"
#include "lista_sequencial.h"
#include "lista_encadeada.h"
#include "lista_cabeca.h"
#include "lista_ordenada.h"
#include "lista_generalizado.h"
#include "lista_cruzada.h"

// O header expoe apenas o nome Estrutura para manter os detalhes privados
// Esta definicao completa permite alocar o adaptador e acessar seus membros
struct estrutura_ {
    void *estrutura;
    OpcaoEstrutura tipo;
    bool order;              // true indica a direcao DESC da lista ordenada
    bool ordenar_similaridade;
    Query *query;            // emprestada: main libera a query depois das estruturas
};

//cria a estrutura de dados escolhida na configuracao.
Estrutura *estrutura_criar(OpcaoEstrutura tipo, Query *query) {
    Estrutura *nova = malloc(sizeof(*nova));

    if (nova == NULL) {
        return NULL;
    }

    nova->estrutura = NULL;
    nova->tipo = tipo;
    nova->order = false;
    nova->ordenar_similaridade = false;
    nova->query = query;

    switch (tipo) {
        case pilha:
            nova->estrutura = criar_pilha();
            break;

        case fila:
            nova->estrutura = fila_criar();
            break;

        case lista_seq:
            nova->estrutura = lista_seq_criar();
            break;

        case lista_enc:
            nova->estrutura = lista_enc_criar();
            break;

        case lista_cabeca:
            nova->estrutura = lista_cabeca_criar();
            break;

        case lista_ord: {
            const char *campo;
            DirecaoQuery direcao;

            if (query == NULL ||
                !query_get_order(query, &campo, &direcao)) {
                free(nova);
                return NULL;
            }

            nova->order = (direcao == QUERY_DESC);
            nova->ordenar_similaridade = strcmp(campo, "similarity") == 0;
            nova->estrutura = lista_ord_criar(campo);
            break;
        }

        case lista_gen: {
            const char *campo1;
            const char *campo2;

            if (query == NULL ||
                !query_get_group(query, &campo1, &campo2)) {
                free(nova);
                return NULL;
            }

            nova->estrutura = lista_gen_criar_campos(campo1, campo2);
            break;
        }

        case lista_cruz: {
            const char *campo1;
            const char *campo2;

            if (query == NULL ||
                !query_get_cross(query, &campo1, &campo2)) {
                free(nova);
                return NULL;
            }

            nova->estrutura = lista_cruz_criar_campos(campo1, campo2);
            break;
        }

        default:
            free(nova);
            return NULL;
    }

    if (nova->estrutura == NULL) {
        free(nova);
        return NULL;
    }

    return nova;
}

//insere um item na estrutura selecionada.
bool estrutura_inserir(Estrutura *est, ITEM *item) {
    if (est == NULL || est->estrutura == NULL || item == NULL) {
        return false;
    }

    switch (est->tipo) {
        case pilha:
            return pilha_push(est->estrutura, item);

        case fila:
            return fila_enqueue(est->estrutura, item);

        case lista_seq:
            return lista_seq_inserir(est->estrutura, item);

        case lista_enc:
            return lista_enc_adicionar_fim(est->estrutura, item);

        case lista_cabeca:
            return lista_cabeca_adicionar(est->estrutura, item);

        case lista_ord:
            // Quando a BASE e ordenada por similarity, a pontuacao ainda
            // nao foi calculada pela Search. Calcula antes de inserir,
            // para manter a ordem durante a propria leitura.
            if (est->ordenar_similaridade) {
                ValorQuery *valor = NULL;
                bool tem_nota = item_ler_campo(item, "similarity", &valor);
                query_valor_apagar(&valor);
                if (!tem_nota) {
                    double nota = 0.0;
                    query_avaliar(est->query, item, item_ler_campo, &nota);
                    if (!item_definir_similaridade(item, nota)) return false;
                }
            }
            return lista_ord_inserir(est->estrutura, item, est->order);

        case lista_gen:
            return lista_gen_inserir_item(est->estrutura, item);

        case lista_cruz:
            return lista_cruz_inserir_item(est->estrutura, item);

        default:
            return false;
    }
}

int estrutura_percorrer(Estrutura *est, ItemVisita visita, void *contexto) {
    if (est == NULL || est->estrutura == NULL || visita == NULL) return -1;

    // Search e Writer recebem sempre ITEMs e nao precisam conhecer os nos
    // A ordem observada e o percurso natural de cada TAD
    switch (est->tipo) {
        case pilha:
            return pilha_percorrer(est->estrutura, visita, contexto);
        case fila:
            return fila_percorrer(est->estrutura, visita, contexto);
        case lista_seq:
            return lista_seq_percorrer(est->estrutura, visita, contexto);
        case lista_enc:
            return lista_enc_percorrer(est->estrutura, visita, contexto);
        case lista_cabeca:
            return lista_cabeca_percorrer(est->estrutura, visita, contexto);
        case lista_ord:
            return lista_ord_percorrer(est->estrutura, visita, contexto);
        case lista_gen:
            return lista_gen_percorrer(est->estrutura, visita, contexto);
        case lista_cruz:
            // O percurso global segue linhas e visita a lista interna da celula
            // Percorrer colunas novamente repetiria os mesmos registros
            return lista_cruz_percorrer(est->estrutura, visita, contexto);
        default:
            return -1;
    }
}

int estrutura_tamanho(const Estrutura *est) {
    if (est == NULL || est->estrutura == NULL) return -1;

    switch (est->tipo) {
        case pilha:        return pilha_tamanho(est->estrutura);
        case fila:         return fila_tamanho(est->estrutura);
        case lista_seq:    return lista_seq_tam(est->estrutura);
        case lista_enc:    return lista_enc_tam(est->estrutura);
        case lista_cabeca: return lista_cabeca_tamanho(est->estrutura);
        case lista_ord:    return lista_ord_tamanho(est->estrutura);
        case lista_gen:    return lista_gen_tamanho(est->estrutura);
        case lista_cruz:   return lista_cruz_tamanho(est->estrutura);
        default:          return -1;
    }
}

//desaloca a estrutura e os recursos que ela administra.
void estrutura_apagar(Estrutura **est) {
    if (est == NULL || *est == NULL) {
        return;
    }

    switch ((*est)->tipo) {
        case pilha: {
            PILHA *estrutura = (*est)->estrutura;
            apagar_pilha(&estrutura);
            break;
        }

        case fila: {
            FILA *estrutura = (*est)->estrutura;
            apagar_fila(&estrutura);
            break;
        }

        case lista_seq: {
            LISTA_SEQ *estrutura = (*est)->estrutura;
            lista_seq_apagar(&estrutura);
            break;
        }

        case lista_enc: {
            LISTA_ENC *estrutura = (*est)->estrutura;
            lista_enc_apagar(&estrutura);
            break;
        }

        case lista_cabeca: {
            LISTA_CAB *estrutura = (*est)->estrutura;
            lista_cabeca_apagar(&estrutura);
            break;
        }

        case lista_ord: {
            LISTA_ORD *estrutura = (*est)->estrutura;
            lista_ord_apagar(&estrutura);
            break;
        }

        case lista_gen: {
            LISTA_GEN *estrutura = (*est)->estrutura;
            lista_gen_apagar(&estrutura);
            break;
        }

        case lista_cruz: {
            LISTA_CRUZ *estrutura = (*est)->estrutura;
            lista_apagar(&estrutura);
            break;
        }

        default:
            break;
    }

    free(*est);
    *est = NULL;
}
