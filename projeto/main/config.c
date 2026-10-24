#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

// traduz o nome que veio do terminal pro enum da base
// se não bater com nenhum dos oito, devolve false
static bool config_base(Config *config, char *str){
if(!strcmp(str, "pilha")){
config->base = pilha;
return true;
}
if(!strcmp(str, "fila")){
config->base = fila;
return true;
}
if(!strcmp(str, "lista_seq")){
config->base = lista_seq;
return true;
}
if(!strcmp(str, "lista_enc")){
config->base = lista_enc;
return true;
}
if(!strcmp(str, "lista_cabeca")){
config->base = lista_cabeca;
return true;
}
if(!strcmp(str, "lista_ord")){
config->base = lista_ord;
return true;
}
if(!strcmp(str, "lista_gen")){
config->base = lista_gen;
return true;
}
if(!strcmp(str, "lista_cruz")){
config->base = lista_cruz;
return true;
}

return false;
}

// mesma coisa da de cima, só muda o campo
static bool config_resultado(Config *config, char *str){
if(!strcmp(str, "pilha")){
config->resultado = pilha;
return true;
}
if(!strcmp(str, "fila")){
config->resultado = fila;
return true;
}
if(!strcmp(str, "lista_seq")){
config->resultado = lista_seq;
return true;
}
if(!strcmp(str, "lista_enc")){
config->resultado = lista_enc;
return true;
}
if(!strcmp(str, "lista_cabeca")){
config->resultado = lista_cabeca;
return true;
}
if(!strcmp(str, "lista_ord")){
config->resultado = lista_ord;
return true;
}
if(!strcmp(str, "lista_gen")){
config->resultado = lista_gen;
return true;
}
if(!strcmp(str, "lista_cruz")){
config->resultado = lista_cruz;
return true;
}

return false;
}

// atoi porque os valores são garantidos válidos
static void config_top(Config *config, char *str){
config->top_k = atoi(str);
}

static void config_limit(Config *config, char *str){
config->limit = atoi(str);
}

// os valores vêm sempre na mesma ordem, então dá pra ler por posição:
// argv[1] --reader  argv[2] valor
// argv[3] --input   argv[4] valor
// argv[5] --base    argv[6] valor
// argv[7] --result  argv[8] valor
// argv[9] --query   argv[10] valor
// argv[11] --output argv[12] valor
// argv[13] --top-k  argv[14] valor
// argv[15] --limit  argv[16] valor
bool config_init(Config *config, char **argv){
// zera antes de alocar: se um malloc falhar no meio, o apagar não pega lixo
config->reader = NULL;
config->input = NULL;
config->path = NULL;
config->output = NULL;

// reader: tamanho do texto mais o '\0'
config->reader = (char*)malloc(sizeof(char)*strlen(argv[2])+sizeof(char));
if(config->reader == NULL){
return false;
}

// input: se falhar, libera o reader antes de sair
config->input = (char*)malloc(sizeof(char)*strlen(argv[4])+sizeof(char));
if(config->input == NULL){
free(config->reader);
config->reader = NULL;
return false;
}

// só copia depois que os dois buffers existem
strcpy(config->reader, argv[2]);
strcpy(config->input, argv[4]);

// base e resultado podem falhar se o nome não for um dos oito
if(!config_base(config, argv[6])){
free(config->reader);
free(config->input);
config->reader = NULL;
config->input = NULL;
return false;
}
if(!config_resultado(config, argv[8])){
free(config->reader);
free(config->input);
config->reader = NULL;
config->input = NULL;
return false;
}

// caminho da consulta
config->path = (char*)malloc(sizeof(char)*strlen(argv[10])+sizeof(char));
if(config->path == NULL){
free(config->reader);
free(config->input);
config->reader = NULL;
config->input = NULL;
return false;
}
strcpy(config->path, argv[10]);

// caminho de saída
config->output = (char*)malloc(sizeof(char)*strlen(argv[12])+sizeof(char));
if(config->output == NULL){
free(config->reader);
free(config->input);
free(config->path);
config->reader = NULL;
config->input = NULL;
config->path = NULL;
return false;
}
strcpy(config->output, argv[12]);

// os dois números são os últimos, nada mais pode falhar daqui pra frente
config_top(config, argv[14]);
config_limit(config, argv[16]);

return true;
}

// libera o que foi alocado e zera, pra não ficar ponteiro solto
// não libera o próprio config, ele está na pilha do main
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