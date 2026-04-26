#include "hash_index.h"

// ---------------------------------------------------------------------------
// Definição da variável global.
// REGRA: apenas um bucket pode estar aberto (em memória) por vez.
//        Sempre feche pagina antes de abrir outro arquivo de bucket.
// ---------------------------------------------------------------------------
fstream pagina;

// ---------------------------------------------------------------------------
// Função Hash
// ---------------------------------------------------------------------------

/**
 * Retorna os 'pg' bits menos significativos de x, aplicando uma máscara de bits.
 *
 *   mascara = (1 << pg) - 1   →  pg=4 → 0b00001111 = 15
 *   resultado = x & mascara
 *
 * Esse valor é o índice da entrada do diretório para a chave x.
 */
unsigned int funcaoHash(unsigned int x, unsigned int pg) {
    unsigned int mascara = (1u << pg) - 1u;
    return x & mascara;
}

// ---------------------------------------------------------------------------
// Gerenciamento do Diretório
// ---------------------------------------------------------------------------

/**
 * Tenta abrir ARQUIVO_DIRETORIO e carregar o diretório existente.
 * Caso não exista, cria um diretório do zero:
 *   - profundidade global = pg_inicial
 *   - 2^pg_inicial entradas, cada uma apontando para um bucket próprio
 *   - cada bucket é criado como arquivo vazio com sua profundidade local
 */
Diretorio inicializarDiretorio(unsigned int pg_inicial) {
    Diretorio dir;

    // Garante que a pasta index/ existe antes de qualquer leitura/escrita
    filesystem::create_directories(INDEX_DIR);

    ifstream arqDir(ARQUIVO_DIRETORIO);

    if (arqDir.is_open()) {
        // ----------------------------------------------------------------
        // Diretório já existe: carrega PG e todas as referências de bucket
        // ----------------------------------------------------------------
        string linha;

        getline(arqDir, linha);
        dir.pg = stoul(linha);  // primeira linha = profundidade global

        while (getline(arqDir, linha)) {
            if (!linha.empty())
                dir.bucket_ref.push_back(stoul(linha));
        }
        arqDir.close();

        cout << "[INFO] Diretório carregado do disco."
             << " PG=" << dir.pg
             << " | Entradas=" << dir.bucket_ref.size() << endl;

    } else {
        // ----------------------------------------------------------------
        // Diretório novo: cria estrutura inicial
        // ----------------------------------------------------------------
        dir.pg = pg_inicial;
        unsigned int numEntradas = 1u << pg_inicial;  // 2^pg_inicial

        cout << "[INFO] Criando novo diretório."
             << " PG=" << pg_inicial
             << " | Entradas=" << numEntradas << endl;

        for (unsigned int i = 0; i < numEntradas; i++) {
            dir.bucket_ref.push_back(i);  // entrada i → bucket i

            // Cria o arquivo do bucket i dentro de index/ (uma página por vez via 'pagina')
            pagina.open(INDEX_DIR + to_string(i) + ".txt", ios::out);
            if (!pagina.is_open()) {
                cerr << "[ERRO] Não foi possível criar o bucket " << i << ".txt" << endl;
                exit(1);
            }
            // Primeira linha do bucket = profundidade local
            pagina << pg_inicial << "\n";
            pagina.close();  // libera a página antes de abrir a próxima
        }

        // Persiste o diretório recém-criado
        salvarDiretorio(dir);
    }

    return dir;
}

/**
 * Grava o diretório completo em ARQUIVO_DIRETORIO.
 * Formato:
 *   <pg>
 *   <bucket_ref[0]>
 *   <bucket_ref[1]>
 *   ...
 *
 * Usa ofstream separado (o diretório é exceção à regra de uma página por vez).
 */
void salvarDiretorio(const Diretorio& dir) {
    ofstream arqDir(ARQUIVO_DIRETORIO);
    if (!arqDir.is_open()) {
        cerr << "[ERRO] Não foi possível salvar o diretório." << endl;
        return;
    }

    arqDir << dir.pg << "\n";
    for (unsigned int ref : dir.bucket_ref)
        arqDir << ref << "\n";

    arqDir.close();
}

