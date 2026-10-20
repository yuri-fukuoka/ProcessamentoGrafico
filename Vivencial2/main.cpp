/*
 * Atividade Vivencial 2 - Parallax Scrolling
 *
 * Um foguete (sprite animado) voa pelo espaço controlado pelo teclado. A câmera segue
 * o foguete e cada camada do cenário se desloca proporcionalmente ao movimento da
 * câmera multiplicado pelo seu "fator" de profundidade:
 *
 *     deslocamento da camada = deslocamento da câmera * fator
 *
 * Camadas distantes (fator pequeno) quase não se mexem; camadas próximas (fator
 * grande) se deslocam mais rápido. No início a câmera está na origem, então todas as
 * camadas começam com deslocamento zero (alinhadas).
 *
 * Camadas, de trás para frente:
 *   0. Nebulosa           fator 0.05   (textura repetida)
 *   1. Estrelas pequenas  fator 0.12   (textura repetida)
 *   2. Estrelas grandes   fator 0.25   (textura repetida)
 *   3. Gigante gasoso     fator 0.40   (objeto)
 *   4. Luas               fator 0.60   (objetos)
 *   5. Foguete            fator 1.00   (o jogador; a câmera o acompanha)
 *   6. Meteoros           fator 1.40   (objetos que passam na frente do foguete)
 *
 * Controles:
 *   setas / WASD - move o foguete      clique esquerdo - voa até o ponto clicado
 *   1 2 3 - troca o foguete            R - volta à posição inicial (camadas alinhadas)
 *   ESC - sai
 */

#include <algorithm>
#include <cmath>
#include <iostream>
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
const int WIDTH = 800, HEIGHT = 600;
const glm::vec2 CENTRO(WIDTH / 2.0f, HEIGHT / 2.0f);
const glm::vec2 VELOCIDADE(320.0f, 240.0f);  // px/s do foguete
const float LIMITE_Y = 200.0f;               // o foguete sobe/desce no máximo isso (evita mostrar a borda das imagens)
const float SEGUIR_CAMERA = 5.0f;            // quanto maior, mais rápido a câmera alcança o foguete
const float ESCALA_FOGUETE = 0.22f;
const float INCLINACAO = 20.0f;              // graus de inclinação ao subir/descer
const int FRAMES_FOGUETE = 10;

struct Textura {
    GLuint id = 0;
    int largura = 1, altura = 1;
};

// Camada que cobre a tela inteira com uma textura repetida (GL_REPEAT)
struct CamadaRepetida {
    Textura tex;
    float fator;   // profundidade: 0 = infinitamente longe, 1 = mesma distância do foguete
    float escala;  // tamanho da textura na tela
};

struct Objeto {
    Textura *tex;
    glm::vec2 pos;   // posição na própria camada (origem no centro da tela)
    float escala;
    float velGiro;   // graus por segundo
};

// Camada com objetos soltos; eles se repetem a cada "periodo" pixels na horizontal
struct CamadaObjetos {
    float fator;
    float periodo;
    std::vector<Objeto> objetos;
};

// ---------------- Estado ----------------
glm::vec2 foguete(0.0f), camera(0.0f), alvoClique(0.0f);
glm::vec2 velocidade(0.0f);
bool indoAoClique = false;
int sentido = 1;               // 1 = direita, -1 = esquerda
float angulo = 90.0f, tempo = 0.0f;
int corFoguete = 0;

std::vector<CamadaRepetida> camadasFundo;
CamadaObjetos camadaPlaneta, camadaLuas, camadaMeteoros;
std::vector<std::vector<Textura>> framesFoguete;  // [cor][frame]
Textura texPlaneta;
std::vector<Textura> texLuas, texMeteoros;

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

// repeticoes = quantas vezes a textura cabe no retângulo; deslocamento = offset da
// camada em unidades de textura. A conta é feita a partir do centro (uv - 0.5) para
// que, com deslocamento zero, o meio da imagem fique no meio da tela.
const GLchar *fragmentShaderSource = R"(
#version 330 core
in vec2 uv;
uniform sampler2D tex;
uniform vec2 repeticoes;
uniform vec2 deslocamento;
uniform float alpha;
out vec4 frg;
void main() {
    vec4 c = texture(tex, (uv - 0.5) * repeticoes + 0.5 + deslocamento);
    c.a *= alpha;
    if (c.a < 0.01) discard;
    frg = c;
}
)";

GLint modelLoc, repeticoesLoc, deslocamentoLoc, alphaLoc;
GLuint VAO;

// ---------------- Utilidades ----------------
Textura carregarTextura(const std::string &arquivo, bool repetir) {
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
    GLint wrap = repetir ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dados);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(dados);
    glBindTexture(GL_TEXTURE_2D, 0);
    return {id, w, h};
}

