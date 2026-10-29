#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "reader_nasa.h"
#include "item_nasa.h"
#include "item.h"

//reader_nasa.c - CSV -> ITEM generico.

//duas partes bem separadas: o parse do texto (dividir, sem_aspas, ler_*) e a
//montagem do ITEM. Se a Parte 2 usar outro dataset, este arquivo e o unico
//que precisa de um irmao - as estruturas nao mudam.

#define NUM_CAMPOS 20    //o CSV da Parte 1 tem exatamente 20 colunas
#define TAM_LINHA  4096  //acima disso a linha e considerada truncada

//adaptadores: traduzem o ITEM_NASA para o contrato do ITEM generico.
//ficam aqui (e nao no item.c) para manter o envelope cego ao dataset.
//libera os recursos associados a estrutura.
static void apagar_nasa(void **dado) {
    item_nasa_apagar((ITEM_NASA **)dado);
}
//cria uma copia dos dados recebidos.

static void *copiar_nasa(const void *dado) {
    //item_nasa_copiar ainda recebe ponteiro nao-const, daí o cast
    return item_nasa_copiar((ITEM_NASA *)dado);
}

//divide a linha por virgula respeitando aspas: uma virgula dentro de
//"...", como em "2020 AB1, fragmento B", nao separa campo.
//n comeca em 1 porque campos[0] e sempre o comeco da linha.
//retorna 0 se houver mais de 20 campos ou se uma aspa nao fechar.
//separa os campos do CSV respeitando virgulas dentro de aspas.
static size_t dividir(char *linha, char *campos[]) {
    size_t n = 0;
    bool dentro = false;
    campos[n++] = linha;

    for (char *p = linha; *p != '\0'; p++) {
        if (*p == '"') {
            dentro = !dentro;   //alterna: abre, fecha, abre...
        } else if (*p == ',' && !dentro) {
            if (n == NUM_CAMPOS) return 0;  //21o campo: layout inesperado
            *p = '\0';                      //fecha o campo atual
            campos[n++] = p + 1;            //o proximo comeca depois da virgula
        }
    }
    if (dentro) return 0;   //aspa aberta ate o fim: linha invalida
    return n;
}

//remove as aspas das pontas. O ITEM guarda o valor puro; quem devolve as
//aspas na saida e o writer, junto com o escape das aspas internas.
//remove as aspas externas de um campo do CSV.
static char *sem_aspas(char *campo) {
    size_t n = strlen(campo);
    if (n >= 2 && campo[0] == '"' && campo[n - 1] == '"') {
        campo[n - 1] = '\0';
        return campo + 1;   //devolve o ponteiro ja deslocado
    }
    return campo;           //campo sem aspas: devolve como esta
}

//strtod com validacao completa. Celula vazia e rejeitada antes do parse:
//sem isso strtod("") devolveria 0 e o registro entraria com dado inventado.
//converte o campo para double e rejeita valores invalidos.
static bool ler_double(const char *s, double *saida) {
    if (*s == '\0') return false;
    char *fim;
    errno = 0;
    double x = strtod(s, &fim);
    //rejeita cauda nao numerica ("12abc"), overflow e campo parcial
    if (fim == s || *fim != '\0' || errno == ERANGE) return false;
    *saida = x;
    return true;
}

//mesmo criterio do double, com strtol. A faixa de int e bem menor que a de
//long, entao o cast so acontece depois da validacao de fim de string.
//converte o campo para inteiro e rejeita valores invalidos.
static bool ler_int(const char *s, int *saida) {
    if (*s == '\0') return false;
    char *fim;
    errno = 0;
    long x = strtol(s, &fim, 10);
    if (fim == s || *fim != '\0' || errno == ERANGE) return false;
    *saida = (int)x;
    return true;
}

//o CSV traz True/False capitalizado. Aceita as duas grafias para nao
//depender de como o arquivo foi gerado.
//converte o texto do CSV para valor booleano.
static bool ler_bool(const char *s, bool *saida) {
    if (!strcmp(s, "True")  || !strcmp(s, "true"))  { *saida = true;  return true; }
    if (!strcmp(s, "False") || !strcmp(s, "false")) { *saida = false; return true; }
    return false;   //celula vazia ou valor inesperado
}
//verifica se o nome corresponde a um campo conhecido do dataset.

bool reader_nasa_tem_campo(const char *campo) {
    //a tabela de campos vive no item_nasa.c: manter uma copia aqui faria
    //as duas divergirem na primeira mudanca de schema.
    return item_nasa_tem_campo(campo);
}
//le o CSV, valida os registros e insere os itens na estrutura base.

