#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "writer.h"
#include "item.h"
#include "query.h"

#define TAM_CABECALHO 8192
#define MAX_CAMPOS 64

// armazena o estado da exportacao durante o percurso da estrutura.
typedef struct {
    FILE *arquivo;
    char delimitador;
    char *campos[MAX_CAMPOS];
    size_t n_campos;
    size_t n_escritos;
    int top_k;
    bool sucesso;
} ContextoWriter;


// identifica o delimitador predominante no cabecalho.
// delimitadores dentro de campos entre aspas sao ignorados.
static char identificar_delimitador(const char *linha) {
    size_t virgulas = 0;
    size_t ponto_virgulas = 0;
    bool dentro = false;

    for (const char *p = linha; *p != '\0'; p++) {
        if (*p == '"') {
            // aspas duplas consecutivas representam uma aspa escapada.
            if (dentro && p[1] == '"') {
                p++;
            } else {
                dentro = !dentro;
            }
        } else if (!dentro) {
            if (*p == ',') virgulas++;
            if (*p == ';') ponto_virgulas++;
        }
    }

    if (ponto_virgulas > virgulas) return ';';
    return ',';
}


// divide o cabecalho em campos, substituindo os delimitadores por '\0'.
// os ponteiros resultantes apontam para dentro da propria linha.
static bool separar_cabecalho(char *linha, char delimitador,
                              char *campos[], size_t *quantidade) {
    if (linha == NULL || campos == NULL || quantidade == NULL) {
        return false;
    }

    size_t n = 0;
    bool dentro = false;

    campos[n++] = linha;

    for (char *p = linha; *p != '\0'; p++) {
        if (*p == '"') {
            // ignora aspas escapadas durante a identificacao dos campos.
            if (dentro && p[1] == '"') {
                p++;
            } else {
                dentro = !dentro;
            }
        } else if (*p == delimitador && !dentro) {
            // impede que a quantidade de campos ultrapasse o limite.
            if (n == MAX_CAMPOS) return false;

            *p = '\0';
            campos[n++] = p + 1;
        }
    }

    // um numero impar de aspas indica um cabecalho malformado.
    if (dentro) return false;

    // remove as aspas externas dos nomes dos campos.
    for (size_t i = 0; i < n; i++) {
        size_t tamanho = strlen(campos[i]);

        if (tamanho >= 2 &&
            campos[i][0] == '"' &&
            campos[i][tamanho - 1] == '"') {
            campos[i][tamanho - 1] = '\0';
            campos[i]++;
        }
    }

    *quantidade = n;
    return true;
}


// escreve um texto seguindo as regras de escape do csv.
// campos que possuem delimitadores, aspas ou quebras de linha
// sao envolvidos por aspas duplas.
static bool escrever_texto(FILE *arquivo, const char *texto,
                           char delimitador) {
    // um ponteiro nulo representa uma celula vazia.
    if (texto == NULL) return true;

    bool precisa_aspas = false;

    // verifica se o campo precisa ser delimitado por aspas.
    for (const char *p = texto; *p != '\0'; p++) {
        if (*p == delimitador || *p == '"' ||
            *p == '\n' || *p == '\r') {
            precisa_aspas = true;
            break;
        }
    }

    // abre o campo entre aspas, quando necessario.
    if (precisa_aspas && fputc('"', arquivo) == EOF) return false;

    // duplica cada aspa encontrada, conforme o escape csv.
    for (const char *p = texto; *p != '\0'; p++) {
        if (*p == '"' && fputc('"', arquivo) == EOF) return false;
        if (fputc(*p, arquivo) == EOF) return false;
    }

    // fecha o campo entre aspas, quando necessario.
    if (precisa_aspas && fputc('"', arquivo) == EOF) return false;

    return true;
}


// consulta e escreve um campo de um item.
// a conversao depende do tipo de valor retornado pela query.
static bool escrever_campo(FILE *arquivo, ITEM *item,
                           const char *campo, char delimitador) {
    ValorQuery *valor = NULL;

    if (!item_ler_campo(item, campo, &valor)) {
        // campo inexistente: escreve uma celula vazia.
        query_valor_apagar(&valor);
        return true;
    }

    bool sucesso = true;

    switch (query_valor_tipo(valor)) {
        case QUERY_TEXTO:
            sucesso = escrever_texto(
                arquivo, query_valor_get_texto(valor), delimitador
            );
            break;

        case QUERY_NUMERO: {
            double numero = query_valor_get_numero(valor);

            // valores nao finitos sao representados por celulas vazias.
            if (isfinite(numero)) {
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "%.17g", numero);
                sucesso = escrever_texto(arquivo, buffer, delimitador);
            }
            break;
        }

        case QUERY_BOOLEANO:
            if (query_valor_get_booleano(valor)) {
                sucesso = escrever_texto(arquivo, "true", delimitador);
            } else {
                sucesso = escrever_texto(arquivo, "false", delimitador);
            }
            break;

        default:
            // um tipo desconhecido nao pode ser exportado.
            sucesso = false;
            break;
    }

    // libera o valor independentemente do resultado da escrita.
    query_valor_apagar(&valor);
    return sucesso;
}


