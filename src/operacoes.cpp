#include "hash_index.h"
#include <vector>
#include <algorithm>

using namespace std;

// ===========================================================================
// operacoes.cpp — Implementação das operações INC e REM
//
// Regras obrigatórias:
//   • Use sempre 'pagina' (fstream global) para ler/escrever buckets.
//   • Feche 'pagina' ANTES de abrir qualquer outro arquivo.
//   • O diretório (struct Diretorio) pode ficar em memória normalmente.
//   • Chame salvarDiretorio(dir) sempre que modificar dir.
//
// Formato dos arquivos de bucket (<id>.txt):
//   Linha 1      = profundidade local (PL)
//   Linhas 2..N  = entradas (LinhaNum), até BUCKET_CAP entradas por bucket.
// ===========================================================================


// ---------------------------------------------------------------------------
// Funções auxiliares (static = visíveis apenas neste arquivo)
// ---------------------------------------------------------------------------

/**
 * Retorna o próximo ID de bucket disponível.
 * Encontra o maior ID existente no diretório e soma 1.
 */
static unsigned int proximoBucketId(const Diretorio& dir) {
    unsigned int maxId = 0;
    for (unsigned int ref : dir.bucket_ref)
        if (ref > maxId) maxId = ref;
    return maxId + 1;
}

/**
 * Lê o bucket que está aberto em 'pagina' (ios::in já aberto pelo chamador).
 * Preenche o vetor 'entries' com as chaves encontradas.
 * Retorna a profundidade local (primeira linha do arquivo).
 */
static unsigned int lerBucket(vector<unsigned int>& entries) {
    string linha;
    getline(pagina, linha);
    unsigned int pl = stoul(linha);
    while (getline(pagina, linha))
        if (!linha.empty())
            entries.push_back(stoul(linha));
    return pl;
}

/**
 * Cria ou sobrescreve um arquivo de bucket com o pl e as entradas fornecidas.
 * Abre, escreve e fecha 'pagina' — garante que só um bucket fica em memória.
 */
static void escreverBucket(const string& nome, unsigned int pl,
                            const vector<unsigned int>& entries) {
    pagina.open(nome, ios::out);
    pagina << pl << "\n";
    for (unsigned int e : entries)
        pagina << e << "\n";
    pagina.close();
}

/**
 * Duplica o diretório: dobra o número de entradas e incrementa PG.
 *
 * A duplicação é feita CONCATENANDO o vetor consigo mesmo:
 *   [A, B, C, D]  →  [A, B, C, D, A, B, C, D]
 *
 * Isso preserva a propriedade de que funcaoHash(x, novo_pg) aponta para
 * o mesmo bucket que funcaoHash(x, pg) apontava antes. Entradas na segunda
 * metade que precisarem apontar para um novo bucket serão atualizadas pelo
 * splitBucket logo em seguida.
 *
 * Escreve DUP DIR no arqSaida ANTES da linha INC:x.
 *
 * @param pl  Profundidade local do bucket que causou a duplicação.
 */
static void duplicarDiretorio(Diretorio& dir, unsigned int pl, ofstream& arqSaida) {
    // Concatena o vetor consigo mesmo (NÃO intercala)
    vector<unsigned int> novoRef = dir.bucket_ref;
    novoRef.insert(novoRef.end(), dir.bucket_ref.begin(), dir.bucket_ref.end());

    dir.pg++;
    dir.bucket_ref = novoRef;
    salvarDiretorio(dir);

    // Linha DUP DIR é escrita ANTES da linha INC:x
    cout     << "DUP DIR:/" << dir.pg << "," << pl << endl;
    arqSaida << "DUP DIR:/" << dir.pg << "," << pl << "\n";
}

/**
 * Divide (split) o bucket apontado pelo índice 'idx' do diretório.
 *
 * Algoritmo:
 *   1. Lê o bucket atual (pl, entries).
 *   2. Se pl == pg, duplica o diretório primeiro.
 *   3. Incrementa pl → novoPl.
 *   4. Redistribui as entradas pelo bit de posição 'pl' (0-indexed):
 *        (e >> pl) & 1 == 0  →  fica no bucket antigo
 *        (e >> pl) & 1 == 1  →  vai para o bucket novo
 *   5. Atualiza o diretório: entradas que apontavam para o bucket antigo
 *      e têm bit 'pl' = 1 passam a apontar para o novo bucket.
 *   6. Persiste os dois buckets e o diretório.
 *
 * @param chave  Chave que disparou o split — usada para recalcular idx
 *               após uma possível duplicação do diretório.
 * @return       Profundidade local do bucket resultante (novoPl).
 */
