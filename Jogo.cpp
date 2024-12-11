#include <iostream>
#include <vector>
#include <termios.h> 
#include <unistd.h>
#include <cstdlib> 
#include <thread>
#include <semaphore>
#include <mutex>
#include <atomic> // Para sinalizador thread-safe


using namespace std;

std::atomic<bool> gameRunning(true); // Controla a execução da thread
std::mutex semaforo;// Semáforo binário para exclusão mútua
std::mutex outputMutex;            // Mutex para sincronizar saída no terminal

// ADHEMAR MOLON NETO - 14687681
// LUIZ FELLIPE CATUZZI -11871198   
// GUILHERME ANTONIO COSTA BANDEIRA - 14575620
// LUCCA TOMMASO MONZANI - 5342324



//************************************************************************************************************************/


// O código cria um jogo simples de labirinto onde o nosso pequeno arroba aventureiro deve coletar todos os (*) sem tocar nas paredes (#).

// A função `getch` faz com que o arroba aventureiro se movimente melhor ( captura uma tecla pressionada sem precisar apertar ENTER)

// A função `mostrarHistorinha` mostra a historia com um pequeno tutorial 

// A função `printLabirinto` limpa o terminal e mostra o labirinto atualizado, contando tambem os * coletados e a quantidade de passos

// A função `jogarNivel` é o caule do jogo. Ela:
// 1. spawna o nosso pequeno arroba aventureiro no labirinto e o mantém em loop até que o nível seja completado ( ou ele morra :v )
// 2. pega os W,A,S,D do arroba aventureiro e chama a função `processarComando` para lidar com o movimento
// 3. ve se o arroba aventureiro morreu ou completou o nível
// 4. Atualiza o labirinto quando o arroba aventureiro da um passo e termina o nível quando todos os * são coletados.

// A função `contarItens` apenas conta os itens (*) no labirinto para saber quando o nível está completo.

// A função `processarComando` lida com o movimento do jogador, verificando se ele perdeu, pegou um item ou completou o nível.

// Na função `main`, o jogo começa com a introdução, passa pelos três níveis (cada um com um labirinto maior), e ao final, mostra quantos passos o jogador deu no total.

// Resumindo: o jogador usa W, A, S, D para se mover, coleta todos os itens e tenta não encostar nas paredes. Completar os três níveis é a condição de vitória.


//************************************************************************************************************************/



// Declaração dos protótipos das funções
int contarItens(const vector<vector<char>> &labirinto);
bool processarComando(vector<vector<char>> &labirinto, char comando, int &jogadorX, int &jogadorY, int &contador, int &passos, int &totalPassos, int totalItens);

pair<int, int> direcaoAleatoria() {
    // Retorna uma direção aleatória: {dx, dy}
    vector<pair<int, int>> direcoes = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}}; // Cima, Baixo, Esquerda, Direita
    int escolha = rand() % direcoes.size();
    return direcoes[escolha];
}

void movimentarObstaculos(vector<vector<char>> &labirinto, std::mutex &semaforo) {
    while (gameRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Tempo entre movimentos

        semaforo.lock();
        vector<pair<pair<int, int>, pair<int, int>>> movimentos; // Posições para mover

        for (int i = 1; i < labirinto.size() - 1; ++i) {
            for (int j = 1; j < labirinto[i].size() - 1; ++j) {
                if (labirinto[i][j] == '#') {
                    auto [dx, dy] = direcaoAleatoria();
                    int novoX = i + dx, novoY = j + dy;

                    // Verifica se a nova posição é válida
                    if (novoX > 0 && novoX < labirinto.size() - 1 &&
                        novoY > 0 && novoY < labirinto[i].size() - 1 &&
                        labirinto[novoX][novoY] == ' ') {
                        movimentos.push_back({{i, j}, {novoX, novoY}});
                    }
                }
            }
        }

        // Aplica os movimentos após verificar o labirinto
        for (const auto &[origem, destino] : movimentos) {
            labirinto[origem.first][origem.second] = ' ';
            labirinto[destino.first][destino.second] = '#';
        }

        semaforo.unlock();
    }
}





// Função para capturar entrada sem pressionar ENTER
char getch() {
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0) perror("tcgetattr");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0) perror("tcsetattr");
    if (read(0, &buf, 1) < 0) perror("read");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0) perror("tcsetattr");
    return buf;
}