// Desenha um quadrado texturizado (translação * rotação * escala)
void desenhar(const Textura &t, glm::vec2 centro, glm::vec2 tamanho, float anguloGraus = 0.0f,
              glm::vec2 repeticoes = glm::vec2(1.0f), glm::vec2 desloc = glm::vec2(0.0f), float alpha = 1.0f) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(centro, 0.0f));
    model = glm::rotate(model, glm::radians(anguloGraus), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(tamanho, 1.0f));
    glBindTexture(GL_TEXTURE_2D, t.id);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform2fv(repeticoesLoc, 1, glm::value_ptr(repeticoes));
    glUniform2fv(deslocamentoLoc, 1, glm::value_ptr(desloc));
    glUniform1f(alphaLoc, alpha);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

glm::vec2 tamanho(const Textura &t, float escala) { return glm::vec2(t.largura, t.altura) * escala; }

// Traz x para o intervalo [-periodo/2, periodo/2) (repetição dos objetos na horizontal)
float envolver(float x, float periodo) { return x - periodo * std::floor(x / periodo + 0.5f); }

// ---------------- Parallax ----------------
// Camada repetida: o retângulo cobre a tela e só a coordenada de textura é deslocada
void desenharCamadaRepetida(const CamadaRepetida &c) {
    glm::vec2 tamTela = tamanho(c.tex, c.escala);
    glm::vec2 repeticoes = glm::vec2(WIDTH, HEIGHT) / tamTela;
    glm::vec2 desloc = camera * c.fator / tamTela;
    desenhar(c.tex, CENTRO, glm::vec2(WIDTH, HEIGHT), 0.0f, repeticoes, desloc);
}

// Camada de objetos: cada objeto é deslocado (na tela) por -câmera * fator
void desenharCamadaObjetos(const CamadaObjetos &c) {
    for (auto &o : c.objetos) {
        glm::vec2 p = o.pos - camera * c.fator;
        p.x = envolver(p.x, c.periodo);
        desenhar(*o.tex, CENTRO + p, tamanho(*o.tex, o.escala), o.velGiro * tempo);
    }
}

// ---------------- Lógica ----------------
void reiniciar() {
    foguete = camera = velocidade = glm::vec2(0.0f);
    indoAoClique = false;
    sentido = 1;
    angulo = 90.0f;
}

void atualizar(GLFWwindow *w, float dt) {
    tempo += dt;
    auto tecla = [w](int a, int b) { return glfwGetKey(w, a) == GLFW_PRESS || glfwGetKey(w, b) == GLFW_PRESS; };
    glm::vec2 dir(0.0f);
    if (tecla(GLFW_KEY_LEFT, GLFW_KEY_A)) dir.x -= 1.0f;
    if (tecla(GLFW_KEY_RIGHT, GLFW_KEY_D)) dir.x += 1.0f;
    if (tecla(GLFW_KEY_UP, GLFW_KEY_W)) dir.y -= 1.0f;
    if (tecla(GLFW_KEY_DOWN, GLFW_KEY_S)) dir.y += 1.0f;

    if (dir != glm::vec2(0.0f)) {
        indoAoClique = false;
    } else if (indoAoClique) { // sem teclas: voa em direção ao ponto clicado
        glm::vec2 d = alvoClique - foguete;
        if (glm::length(d) < 5.0f) indoAoClique = false;
        else dir = glm::clamp(d / 40.0f, -1.0f, 1.0f);
    }

    velocidade = dir * VELOCIDADE;
    foguete += velocidade * dt;
    foguete.y = glm::clamp(foguete.y, -LIMITE_Y, LIMITE_Y);
    if (dir.x != 0.0f) sentido = dir.x > 0.0f ? 1 : -1;

    // A câmera persegue o foguete com suavização (o foguete se adianta um pouco na tela)
    camera += (foguete - camera) * std::min(1.0f, SEGUIR_CAMERA * dt);

    // O sprite aponta para cima; +90° aponta para a direita e -90° para a esquerda.
    // Ao subir/descer o nariz inclina. Ao trocar de sentido ele gira passando pelo "nariz para cima".
    float alvo = sentido * (90.0f + INCLINACAO * velocidade.y / VELOCIDADE.y);
    angulo += (alvo - angulo) * std::min(1.0f, 8.0f * dt);
}

// ---------------- Desenho ----------------
void desenharCena() {
    for (auto &c : camadasFundo) desenharCamadaRepetida(c);
    desenharCamadaObjetos(camadaPlaneta);
    desenharCamadaObjetos(camadaLuas);

    // Foguete: animação por frames (a chama), mais rápida quando está se movendo
    float fps = glm::length(velocidade) > 0.0f ? 20.0f : 10.0f;
    const Textura &f = framesFoguete[corFoguete][(int)(tempo * fps) % FRAMES_FOGUETE];
    desenhar(f, CENTRO + (foguete - camera), tamanho(f, ESCALA_FOGUETE), angulo);

    desenharCamadaObjetos(camadaMeteoros);
}

void atualizarTitulo(GLFWwindow *window) {
    static std::string anterior;
    char buf[160];
    snprintf(buf, sizeof(buf), "Vivencial 2 - Parallax | camera (%.0f, %.0f) | nebulosa (%.0f, %.0f) | meteoros (%.0f, %.0f)",
             camera.x, camera.y, camera.x * camadasFundo[0].fator, camera.y * camadasFundo[0].fator,
             camera.x * camadaMeteoros.fator, camera.y * camadaMeteoros.fator);
    if (anterior != buf) glfwSetWindowTitle(window, buf);
    anterior = buf;
}