static unsigned int splitBucket(unsigned int idx, Diretorio& dir,
                                 ofstream& arqSaida, unsigned int chave) {
    unsigned int bucketId = dir.bucket_ref[idx];
    string nomeBucket     = INDEX_DIR + to_string(bucketId) + ".txt";

    // Passo 1 — lê o bucket que será dividido
    pagina.open(nomeBucket, ios::in);
    vector<unsigned int> entries;
    unsigned int pl = lerBucket(entries);
    pagina.close();

    // Passo 2 — duplica o diretório se necessário (pl == pg)
    if (pl == dir.pg) {
        duplicarDiretorio(dir, pl, arqSaida);
        // Recalcula idx com o novo PG (o bucket apontado não muda)
        idx      = funcaoHash(chave, dir.pg);
        bucketId = dir.bucket_ref[idx];
    }

    // Passo 3 — nova profundidade local e novo bucket
    unsigned int novoPl       = pl + 1;
    unsigned int novoBucketId = proximoBucketId(dir);
    string nomeNovoBucket     = INDEX_DIR + to_string(novoBucketId) + ".txt";

    // Passo 4 — redistribui as entradas pelo bit na posição 'pl'
    vector<unsigned int> entriesAntigo, entriesNovo;
    for (unsigned int e : entries) {
        if ((e >> pl) & 1u)
            entriesNovo.push_back(e);
        else
            entriesAntigo.push_back(e);
    }

    // Passo 5 — persiste os dois buckets (um de cada vez via 'pagina')
    escreverBucket(nomeBucket,     novoPl, entriesAntigo);
    escreverBucket(nomeNovoBucket, novoPl, entriesNovo);

    // Passo 6 — atualiza o diretório:
    //   entradas que apontavam para bucketId E têm bit 'pl' = 1 → novoBucketId
    for (unsigned int i = 0; i < dir.bucket_ref.size(); i++) {
        if (dir.bucket_ref[i] == bucketId && ((i >> pl) & 1u))
            dir.bucket_ref[i] = novoBucketId;
    }
    salvarDiretorio(dir);

    return novoPl;
}


// ---------------------------------------------------------------------------
// operacaoINC — Insere a chave x no índice hash extensível
// ---------------------------------------------------------------------------

/**
 * Algoritmo:
 *   1. Calcula o bucket destino via funcaoHash.
 *   2. Se o bucket tem espaço: insere e registra o resultado.
 *   3. Se o bucket está cheio: chama splitBucket e tenta inserir novamente.
 *      O loop trata splits sucessivos (quando todas as chaves redistribuídas
 *      caem no mesmo lado e o novo bucket também fica cheio).
 *
 * Saída:
 *   [DUP DIR:/<pg>,<pl>\n]   ← escrito dentro de splitBucket quando necessário
 *   INC:x/<pg>,<pl>\n
 */
void operacaoINC(unsigned int x, Diretorio& dir, ofstream& arqSaida) {

    while (true) {
        unsigned int idx      = funcaoHash(x, dir.pg);
        unsigned int bucketId = dir.bucket_ref[idx];
        string nomeBucket     = INDEX_DIR + to_string(bucketId) + ".txt";

        // Lê o bucket para verificar capacidade
        pagina.open(nomeBucket, ios::in);
        vector<unsigned int> entries;
        unsigned int pl = lerBucket(entries);
        pagina.close();

        if ((int)entries.size() < BUCKET_CAP) {
            // Há espaço: insere a chave e salva o bucket
            entries.push_back(x);
            escreverBucket(nomeBucket, pl, entries);

            cout     << "INC:" << x << "/" << dir.pg << "," << pl << endl;
            arqSaida << "INC:" << x << "/" << dir.pg << "," << pl << "\n";
            return;
        }

        // Bucket cheio: divide e tenta novamente no próximo ciclo do loop
        splitBucket(idx, dir, arqSaida, x);
    }
}


// ---------------------------------------------------------------------------
// operacaoREM — Remove a chave x do índice hash extensível
// ---------------------------------------------------------------------------

/**
 * Algoritmo:
 *   1. Calcula o bucket via funcaoHash.
 *   2. Lê o bucket e procura x.
 *   3. Se encontrou: remove, reescreve o bucket sem x, registra qtd=1.
 *   4. Se não encontrou: registra qtd=0 (bucket não é alterado).
 *
 * Merge de buckets após remoção é opcional e não foi implementado.
 *
 * Saída:
 *   REM:x/<qtd removidas>,<pg>,<pl>\n
 */
void operacaoREM(unsigned int x, Diretorio& dir, ofstream& arqSaida) {

    unsigned int idx      = funcaoHash(x, dir.pg);
    unsigned int bucketId = dir.bucket_ref[idx];
    string nomeBucket     = INDEX_DIR + to_string(bucketId) + ".txt";

    // Lê o bucket
    pagina.open(nomeBucket, ios::in);
    if (!pagina.is_open()) {
        cout     << "REM:" << x << "/0," << dir.pg << ",0" << endl;
        arqSaida << "REM:" << x << "/0," << dir.pg << ",0\n";
        return;
    }
    vector<unsigned int> entries;
    unsigned int pl = lerBucket(entries);
    pagina.close();

    // Procura e remove x (no máximo uma ocorrência — chave primária)
    int qtdRemovida = 0;
    auto it = find(entries.begin(), entries.end(), x);
    if (it != entries.end()) {
        entries.erase(it);
        qtdRemovida = 1;
        escreverBucket(nomeBucket, pl, entries);
    }

    cout     << "REM:" << x << "/" << qtdRemovida << "," << dir.pg << "," << pl << endl;
    arqSaida << "REM:" << x << "/" << qtdRemovida << "," << dir.pg << "," << pl << "\n";
}