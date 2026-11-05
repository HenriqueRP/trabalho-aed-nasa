# Trabalho 1 — Algoritmos e Estruturas de Dados I

Projeto da disciplina SCC0202. O programa foi desenvolvido em C para carregar registros de arquivos CSV, realizar consultas e salvar os resultados em outro CSV. Foram utilizadas duas bases de dados: asteroides (NASA) e obras audiovisuais (ANCINE).

## Compilação

Na pasta principal do projeto, execute:

```bash
make
```

Isso gera o executável `trabalho`. Para remover os arquivos de compilação, use `make clean`.

## Execução

Exemplo com a base NASA:

```bash
./trabalho --reader nasa --input dados/asteroids.csv --base lista_enc --result lista_ord --query consultas/q1.txt --output resultado.csv --top-k 100 --limit 10000
```

Exemplo com a base ANCINE:

```bash
./trabalho --reader ancine --input dados/ancine.csv --base lista_gen --result lista_cruz --query consultas/consulta_ancine_2.txt --output resultado_ancine.csv --top-k 50 --limit 0
```

Os parâmetros `--base` e `--result` definem, respectivamente, a estrutura usada para armazenar os dados e a estrutura usada para guardar os resultados. As opções disponíveis são `pilha`, `fila`, `lista_seq`, `lista_enc`, `lista_cabeca`, `lista_ord`, `lista_gen` e `lista_cruz`.

O parâmetro `--limit` limita a quantidade de registros carregados e `--top-k` limita a quantidade exportada. O valor `0` indica que não há limite.

## Arquivos

- `src/`: código-fonte, TADs, leitores, busca e escrita dos resultados.
- `dados/`: arquivos CSV das duas bases.
- `consultas/`: consultas utilizadas nas buscas.
- `Makefile`: compilação do projeto.

As consultas são arquivos de texto e podem conter comandos `FILTER`, `SIM`, `ACCEPT`, `ORDER`, `GROUP` e `CROSS`. Ao final, o programa informa a quantidade de registros carregados, encontrados e exportados, além dos tempos de execução.
