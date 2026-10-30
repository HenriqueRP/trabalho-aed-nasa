param(
    [string]$DiretorioSaida = (Join-Path ([System.IO.Path]::GetTempPath()) ('aed-testes-' + [guid]::NewGuid()))
)

$ErrorActionPreference = 'Stop'
$taskProjeto = Split-Path -Parent $PSScriptRoot
New-Item -ItemType Directory -Force -Path $DiretorioSaida | Out-Null
$taskSaida = (Resolve-Path -LiteralPath $DiretorioSaida).Path

# Cada executavel liga apenas os modulos necessarios, sem depender do main incompleto
$taskIncludes = Get-ChildItem -LiteralPath $taskProjeto -Recurse -Filter '*.h' |
    ForEach-Object { $_.DirectoryName } | Sort-Object -Unique |
    ForEach-Object { '-I' + $_ }
$taskComuns = @(
    (Join-Path $taskProjeto 'eds/tad_item/item.c'),
    (Join-Path $taskProjeto 'query/query.c')
)
$taskCasos = @(
    @{Nome = 'lista_cabeca'; Fontes = @('eds/lista_cabeca/lista_cabeca.c')},
    @{Nome = 'lista_ordenada'; Fontes = @('eds/lista_ordenada/lista_ordenada.c')},
    @{
        Nome = 'integracao_listas'
        Argumentos = @((Join-Path $PSScriptRoot 'consulta_lista_ordenada.txt'))
        Fontes = @(
            'eds/estrutura/estrutura.c',
            'eds/fila/fila.c',
            'eds/pilha/pilha.c',
            'eds/lista_encadeada/lista_encadeada.c',
            'eds/lista_sequencial/lista_sequencial.c',
            'eds/lista_generalizada/lista_generalizada.c',
            'eds/listas_cruzada/lista_cruzada.c',
            'eds/lista_cabeca/lista_cabeca.c',
            'eds/lista_ordenada/lista_ordenada.c',
            'eds/tad_item_nasa/item_nasa.c'
        )
    }
)

foreach ($taskCaso in $taskCasos) {
    $taskExecutavel = Join-Path $taskSaida ('test_' + $taskCaso.Nome + '.exe')
    $taskFontes = @((Join-Path $PSScriptRoot ('test_' + $taskCaso.Nome + '.c')))
    $taskFontes += $taskComuns
    $taskFontes += $taskCaso.Fontes | ForEach-Object { Join-Path $taskProjeto $_ }

    & gcc -std=c11 -Wall -Wextra -pedantic -Werror -g @taskIncludes @taskFontes -lm -o $taskExecutavel
    if ($LASTEXITCODE -ne 0) { throw ('Falha ao compilar ' + $taskCaso.Nome) }
    $taskArgumentos = @($taskCaso.Argumentos | Where-Object { $null -ne $_ })
    & $taskExecutavel @taskArgumentos
    if ($LASTEXITCODE -ne 0) { throw ('Falha ao executar ' + $taskCaso.Nome) }
}
