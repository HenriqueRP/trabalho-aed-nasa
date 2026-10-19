#ifndef LISTA_H
    #define LISTA_H
    #include "stdbool.h"
    #include "item.h"
    
    typedef struct lista_ LISTA;
    typedef struct no_ NO;

    LISTA* lista_criar (int nlinhas, int ncolunas);
    bool lista_inserir (LISTA* lista, ITEM* item, int posX, int posY);
    ITEM* lista_get (LISTA* lista, int posX, int posY);
    ITEM* lista_remover (LISTA* lista, int posX, int posY);
    void lista_apagar (LISTA** lista);
    
#endif