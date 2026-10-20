/*
 * Desafio M5 - Navegação em Camadas (cena de jogo 2D com parallax)
 *
 * Um "Tappy Plane" (estilo Flappy Bird) com os assets da Kenney. A cena é
 * composta por 6 camadas desenhadas de trás para frente, cada uma rolando com
 * uma velocidade diferente (parallax):
 *
 *   0. Céu e nuvens        (fator 0.10)
 *   1. Montanhas distantes (fator 0.30, com névoa)
 *   2. Colinas             (fator 0.55, com névoa mais fraca)
 *   3. Camada do jogo      (fator 1.00) - rochas, estrelas, fumaça e o avião
 *   4. Chão                (fator 1.00) - o avião colide com o relevo dele
 *   5. Nuvens frontais     (fator 1.60, semitransparentes, passam na frente)
 *
 * Objetos animados que interagem com o cenário:
 *   - Avião: animação por frames (hélice), gravidade, impulso e rotação conforme
 *     a velocidade; colide com as rochas (círculo x triângulo) e com o relevo do
 *     chão (perfil de altura extraído do canal alfa da textura).
 *   - Estrelas: giram e flutuam; ao serem tocadas são coletadas (+1 ponto).
 *   - Fumaça: partículas emitidas pelo avião que crescem e desaparecem.
 *
 * Controles:
 *   ESPAÇO / seta para cima / clique - impulso (e começa / reinicia o jogo)
 *   P   - pausa
 *   H   - mostra / esconde as caixas de colisão
 *   C   - troca a cor do avião
 *   R   - reinicia
 *   ESC - sai
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <random>
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
const int WIDTH = 800, HEIGHT = 480;
const float GRAVIDADE = 1500.0f;      // px/s²
const float IMPULSO = -430.0f;        // velocidade vertical após o toque (px/s)
const float VEL_JOGO = 180.0f;        // velocidade de rolagem da camada do jogo (px/s)
const float AVIAO_X = 200.0f;
const float RAIO_AVIAO = 27.0f;       // raio do círculo de colisão do avião
const float ESCALA_ROCHA = 1.3f;
const float ESPACO_ROCHAS = 290.0f;   // distância horizontal entre pares de rochas
const float ABERTURA = 175.0f;        // distância vertical entre as pontas das rochas
const int NUM_OBSTACULOS = 4;
const float FPS_ANIMACAO = 14.0f;     // frames por segundo da hélice
const glm::vec3 COR_CEU(0.83f, 0.92f, 0.96f);

// Ponta e base da rocha em coordenadas normalizadas da textura (medidas no PNG)
const float PONTA_ROCHA_U = 66.0f / 108.0f;
const float BASE_ROCHA_V = 237.0f / 239.0f;

const std::vector<std::string> CORES_AVIAO = {"Red", "Blue", "Green", "Yellow"};

struct Textura {
    GLuint id = 0;
    int largura = 1, altura = 1;
};

// Faixa horizontal que se repete e rola com velocidade proporcional a "fator"
struct Camada {
    Textura tex;
    float fator;    // 1 = mesma velocidade da camada do jogo
    float y;        // topo da faixa
    float altura;   // altura da faixa na tela
    float nevoa;    // quanto a cor se mistura com o céu (profundidade)
};

struct Obstaculo {
    float x;              // centro horizontal
    float centroAbertura; // y do meio da passagem
    bool pontuado;
    bool estrelaColetada;
    float tempoColeta;    // animação da estrela coletada
};

struct Particula {
    glm::vec2 pos, vel;
    float vida, duracao, escala;
    float alpha;
};

enum Estado { PRONTO, JOGANDO, CAINDO, FIM };

// ---------------- Estado do jogo ----------------
std::mt19937 rng(std::random_device{}());
Estado estado = PRONTO;
float scroll = 0.0f;  // distância total percorrida pela camada do jogo
float tempo = 0.0f;
float tempoEstado = 0.0f;
glm::vec2 posAviao(AVIAO_X, HEIGHT / 2.0f);
float velAviao = 0.0f;
float anguloAviao = 0.0f;
int corAviao = 0;
int pontos = 0, recorde = 0;
bool pausado = false, mostrarHitbox = false;
std::vector<Obstaculo> obstaculos;
std::vector<Particula> fumaca;
std::vector<Particula> nuvensFrente;
float tempoFumaca = 0.0f;

// Texturas
Textura texCeu, texMontanhas, texColinas, texChao, texRocha, texRochaBaixo, texEstrela;
Textura texFumaca, texNuvem, texPronto, texFim, texPainel, texToque, texToqueOk;
std::array<Textura, 3> texMedalhas;
std::array<Textura, 10> texNumeros;
std::vector<std::array<Textura, 3>> texAvioes;
std::vector<Camada> camadasFundo; // camadas 0..2
Camada camadaChao;                // camada 4
std::vector<int> perfilChao;      // y do topo do chão para cada coluna da textura

// ---------------- Shaders ----------------
const GLchar *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 vp;
uniform mat4 projection;
uniform mat4 model;
out vec2 uv;
void main() {
    uv = vp + 0.5;
    gl_Position = projection * model * vec4(vp, 0.0, 1.0);
}
)";

// repeticoes/deslocamento fazem a textura rolar e se repetir (parallax);
// nevoa mistura a cor com o céu para dar sensação de profundidade.
const GLchar *fragmentShaderSource = R"(
#version 330 core
in vec2 uv;
uniform sampler2D tex;
uniform vec2 repeticoes;
uniform vec2 deslocamento;
uniform vec4 nevoa;     // rgb = cor, a = intensidade
uniform float alpha;
uniform bool usarCor;   // desenha linhas de depuração com cor sólida
uniform vec4 cor;
out vec4 frg;
void main() {
    if (usarCor) { frg = cor; return; }
    vec4 c = texture(tex, uv * repeticoes + deslocamento);
    c.rgb = mix(c.rgb, nevoa.rgb, nevoa.a);
    c.a *= alpha;
    if (c.a < 0.01) discard;
    frg = c;
}
)";

GLint modelLoc, repeticoesLoc, deslocamentoLoc, nevoaLoc, alphaLoc, usarCorLoc, corLoc;
GLuint VAO, VAOlinhas, VBOlinhas;

// ---------------- Utilidades ----------------
float aleatorio(float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); }

// Carrega uma imagem e cria a textura (repete na horizontal, prende na vertical)
Textura carregarTextura(const std::string &arquivo) {
    std::string caminho = std::string(ASSETS_DIR) + arquivo;
    int w, h, canais;
    unsigned char *dados = stbi_load(caminho.c_str(), &w, &h, &canais, 4);
    if (!dados) {
        std::cerr << "Falha ao carregar textura: " << caminho << std::endl;
        return {};
    }
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dados);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(dados);
    glBindTexture(GL_TEXTURE_2D, 0);
    return {id, w, h};
}

// Lê o canal alfa da textura do chão e guarda, para cada coluna, a primeira linha opaca
std::vector<int> carregarPerfil(const std::string &arquivo) {
    std::string caminho = std::string(ASSETS_DIR) + arquivo;
    int w, h, canais;
    unsigned char *dados = stbi_load(caminho.c_str(), &w, &h, &canais, 4);
    std::vector<int> perfil(dados ? w : 1, 0);
    if (!dados) return perfil;
    for (int x = 0; x < w; x++) {
        perfil[x] = h;
        for (int y = 0; y < h; y++)
            if (dados[(y * w + x) * 4 + 3] > 128) { perfil[x] = y; break; }
    }
    stbi_image_free(dados);
    return perfil;
}

// Desenha um quadrado texturizado centrado em "centro"
void desenhar(const Textura &t, glm::vec2 centro, glm::vec2 tamanho, float angulo = 0.0f, float alpha = 1.0f,
              float nevoa = 0.0f, glm::vec2 repeticoes = glm::vec2(1.0f), glm::vec2 desloc = glm::vec2(0.0f)) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(centro, 0.0f));
    model = glm::rotate(model, glm::radians(angulo), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(tamanho, 1.0f));
    glBindTexture(GL_TEXTURE_2D, t.id);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform2fv(repeticoesLoc, 1, glm::value_ptr(repeticoes));
    glUniform2fv(deslocamentoLoc, 1, glm::value_ptr(desloc));
    glUniform4f(nevoaLoc, COR_CEU.r, COR_CEU.g, COR_CEU.b, nevoa);
    glUniform1f(alphaLoc, alpha);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Largura (na tela) de uma repetição da textura da camada
float larguraRepeticao(const Camada &c) { return c.tex.largura * c.altura / c.tex.altura; }

// Desenha a faixa da camada deslocando a coordenada de textura (parallax)
void desenharCamada(const Camada &c) {
    float larg = larguraRepeticao(c);
    float desloc = std::fmod(scroll * c.fator, larg) / larg;
    desenhar(c.tex, {WIDTH / 2.0f, c.y + c.altura / 2.0f}, {(float)WIDTH, c.altura}, 0.0f, 1.0f, c.nevoa,
             {WIDTH / larg, 1.0f}, {desloc, 0.0f});
}

// y (na tela) do topo do chão na coluna x da tela
float alturaChao(float x) {
    float larg = larguraRepeticao(camadaChao);
    float u = std::fmod(x + scroll * camadaChao.fator, larg);
    if (u < 0) u += larg;
    int coluna = std::min((int)(u / larg * perfilChao.size()), (int)perfilChao.size() - 1);
    return camadaChao.y + perfilChao[coluna] * camadaChao.altura / camadaChao.tex.altura;
}

glm::vec2 tamanhoRocha() { return glm::vec2(texRocha.largura, texRocha.altura) * ESCALA_ROCHA; }

// Os dois triângulos (rocha de cima e de baixo) de um obstáculo, em coordenadas da tela
std::array<std::array<glm::vec2, 3>, 2> triangulosObstaculo(const Obstaculo &o) {
    glm::vec2 t = tamanhoRocha();
    float esq = o.x - t.x / 2.0f;
    float pontaBaixo = o.centroAbertura + ABERTURA / 2.0f; // ponta da rocha que sobe do chão
    float pontaCima = o.centroAbertura - ABERTURA / 2.0f;  // ponta da rocha que desce do teto
    return {{{glm::vec2(esq + PONTA_ROCHA_U * t.x, pontaBaixo), glm::vec2(esq, pontaBaixo + BASE_ROCHA_V * t.y),
              glm::vec2(esq + t.x, pontaBaixo + BASE_ROCHA_V * t.y)},
             {glm::vec2(esq + PONTA_ROCHA_U * t.x, pontaCima), glm::vec2(esq, pontaCima - BASE_ROCHA_V * t.y),
              glm::vec2(esq + t.x, pontaCima - BASE_ROCHA_V * t.y)}}};
}

glm::vec2 posEstrela(const Obstaculo &o) {
    return {o.x, o.centroAbertura + 8.0f * std::sin(tempo * 3.0f + o.x * 0.01f)};
}

// ---------------- Colisão ----------------
float distanciaSegmento(glm::vec2 p, glm::vec2 a, glm::vec2 b) {
    glm::vec2 ab = b - a;
    float t = glm::clamp(glm::dot(p - a, ab) / glm::dot(ab, ab), 0.0f, 1.0f);
    return glm::length(p - (a + t * ab));
}

float cruz(glm::vec2 a, glm::vec2 b, glm::vec2 c) { return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x); }

bool circuloTriangulo(glm::vec2 c, float r, const std::array<glm::vec2, 3> &t) {
    float d1 = cruz(t[0], t[1], c), d2 = cruz(t[1], t[2], c), d3 = cruz(t[2], t[0], c);
    bool dentro = !((d1 < 0 || d2 < 0 || d3 < 0) && (d1 > 0 || d2 > 0 || d3 > 0));
    return dentro || distanciaSegmento(c, t[0], t[1]) < r || distanciaSegmento(c, t[1], t[2]) < r ||
           distanciaSegmento(c, t[2], t[0]) < r;
}

// Testa vários pontos da metade inferior do círculo contra o relevo do chão
bool colideComChao(glm::vec2 c, float r) {
    for (float dx = -r; dx <= r; dx += 3.0f)
        if (c.y + std::sqrt(r * r - dx * dx) >= alturaChao(c.x + dx)) return true;
    return false;
}

// ---------------- Lógica ----------------
void reiniciar() {
    estado = PRONTO;
    tempoEstado = 0.0f;
    posAviao = glm::vec2(AVIAO_X, HEIGHT / 2.0f - 30.0f);
    velAviao = 0.0f;
    anguloAviao = 0.0f;
    pontos = 0;
    fumaca.clear();
    obstaculos.clear();
    for (int i = 0; i < NUM_OBSTACULOS; i++)
        obstaculos.push_back({WIDTH + 200.0f + i * ESPACO_ROCHAS, aleatorio(140.0f, 290.0f), false, false, 0.0f});
}

void impulso() {
    if (pausado) return;
    if (estado == PRONTO) estado = JOGANDO;
    if (estado == JOGANDO) velAviao = IMPULSO;
    else if (estado == FIM && tempoEstado > 0.6f) reiniciar();
}

void perder() {
    estado = CAINDO;
    tempoEstado = 0.0f;
    velAviao = std::min(velAviao, -150.0f); // pequeno "pulo" ao bater
    recorde = std::max(recorde, pontos);
}

void emitirFumaca(float dt) {
    tempoFumaca += dt;
    while (tempoFumaca > 0.07f) {
        tempoFumaca -= 0.07f;
        float a = glm::radians(anguloAviao);
        glm::vec2 traseira = posAviao - glm::vec2(std::cos(a), std::sin(a)) * 40.0f;
        fumaca.push_back({traseira, {-VEL_JOGO * 0.3f, aleatorio(-20.0f, 20.0f)}, 0.0f, 0.6f, aleatorio(0.5f, 0.8f), 0.8f});
    }
}

void atualizar(float dt) {
    tempo += dt;
    tempoEstado += dt;
    bool rolando = estado == PRONTO || estado == JOGANDO;
    if (rolando) scroll += VEL_JOGO * dt;

    // Avião
    if (estado == PRONTO) {
        posAviao.y = HEIGHT / 2.0f - 30.0f + 10.0f * std::sin(tempo * 4.0f);
        anguloAviao = 0.0f;
    } else if (estado == JOGANDO || estado == CAINDO) {
        velAviao += GRAVIDADE * dt;
        posAviao.y += velAviao * dt;
        float alvo = glm::clamp(velAviao * 0.08f, -25.0f, estado == CAINDO ? 90.0f : 60.0f);
        anguloAviao += (alvo - anguloAviao) * std::min(1.0f, 10.0f * dt);
        if (posAviao.y < RAIO_AVIAO) { posAviao.y = RAIO_AVIAO; velAviao = 0.0f; }
    }
    if (estado != FIM && estado != CAINDO) emitirFumaca(dt);

    // Obstáculos: rolam com a camada do jogo e são reciclados ao sair da tela
    for (auto &o : obstaculos) {
        if (rolando) o.x -= VEL_JOGO * dt;
        if (o.x < -tamanhoRocha().x) {
            float maisDireita = 0.0f;
            for (auto &q : obstaculos) maisDireita = std::max(maisDireita, q.x);
            o = {maisDireita + ESPACO_ROCHAS, aleatorio(140.0f, 290.0f), false, false, 0.0f};
        }
        if (o.estrelaColetada) o.tempoColeta += dt;
    }

    // Colisões do avião com o cenário
    if (estado == JOGANDO) {
        for (auto &o : obstaculos) {
            for (auto &t : triangulosObstaculo(o))
                if (circuloTriangulo(posAviao, RAIO_AVIAO, t)) { perder(); break; }
            if (estado != JOGANDO) break;
            if (!o.estrelaColetada && glm::length(posEstrela(o) - posAviao) < RAIO_AVIAO + 16.0f) {
                o.estrelaColetada = true;
                pontos++;
            }
            if (!o.pontuado && o.x < posAviao.x) {
                o.pontuado = true;
                pontos++;
            }
        }
        if (estado == JOGANDO && colideComChao(posAviao, RAIO_AVIAO)) perder();
    }
    if (estado == CAINDO && colideComChao(posAviao, RAIO_AVIAO)) {
        estado = FIM;
        tempoEstado = 0.0f;
        recorde = std::max(recorde, pontos);
    }

    // Partículas de fumaça (ficam paradas no mundo, então rolam junto com o cenário)
    for (auto &p : fumaca) {
        p.vida += dt;
        p.pos += (p.vel - glm::vec2(rolando ? VEL_JOGO : 0.0f, 0.0f)) * dt;
    }
    fumaca.erase(std::remove_if(fumaca.begin(), fumaca.end(), [](const Particula &p) { return p.vida > p.duracao; }),
                 fumaca.end());

    // Nuvens da camada frontal (mais rápidas que o jogo)
    for (auto &n : nuvensFrente) {
        if (rolando) n.pos.x -= VEL_JOGO * 1.6f * dt;
        if (n.pos.x < -150.0f) n = {{WIDTH + aleatorio(100.0f, 500.0f), aleatorio(20.0f, 140.0f)}, {}, 0, 0,
                                    aleatorio(1.4f, 2.0f), aleatorio(0.5f, 0.7f)};
    }
}

// ---------------- Desenho ----------------
void desenharNumero(int valor, glm::vec2 centro, float escala) {
    std::string s = std::to_string(valor);
    float w = texNumeros[0].largura * escala, h = texNumeros[0].altura * escala;
    float x = centro.x - w * (s.size() - 1) / 2.0f;
    for (char ch : s) {
        desenhar(texNumeros[ch - '0'], {x, centro.y}, {w, h});
        x += w;
    }
}

void desenharLinhas(const std::vector<glm::vec2> &pts, GLenum modo, glm::vec4 c) {
    glBindVertexArray(VAOlinhas);
    glBindBuffer(GL_ARRAY_BUFFER, VBOlinhas);
    glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(glm::vec2), pts.data(), GL_DYNAMIC_DRAW);
    glm::mat4 id(1.0f);
    // O vertex shader soma 0.5 ao vértice só para a coordenada de textura; a posição usa vp direto
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(id));
    glUniform1i(usarCorLoc, 1);
    glUniform4fv(corLoc, 1, glm::value_ptr(c));
    glDrawArrays(modo, 0, (GLsizei)pts.size());
    glUniform1i(usarCorLoc, 0);
    glBindVertexArray(VAO);
}

void desenharHitboxes() {
    glm::vec4 vermelho(1.0f, 0.1f, 0.1f, 1.0f), amarelo(1.0f, 0.9f, 0.0f, 1.0f);
    std::vector<glm::vec2> circulo;
    for (int i = 0; i < 32; i++) {
        float a = i * 2.0f * 3.14159265f / 32.0f;
        circulo.push_back(posAviao + RAIO_AVIAO * glm::vec2(std::cos(a), std::sin(a)));
    }
    desenharLinhas(circulo, GL_LINE_LOOP, amarelo);
    for (auto &o : obstaculos) {
        for (auto &t : triangulosObstaculo(o)) desenharLinhas({t[0], t[1], t[2]}, GL_LINE_LOOP, vermelho);
        if (!o.estrelaColetada) {
            std::vector<glm::vec2> c;
            for (int i = 0; i < 16; i++) {
                float a = i * 2.0f * 3.14159265f / 16.0f;
                c.push_back(posEstrela(o) + 16.0f * glm::vec2(std::cos(a), std::sin(a)));
            }
            desenharLinhas(c, GL_LINE_LOOP, amarelo);
        }
    }
    std::vector<glm::vec2> chao;
    for (int x = 0; x <= WIDTH; x += 4) chao.push_back({(float)x, alturaChao((float)x)});
    desenharLinhas(chao, GL_LINE_STRIP, vermelho);
}

void desenharCena() {
    // Camadas 0, 1 e 2: céu, montanhas e colinas
    for (auto &c : camadasFundo) desenharCamada(c);

    // Camada 3: jogo (rochas, estrelas, fumaça e avião)
    glm::vec2 tr = tamanhoRocha();
    for (auto &o : obstaculos) {
        float pontaBaixo = o.centroAbertura + ABERTURA / 2.0f, pontaCima = o.centroAbertura - ABERTURA / 2.0f;
        desenhar(texRocha, {o.x, pontaBaixo + tr.y / 2.0f}, tr);
        desenhar(texRochaBaixo, {o.x, pontaCima - tr.y / 2.0f}, tr);
        if (!o.estrelaColetada) {
            float giro = 15.0f * std::sin(tempo * 4.0f);
            desenhar(texEstrela, posEstrela(o), glm::vec2(texEstrela.largura, texEstrela.altura), giro);
        } else if (o.tempoColeta < 0.4f) { // estrela coletada: cresce, sobe e some
            float k = o.tempoColeta / 0.4f;
            desenhar(texEstrela, posEstrela(o) - glm::vec2(0.0f, 40.0f * k),
                     glm::vec2(texEstrela.largura, texEstrela.altura) * (1.0f + k), 360.0f * k, 1.0f - k);
        }
    }
    for (auto &p : fumaca) {
        float k = p.vida / p.duracao;
        desenhar(texFumaca, p.pos, glm::vec2(texFumaca.largura, texFumaca.altura) * p.escala * (1.0f + k),
                 0.0f, p.alpha * (1.0f - k));
    }
    // Hélice animada (frames 1-2-3-2); parada depois da queda
    static const int SEQUENCIA[] = {0, 1, 2, 1};
    int frame = estado == FIM ? 0 : SEQUENCIA[(int)(tempo * FPS_ANIMACAO) % 4];
    const Textura &ta = texAvioes[corAviao][frame];
    desenhar(ta, posAviao, glm::vec2(ta.largura, ta.altura), anguloAviao);

    // Camada 4: chão (na frente das bases das rochas)
    desenharCamada(camadaChao);

    // Camada 5: nuvens frontais semitransparentes
    for (auto &n : nuvensFrente)
        desenhar(texNuvem, n.pos, glm::vec2(texNuvem.largura, texNuvem.altura) * n.escala, 0.0f, n.alpha);

    if (mostrarHitbox) desenharHitboxes();

    // Interface
    if (estado == PRONTO) {
        desenhar(texPronto, {WIDTH / 2.0f, 110.0f}, glm::vec2(texPronto.largura, texPronto.altura) * 0.8f);
        const Textura &tt = std::fmod(tempo, 0.8f) < 0.4f ? texToque : texToqueOk;
        desenhar(tt, {AVIAO_X + 90.0f, HEIGHT / 2.0f - 10.0f}, glm::vec2(tt.largura, tt.altura));
    } else if (estado == JOGANDO || estado == CAINDO) {
        desenharNumero(pontos, {WIDTH / 2.0f, 50.0f}, 0.7f);
    } else {
        desenhar(texFim, {WIDTH / 2.0f, 100.0f}, glm::vec2(texFim.largura, texFim.altura) * 0.8f);
        desenhar(texPainel, {WIDTH / 2.0f, 260.0f}, {300.0f, 160.0f});
        int medalha = pontos >= 30 ? 2 : pontos >= 15 ? 1 : pontos >= 5 ? 0 : -1;
        if (medalha >= 0) desenhar(texMedalhas[medalha], {WIDTH / 2.0f - 80.0f, 260.0f}, {90.0f, 94.0f});
        float xn = medalha >= 0 ? WIDTH / 2.0f + 50.0f : WIDTH / 2.0f;
        desenharNumero(pontos, {xn, 230.0f}, 0.6f);
        desenharNumero(recorde, {xn, 295.0f}, 0.4f);
    }
}

void atualizarTitulo(GLFWwindow *window) {
    static std::string anterior;
    std::string titulo = "M5 - Tappy Plane | Pontos: " + std::to_string(pontos) + " | Recorde: " +
                         std::to_string(recorde);
    if (pausado) titulo += " | PAUSADO";
    if (mostrarHitbox) titulo += " | Hitboxes";
    if (titulo != anterior) glfwSetWindowTitle(window, titulo.c_str());
    anterior = titulo;
}

// ---------------- Entrada ----------------
void key_callback(GLFWwindow *window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GL_TRUE);
    if (key == GLFW_KEY_SPACE || key == GLFW_KEY_UP || key == GLFW_KEY_W) impulso();
    if (key == GLFW_KEY_P) pausado = !pausado;
    if (key == GLFW_KEY_H) mostrarHitbox = !mostrarHitbox;
    if (key == GLFW_KEY_C) corAviao = (corAviao + 1) % texAvioes.size();
    if (key == GLFW_KEY_R) { pausado = false; reiniciar(); }
}

// Só conta um clique novo (solto -> pressionado); ignora "press" repetidos sem "release",
// que alguns ambientes (ex.: WSLg) geram ao mover o mouse
void mouse_button_callback(GLFWwindow *window, int button, int action, int) {
    static bool pressionado = false;
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    if (action == GLFW_RELEASE) pressionado = false;
    else if (action == GLFW_PRESS && !pressionado && glfwGetMouseButton(window, button) == GLFW_PRESS) {
        pressionado = true;
        impulso();
    }
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

// Quadrado unitário centralizado na origem + um VAO dinâmico para as linhas de depuração
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

    glGenVertexArrays(1, &VAOlinhas);
    glGenBuffers(1, &VBOlinhas);
    glBindVertexArray(VAOlinhas);
    glBindBuffer(GL_ARRAY_BUFFER, VBOlinhas);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void carregarAssets() {
    texCeu = carregarTextura("background.png");
    texMontanhas = carregarTextura("groundRock.png");
    texColinas = carregarTextura("groundDirt.png");
    texChao = carregarTextura("groundGrass.png");
    perfilChao = carregarPerfil("groundGrass.png");
    texRocha = carregarTextura("rockGrass.png");
    texRochaBaixo = carregarTextura("rockGrassDown.png");
    texEstrela = carregarTextura("starGold.png");
    texFumaca = carregarTextura("puffSmall.png");
    texNuvem = carregarTextura("puffLarge.png");
    texPronto = carregarTextura("UI/textGetReady.png");
    texFim = carregarTextura("UI/textGameOver.png");
    texPainel = carregarTextura("UI/UIbg.png");
    texToque = carregarTextura("UI/tap.png");
    texToqueOk = carregarTextura("UI/tapTick.png");
    texMedalhas = {carregarTextura("UI/medalBronze.png"), carregarTextura("UI/medalSilver.png"),
                   carregarTextura("UI/medalGold.png")};
    for (int i = 0; i < 10; i++) texNumeros[i] = carregarTextura("Numbers/number" + std::to_string(i) + ".png");
    for (auto &cor : CORES_AVIAO) {
        std::array<Textura, 3> frames;
        for (int i = 0; i < 3; i++) frames[i] = carregarTextura("Planes/plane" + cor + std::to_string(i + 1) + ".png");
        texAvioes.push_back(frames);
    }

    camadasFundo = {
        {texCeu, 0.10f, 0.0f, (float)HEIGHT, 0.0f},
        {texMontanhas, 0.30f, HEIGHT - 200.0f, 180.0f, 0.65f},
        {texColinas, 0.55f, HEIGHT - 125.0f, 110.0f, 0.35f},
    };
    camadaChao = {texChao, 1.0f, HEIGHT - 71.0f, 71.0f, 0.0f};
    for (int i = 0; i < 3; i++)
        nuvensFrente.push_back({{200.0f + i * 320.0f, aleatorio(20.0f, 140.0f)}, {}, 0, 0, aleatorio(1.4f, 2.0f),
                                aleatorio(0.5f, 0.7f)});
}

int main() {
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

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "M5 - Tappy Plane", nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
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
    repeticoesLoc = glGetUniformLocation(shaderID, "repeticoes");
    deslocamentoLoc = glGetUniformLocation(shaderID, "deslocamento");
    nevoaLoc = glGetUniformLocation(shaderID, "nevoa");
    alphaLoc = glGetUniformLocation(shaderID, "alpha");
    usarCorLoc = glGetUniformLocation(shaderID, "usarCor");
    corLoc = glGetUniformLocation(shaderID, "cor");
    glUniform1i(glGetUniformLocation(shaderID, "tex"), 0);
    glUniform1i(usarCorLoc, 0);
    glActiveTexture(GL_TEXTURE0);

    // Projeção ortográfica em pixels, origem no canto superior esquerdo (y para baixo)
    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);

    carregarAssets();
    reiniciar();

    std::cout << "\nControles:\n"
                 "  ESPACO / seta para cima / clique: impulso (comeca e reinicia)\n"
                 "  P: pausa | H: caixas de colisao | C: cor do aviao | R: reinicia | ESC: sai\n";

    double ultimoTempo = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        double agora = glfwGetTime();
        float dt = std::min((float)(agora - ultimoTempo), 1.0f / 30.0f);
        ultimoTempo = agora;
        if (!pausado) atualizar(dt);
        atualizarTitulo(window);

        glClearColor(COR_CEU.r, COR_CEU.g, COR_CEU.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);
        desenharCena();
        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &VAOlinhas);
    glDeleteBuffers(1, &VBOlinhas);
    glDeleteProgram(shaderID);
    glfwTerminate();
    return 0;
}
