#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "reader_ancine.h"
#include "item_ancine.h"
#include "item.h"

// reader_ancine.c - CSV do ANCINE -> ITEM generico.
//
// Irmao do reader_nasa.c. A estrutura e a mesma de proposito: parse do texto
// separado da montagem do ITEM, para que trocar de dataset nao mexa nas
// estruturas nem no writer.
//
// Diferencas de formato em relacao ao NASA:
//   1. separador ';' em vez de ','
//   2. campo vazio aparece como ";;", que o dividir entrega como "" e nao
//      como NULL. O ou_nulo() converte "" em NULL antes do construtor, para
//      o item nunca guardar string vazia.
//   3. sem tratamento de aspas: o CSV do ANCINE nao usa aspas, entao a
//      string e copiada inteira, do jeito que veio.
//
// Os contadores sao opacos: o struct contadores_leitura e definido apenas no
// reader.c. Aqui so existem os acessores e as funcoes de incremento.

#define NUM_CAMPOS 18    // o CSV do ANCINE tem exatamente 18 colunas
#define TAM_LINHA  4096  // acima disso a linha e considerada truncada

// Verifica se fgets leu toda a linha; descarta os fragmentos excedentes.
static bool linha_completa(FILE *arquivo, const char *linha, size_t capacidade) {
    size_t tamanho = strlen(linha);
    if (tamanho < capacidade - 1 || strchr(linha, '\n') != NULL) return true;

    int proximo = fgetc(arquivo);
    if (proximo == EOF) return true; // linha no limite e sem quebra final
    while (proximo != '\n' && proximo != EOF) {
        proximo = fgetc(arquivo);
    }
    return false;
}

// Adaptadores: traduzem o ITEM_ANCINE para o contrato do ITEM generico.
static void apagar_ancine(void **dado) {
    item_ancine_apagar((ITEM_ANCINE **)dado);
}

static void *copiar_ancine(const void *dado) {
    // item_ancine_copiar ainda recebe ponteiro nao-const, daí o cast
    return item_ancine_copiar((ITEM_ANCINE *)dado);
}

// Divide a linha por ';'. Sem aspas no CSV, nao ha ';' protegido dentro de
// um campo: cada ';' fecha um campo e abre o proximo.
// n comeca em 1 porque campos[0] e sempre o comeco da linha.
// Retorna 0 se houver mais de 18 campos.
static size_t dividir(char *linha, char *campos[]) {
    size_t n = 0;
    campos[n++] = linha;

    for (char *p = linha; *p != '\0'; p++) {
        if (*p == ';') {
            if (n == NUM_CAMPOS) return 0;  // 19o campo: layout inesperado
            *p = '\0';                      // fecha o campo atual
            campos[n++] = p + 1;            // o proximo comeca depois do ';'
        }
    }
    return n;
}

// ";;" chega aqui como "". Converter para NULL e o que faz o item tratar o
// campo como ausente em vez de guardar string vazia.
//
// Como funciona: em "a;;b", o dividir fecha 'a' no primeiro ';' e o segundo
// ';' e encontrado imediatamente, entao o campo entre eles fica com "".
// Sem o ou_nulo, esse "" seria copiado pelo item_ancine_criar e o campo
// pareceria preenchido - um filtro por texto nunca casaria, mas tambem nao
// seria rejeitado. Com o ou_nulo, "" vira NULL e o item_ancine_ler_campo
// responde "campo ausente", que e o comportamento correto.
static const char *ou_nulo(const char *campo) {
    if (campo == NULL) return NULL;
    if (*campo == '\0') return NULL;   // ";;" virou "" -> trata como ausente
    return campo;
}

// strtod com validacao. Celula vazia e rejeitada antes do parse: sem isso
// strtod("") devolveria 0 e o registro entraria com dado inventado.
static bool ler_double(const char *s, double *saida) {
    if (*s == '\0') return false;
    char *fim;
    errno = 0;
    double x = strtod(s, &fim);
    if (fim == s || *fim != '\0' || errno == ERANGE || !isfinite(x)) return false;
    *saida = x;
    return true;
}

// Mesmo criterio do double, com strtol.
static bool ler_int(const char *s, int *saida) {
    if (s == NULL || saida == NULL || *s == '\0') return false;

    char *fim;
    errno = 0;
    long x = strtol(s, &fim, 10);

    if (fim == s || *fim != '\0' || errno == ERANGE) return false;
    if (x < INT_MIN || x > INT_MAX) return false;

    *saida = (int)x;
    return true;
}

// O CSV traz True/False capitalizado. Aceita as duas grafias.
static bool ler_bool(const char *s, bool *saida) {
    if (!strcmp(s, "True")  || !strcmp(s, "true"))  { *saida = true;  return true; }
    if (!strcmp(s, "False") || !strcmp(s, "false")) { *saida = false; return true; }
    return false;   // celula vazia ou valor inesperado
}

