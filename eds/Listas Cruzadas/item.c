#include "item.h"
#include "stdbool.h"
#include "stdlib.h"

struct item_ {
    int x;
    int y;
    int valor;
};

ITEM* item_criar (int x, int y, int valor) {
    ITEM* item = malloc(sizeof(ITEM));
    if (item == NULL)
        return NULL;
    item->x = x;
    item->y = y;
    item->valor = valor;
    return item;
}

bool item_apagar(ITEM** item) {
    if (item == NULL || *item == NULL)
        return false;
    free(*item);
    *item = NULL;
    return true;
}

int item_get_x(ITEM* item) {
    if (item == NULL)
        return 0;
    return item->x;
}

int item_get_y(ITEM* item) {
    if (item == NULL)
        return 0;
    return item->y;
}

int item_get_valor (ITEM* item) {
    if (item == NULL)
        return 0;
    return item->valor;
}