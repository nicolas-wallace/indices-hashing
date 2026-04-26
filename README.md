# Trabalho 2 — SGBD: Índice Hash Extensível

## Descrição

Implementação de um índice **Hash Extensível** sobre o atributo `LinhaNum` de uma
tabela que armazena trechos do livro *Dom Casmurro*.

O banco de dados é composto por três componentes em disco:

| Arquivo | Conteúdo |
|---|---|
| `data/bd-trab2-dataset.csv` | Arquivo de dados: cada linha é uma tupla `LinhaNum\tLinhaTexto` |
| `index/diretorio.txt` | Diretório do índice: profundidade global + vetor de referências para buckets |
| `index/<id>.txt` | Arquivos de bucket: profundidade local + até 5 entradas (`LinhaNum`) por arquivo |

### Restrição de memória

Somente **uma página de dados** (bucket) pode estar em memória por vez. Essa
restrição é garantida pelo uso do `fstream pagina` global declarado em
`src/hash_index.h`. O diretório é a única exceção e pode ficar inteiro em memória.

---

## Estrutura de arquivos

```
sgbd_hash/
├── Makefile
├── README.md
├── src/
│   ├── main.cpp          # Ponto de entrada: lê in.txt, despacha operações, gera out.txt
│   ├── hash_index.h      # Estruturas (Diretorio) e protótipos de todas as funções
│   ├── hash_index.cpp    # Função hash, gerenciamento do diretório, operação BUS=
│   └── operacoes.cpp     # Operações INC e REM (implementadas pelo colega)
├── data/                 # Criada pelo usuário — colocar os arquivos de entrada aqui
│   ├── bd-trab2-dataset.csv
│   └── in.txt
├── index/                # Criada automaticamente pelo programa
│   ├── diretorio.txt
│   └── *.txt             # Um arquivo por bucket (2^PG buckets no total)
├── out.txt               # Gerado automaticamente pelo programa
└── sgbd_hash             # Executável gerado pelo make
```

---

## Compilação

```bash
make
```

O executável gerado se chama `sgbd_hash` e fica na raiz do projeto.

---

## Execução

Coloque o dataset e o arquivo de comandos dentro de `data/`:

```
data/
├── bd-trab2-dataset.csv
└── in.txt
```

Depois execute a partir da raiz do projeto:

```bash
./sgbd_hash
```

O programa lê `data/in.txt`, cria a pasta `index/` automaticamente e gera `out.txt`.

Também é possível passar o caminho do arquivo de entrada como argumento:

```bash
./sgbd_hash data/meus_comandos.csv
```

---

## Formato de entrada (`data/in.txt`)

```
PG/<profundidade global inicial>
INC:x
REM:x
BUS=:x
...
```

## Formato de saída (`out.txt`)

```
PG/<profundidade global inicial>
DUP DIR:/<pg>,<pl>          <- somente quando INC duplica o diretório (antes da linha INC)
INC:x/<pg>,<pl>
REM:x/<qtd removidas>,<pg>,<pl>
BUS:x/<qtd selecionadas>
...
P:/<profundidade global final>
```

---

## Função Hash

```
funcaoHash(x, pg) = x & ((1 << pg) - 1)
```

Retorna os `pg` bits menos significativos de `x`, que é o índice da entrada do
diretório correspondente à chave `x`.

---

## Divisão de responsabilidades

| Arquivo | Responsável | Conteúdo |
|---|---|---|
| `src/main.cpp` | Aluno 1 | Loop de entrada, despacho de operações, escrita da primeira e última linha |
| `src/hash_index.cpp` | Aluno 1 | `funcaoHash`, `inicializarDiretorio`, `salvarDiretorio`, `operacaoBUS` |
| `src/operacoes.cpp` | Aluno 2 | `operacaoINC`, `operacaoREM` |

---

## Limpeza

```bash
make clean      # remove objetos (.o) e executável
make cleanall   # remove também out.txt e a pasta index/ inteira
```