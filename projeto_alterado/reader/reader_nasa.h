#ifndef READER_NASA_H
#define READER_NASA_H

#include <stdbool.h>
#include <stddef.h>

#include "config.h"
#include "estrutura.h"

//reader_nasa.h - carrega o CSV da Parte 1 dentro de uma Estrutura.

//este e o unico modulo que conhece o formato do dataset. Tudo o que sai
//daqui ja virou um ITEM generico, entao as estruturas de dados e a busca
//nao sabem de onde o registro veio.

//contadores para as linhas INPUT_RECORDS e de diagnostico do relatorio.
//n_input - linhas de dados lidas (o cabecalho nao conta)
//n_inseridos - itens que entraram de fato na base
//n_invalidos - linhas descartadas por parse ou por recusa da estrutura
typedef struct {
    size_t n_input;
    size_t n_inseridos;
    size_t n_invalidos;
} ContadoresLeitura;

//le o CSV em config->input e insere cada registro em 'base'.
//para ao atingir config->limit (0 = sem limite).
//retorna false apenas em erro de arquivo - linha malformada nao interrompe.
bool reader_nasa_loader(Estrutura *base, const Config *config,
                        ContadoresLeitura *contadores);

//schema do dataset: usado pelo main para recusar uma consulta com campo
//inexistente antes de varrer a base inteira. Sem isso, um nome digitado
//errado devolve um CSV vazio sem nenhuma mensagem de erro.
bool reader_nasa_tem_campo(const char *campo);

#endif
