/*
 * Grau B - DUNGEON ESCAPE ISOMÉTRICO
 *
 * O cavaleiro precisa coletar todas as moedas e pisar em todas as placas da masmorra.
 * Quando as duas coisas estão completas, a saída vira escada; pisar nela vence o jogo.
 * Buracos, espinhos levantados e monstros tiram vidas; sem vidas (ou sem tempo) é derrota.
 *
 * Tilemap isométrico diamond:
 *   - map.txt segue o formato do enunciado (tileset, tamanho do tile, linhas/colunas e
 *     a matriz de índices) e depois traz as regras (caminháveis, perigosos, troca de tile,
 *     saída, vitória, animação) e os objetos (jogador, moedas, poções, inimigos).
 *   - Posição na tela do ponto (linha, coluna) do grid:
 *         x = origem.x + (coluna - linha) * largura/2
 *         y = origem.y + (coluna + linha) * altura/2
 *     e a conversão inversa (usada pelo mouse) desfaz essa conta.
 *   - Ordem de desenho: primeiro o chão; depois paredes e personagens ordenados por
 *     profundidade (linha + coluna): quem está mais "no fundo" é desenhado antes.
 *
 * Controles:
 *   setas / WASD (combinadas fazem as diagonais) ou Q E Z C - move nas 8 direções
 *   ENTER - começa   P - pausa   R - reinicia   H - grade   mouse - mostra o tile   ESC - sai
 *   V - gira a visão 90°   G - modo invencível (para testes: não leva dano)
 */

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// ---------------- Configurações ----------------
const int ESCALA = 2;                 // cada pixel do pack vira 2x2 na tela
const int WIDTH = 1200, HEIGHT = 720;
const float HUD = 48.0f;
const float VEL_JOGADOR = 170.0f;     // px/s na tela (passos curtos e longos levam o tempo proporcional)
const int VIDAS = 3;
const float TEMPO_LIMITE = 240.0f;
const float INVULNERAVEL = 1.5f;
const float FPS_PERSONAGEM = 8.0f;
const int ESPELHAR = 1;
const int TILE_CURSOR = 16;           // contorno do losango (último tile do tileset)