//************************************************************************************************************************/


// função que mostra a historinha do jogo
void mostrarHistorinha() {
    cout << "Bem-vindo ao desafio do labirinto!\n\n";
    cout << "Você é um pequeno arroba aventureiro (@) que, em uma caminhada despretensiosa, se perdeu em um labirinto sombrio. As paredes são feitas de barras de ferro afiadas (#), e você deve evitar encostá-las.\n";
    cout << "Para escapar, você precisa coletar todos os tesouros (*) espalhados pelo labirinto. Mas cuidado! Tocar nas paredes resultará no seu fim.\n";
    cout << "Você pode encerrar o jogo pressionando a tecla Q a qualquer momento.\n";
    cout << "Use as teclas W, A, S e D para se movimentar e boa sorte!\n\n";
    cout << "Pressione ENTER para começar sua jornada...";
    cin.ignore(); // espera o enter do arroba aventueiro
}

// função que imprime o labirinto
void printLabirinto(const vector<vector<char>> &labirinto, int contador, int totalItens, int passos) {
    system("clear"); // fica limpando o terminal ( talvez não funcione no windows )
    for (const auto &linha : labirinto) {
        for (char c : linha) {
            cout << c << ' ';
        }
        cout << endl;
    }
    cout << "\nItens coletados: " << contador << " de " << totalItens;
    cout << " | Passos: " << passos << endl;
}

//************************************************************************************************************************/


// função que joga o nível
void jogarNivel(vector<vector<char>> labirinto, int &totalPassos) {
    int jogadorX = 1, jogadorY = 1;
    int contador = 0;
    int totalItens = contarItens(labirinto);
    int passos = 0;

    gameRunning = true; // Ativa o loop do jogo

    // Thread para movimentar obstáculos
    std::thread obstaculosThread([&]() {
        movimentarObstaculos(labirinto, semaforo);
    });

    // Thread para imprimir o labirinto
    std::thread imprimirThread([&]() {
        while (gameRunning) {
            semaforo.lock();
            printLabirinto(labirinto, contador, totalItens, passos);
            semaforo.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Intervalo para atualização da tela
        }
    });

    // Thread para entrada e movimentação do jogador
    std::thread jogadorThread([&]() {
        while (true) {
            char comando = getch(); // Captura o comando do jogador
            semaforo.lock();
            if (processarComando(labirinto, comando, jogadorX, jogadorY, contador, passos, totalPassos, totalItens)) {
                gameRunning = false; // Encerra o jogo
                semaforo.unlock();
                break;
            }
            semaforo.unlock();
        }
    });

    // Aguarda o término da thread do jogador para encerrar o nível
    jogadorThread.join();

    // Encerra as outras threads
    gameRunning = false;
    obstaculosThread.join();
    imprimirThread.join();

    cin.sync(); // Limpa estado do cin
    cin.get();  // Aguarda ENTER
}


// Conta os itens (*) no labirinto
int contarItens(const vector<vector<char>> &labirinto) {
    int total = 0;
    for (const auto &linha : labirinto) {
        for (char c : linha) {
            if (c == '*') total++;
        }
    }
    return total;
}

//************************************************************************************************************************/


// função que processa o comando do arroba aventueiro
bool processarComando(vector<vector<char>> &labirinto, char comando, int &jogadorX, int &jogadorY, int &contador, int &passos, int &totalPassos, int totalItens) {
    int novoX = jogadorX, novoY = jogadorY;

    if (comando == 'w') novoX--;
    else if (comando == 's') novoX++;
    else if (comando == 'a') novoY--;
    else if (comando == 'd') novoY++;
    else if (comando == 'q') exit(0);

    if (labirinto[novoX][novoY] == '#') {
        system("clear");
        cout << "\nVocê tentou atravessar uma parede afiada e perdeu!\n";
        cout << "Fim de jogo.\n";
        exit(0);
    } else if (labirinto[novoX][novoY] == '*') {
        contador++;
    }

    labirinto[jogadorX][jogadorY] = ' ';
    jogadorX = novoX;
    jogadorY = novoY;
    labirinto[jogadorX][jogadorY] = '@';

    passos++;
    totalPassos++;

    if (contador == totalItens) {
        system("clear");
        cout << "\nParabéns! Você completou o nível com " << passos << " passos.\n";
        cout << "Pressione ENTER para continuar para o próximo nível...";
        return true;
    }
    return false;
}


