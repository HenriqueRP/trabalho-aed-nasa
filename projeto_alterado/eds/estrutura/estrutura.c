#include "estrutura.h"


struct estrutura_{
    void *estrutura;
    opcao_estrutura tipo;
};
//cria a estrutura de dados escolhida na configuracao.

Estrutura *estrutura_criar(int tipo){
    Estrutura *nova = (Estrutura *)malloc(sizeof(Estrutura));
    if(nova == NULL){
        return NULL;
    }
    switch(tipo){
        case 0 : nova->estrutura = (PILHA*)criar_pilha(); break;
        case 1 : nova->estrutura = (FILA*)fila_criar();break;
        case 2 : nova->estrutura = (LISTA_SEQ*)lista_seq_criar();break;
        case 3 : nova->estrutura = (LISTA_ENC*)lista_enc_criar();break;
        case 4 : nova->estrutura = (LISTA_CAB*)lista_cabeca_criar();break;
        case 5 : nova->estrutura = (LISTA_ORD*)lista_ord_criar();break;
        case 6 : nova->estrutura = (LISTA_GEN*)lista_gen_criar();break;
        case 7 : nova->estrutura = (LISTA_CRUZ*)lista_cruz_criar();break;
    }

    if(nova->estrutura == NULL){
        free(nova);
        return NULL;
    }
    nova->tipo = tipo;

    return nova;
}
//insere um item na estrutura selecionada.

bool estrutura_inserir(Estrutura *est, ITEM *item){
    int tipo = est->tipo;
    switch(tipo){
        case 0 : return pilha_push(est->estrutura, item);
        case 1 : return fila_enqueue(est->estrutura, item);
        case 2 : return lista_seq_inserir(est->estrutura, item);
        case 3 : return lista_enc_adicionar_fim(est->estrutura, item);
        case 4 : return lista_cabeca_adicionar(est->estrutura, item);
        case 5 : return lista_ord_adicionar();
        case 6 : return lista_gen_inserir_item(est->estrutura, item);
        case 7 : return lista_cruz_inserir_item(est->estrutura, item);
    }
}