// ---------------- Fonte bitmap 5x7 ----------------
const std::string CARACTERES = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:!.-/+ ";
const char *FONTE[][7] = {
    {".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}, // A
    {"####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."}, // B
    {".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."}, // C
    {"####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."}, // D
    {"#####", "#....", "#....", "####.", "#....", "#....", "#####"}, // E
    {"#####", "#....", "#....", "####.", "#....", "#....", "#...."}, // F
    {".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####"}, // G
    {"#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}, // H
    {".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."}, // I
    {"..###", "...#.", "...#.", "...#.", "#..#.", "#..#.", ".##.."}, // J
    {"#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"}, // K
    {"#....", "#....", "#....", "#....", "#....", "#....", "#####"}, // L
    {"#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"}, // M
    {"#...#", "#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#"}, // N
    {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}, // O
    {"####.", "#...#", "#...#", "####.", "#....", "#....", "#...."}, // P
    {".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"}, // Q
    {"####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"}, // R
    {".####", "#....", "#....", ".###.", "....#", "....#", "####."}, // S
    {"#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."}, // T
    {"#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}, // U
    {"#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."}, // V
    {"#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#."}, // W
    {"#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"}, // X
    {"#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."}, // Y
    {"#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"}, // Z
    {".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."}, // 0
    {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."}, // 1
    {".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"}, // 2
    {"####.", "....#", "....#", ".###.", "....#", "....#", "####."}, // 3
    {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."}, // 4
    {"#####", "#....", "####.", "....#", "....#", "#...#", ".###."}, // 5
    {".###.", "#....", "#....", "####.", "#...#", "#...#", ".###."}, // 6
    {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."}, // 7
    {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."}, // 8
    {".###.", "#...#", "#...#", ".####", "....#", "....#", ".###."}, // 9
    {".....", "..#..", "..#..", ".....", "..#..", "..#..", "....."}, // :
    {"..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.."}, // !
    {".....", ".....", ".....", ".....", ".....", ".....", "..#.."}, // .
    {".....", ".....", ".....", "#####", ".....", ".....", "....."}, // -
    {"....#", "....#", "...#.", "..#..", ".#...", "#....", "#...."}, // /
    {".....", "..#..", "..#..", "#####", "..#..", "..#..", "....."}, // +
    {".....", ".....", ".....", ".....", ".....", ".....", "....."}, // espaço
};

// ---------------- Tipos ----------------
enum Estado { INICIO, JOGANDO, VITORIA, DERROTA };
enum Modo { VAIVEM, CIRCUITO };

struct Textura {
    GLuint id = 0;
    int largura = 1, altura = 1;
};

// Tudo o que vem do map.txt
struct Mapa {
    std::string tileset;
    int numTiles = 0, tileW = 0, tileH = 0;
    int linhas = 0, colunas = 0;
    std::vector<std::vector<int>> tiles;  // índice de cada posição (muda com as trocas)
    std::set<int> caminhaveis, perigosos, vitoria;
    std::map<int, int> trocas;            // tile pisado -> tile novo
    int saidaFechada = -1, saidaAberta = -1;
    float fpsAnimacao = 0.0f;
    std::vector<int> sequenciaAnimacao;   // quadros dos tiles animados (espinhos)
};

struct Inimigo {
    std::string sprite;
    Modo modo;
    float vel;
    std::vector<glm::vec2> pontos;  // (linha, coluna) do centro dos tiles
    glm::vec2 pos;
    int alvo = 0, sentido = 1;
    bool esquerda = false;
};

struct Item {
    glm::ivec2 tile;
    bool pego;
};

struct Jogador {
    glm::ivec2 tile, inicio;
    glm::vec2 pos, origem;  // posição em (linha, coluna); centro do tile = tile + 0.5
    bool movendo = false, esquerda = false;
    float progresso = 0.0f, duracaoPasso = 1.0f, invulneravel = 0.0f;
    std::string direcao = "-";
};

// ---------------- Estado do jogo ----------------
Mapa mapa;
std::vector<std::vector<int>> tilesIniciais;
std::vector<Inimigo> inimigosIniciais, inimigos;
std::vector<Item> moedas, pocoes;
Jogador jogador;
Estado estado = INICIO;
float tempo = 0.0f, tempoEstado = 0.0f, tempoRestante = TEMPO_LIMITE, flashDano = 0.0f;
int vidas = VIDAS, totalPlacas = 0;
bool pausado = false, mostrarGrade = false, saidaAberta = false, invencivel = false;
int rotacao = 0;             // visão girada em 0, 90, 180 ou 270 graus (0..3)
std::string mensagem, motivoDerrota;
float tempoMensagem = 0.0f;
glm::vec2 origem;            // tela: canto de cima do tile (0, 0)
glm::ivec2 tileMouse(-1);

Textura texTileset, texFonte, texMoeda[4], texCoracao, texCoracaoVazio, texPocao, texPlaca;
std::map<std::string, std::vector<Textura>> animacoes;  // "knight_m_run" -> 4 quadros
GLuint VAO;
GLint modelLoc, uvRectLoc, flagsLoc, corLoc, solidoLoc;

// ---------------- Shaders ----------------
const GLchar *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 vp;
uniform mat4 projection;
uniform mat4 model;
uniform vec4 uvRect;   // pedaço da textura: canto (xy) e tamanho (zw)
uniform int flags;     // 1 = espelha na horizontal
out vec2 uv;
void main() {
    vec2 l = vp + 0.5;
    if ((flags & 1) != 0) l.x = 1.0 - l.x;
    uv = uvRect.xy + l * uvRect.zw;
    gl_Position = projection * model * vec4(vp, 0.0, 1.0);
}
)";

const GLchar *fragmentShaderSource = R"(
#version 330 core
in vec2 uv;
uniform sampler2D tex;
uniform vec4 cor;
uniform bool solido;
out vec4 frg;
void main() {
    vec4 c = solido ? cor : texture(tex, uv) * cor;
    if (c.a < 0.01) discard;
    frg = c;
}
)";

// ---------------- Projeção isométrica ----------------
float meiaLargura() { return mapa.tileW * ESCALA / 2.0f; }   // 32 px
float meiaAltura() { return mapa.tileW * ESCALA / 4.0f; }    // 16 px: losango 2:1

// Rotação da visão (tecla V): antes de projetar, o grid é girado de 90° em 90°. O mapa
// continua o mesmo; só muda qual canto fica "no fundo", e o que estava escondido atrás
// de uma parede passa a aparecer. p = (linha, coluna) contínuos, entre 0 e linhas/colunas.
glm::vec2 girar(glm::vec2 p) {
    float L = mapa.linhas, C = mapa.colunas;
    switch (rotacao) {
    case 1: return {p.y, L - p.x};
    case 2: return {L - p.x, C - p.y};
    case 3: return {C - p.y, p.x};
    default: return p;
    }
}

glm::vec2 desgirar(glm::vec2 v) {
    float L = mapa.linhas, C = mapa.colunas;
    switch (rotacao) {
    case 1: return {L - v.y, v.x};
    case 2: return {L - v.x, C - v.y};
    case 3: return {v.y, C - v.x};
    default: return v;
    }
}

// Mesma ideia para um deslocamento (sem a translação): direção da visão -> direção do mapa
glm::ivec2 desgirarDirecao(glm::ivec2 d) {
    switch (rotacao) {
    case 1: return {-d.y, d.x};
    case 2: return -d;
    case 3: return {d.y, -d.x};
    default: return d;
    }
}

// (linha, coluna) já girados -> tela. Valores fracionários dão pontos dentro do tile.
glm::vec2 projetar(glm::vec2 v) {
    return origem + glm::vec2((v.y - v.x) * meiaLargura(), (v.y + v.x) * meiaAltura());
}

// (linha, coluna) do mapa -> tela
glm::vec2 isoParaTela(float linha, float coluna) { return projetar(girar({linha, coluna})); }

// Profundidade para a ordem de desenho: linha + coluna na visão atual
float profundidade(glm::vec2 p) {
    glm::vec2 v = girar(p);
    return v.x + v.y;
}

// Tela -> (linha, coluna) do mapa: resolve a projeção para linha e coluna e desfaz a rotação
glm::ivec2 telaParaIso(glm::vec2 p) {
    float a = (p.x - origem.x) / meiaLargura(), b = (p.y - origem.y) / meiaAltura();
    glm::vec2 v(std::floor((b - a) / 2.0f), std::floor((b + a) / 2.0f));
    return glm::ivec2(glm::floor(desgirar(v + 0.5f)));
}

// Centraliza o losango do mapa na horizontal, logo abaixo do HUD (depende da rotação,
// porque girar 90° troca o número de linhas e colunas da visão)
void atualizarOrigem() {
    int linhasVisao = rotacao % 2 ? mapa.colunas : mapa.linhas, colunasVisao = rotacao % 2 ? mapa.linhas : mapa.colunas;
    origem = glm::vec2(WIDTH / 2.0f + (linhasVisao - colunasVisao) * meiaLargura() / 2.0f, HUD + 40.0f);
}

bool dentroDoMapa(glm::ivec2 t) { return t.x >= 0 && t.y >= 0 && t.x < mapa.linhas && t.y < mapa.colunas; }

// Índice exibido no tile: tiles animados (espinhos) percorrem a sequência com uma
// defasagem que depende da posição, formando uma "onda"
int tileEm(glm::ivec2 t) {
    if (!dentroDoMapa(t)) return -1;
    int base = mapa.tiles[t.x][t.y];
    auto &seq = mapa.sequenciaAnimacao;
    if (!seq.empty() && base == seq[0]) {
        int quadro = (int)(tempo * mapa.fpsAnimacao + (t.x + t.y) * 1.5f);
        return seq[quadro % seq.size()];
    }
    return base;
}

bool caminhavel(glm::ivec2 t) { return mapa.caminhaveis.count(tileEm(t)) > 0; }

// ---------------- Desenho básico ----------------
void desenharQuad(const Textura &t, glm::vec4 uvRect, glm::vec2 centro, glm::vec2 tamanho, int flags = 0,
                  glm::vec4 cor = glm::vec4(1.0f)) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(centro, 0.0f));
    model = glm::scale(model, glm::vec3(tamanho, 1.0f));
    glBindTexture(GL_TEXTURE_2D, t.id);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(uvRectLoc, 1, glm::value_ptr(uvRect));
    glUniform1i(flagsLoc, flags);
    glUniform4fv(corLoc, 1, glm::value_ptr(cor));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void desenharSprite(const Textura &t, glm::vec2 centro, float escala, int flags = 0, glm::vec4 cor = glm::vec4(1.0f)) {
    desenharQuad(t, glm::vec4(0, 0, 1, 1), centro, glm::vec2(t.largura, t.altura) * escala, flags, cor);
}

// Desenha o tile "indice" do tileset na posição (linha, coluna) do mapa
void desenharTile(int indice, glm::ivec2 t, glm::vec4 cor = glm::vec4(1.0f)) {
    int porLinha = texTileset.largura / mapa.tileW;
    float du = (float)mapa.tileW / texTileset.largura, dv = (float)mapa.tileH / texTileset.altura;
    glm::vec4 uv((indice % porLinha) * du, (indice / porLinha) * dv, du, dv);
    // A célula do tileset tem o losango do chão na metade de baixo, então o centro da
    // célula coincide com o canto de cima do losango. Com a visão girada, o "canto de cima"
    // é outro canto do tile, por isso ele é calculado a partir do centro já girado.
    glm::vec2 cantoDeCima = girar(glm::vec2(t) + 0.5f) - 0.5f;
    desenharQuad(texTileset, uv, projetar(cantoDeCima), glm::vec2(mapa.tileW, mapa.tileH) * (float)ESCALA, 0, cor);
}

void desenharRetangulo(glm::vec2 centro, glm::vec2 tamanho, glm::vec4 cor) {
    glUniform1i(solidoLoc, 1);
    desenharQuad(texTileset, glm::vec4(0.0f), centro, tamanho, 0, cor);
    glUniform1i(solidoLoc, 0);
}

float larguraTexto(const std::string &s, float escala) { return s.size() * 6.0f * escala - escala; }

void desenharTexto(const std::string &s, glm::vec2 centro, float escala, glm::vec4 cor) {
    float x = centro.x - larguraTexto(s, escala) / 2.0f;
    for (int passada = 0; passada < 2; passada++) { // sombra e depois o texto
        glm::vec2 desloc = passada == 0 ? glm::vec2(escala) : glm::vec2(0.0f);
        glm::vec4 c = passada == 0 ? glm::vec4(0.0f, 0.0f, 0.0f, 0.7f * cor.a) : cor;
        for (size_t i = 0; i < s.size(); i++) {
            size_t k = CARACTERES.find((char)std::toupper((unsigned char)s[i]));
            if (k == std::string::npos) k = CARACTERES.size() - 1;
            glm::vec4 uv(k * 6.0f / texFonte.largura, 0.0f, 5.0f / texFonte.largura, 1.0f);
            glm::vec2 p(x + i * 6.0f * escala + 2.5f * escala, centro.y);
            desenharQuad(texFonte, uv, p + desloc, glm::vec2(5.0f, 7.0f) * escala, 0, c);
        }
    }
}

std::string formatarTempo(float s) {
    int t = std::max(0, (int)std::ceil(s));
    std::string seg = std::to_string(t % 60);
    return std::to_string(t / 60) + ":" + (seg.size() < 2 ? "0" : "") + seg;
}

void mostrarMensagem(const std::string &m, float duracao = 3.0f) {
    mensagem = m;
    tempoMensagem = duracao;
}

// ---------------- Carregamento ----------------
Textura criarTextura(const unsigned char *dados, int w, int h) {
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // pixel art: sem suavização
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dados);
    glBindTexture(GL_TEXTURE_2D, 0);
    return {id, w, h};
}

Textura carregarTextura(const std::string &caminho) {
    int w, h, canais;
    unsigned char *dados = stbi_load(caminho.c_str(), &w, &h, &canais, 4);
    if (!dados) {
        std::cerr << "Falha ao carregar textura: " << caminho << std::endl;
        return {};
    }
    Textura t = criarTextura(dados, w, h);
    stbi_image_free(dados);
    return t;
}

Textura carregarFrame(const std::string &nome) { return carregarTextura(std::string(FRAMES_DIR) + nome + ".png"); }

// Carrega "<nome>_f0" .. "<nome>_f3" (uma animação do pack 0x72) só uma vez
const std::vector<Textura> &animacao(const std::string &nome) {
    auto it = animacoes.find(nome);
    if (it != animacoes.end()) return it->second;
    std::vector<Textura> quadros;
    for (int i = 0; i < 4; i++) quadros.push_back(carregarFrame(nome + "_f" + std::to_string(i)));
    return animacoes[nome] = quadros;
}

void criarFonte() {
    int n = CARACTERES.size(), largura = n * 6;
    std::vector<unsigned char> px(largura * 7 * 4, 0);
    for (int g = 0; g < n; g++)
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (FONTE[g][y][x] == '#') {
                    unsigned char *p = &px[(y * largura + g * 6 + x) * 4];
                    p[0] = p[1] = p[2] = p[3] = 255;
                }
    texFonte = criarTextura(px.data(), largura, 7);
}

// Lê o map.txt: cabeçalho no formato do enunciado + regras + objetos
bool carregarMapa(const std::string &caminho) {
    std::ifstream arq(caminho);
    if (!arq) {
        std::cerr << "Falha ao abrir o mapa: " << caminho << std::endl;
        return false;
    }
    // Remove comentários (#) de uma linha
    auto limpar = [](std::string l) { return l.substr(0, l.find('#')); };
    std::string linha;
    auto proximaLinha = [&]() {
        while (std::getline(arq, linha)) {
            linha = limpar(linha);
            if (linha.find_first_not_of(" \t\r") != std::string::npos) return true;
        }
        return false;
    };

    proximaLinha();
    std::istringstream(linha) >> mapa.tileset >> mapa.numTiles >> mapa.tileW >> mapa.tileH;
    proximaLinha();
    std::istringstream(linha) >> mapa.linhas >> mapa.colunas;
    mapa.tiles.assign(mapa.linhas, std::vector<int>(mapa.colunas, 0));
    for (int l = 0; l < mapa.linhas && proximaLinha(); l++) {
        std::istringstream ss(linha);
        for (int c = 0; c < mapa.colunas; c++) ss >> mapa.tiles[l][c];
    }

    while (proximaLinha()) {
        std::istringstream ss(linha);
        std::string chave;
        ss >> chave;
        int a, b;
        if (chave == "caminhaveis") while (ss >> a) mapa.caminhaveis.insert(a);
        else if (chave == "perigosos") while (ss >> a) mapa.perigosos.insert(a);
        else if (chave == "vitoria") while (ss >> a) mapa.vitoria.insert(a);
        else if (chave == "troca" && ss >> a >> b) mapa.trocas[a] = b;
        else if (chave == "saida") ss >> mapa.saidaFechada >> mapa.saidaAberta;
        else if (chave == "animacao") {
            ss >> mapa.fpsAnimacao;
            while (ss >> a) mapa.sequenciaAnimacao.push_back(a);
        } else if (chave == "jogador") ss >> jogador.inicio.x >> jogador.inicio.y;
        else if (chave == "moeda" && ss >> a >> b) moedas.push_back({{a, b}, false});
        else if (chave == "pocao" && ss >> a >> b) pocoes.push_back({{a, b}, false});
        else if (chave == "inimigo") {
            Inimigo e{};
            std::string modo;
            ss >> e.sprite >> modo >> e.vel;
            e.modo = modo == "circuito" ? CIRCUITO : VAIVEM;
            float l, c;
            while (ss >> l >> c) e.pontos.push_back({l + 0.5f, c + 0.5f});
            if (e.pontos.empty()) continue;
            e.pos = e.pontos[0];
            e.alvo = e.pontos.size() > 1 ? 1 : 0;
            inimigosIniciais.push_back(e);
        } else std::cerr << "Linha ignorada no mapa: " << linha << std::endl;
    }
    tilesIniciais = mapa.tiles;
    return mapa.linhas >= 1 && mapa.colunas >= 1 && mapa.tileW > 0;
}

// ---------------- Lógica ----------------
int contarTiles(int indice) {
    int n = 0;
    for (auto &l : mapa.tiles) n += (int)std::count(l.begin(), l.end(), indice);
    return n;
}

int placasRestantes() {
    int n = 0;
    for (auto &t : mapa.trocas) n += contarTiles(t.first);
    return n;
}

int moedasRestantes() {
    return (int)std::count_if(moedas.begin(), moedas.end(), [](const Item &m) { return !m.pego; });
}

void reiniciar(bool comecar) {
    mapa.tiles = tilesIniciais;
    inimigos = inimigosIniciais;
    for (auto &m : moedas) m.pego = false;
    for (auto &p : pocoes) p.pego = false;
    jogador.tile = jogador.inicio;
    jogador.pos = glm::vec2(jogador.tile) + 0.5f;
    jogador.movendo = false;
    jogador.invulneravel = 0.0f;
    jogador.direcao = "-";
    vidas = VIDAS;
    totalPlacas = placasRestantes();
    saidaAberta = false;
    tempoRestante = TEMPO_LIMITE;
    flashDano = 0.0f;
    estado = comecar ? JOGANDO : INICIO;
    tempoEstado = 0.0f;
    pausado = false;
    if (comecar) mostrarMensagem("PEGUE AS MOEDAS E PISE NAS PLACAS PARA ABRIR A SAIDA", 4.0f);
}

void terminar(Estado e, const std::string &motivo = "") {
    estado = e;
    tempoEstado = 0.0f;
    motivoDerrota = motivo;
    tempoMensagem = 0.0f;
}

void levarDano(const std::string &causa) {
    vidas--;
    flashDano = 1.0f;
    if (vidas <= 0) { terminar(DERROTA, causa); return; }
    jogador.tile = jogador.inicio;
    jogador.pos = glm::vec2(jogador.tile) + 0.5f;
    jogador.movendo = false;
    jogador.invulneravel = INVULNERAVEL;
    mostrarMensagem("AI! VOCE PERDEU UMA VIDA", 2.0f);
}

// Quando moedas e placas estão completas, troca todos os tiles de saída fechada pela escada
void verificarSaida() {
    if (saidaAberta || moedasRestantes() > 0 || placasRestantes() > 0) return;
    for (auto &l : mapa.tiles)
        for (int &t : l)
            if (t == mapa.saidaFechada) t = mapa.saidaAberta;
    saidaAberta = true;
    mostrarMensagem("A SAIDA SE ABRIU! VA ATE A ESCADA", 4.0f);
}

// Chegou no centro de um tile: troca de tile (placa), vitória
void chegouNoTile() {
    int &t = mapa.tiles[jogador.tile.x][jogador.tile.y];
    auto troca = mapa.trocas.find(t);
    if (troca != mapa.trocas.end()) {
        t = troca->second;
        int faltam = placasRestantes();
        mostrarMensagem(faltam ? "PLACA ATIVADA! FALTAM " + std::to_string(faltam) : "TODAS AS PLACAS ATIVADAS!", 2.0f);
        verificarSaida();
    }
    if (mapa.vitoria.count(tileEm(jogador.tile))) terminar(VITORIA);
}

// Direção na tela (dx, dy em -1..1) -> deslocamento no grid. No diamond, "para cima"
// na tela é (linha-1, coluna-1), "para a direita" é (linha-1, coluna+1) e as diagonais da
// tela andam em só um eixo do grid (ex.: NE = linha-1).
glm::ivec2 direcaoGrid(int dx, int dy) {
    auto sinal = [](int v) { return (v > 0) - (v < 0); };
    return {sinal(dy - dx), sinal(dy + dx)};
}

std::string nomeDirecao(int dx, int dy) {
    static const char *nomes[3][3] = {{"NO", "N", "NE"}, {"O", "-", "L"}, {"SO", "S", "SE"}};
    return nomes[dy + 1][dx + 1];
}

void lerTeclado(GLFWwindow *w, int &dx, int &dy) {
    auto k = [w](int a, int b) { return glfwGetKey(w, a) == GLFW_PRESS || glfwGetKey(w, b) == GLFW_PRESS; };
    dx = (k(GLFW_KEY_RIGHT, GLFW_KEY_D) ? 1 : 0) - (k(GLFW_KEY_LEFT, GLFW_KEY_A) ? 1 : 0);
    dy = (k(GLFW_KEY_DOWN, GLFW_KEY_S) ? 1 : 0) - (k(GLFW_KEY_UP, GLFW_KEY_W) ? 1 : 0);
    if (glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS) { dx = -1; dy = -1; }
    if (glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS) { dx = 1; dy = -1; }
    if (glfwGetKey(w, GLFW_KEY_Z) == GLFW_PRESS) { dx = -1; dy = 1; }
    if (glfwGetKey(w, GLFW_KEY_C) == GLFW_PRESS) { dx = 1; dy = 1; }
}

bool tentarMover(int dx, int dy) {
    // A direção é lida na tela; com a visão girada, ela é "desgirada" para o mapa
    glm::ivec2 d = desgirarDirecao(direcaoGrid(dx, dy)), alvo = jogador.tile + d;
    if (dx != 0) jogador.esquerda = dx < 0;
    jogador.direcao = nomeDirecao(dx, dy);
    if (!caminhavel(alvo)) return false;
    // Nas diagonais do grid não deixa "cortar a quina" entre duas paredes
    if (d.x != 0 && d.y != 0 && (!caminhavel(jogador.tile + glm::ivec2(d.x, 0)) || !caminhavel(jogador.tile + glm::ivec2(0, d.y))))
        return false;
    jogador.origem = jogador.pos;
    jogador.tile = alvo;
    jogador.movendo = true;
    jogador.progresso = 0.0f;
    // A duração depende da distância na tela (andar "para o lado" percorre o dobro de pixels)
    glm::vec2 a = isoParaTela(jogador.origem.x, jogador.origem.y), b = isoParaTela(alvo.x + 0.5f, alvo.y + 0.5f);
    jogador.duracaoPasso = glm::length(b - a) / VEL_JOGADOR;
    return true;
}

void atualizarJogador(float dt, GLFWwindow *w) {
    if (jogador.invulneravel > 0.0f) jogador.invulneravel -= dt;
    int dx, dy;
    if (jogador.movendo) {
        jogador.progresso += dt / jogador.duracaoPasso;
        if (jogador.progresso >= 1.0f) {
            jogador.movendo = false;
            jogador.pos = glm::vec2(jogador.tile) + 0.5f;
            chegouNoTile();
            lerTeclado(w, dx, dy);
            if (estado == JOGANDO && (dx || dy)) tentarMover(dx, dy); // emenda o próximo passo
        }
    } else {
        lerTeclado(w, dx, dy);
        if (dx || dy) tentarMover(dx, dy);
    }
    if (jogador.movendo)
        jogador.pos = glm::mix(jogador.origem, glm::vec2(jogador.tile) + 0.5f, std::min(1.0f, jogador.progresso));
}

void moverInimigo(Inimigo &e, float dt) {
    glm::vec2 alvo = e.pontos[e.alvo], d = alvo - e.pos;
    float dist = glm::length(d), passo = e.vel * dt;
    float dxTela = isoParaTela(alvo.x, alvo.y).x - isoParaTela(e.pos.x, e.pos.y).x; // sentido horizontal na tela
    if (std::abs(dxTela) > 0.01f) e.esquerda = dxTela < 0.0f;
    if (dist > passo) { e.pos += d / dist * passo; return; }
    e.pos = alvo;
    int n = e.pontos.size();
    if (e.modo == CIRCUITO) e.alvo = (e.alvo + 1) % n;
    else if (n > 1) {
        if (e.alvo + e.sentido >= n || e.alvo + e.sentido < 0) e.sentido = -e.sentido;
        e.alvo += e.sentido;
    }
}

void atualizar(float dt, GLFWwindow *w) {
    tempo += dt;
    tempoEstado += dt;
    tempoMensagem -= dt;
    flashDano = std::max(0.0f, flashDano - dt * 2.5f);
    for (auto &e : inimigos) moverInimigo(e, dt);
    if (estado != JOGANDO) return;

    tempoRestante -= dt;
    if (tempoRestante <= 0.0f) { terminar(DERROTA, "O TEMPO ACABOU"); return; }
    atualizarJogador(dt, w);
    if (estado != JOGANDO) return;

    // Coleta: moedas e poções perto do centro do jogador
    for (auto &m : moedas)
        if (!m.pego && glm::length(glm::vec2(m.tile) + 0.5f - jogador.pos) < 0.45f) {
            m.pego = true;
            int faltam = moedasRestantes();
            mostrarMensagem(faltam ? "MOEDA! FALTAM " + std::to_string(faltam) : "TODAS AS MOEDAS COLETADAS!", 2.0f);
            verificarSaida();
        }
    for (auto &p : pocoes)
        if (!p.pego && vidas < VIDAS && glm::length(glm::vec2(p.tile) + 0.5f - jogador.pos) < 0.45f) {
            p.pego = true;
            vidas++;
            mostrarMensagem("POCAO! +1 VIDA", 2.0f);
        }

    // Evitação: tile perigoso sob o jogador (buraco, espinhos levantados) e monstros
    if (jogador.invulneravel > 0.0f || invencivel) return;
    glm::ivec2 sob(glm::floor(jogador.pos));
    if (mapa.perigosos.count(tileEm(sob))) {
        levarDano(tileEm(sob) == 4 ? "VOCE CAIU NO BURACO" : "OS ESPINHOS TE PEGARAM");
        return;
    }
    for (auto &e : inimigos)
        if (glm::length(e.pos - jogador.pos) < 0.6f) { levarDano("UM MONSTRO TE DERROTOU"); return; }
}

// ---------------- Desenho da cena ----------------
// Personagem "em pé": os pés ficam no ponto (linha, coluna) e o sprite sobe a partir dali
void desenharEmPe(const Textura &t, glm::vec2 pos, bool esquerda, glm::vec4 cor = glm::vec4(1.0f), float sobe = 0.0f,
                  float escala = ESCALA) {
    glm::vec2 pes = isoParaTela(pos.x, pos.y);
    desenharSprite(t, pes - glm::vec2(0.0f, t.altura * escala / 2.0f - 4.0f + sobe), escala, esquerda ? ESPELHAR : 0, cor);
}

void desenharMundo() {
    // 1) Chão: tiles caminháveis nunca cobrem nada, então vão primeiro
    for (int l = 0; l < mapa.linhas; l++)
        for (int c = 0; c < mapa.colunas; c++)
            if (caminhavel({l, c})) desenharTile(tileEm({l, c}), {l, c});
    if (mostrarGrade)
        for (int l = 0; l < mapa.linhas; l++)
            for (int c = 0; c < mapa.colunas; c++) desenharTile(TILE_CURSOR, {l, c}, {1, 1, 1, 0.25f});
    if (dentroDoMapa(tileMouse)) desenharTile(TILE_CURSOR, tileMouse, {1.0f, 0.9f, 0.3f, 0.9f});

    // 2) Paredes e tudo que fica em pé, ordenados por profundidade (linha + coluna do centro)
    std::vector<std::pair<float, std::function<void()>>> fila;
    for (int l = 0; l < mapa.linhas; l++)
        for (int c = 0; c < mapa.colunas; c++)
            if (!caminhavel({l, c}))
                fila.push_back({profundidade(glm::vec2(l, c) + 0.5f), [=] { desenharTile(tileEm({l, c}), {l, c}); }});

    int quadroMoeda = (int)(tempo * 8.0f) % 4;
    for (auto &m : moedas)
        if (!m.pego) {
            glm::vec2 p = glm::vec2(m.tile) + 0.5f;
            fila.push_back({profundidade(p), [=] { desenharEmPe(texMoeda[quadroMoeda], p, false, glm::vec4(1.0f), 6.0f + 3.0f * std::sin(tempo * 4.0f), 3.0f); }});
        }
    for (auto &po : pocoes)
        if (!po.pego) {
            glm::vec2 p = glm::vec2(po.tile) + 0.5f;
            fila.push_back({profundidade(p), [=] { desenharEmPe(texPocao, p, false, glm::vec4(1.0f), 2.0f * std::sin(tempo * 3.0f)); }});
        }
    for (auto &e : inimigos) {
        const Textura &t = animacao(e.sprite + "_run_anim")[(int)(tempo * FPS_PERSONAGEM) % 4];
        fila.push_back({profundidade(e.pos), [&e, &t] { desenharEmPe(t, e.pos, e.esquerda); }});
    }
    bool pisca = jogador.invulneravel > 0.0f && std::fmod(tempo, 0.16f) < 0.08f;
    if (!pisca && !(estado == VITORIA && tempoEstado > 0.5f)) {
        // Sprite animado: "run" enquanto anda, "idle" parado
        const Textura &t = animacao(jogador.movendo ? "knight_m_run_anim" : "knight_m_idle_anim")[(int)(tempo * FPS_PERSONAGEM) % 4];
        float alpha = estado == VITORIA ? 1.0f - tempoEstado / 0.5f : 1.0f;
        fila.push_back({profundidade(jogador.pos) + 0.01f,
                        [&t, alpha] { desenharEmPe(t, jogador.pos, jogador.esquerda, {1, 1, 1, alpha}); }});
    }

    std::stable_sort(fila.begin(), fila.end(), [](auto &a, auto &b) { return a.first < b.first; });
    for (auto &f : fila) f.second();
}

void desenharHUD() {
    glm::vec4 branco(1.0f), dourado(1.0f, 0.82f, 0.3f, 1.0f);
    desenharRetangulo({WIDTH / 2.0f, HUD / 2.0f}, {(float)WIDTH, HUD}, {0.06f, 0.03f, 0.06f, 0.85f});
    for (int i = 0; i < VIDAS; i++)
        desenharSprite(i < vidas ? texCoracao : texCoracaoVazio, {30.0f + i * 34.0f, HUD / 2.0f}, 2.0f);

    std::string txtMoedas = std::to_string(moedas.size() - moedasRestantes()) + "/" + std::to_string(moedas.size());
    std::string txtPlacas = std::to_string(totalPlacas - placasRestantes()) + "/" + std::to_string(totalPlacas);
    desenharSprite(texMoeda[(int)(tempo * 8.0f) % 4], {WIDTH / 2.0f - 150.0f, HUD / 2.0f}, 3.0f);
    desenharTexto(txtMoedas, {WIDTH / 2.0f - 100.0f, HUD / 2.0f}, 3.0f, moedasRestantes() ? branco : dourado);
    desenharSprite(texPlaca, {WIDTH / 2.0f + 40.0f, HUD / 2.0f}, 2.0f);
    desenharTexto(txtPlacas, {WIDTH / 2.0f + 90.0f, HUD / 2.0f}, 3.0f, placasRestantes() ? branco : dourado);

    std::string txtTempo = "TEMPO " + formatarTempo(tempoRestante);
    bool urgente = tempoRestante < 30.0f && std::fmod(tempo, 0.6f) < 0.3f;
    desenharTexto(txtTempo, {WIDTH - 20.0f - larguraTexto(txtTempo, 3.0f) / 2.0f, HUD / 2.0f}, 3.0f,
                  urgente ? glm::vec4(1.0f, 0.3f, 0.3f, 1.0f) : branco);

    if (tempoMensagem > 0.0f && estado == JOGANDO) {
        float a = std::min(1.0f, tempoMensagem * 2.0f);
        desenharRetangulo({WIDTH / 2.0f, HEIGHT - 26.0f}, {(float)WIDTH, 44.0f}, {0.06f, 0.03f, 0.06f, 0.8f * a});
        desenharTexto(mensagem, {WIDTH / 2.0f, HEIGHT - 26.0f}, 3.0f, {1.0f, 1.0f, 1.0f, a});
    }
}

void desenharTelas() {
    glm::vec4 branco(1.0f), cinza(0.7f, 0.7f, 0.7f, 1.0f), dourado(1.0f, 0.82f, 0.3f, 1.0f);
    glm::vec2 c(WIDTH / 2.0f, HEIGHT / 2.0f);
    bool piscar = std::fmod(tempo, 1.0f) < 0.6f;
    if (estado == INICIO) {
        desenharRetangulo(c, {(float)WIDTH, (float)HEIGHT}, {0.0f, 0.0f, 0.0f, 0.7f});
        desenharTexto("DUNGEON ESCAPE ISO", c - glm::vec2(0.0f, 200.0f), 9.0f, dourado);
        desenharTexto("COLETE TODAS AS MOEDAS E PISE EM TODAS AS PLACAS", c - glm::vec2(0.0f, 90.0f), 3.0f, branco);
        desenharTexto("PARA ABRIR A SAIDA. DEPOIS DESCA PELA ESCADA", c - glm::vec2(0.0f, 60.0f), 3.0f, branco);
        desenharTexto("CUIDADO COM BURACOS ESPINHOS E MONSTROS", c - glm::vec2(0.0f, 30.0f), 3.0f, branco);
        desenharTexto("SETAS/WASD - COMBINE PARA AS DIAGONAIS - OU Q E Z C", c + glm::vec2(0.0f, 30.0f), 3.0f, cinza);
        desenharTexto("V GIRA A VISAO  P PAUSA  R REINICIA  H GRADE  ESC SAIR", c + glm::vec2(0.0f, 60.0f), 3.0f, cinza);
        if (piscar) desenharTexto("PRESSIONE ENTER PARA COMECAR", c + glm::vec2(0.0f, 160.0f), 4.0f, dourado);
    } else if (estado == VITORIA || estado == DERROTA) {
        float a = std::min(1.0f, tempoEstado * 2.0f);
        desenharRetangulo(c, {(float)WIDTH, (float)HEIGHT}, {0.0f, 0.0f, 0.0f, 0.7f * a});
        if (estado == VITORIA) {
            desenharTexto("VOCE ESCAPOU!", c - glm::vec2(0.0f, 110.0f), 9.0f, {0.4f, 1.0f, 0.5f, a});
            desenharTexto("TEMPO RESTANTE " + formatarTempo(tempoRestante) + "   VIDAS " + std::to_string(vidas), c, 4.0f,
                          {1, 1, 1, a});
        } else {
            desenharTexto("GAME OVER", c - glm::vec2(0.0f, 110.0f), 9.0f, {1.0f, 0.3f, 0.3f, a});
            desenharTexto(motivoDerrota, c, 4.0f, {1, 1, 1, a});
        }
        if (piscar && tempoEstado > 0.8f) desenharTexto("PRESSIONE R PARA JOGAR DE NOVO", c + glm::vec2(0.0f, 120.0f), 4.0f, dourado);
    }
    if (pausado) {
        desenharRetangulo(c, {(float)WIDTH, (float)HEIGHT}, {0.0f, 0.0f, 0.0f, 0.5f});
        desenharTexto("PAUSADO", c, 8.0f, branco);
    }
}

void desenharCena() {
    desenharMundo();
    if (flashDano > 0.0f) desenharRetangulo({WIDTH / 2.0f, HEIGHT / 2.0f}, {(float)WIDTH, (float)HEIGHT}, {0.8f, 0.0f, 0.0f, 0.4f * flashDano});
    desenharHUD();
    desenharTelas();
}

void atualizarTitulo(GLFWwindow *window) {
    static std::string anterior;
    std::string titulo = "Grau B - Dungeon Escape Iso | Moedas " + std::to_string(moedas.size() - moedasRestantes()) + "/" +
                         std::to_string(moedas.size()) + " | Placas " + std::to_string(totalPlacas - placasRestantes()) + "/" +
                         std::to_string(totalPlacas) + " | Vidas " + std::to_string(vidas) + " | Tempo " +
                         formatarTempo(tempoRestante) + " | Direcao " + jogador.direcao;
    if (dentroDoMapa(tileMouse))
        titulo += " | Mouse: linha " + std::to_string(tileMouse.x) + " coluna " + std::to_string(tileMouse.y) + " tile " +
                  std::to_string(tileEm(tileMouse));
    if (pausado) titulo += " | PAUSADO";
    if (invencivel) titulo += " | INVENCIVEL";
    if (titulo != anterior) glfwSetWindowTitle(window, titulo.c_str());
    anterior = titulo;
}

// ---------------- Entrada ----------------
void key_callback(GLFWwindow *window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GL_TRUE);
    if ((key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE) && estado != JOGANDO) reiniciar(true);
    if (key == GLFW_KEY_R) reiniciar(true);
    if (key == GLFW_KEY_P && estado == JOGANDO) pausado = !pausado;
    if (key == GLFW_KEY_H) mostrarGrade = !mostrarGrade;
    if (key == GLFW_KEY_G) invencivel = !invencivel;
    if (key == GLFW_KEY_V) {
        rotacao = (rotacao + 1) % 4;
        atualizarOrigem();
        mostrarMensagem("VISAO GIRADA " + std::to_string(rotacao * 90) + " GRAUS", 1.5f);
    }
}

// O cursor é convertido de tela para tile com a projeção isométrica inversa
void cursor_callback(GLFWwindow *window, double x, double y) {
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    tileMouse = telaParaIso({(float)x * WIDTH / w, (float)y * HEIGHT / h});
}

void framebuffer_size_callback(GLFWwindow *, int width, int height) { glViewport(0, 0, width, height); }

// ---------------- Inicialização OpenGL ----------------
GLuint compilarShader(GLenum tipo, const GLchar *fonte) {
    GLuint shader = glCreateShader(tipo);
    glShaderSource(shader, 1, &fonte, nullptr);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLchar log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Erro ao compilar shader:\n" << log << std::endl;
    }
    return shader;
}

GLuint setupShader() {
    GLuint vs = compilarShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compilarShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLchar log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "Erro ao linkar programa:\n" << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

void setupGeometria() {
    GLfloat vertices[] = {-0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};
    GLuint VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void carregarAssets() {
    texTileset = carregarTextura(std::string(GRAUB_DIR) + mapa.tileset);
    criarFonte();
    for (int i = 0; i < 4; i++) texMoeda[i] = carregarFrame("coin_anim_f" + std::to_string(i));
    texCoracao = carregarFrame("ui_heart_full");
    texCoracaoVazio = carregarFrame("ui_heart_empty");
    texPocao = carregarFrame("flask_big_red");
    texPlaca = carregarFrame("button_red_up");
    animacao("knight_m_idle_anim");
    animacao("knight_m_run_anim");
    for (auto &e : inimigosIniciais) animacao(e.sprite + "_run_anim");
}

int main() {
    if (!carregarMapa(std::string(GRAUB_DIR) + "map.txt")) return -1;
    atualizarOrigem();

    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Grau B - Dungeon Escape Iso", nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }
    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    GLuint shaderID = setupShader();
    setupGeometria();
    glUseProgram(shaderID);
    modelLoc = glGetUniformLocation(shaderID, "model");
    uvRectLoc = glGetUniformLocation(shaderID, "uvRect");
    flagsLoc = glGetUniformLocation(shaderID, "flags");
    corLoc = glGetUniformLocation(shaderID, "cor");
    solidoLoc = glGetUniformLocation(shaderID, "solido");
    glUniform1i(glGetUniformLocation(shaderID, "tex"), 0);
    glUniform1i(solidoLoc, 0);
    glActiveTexture(GL_TEXTURE0);

    // Projeção ortográfica em pixels, origem no canto superior esquerdo (y para baixo)
    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    carregarAssets();
    reiniciar(false);

    std::cout << "\nDungeon Escape Iso - colete as moedas, pise nas placas e fuja pela escada!\n"
                 "  Setas / WASD (combine para diagonais) ou Q E Z C: mover nas 8 direcoes\n"
                 "  ENTER: comecar | V: gira a visao | P: pausa | R: reinicia | H: grade | G: invencivel | ESC: sai\n";

    double ultimoTempo = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        double agora = glfwGetTime();
        float dt = std::min((float)(agora - ultimoTempo), 1.0f / 30.0f);
        ultimoTempo = agora;
        if (!pausado) atualizar(dt, window);
        atualizarTitulo(window);

        glClearColor(0.09f, 0.06f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);
        desenharCena();
        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderID);
    glfwTerminate();
    return 0;
}
