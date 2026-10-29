#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

//traduz o nome que veio do terminal pro enum da base
//se não bater com nenhum dos oito, devolve uma flag invalida
//converte o nome da estrutura informado na linha de comando para o enum correspondente.
static OpcaoEstrutura config_tipo_estrutura (char *str) {
    if(!strcmp(str, "pilha")){
    return pilha;
    }
    if(!strcmp(str, "fila")){
    return fila;
    }
    if(!strcmp(str, "lista_seq")){
    return lista_seq;
    }
    if(!strcmp(str, "lista_enc")){
    return lista_enc;
    }
    if(!strcmp(str, "lista_cabeca")){
    return lista_cabeca;
    }
    if(!strcmp(str, "lista_ord")){
    return lista_ord;
    }
    if(!strcmp(str, "lista_gen")){
    return lista_gen;
    }
    if(!strcmp(str, "lista_cruz")){
    return lista_cruz;
    }

    return invalid;
}

//valida os valores numericos: atoi nao diferencia texto invalido de zero
//valida e converte um texto numerico para inteiro nao negativo.
static bool config_numero(char *str, int *valor){
    char *fim;
    errno = 0;
    long numero = strtol(str, &fim, 10);
    if(str == fim || *fim != '\0' || errno == ERANGE || numero < 0 || numero > INT_MAX){
    return false;
    }
    *valor = (int)numero;
    return true;
}
//valida e armazena o limite de resultados.

static bool config_top(Config *config, char *str){
    return config_numero(str, &config->top_k);
}
//valida e armazena o limite de registros lidos.

static bool config_limit(Config *config, char *str){
    return config_numero(str, &config->limit);
}

//os argumentos vêm em pares --opcao valor, mas a ordem pode mudar
//percorre argv e associa cada valor ao nome correspondente
//valida os argumentos e preenche a configuracao do programa.
bool config_init(Config *config, int argc, char **argv){
    if(config == NULL) return false;
    //zera antes de alocar: se um malloc falhar no meio, o apagar não pega lixo
    config->reader = NULL;
    config->input = NULL;
    config->path = NULL;
    config->output = NULL;
    config->base = invalid;
    config->resultado = invalid;
    config->top_k = 0;
    config->limit = 0;

    //checa a quantidade antes de acessar argv, evitando indices inexistentes
    if(argv == NULL || argc != 17) return false;

    const char *opcoes[8] = {
    "--reader", "--input", "--base", "--result",
    "--query", "--output", "--top-k", "--limit"
    };
    char *valores[8] = {NULL};

    for(int i = 1; i < argc; i += 2){
        if(argv[i] == NULL || argv[i + 1] == NULL) return false;
        int indice = -1;
        for(int j = 0; j < 8; j++){
            if(!strcmp(argv[i], opcoes[j])){
                indice = j;
                break;
            }
        }
        if(indice == -1 || valores[indice] != NULL) return false;
        valores[indice] = argv[i + 1];
    }
    
    for(int i = 0; i < 8; i++){
        if(valores[i] == NULL) return false;
    }

    //reader: tamanho do texto mais o '\0'
    config->reader = (char*)malloc(sizeof(char)*strlen(valores[0])+sizeof(char));
    if(config->reader == NULL){
        return false;
    }

    //input: se falhar, libera o reader antes de sair
    config->input = (char*)malloc(sizeof(char)*strlen(valores[1])+sizeof(char));
    if(config->input == NULL){
        free(config->reader);
        config->reader = NULL;
        return false;
    }

    //só copia depois que os dois buffers existem
    strcpy(config->reader, valores[0]);
    strcpy(config->input, valores[1]);

    //atribuicao dos campos base e resultado conforme a traducao para as constantes do enum
    config->base = config_tipo_estrutura(valores[2]);
    config->resultado = config_tipo_estrutura(valores[3]);
    
    //base e resultado podem falhar se o nome não for um dos oito
    if(config->base == invalid || config->resultado == invalid){
        config_apagar(config);
        return false;
    }

    //caminho da consulta
    config->path = (char*)malloc(sizeof(char)*strlen(valores[4])+sizeof(char));
    if(config->path == NULL){
        free(config->reader);
        free(config->input);
        config->reader = NULL;
        config->input = NULL;
        return false;
    }
    strcpy(config->path, valores[4]);

    //caminho de saída
    config->output = (char*)malloc(sizeof(char)*strlen(valores[5])+sizeof(char));
    if(config->output == NULL){
        free(config->reader);
        free(config->input);
        free(config->path);
        config->reader = NULL;
        config->input = NULL;
        config->path = NULL;
        return false;
    }
    strcpy(config->output, valores[5]);

    //os dois numeros tambem precisam ser validados
    if(!config_top(config, valores[6]) || !config_limit(config, valores[7])){
        config_apagar(config);
        return false;
    }

    return true;
}

//libera o que foi alocado e zera, pra não ficar ponteiro solto
//não libera o próprio config, ele está na pilha do main
//libera os caminhos alocados e zera os ponteiros da configuracao.
void config_apagar(Config *config){
    if(config == NULL){
        return;
    }
    if(config->reader != NULL)
        free(config->reader);
    if(config->input != NULL)
        free(config->input);
    if(config->path != NULL)
        free(config->path);
    if(config->output != NULL)
        free(config->output);

    config->reader = NULL;
    config->input = NULL;
    config->path = NULL;
    config->output = NULL;
}