bool reader_ancine_tem_campo(const char *campo) {
    // A tabela de campos vive no item_ancine.c: manter uma copia aqui faria
    // as duas divergirem na primeira mudanca de schema.
    return item_ancine_tem_campo(campo);
}

bool reader_ancine_loader(Estrutura *base, const Config *config,
                          ContadoresLeitura *contadores) {
    if (base == NULL || config == NULL || contadores == NULL) return false;

    FILE *arquivo = fopen(config_get_input(config), "r");
    if (arquivo == NULL) {
        perror("Reader: nao foi possivel abrir o CSV");
        return false;
    }

    char linha[TAM_LINHA];
    char *campos[NUM_CAMPOS];

    // A primeira linha e o cabecalho: descartada aqui e recopiada do proprio
    // input pelo writer, o que garante que a saida tem o mesmo cabecalho.
    if (fgets(linha, sizeof linha, arquivo) == NULL) {
        fclose(arquivo);
        return false;   // arquivo vazio ou ilegivel
    }
    if (!linha_completa(arquivo, linha, sizeof linha)) {
        fclose(arquivo);
        return false;   // cabecalho truncado
    }

    while (fgets(linha, sizeof linha, arquivo) != NULL) {
        // -limit corta por registros INSERIDOS, nao por linhas lidas: uma
        // linha invalida nao consome cota.
        if (config_get_limit(config) > 0 &&
            contadores_inseridos(contadores) >= (size_t)config_get_limit(config))
            break;

        contadores_inc_input(contadores);
        if (!linha_completa(arquivo, linha, sizeof linha)) {
            contadores_inc_invalidos(contadores);
            continue;
        }
        linha[strcspn(linha, "\r\n")] = '\0';   // tolera final CRLF

        // Layout inesperado: conta como invalida e segue para a proxima linha.
        if (dividir(linha, campos) != NUM_CAMPOS) {
            contadores_inc_invalidos(contadores);
            continue;
        }

        // Campos obrigatorios. Os quatro numericos e o booleano no mesmo if:
        // se qualquer um falhar, a linha inteira e descartada. O booleano
        // entra aqui porque coproducao_internacional nao tem par "_valido" -
        // ";;" nele significa registro incompleto, nao "nao sei".
        double duracao;
        int episodios, ano_inicial, ano_final;
        bool coproducao;

        if (!ler_double(campos[8], &duracao)      ||
            !ler_int(campos[9], &episodios)       ||
            !ler_int(campos[10], &ano_inicial)    ||
            !ler_int(campos[11], &ano_final)      ||
            !ler_bool(campos[13], &coproducao)) {
            contadores_inc_invalidos(contadores);
            continue;
        }

        // Strings passam por ou_nulo: ";;" vira NULL e o item trata como
        // ausente. Nao ha validacao adicional - texto vazio e o unico caso
        // que o formato produz, e ele ja esta coberto.
        ITEM_ANCINE *obra = item_ancine_criar(
            ou_nulo(campos[0]),   // titulo_original
            ou_nulo(campos[1]),   // cpb
            ou_nulo(campos[2]),   // data_emissao_cpb
            ou_nulo(campos[3]),   // situacao_obra
            ou_nulo(campos[4]),   // tipo_obra
            ou_nulo(campos[5]),   // subtipo_obra
            ou_nulo(campos[6]),   // classificacao_obra
            ou_nulo(campos[7]),   // organizacao_temporal
            duracao,
            episodios,
            ano_inicial,
            ano_final,
            ou_nulo(campos[12]),  // segmento_destinacao_inicial
            coproducao,
            ou_nulo(campos[14]),  // requerente
            ou_nulo(campos[15]),  // cnpj_requerente
            ou_nulo(campos[16]),  // uf_requerente
            ou_nulo(campos[17])   // municipio_requerente
        );

        // Envelope generico. A chave e o numero da linha: desempate estavel
        // no top-k quando duas notas de similaridade empatam.
        ITEM *item = NULL;
        if (obra != NULL) {
            item = item_criar(obra, apagar_ancine, copiar_ancine,
                              item_ancine_ler_campo,
                              (int)contadores_input(contadores));
            if (item == NULL) item_ancine_apagar(&obra);   // envelope falhou
        }

        // Uma saida de erro para os dois casos: item NULL (falta de memoria)
        // e recusa da estrutura. item_apagar aceita NULL.
        if (item == NULL || !estrutura_inserir(base, item)) {
            item_apagar(&item);
            contadores_inc_invalidos(contadores);
            continue;
        }
        contadores_inc_inseridos(contadores);
    }

    if (ferror(arquivo)) {   // erro de leitura no meio do arquivo
        fclose(arquivo);
        return false;
    }
    fclose(arquivo);
    return true;
}