bool reader_nasa_loader(Estrutura *base, const Config *config,
                        ContadoresLeitura *contadores) {
    if (base == NULL || config == NULL || contadores == NULL) return false;

    contadores->n_input = 0;
    contadores->n_inseridos = 0;
    contadores->n_invalidos = 0;

    FILE *arquivo = fopen(config->input, "r");
    if (arquivo == NULL) {
        perror("Reader: nao foi possivel abrir o CSV");
        return false;
    }

    char linha[TAM_LINHA];
    char *campos[NUM_CAMPOS];

    //a primeira linha e o cabecalho: descartada aqui e recopiada do proprio
    //input pelo writer, o que garante que a saida tem o mesmo cabecalho.
    if (fgets(linha, sizeof linha, arquivo) == NULL) {
        fclose(arquivo);
        return false;   //arquivo vazio ou ilegivel
    }

    while (fgets(linha, sizeof linha, arquivo) != NULL) {
        //-limit corta por registros INSERIDOS, nao por linhas lidas: uma
        //linha invalida nao consome cota.
        if (config->limit > 0 &&
            contadores->n_inseridos >= (size_t)config->limit) break;

        contadores->n_input++;
        linha[strcspn(linha, "\r\n")] = '\0';   //tolera final CRLF

        //layout inesperado: conta como invalida e segue para a proxima linha.
        //uma linha ruim nao pode derrubar a carga inteira.
        if (dividir(linha, campos) != NUM_CAMPOS) {
            contadores->n_invalidos++;
            continue;
        }
        for (size_t i = 0; i < NUM_CAMPOS; i++) campos[i] = sem_aspas(campos[i]);

        int ano, mes, dias, nivel;
        double dist, vel, mag, risco;

        //campos obrigatorios, todos numericos e todos no mesmo if: se
        //qualquer um falhar, a linha inteira e descartada.
        if (!ler_int(campos[3], &ano)     || !ler_int(campos[4], &mes) ||
            !ler_double(campos[5], &dist) || !ler_double(campos[6], &vel) ||
            !ler_double(campos[7], &mag)  || !ler_int(campos[8], &dias) ||
            !ler_double(campos[11], &risco) || !ler_int(campos[12], &nivel)) {
            contadores->n_invalidos++;
            continue;
        }

        //sentry e opcional no dataset: comeca em NAN, que significa "ausente".
        //zero seria um valor legitimo e passaria em filtro numerico como se
        //fosse medicao real. por isso NAN e nao 0.0
        double impacto = NAN, torino = NAN, palermo = NAN, diametro = NAN;

        bool passado = false, futuro = false, sentry = false;
        //o retorno do parse vira a flag de validade do booleano: e assim que
        //"celula vazia" fica distinguivel de "False".
        bool passado_ok = ler_bool(campos[9],  &passado);
        bool futuro_ok  = ler_bool(campos[10], &futuro);
        bool sentry_ok  = ler_bool(campos[15], &sentry);

        //aqui a ausencia e aceitavel, entao o retorno e ignorado de proposito:
        //campo vazio mantem o NAN inicial.
        ler_double(campos[16], &impacto);
        ler_double(campos[17], &torino);
        ler_double(campos[18], &palermo);
        ler_double(campos[19], &diametro);

        ITEM_NASA *nasa = item_nasa_criar(
            campos[0], campos[1], campos[2],
            ano, mes,
            dist, vel, mag, dias,
            passado, passado_ok,
            futuro, futuro_ok,
            risco, nivel,
            campos[13], campos[14],
            sentry, sentry_ok,
            impacto, torino, palermo, diametro
        );

        //envelope generico. A chave e o numero da linha: desempate estavel
        //no top-k quando duas notas de similaridade empatam.
        ITEM *item = NULL;
        if (nasa != NULL) {
            item = item_criar(nasa, apagar_nasa, copiar_nasa,
                              item_nasa_ler_campo, (int)contadores->n_input);
            if (item == NULL) item_nasa_apagar(&nasa);   //envelope falhou
        }

        //uma saida de erro para os dois casos: item NULL (falta de memoria)
        //e recusa da estrutura. item_apagar aceita NULL, entao nao precisa
        //de teste separado.
        if (item == NULL || !estrutura_inserir(base, item)) {
            item_apagar(&item);
            contadores->n_invalidos++;
            continue;
        }
        contadores->n_inseridos++;
    }

    if (ferror(arquivo)) {   //erro de leitura no meio do arquivo
        fclose(arquivo);
        return false;
    }
    fclose(arquivo);
    return true;
}