// ---------------- Entrada ----------------
void key_callback(GLFWwindow *window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GL_TRUE);
    if (key == GLFW_KEY_R) reiniciar();
    if (key >= GLFW_KEY_1 && key < GLFW_KEY_1 + (int)framesFoguete.size()) corFoguete = key - GLFW_KEY_1;
}

// Clique: converte o ponto da tela para o mundo (somando a câmera) e define o destino.
// Só conta um clique novo (solto -> pressionado), pois o WSLg pode repetir "press".
void mouse_button_callback(GLFWwindow *window, int button, int action, int) {
    static bool pressionado = false;
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    if (action == GLFW_RELEASE) pressionado = false;
    else if (action == GLFW_PRESS && !pressionado) {
        pressionado = true;
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        alvoClique = camera + glm::vec2(x, y) - CENTRO;
        alvoClique.y = glm::clamp(alvoClique.y, -LIMITE_Y, LIMITE_Y);
        indoAoClique = true;
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

// Quadrado unitário centralizado na origem
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
    const std::string fundo = "space_gas_giant/", craftpix = "craftpix-781145-free-space-shooter-game-objects/PNG/";
    camadasFundo = {
        {carregarTextura(fundo + "Background.png", true), 0.05f, 0.6f},
        {carregarTextura(fundo + "Stars.png", true), 0.12f, 0.5f},
        {carregarTextura(fundo + "Big_Stars.png", true), 0.25f, 0.7f},
    };

    texPlaneta = carregarTextura(fundo + "Gas_Giant.png", false);
    for (int i = 1; i <= 7; i++) texLuas.push_back(carregarTextura(fundo + "Moon_" + std::to_string(i) + ".png", false));
    for (int i = 1; i <= 10; i++) {
        std::string n = (i < 10 ? "0" : "") + std::to_string(i);
        texMeteoros.push_back(carregarTextura(craftpix + "Meteors/Meteor_" + n + ".png", false));
    }
    for (int cor = 1; cor <= 3; cor++) {
        std::vector<Textura> frames;
        for (int i = 0; i < FRAMES_FOGUETE; i++)
            frames.push_back(carregarTextura(craftpix + "Sprites/Missile/Missile_" + std::to_string(cor) + "_Flying_00" +
                                             std::to_string(i) + ".png", false));
        framesFoguete.push_back(frames);
    }

    // Posições (relativas ao centro da tela) de cada objeto nas suas camadas
    camadaPlaneta = {0.40f, 3000.0f, {{&texPlaneta, {170.0f, -40.0f}, 0.32f, 0.0f}}};
    camadaLuas = {0.60f, 2400.0f, {{&texLuas[0], {-260.0f, -170.0f}, 1.0f, 0.0f},
                                    {&texLuas[1], {380.0f, 150.0f}, 0.9f, 0.0f},
                                    {&texLuas[2], {-620.0f, 120.0f}, 1.1f, 0.0f},
                                    {&texLuas[3], {720.0f, -200.0f}, 1.2f, 0.0f},
                                    {&texLuas[4], {1050.0f, 60.0f}, 1.0f, 0.0f},
                                    {&texLuas[5], {-950.0f, -60.0f}, 0.9f, 0.0f},
                                    {&texLuas[6], {-40.0f, 230.0f}, 0.8f, 0.0f}}};
    camadaMeteoros = {1.40f, 2600.0f, {{&texMeteoros[0], {-330.0f, 230.0f}, 0.30f, 12.0f},
                                        {&texMeteoros[2], {420.0f, -250.0f}, 0.22f, -18.0f},
                                        {&texMeteoros[4], {900.0f, 200.0f}, 0.35f, 8.0f},
                                        {&texMeteoros[6], {-850.0f, -220.0f}, 0.25f, -10.0f},
                                        {&texMeteoros[8], {1250.0f, -30.0f}, 0.18f, 25.0f},
                                        {&texMeteoros[3], {-1200.0f, 120.0f}, 0.28f, 15.0f}}};
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

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Vivencial 2 - Parallax", nullptr, nullptr);
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
    alphaLoc = glGetUniformLocation(shaderID, "alpha");
    glUniform1i(glGetUniformLocation(shaderID, "tex"), 0);
    glActiveTexture(GL_TEXTURE0);

    // Projeção ortográfica com a janela do mundo igual à tela: 1 unidade = 1 pixel (y para baixo)
    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    carregarAssets();
    reiniciar();

    std::cout << "\nControles:\n"
                 "  setas / WASD: move o foguete | clique: voa ate o ponto\n"
                 "  1 2 3: troca o foguete | R: volta ao inicio | ESC: sai\n";

    double ultimoTempo = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        double agora = glfwGetTime();
        float dt = std::min((float)(agora - ultimoTempo), 1.0f / 30.0f);
        ultimoTempo = agora;
        atualizar(window, dt);
        atualizarTitulo(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
