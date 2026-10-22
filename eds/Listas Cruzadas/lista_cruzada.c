#include "item.h"
#include "lista.h"
#include "stdlib.h"
#include "stdio.h"

struct no_ {
    ITEM* item;
    NO* direita;
    NO* abaixo;
};

struct lista_ {
    NO** colunas;
    NO** linhas;
    int nLinhas;
    int nColunas;
};

LISTA* lista_criar (int nlinhas, int ncolunas) {
    LISTA* lista = malloc(sizeof(LISTA));
    NO** colunas = malloc(ncolunas*sizeof(NO*));
    NO** linhas = malloc(nlinhas*sizeof(NO*));
    if (lista == NULL || colunas == NULL || linhas == NULL)
    {
        free(lista);
        free(colunas);
        free(linhas);
        return NULL;
    }
        
    lista->colunas = colunas;
    lista->linhas = linhas;
    lista->nLinhas = nlinhas;
    lista->nColunas = ncolunas;
    for (int i = 0; i < nlinhas; i++) {
        *(linhas + i) = NULL;
    }
    for (int i = 0; i < ncolunas; i++) {
        *(colunas + i) = NULL;
    }
    return lista;
}

bool lista_inserir (LISTA* lista, ITEM* item, int posX, int posY) {
    if (lista == NULL || item == NULL || posX < 0 || posY < 0 || posX >= lista->nLinhas || posY >= lista->nColunas)
        return false;
    NO** colunas = lista->colunas;
    NO** linhas = lista->linhas;
    NO* no = malloc(sizeof(NO));

    if(no == NULL)
        return false;
    
    no->item = item;
    no->abaixo = NULL;
    no->direita = NULL;

    NO* sublistaCol = *(colunas + posY);
    NO* sublistaLin = *(linhas + posX);
    NO* anterior = NULL;
    NO* posterior = sublistaCol;

    while (posterior != NULL && item_get_x(posterior->item) < item_get_x(no->item))
    {
        anterior = posterior;
        posterior = posterior->abaixo;
    }

    no->abaixo = posterior;
    if (anterior != NULL) {anterior->abaixo = no;}
    else {*(colunas + posY) = no;}
    

    anterior = NULL;
    posterior = sublistaLin;
    
    while (posterior != NULL && item_get_y(posterior->item) < item_get_y(no->item))
    {
        anterior = posterior;
        posterior = posterior->direita;
    }

    no->direita = posterior;
    if (anterior != NULL) {anterior->direita = no;}
    else {*(linhas + posX) = no;}

    return true;
}

ITEM* lista_get (LISTA* lista, int posX, int posY) {
    NO* no = *(lista->colunas + posY);

    while (no != NULL && item_get_x(no->item) != posX) no = no->abaixo;
    
    if (no == NULL) return NULL;
    else return no->item;
}

ITEM* lista_remover (LISTA* lista, int posX, int posY) {
    NO* no = *(lista->colunas + posY);
    NO* anterior = NULL;

    while (no != NULL && item_get_x(no->item) != posX) {
        anterior = no;
        no = no->abaixo;
    }

    if (no != NULL)
    {
        if (anterior == NULL) *(lista->colunas + posY) = no->abaixo;
        else anterior->abaixo = no->abaixo;
    }
    else return NULL;

    anterior = NULL;

    NO* aux = lista->linhas[posX];
    while (aux != NULL && aux != no) {
        anterior = aux;
        aux = aux->direita;
    }
    
    if (anterior != NULL)
        anterior->direita = aux->direita;
    else
        *(lista->linhas + posX) = aux->direita;
    

    ITEM* item = no->item;
    aux = NULL;
    free(no);
    return item;
}

void lista_apagar (LISTA** lista) 
{
    LISTA* lst = *(lista);
    NO** colunas = lst->colunas;
    NO** linhas = lst->linhas;

    for (int i = 0; i < lst->nLinhas; i++)
    {
        NO* aux = linhas[i];
        NO* aux2;
        while (aux != NULL)
        {
            free(aux->item);
            aux2 = aux->direita;
            free(aux);
            aux = aux2;
        }
        linhas[i] = NULL;
    }
    free(colunas);
    free(linhas);
    free(lst);
    *(lista) = NULL;
}