//************************************************************************************************************************/


int main() {
    mostrarHistorinha();

     vector<vector<char>> nivel1 = {
        {'#', '#', '#', '#', '#'},
        {'#', '@', '*', ' ', '#'},
        {'#', ' ', ' ', ' ', '#'},
        {'#', '*', '#', '*', '#'},
        {'#', '#', '#', '#', '#'}};

    vector<vector<char>> nivel2 = {
        {'#', '#', '#', '#', '#', '#', '#', '#', '#', '#'},
        {'#', '@', ' ', '*', ' ', ' ', ' ', '*', ' ', '#'},
        {'#', ' ', ' ', ' ', ' ', ' ', '#', ' ', '#', '#'},
        {'#', ' ', ' ', '*', ' ', ' ', ' ', ' ', ' ', '#'},
        {'#', ' ', '#', ' ', ' ', ' ', ' ', '*', ' ', '#'},
        {'#', ' ', ' ', ' ', '#', '*', ' ', ' ', ' ', '#'},
        {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
        {'#', '*', ' ', '#', ' ', ' ', '#', ' ', '*', '#'},
        {'#', ' ', '*', ' ', ' ', '*', ' ', ' ', ' ', '#'},
        {'#', '#', '#', '#', '#', '#', '#', '#', '#', '#'}};

vector<vector<char>> nivel3 = {
        {'#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#'},
        {'#', '@', ' ', '*', ' ', ' ', ' ', ' ', ' ', '*', ' ', ' ', ' ', ' ', '#', ' ', '*', ' ', '*', '#'},
        {'#', ' ', ' ', ' ', ' ', '#', ' ', ' ', ' ', ' ', ' ', '#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
        {'#', ' ', ' ', '*', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '*', ' ', '#', '*', ' ', ' ', '#', ' ', '#'},
        {'#', ' ', '#', ' ', ' ', ' ', ' ', '#', ' ', ' ', '*', ' ', ' ', ' ', ' ', '#', ' ', ' ', '*', '#'},
        {'#', ' ', '*', ' ', ' ', '*', ' ', ' ', ' ', ' ', ' ', '#', ' ', ' ', ' ', ' ', '*', ' ', ' ', '#'},
        {'#', ' ', ' ', ' ', '#', ' ', ' ', ' ', ' ', '#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
        {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '*', ' ', ' ', '#', ' ', '*', ' ', '#', ' ', '#'},
        {'#', ' ', '#', ' ', ' ', ' ', ' ', '#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '*', ' ', '#'},
        {'#', ' ', '*', ' ', '*', ' ', ' ', ' ', '*', ' ', ' ', ' ', ' ', ' ', '*', '#', ' ', ' ', '#', '#'},
        {'#', ' ', '#', ' ', '#', ' ', ' ', '#', ' ', ' ', ' ', '#', '*', '#', ' ', ' ', ' ', ' ', ' ', '#'},
        {'#', '*', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#', ' ', ' ', ' ', '*', ' ', ' ', '*', ' ', ' ', '#'},
        {'#', ' ', ' ', ' ', ' ', ' ', '*', ' ', ' ', ' ', ' ', ' ', ' ', '#', ' ', '#', ' ', '*', ' ', '#'},
        {'#', ' ', ' ', ' ', '#', ' ', ' ', ' ', ' ', '#', ' ', '#', ' ', '*', ' ', '*', ' ', ' ', ' ', '#'},
        {'#', '*', '#', ' ', ' ', ' ', ' ', '#', ' ', '*', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '*', ' ', '#'},
        {'#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#'}};

        
    int totalPassos = 0;

    cout << "\nIniciando Nível 1...\n";
    jogarNivel(nivel1, totalPassos);

    cout << "\nIniciando Nível 2...\n";
    jogarNivel(nivel2, totalPassos);

    cout << "\nIniciando Nível 3...\n";
    jogarNivel(nivel3, totalPassos);

    cout << "\nParabéns! Você completou o jogo com um total de " << totalPassos << " passos.\n";
    return 0;
}