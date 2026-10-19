#ifndef ITEM_H
    #define ITEM_H
    #include <stdbool.h>
    typedef struct item_ ITEM;

    ITEM* item_criar(int x, int y, int valor);
    bool item_apagar(ITEM** item);
    int item_get_x(ITEM* item);
    int item_get_y(ITEM* item);
    int item_get_valor(ITEM* item);

#endif