// callback executado uma vez para cada item da estrutura.
static bool escrever_item(ITEM *item, void *contexto) {
    ContextoWriter *writer = contexto;

    // interrompe a exportacao quando o limite top-k e atingido.
    if (writer->top_k > 0 &&
        writer->n_escritos >= (size_t)writer->top_k) {
        return false;
    }

    // escreve cada campo do item na ordem definida pelo cabecalho.
    for (size_t i = 0; i < writer->n_campos; i++) {
        // escreve o delimitador antes de todos os campos, exceto o primeiro.
        if (i > 0 && fputc(writer->delimitador, writer->arquivo) == EOF) {
            writer->sucesso = false;
            return false;
        }

        if (!escrever_campo(
                writer->arquivo, item,
                writer->campos[i], writer->delimitador)) {
            writer->sucesso = false;
            return false;
        }
    }

    // encerra a linha do item.
    if (fputc('\n', writer->arquivo) == EOF) {
        writer->sucesso = false;
        return false;
    }

    writer->n_escritos++;
    return true;
}


// exporta a estrutura para o arquivo configurado.
bool writer_exportar(Estrutura *resultado, const Config *config) {
    if (resultado == NULL || config == NULL) return false;

    const char *entrada = config_get_input(config);
    const char *saida = config_get_output(config);

    if (entrada == NULL || saida == NULL) return false;

    // abre o arquivo de entrada para recuperar seu cabecalho.
    FILE *arquivo_entrada = fopen(entrada, "r");
    if (arquivo_entrada == NULL) {
        perror("Writer: nao foi possivel abrir o arquivo de entrada");
        return false;
    }

    char linha[TAM_CABECALHO];

    // le a primeira linha, que contem os nomes dos campos.
    if (fgets(linha, sizeof(linha), arquivo_entrada) == NULL) {
        fclose(arquivo_entrada);
        return false;
    }

    // verifica se o cabecalho foi truncado pela limitacao do buffer.
    size_t tamanho = strlen(linha);
    if (tamanho == sizeof(linha) - 1 && linha[tamanho - 1] != '\n') {
        fclose(arquivo_entrada);
        return false;
    }

    fclose(arquivo_entrada);

    // remove os caracteres de quebra de linha do cabecalho.
    linha[strcspn(linha, "\r\n")] = '\0';

    // preserva o cabecalho original antes de separar seus campos.
    // separar_cabecalho modifica a linha, substituindo os delimitadores
    // por '\0', portanto nao podemos utiliza-la para escrever o cabecalho.
    char cabecalho[TAM_CABECALHO];
    strcpy(cabecalho, linha);

    // identifica o delimitador utilizado no arquivo original.
    char delimitador = identificar_delimitador(linha);

    char *campos[MAX_CAMPOS];
    size_t n_campos = 0;

    // separa o cabecalho para obter os nomes individuais dos campos.
    if (!separar_cabecalho(linha, delimitador, campos, &n_campos)) {
        return false;
    }

    // abre o arquivo de saida.
    FILE *arquivo_saida = fopen(saida, "w");
    if (arquivo_saida == NULL) {
        perror("Writer: nao foi possivel abrir o arquivo de saida");
        return false;
    }

    // escreve o cabecalho original, sem as modificacoes da separacao.
    if (fprintf(arquivo_saida, "%s\n", cabecalho) < 0) {
        fclose(arquivo_saida);
        return false;
    }

    // inicializa o contexto compartilhado pelo callback.
    ContextoWriter contexto = {
        .arquivo = arquivo_saida,
        .delimitador = delimitador,
        .n_campos = n_campos,
        .n_escritos = 0,
        .top_k = config_get_top_k(config),
        .sucesso = true
    };

    // associa cada nome de campo ao contexto do writer.
    for (size_t i = 0; i < n_campos; i++) {
        contexto.campos[i] = campos[i];
    }

    // percorre os itens e escreve cada um no arquivo de saida.
    int percorridos = estrutura_percorrer(
        resultado, escrever_item, &contexto
    );

    // verifica se o percurso e as operacoes de escrita foram bem-sucedidos.
    bool sucesso = contexto.sucesso && percorridos >= 0;

    if (ferror(arquivo_saida)) sucesso = false;

    // verifica tambem se o fechamento do arquivo foi bem-sucedido.
    if (fclose(arquivo_saida) != 0) sucesso = false;

    return sucesso;
}