// ---------------------------------------------------------------------------
// Operação BUS= (Busca por Igualdade)
// ---------------------------------------------------------------------------

/**
 * Algoritmo:
 *  1. Calcula o índice do diretório: idx = funcaoHash(x, dir.pg)
 *  2. Recupera o id do bucket:       bucketId = dir.bucket_ref[idx]
 *  3. Abre o arquivo <bucketId>.txt  (única página em memória)
 *  4. Lê a profundidade local (primeira linha)
 *  5. Percorre as entradas buscando x
 *  6. Fecha o bucket
 *  7. Se encontrou, abre o CSV e recupera o LinhaTexto correspondente
 *  8. Registra o resultado
 *
 * Saída no arqSaida: BUS:x/<quantidade de tuplas selecionadas>
 * (será 0 ou 1, pois LinhaNum é chave primária)
 */
void operacaoBUS(unsigned int x, const Diretorio& dir, ofstream& arqSaida) {

    // Passo 1 — índice no diretório
    unsigned int idx = funcaoHash(x, dir.pg);

    // Passo 2 — bucket correspondente
    unsigned int bucketId = dir.bucket_ref[idx];
    string nomeBucket = INDEX_DIR + to_string(bucketId) + ".txt";

    // Passo 3 — carrega o bucket (única página em memória por vez)
    pagina.open(nomeBucket, ios::in);
    if (!pagina.is_open()) {
        // Bucket não existe: nenhuma tupla encontrada
        cout << "BUS:" << x << "/0" << endl;
        arqSaida << "BUS:" << x << "/0\n";
        return;
    }

    // Passo 4 — lê a profundidade local (não usada na busca, mas é a 1ª linha)
    string linha;
    getline(pagina, linha);
    // unsigned int pl = stoul(linha);  // disponível se necessário

    // Passo 5 — busca linear pelas entradas do bucket
    int encontrado = 0;
    while (getline(pagina, linha)) {
        if (!linha.empty() && stoul(linha) == x) {
            encontrado = 1;
            break;  // chave primária: no máximo uma ocorrência
        }
    }

    // Passo 6 — fecha o bucket antes de abrir qualquer outro arquivo
    pagina.close();

    // Passo 7 — se a chave foi encontrada no índice, recupera o texto no CSV.
    // O bucket só armazena o LinhaNum (RID); o conteúdo completo da tupla
    // está no arquivo de dados original. Abrimos o CSV com 'pagina' para
    // manter a restrição de uma única página em memória por vez.
    if (encontrado) {
        pagina.open(ARQUIVO_TEXTO, ios::in);
        if (!pagina.is_open()) {
            cerr << "[AVISO] Não foi possível abrir " << ARQUIVO_TEXTO
                 << " para recuperar o texto da linha " << x << "." << endl;
        } else {
            // Descarta o cabeçalho "key\ttext"
            getline(pagina, linha);

            // Percorre o CSV até encontrar a linha cujo key == x.
            // O separador entre key e text é tabulação ('\t').
            bool achou = false;
            while (getline(pagina, linha)) {
                size_t tab = linha.find('\t');
                if (tab == string::npos) continue; // linha malformada, ignora

                unsigned int key = stoul(linha.substr(0, tab));
                if (key == x) {
                    string texto = linha.substr(tab + 1);
                    cout << "  -> Tupla recuperada: [" << key << "] " << texto << endl;
                    achou = true;
                    break;
                }
            }

            pagina.close(); // libera o CSV da memória

            if (!achou)
                cerr << "[AVISO] Chave " << x
                     << " encontrada no índice mas ausente no CSV." << endl;
        }
    }

    // Passo 8 — registra o resultado no terminal e no arquivo de saída
    cout << "BUS:" << x << "/" << encontrado << endl;
    arqSaida << "BUS:" << x << "/" << encontrado << "\n";
}