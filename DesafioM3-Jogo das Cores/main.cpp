/*
 * Desafio M3 - Jogo das Cores
 *
 * Uma grade de retângulos com cores aleatórias é exibida. Ao clicar em um
 * retângulo, ele e todos os retângulos de cor semelhante (distância euclidiana
 * normalizada no espaço RGB abaixo da tolerância) são removidos.
 * Cada tentativa vale menos pontos que a anterior; o objetivo é limpar a
 * grade com o menor número de tentativas.
 *
 * Controles:
 *   Botão esquerdo do mouse - seleciona uma cor
 *   R                        - reinicia o jogo
 *   ESC                      - sai
 *
 * Implementado com OpenGL 3.3 core: um único VAO/VBO (quadrado unitário),
 * matriz de modelo por retângulo (translação + escala) e cor via uniform.
 */

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include <glad/glad.h>  // carrega as funções do OpenGL moderno
#include <GLFW/glfw3.h> // cria a janela e trata mouse/teclado

#include <glm/glm.hpp>                  // vetores e matrizes
#include <glm/gtc/matrix_transform.hpp> // translate, scale, ortho
#include <glm/gtc/type_ptr.hpp>         // value_ptr (envia matrizes para o shader)

// ---------------- Configurações do jogo ----------------
const GLuint WIDTH = 800, HEIGHT = 600;
const int COLUNAS = 8, LINHAS = 10;
const float TOLERANCIA = 0.2f;   // distância máxima (0..1) para considerar cores semelhantes
const int PONTOS_INICIAIS = 10;  // pontos por retângulo na primeira tentativa
const float ESPACO = 2.0f;       // espaçamento (px) entre retângulos

// Tamanho de cada célula da grade, em pixels
const float CEL_W = (float)WIDTH / COLUNAS;
const float CEL_H = (float)HEIGHT / LINHAS;

// Cada célula guarda sua cor e se ainda está na tela
struct Celula {
    glm::vec3 cor;
    bool ativa;
};

// ---------------- Estado do jogo ----------------
Celula grade[LINHAS][COLUNAS];
int tentativas = 0;
int pontuacao = 0;
int restantes = 0;
std::mt19937 rng{std::random_device{}()}; // gerador de números aleatórios

// Vertex Shader: posiciona cada vértice na tela (projeção * modelo * posição)
const GLchar *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 vp;
uniform mat4 projection;
uniform mat4 model;
void main() {
    gl_Position = projection * model * vec4(vp, 1.0);
}
)";

// Fragment Shader: pinta o retângulo com a cor recebida pelo uniform "fc"
const GLchar *fragmentShaderSource = R"(
#version 330 core
uniform vec4 fc;
out vec4 frg;
void main() {
    frg = fc;
}
)";

// Mostra pontos, tentativas e retângulos restantes no título da janela
void atualizarTitulo(GLFWwindow *window) {
    std::string titulo = "Jogo das Cores | Pontos: " + std::to_string(pontuacao) +
                         " | Tentativas: " + std::to_string(tentativas) +
                         " | Restantes: " + std::to_string(restantes);
    if (restantes == 0) titulo += " | FIM! Pressione R para reiniciar";
    glfwSetWindowTitle(window, titulo.c_str());
}

// Sorteia uma cor RGB aleatória para cada célula e zera o placar
void iniciarJogo(GLFWwindow *window) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (auto &linha : grade)
        for (auto &c : linha) c = {glm::vec3(dist(rng), dist(rng), dist(rng)), true};
    tentativas = 0;
    pontuacao = 0;
    restantes = LINHAS * COLUNAS;
    std::cout << "\n=== Novo jogo ===" << std::endl;
    atualizarTitulo(window);
}

// Distância euclidiana entre duas cores RGB.
// Dividimos por sqrt(3) (maior distância possível) para o resultado ficar entre 0 e 1.
float distanciaCor(const glm::vec3 &a, const glm::vec3 &b) {
    return glm::length(a - b) / std::sqrt(3.0f);
}

// Lógica principal: remove todas as células com cor parecida à clicada e calcula os pontos
void selecionarCelula(GLFWwindow *window, int lin, int col) {
    // Ignora cliques fora da grade ou em células já removidas
    if (lin < 0 || lin >= LINHAS || col < 0 || col >= COLUNAS || !grade[lin][col].ativa) return;

    glm::vec3 alvo = grade[lin][col].cor;
    int removidos = 0;
    // Percorre a grade e desativa as células dentro da tolerância (inclui a própria clicada)
    for (auto &linha : grade)
        for (auto &c : linha)
            if (c.ativa && distanciaCor(c.cor, alvo) <= TOLERANCIA) {
                c.ativa = false;
                removidos++;
            }

    // Pontuação: cada tentativa vale 1 ponto a menos por retângulo (mínimo 1)
    tentativas++;
    int pontosPorRetangulo = std::max(1, PONTOS_INICIAIS - (tentativas - 1));
    int ganho = removidos * pontosPorRetangulo;
    pontuacao += ganho;
    restantes -= removidos;

    std::cout << "Tentativa " << tentativas << ": " << removidos << " retangulo(s) removido(s), +"
              << ganho << " pontos (total: " << pontuacao << ")" << std::endl;
    if (restantes == 0)
        std::cout << "Fim de jogo! Pontuacao final: " << pontuacao << " em " << tentativas
                  << " tentativas. Pressione R para reiniciar." << std::endl;
    atualizarTitulo(window);
}

// Chamada pelo GLFW quando um botão do mouse é pressionado
void mouse_button_callback(GLFWwindow *window, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
    double xpos, ypos;
    int winW, winH;
    glfwGetCursorPos(window, &xpos, &ypos);
    glfwGetWindowSize(window, &winW, &winH);
    if (winW == 0 || winH == 0) return;
    // converte coordenadas da janela para o sistema de coordenadas do mundo (WIDTH x HEIGHT)
    float x = (float)xpos * WIDTH / winW;
    float y = (float)ypos * HEIGHT / winH;
    // Dividir a posição pelo tamanho da célula dá a linha e a coluna clicadas
    selecionarCelula(window, (int)(y / CEL_H), (int)(x / CEL_W));
}

// Chamada pelo GLFW quando uma tecla é pressionada: ESC fecha, R reinicia
void key_callback(GLFWwindow *window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GL_TRUE);
    if (key == GLFW_KEY_R) iniciarJogo(window);
}

// Ajusta a área de desenho quando a janela é redimensionada
void framebuffer_size_callback(GLFWwindow *, int width, int height) {
    glViewport(0, 0, width, height);
}

// Compila um shader (vertex ou fragment) e mostra o erro, se houver
GLuint compilarShader(GLenum tipo, const GLchar *fonte) {
    GLuint shader = glCreateShader(tipo);
    glShaderSource(shader, 1, &fonte, nullptr);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLchar log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Erro ao compilar shader:\n" << log << std::endl;
    }
    return shader;
}

// Junta o vertex e o fragment shader em um programa usado na GPU
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
        GLchar log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Erro ao linkar programa:\n" << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// Fonte bitmap 5x7: cada caractere é descrito por 7 linhas de 5 colunas ('1' = pixel aceso)
const std::unordered_map<char, const char *> FONTE = {
    {'A', "01110" "10001" "10001" "11111" "10001" "10001" "10001"},
    {'B', "11110" "10001" "10001" "11110" "10001" "10001" "11110"},
    {'C', "01110" "10001" "10000" "10000" "10000" "10001" "01110"},
    {'D', "11110" "10001" "10001" "10001" "10001" "10001" "11110"},
    {'E', "11111" "10000" "10000" "11110" "10000" "10000" "11111"},
    {'F', "11111" "10000" "10000" "11110" "10000" "10000" "10000"},
    {'G', "01110" "10001" "10000" "10111" "10001" "10001" "01111"},
    {'H', "10001" "10001" "10001" "11111" "10001" "10001" "10001"},
    {'I', "01110" "00100" "00100" "00100" "00100" "00100" "01110"},
    {'J', "00111" "00010" "00010" "00010" "00010" "10010" "01100"},
    {'K', "10001" "10010" "10100" "11000" "10100" "10010" "10001"},
    {'L', "10000" "10000" "10000" "10000" "10000" "10000" "11111"},
    {'M', "10001" "11011" "10101" "10101" "10001" "10001" "10001"},
    {'N', "10001" "10001" "11001" "10101" "10011" "10001" "10001"},
    {'O', "01110" "10001" "10001" "10001" "10001" "10001" "01110"},
    {'P', "11110" "10001" "10001" "11110" "10000" "10000" "10000"},
    {'Q', "01110" "10001" "10001" "10001" "10101" "10010" "01101"},
    {'R', "11110" "10001" "10001" "11110" "10100" "10010" "10001"},
    {'S', "01111" "10000" "10000" "01110" "00001" "00001" "11110"},
    {'T', "11111" "00100" "00100" "00100" "00100" "00100" "00100"},
    {'U', "10001" "10001" "10001" "10001" "10001" "10001" "01110"},
    {'V', "10001" "10001" "10001" "10001" "10001" "01010" "00100"},
    {'W', "10001" "10001" "10001" "10101" "10101" "10101" "01010"},
    {'X', "10001" "10001" "01010" "00100" "01010" "10001" "10001"},
    {'Y', "10001" "10001" "01010" "00100" "00100" "00100" "00100"},
    {'Z', "11111" "00001" "00010" "00100" "01000" "10000" "11111"},
    {'0', "01110" "10001" "10011" "10101" "11001" "10001" "01110"},
    {'1', "00100" "01100" "00100" "00100" "00100" "00100" "01110"},
    {'2', "01110" "10001" "00001" "00010" "00100" "01000" "11111"},
    {'3', "11111" "00010" "00100" "00010" "00001" "10001" "01110"},
    {'4', "00010" "00110" "01010" "10010" "11111" "00010" "00010"},
    {'5', "11111" "10000" "11110" "00001" "00001" "10001" "01110"},
    {'6', "00110" "01000" "10000" "11110" "10001" "10001" "01110"},
    {'7', "11111" "00001" "00010" "00100" "01000" "01000" "01000"},
    {'8', "01110" "10001" "10001" "01110" "10001" "10001" "01110"},
    {'9', "01110" "10001" "10001" "01111" "00001" "00010" "01100"},
    {':', "00000" "01100" "01100" "00000" "01100" "01100" "00000"},
    {'-', "00000" "00000" "00000" "11111" "00000" "00000" "00000"},
    {'!', "00100" "00100" "00100" "00100" "00100" "00000" "00100"},
};

// Localização dos uniforms "model" e "fc" no shader
GLint modelLoc, colorLoc;

// Desenha um retângulo: o quadrado unitário é movido (translate) para (x,y)
// e esticado (scale) para w x h; a cor vai para o fragment shader via uniform
void desenharRetangulo(float x, float y, float w, float h, const glm::vec4 &cor) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(colorLoc, 1, glm::value_ptr(cor));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Desenha o texto centralizado horizontalmente na linha y; px = tamanho de cada pixel da fonte
void desenharTextoCentralizado(const std::string &texto, float y, float px, const glm::vec4 &cor) {
    // Cada letra ocupa 6 "pixels" de largura (5 da letra + 1 de espaço)
    float x0 = (WIDTH - texto.size() * 6 * px + px) / 2.0f;
    for (size_t i = 0; i < texto.size(); i++) {
        auto it = FONTE.find(texto[i]);
        if (it == FONTE.end()) continue; // caractere sem desenho (ex.: espaço)
        // Para cada '1' do mapa da letra, desenha um quadradinho
        for (int lin = 0; lin < 7; lin++)
            for (int col = 0; col < 5; col++)
                if (it->second[lin * 5 + col] == '1')
                    desenharRetangulo(x0 + (i * 6 + col) * px, y + lin * px, px, px, cor);
    }
}

// Tela de fim de jogo: escurece o fundo e mostra pontuação e instruções
void desenharTelaFinal() {
    // Retângulo preto com alpha 0.75 (semitransparente) cobrindo a tela toda
    desenharRetangulo(0, 0, WIDTH, HEIGHT, glm::vec4(0.0f, 0.0f, 0.0f, 0.75f));
    desenharTextoCentralizado("FIM DE JOGO!", 150, 8, glm::vec4(1.0f, 0.85f, 0.2f, 1.0f));
    desenharTextoCentralizado("PONTUACAO: " + std::to_string(pontuacao), 270, 5, glm::vec4(1.0f));
    desenharTextoCentralizado("TENTATIVAS: " + std::to_string(tentativas), 330, 5, glm::vec4(1.0f));
    desenharTextoCentralizado("R - REINICIAR   ESC - SAIR", 450, 3, glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
}

// Quadrado unitário com origem no canto superior esquerdo (0,0)-(1,1)
// É a única geometria do programa: todos os retângulos e letras reutilizam ela
GLuint setupGeometry() {
    GLfloat vertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
    };
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    // VBO: envia os vértices para a memória da GPU
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // VAO: descreve o formato dos dados (atributo 0 = posição "vp", 3 floats por vértice)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return VAO;
}

int main() {
    // Inicializa o GLFW e pede um contexto OpenGL 3.3 core
    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Jogo das Cores", nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // Registra as funções que respondem ao teclado, mouse e redimensionamento
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // GLAD carrega os ponteiros das funções OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    GLuint shaderID = setupShader();
    GLuint VAO = setupGeometry();

    glUseProgram(shaderID);
    modelLoc = glGetUniformLocation(shaderID, "model");
    colorLoc = glGetUniformLocation(shaderID, "fc");

    // Habilita transparência (usada no fundo escuro da tela final)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Projeção ortográfica com origem no canto superior esquerdo (y para baixo)
    // Assim podemos trabalhar direto em pixels (0..800 x 0..600)
    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    iniciarJogo(window);

    // Loop principal: trata eventos, limpa a tela e desenha tudo a cada frame
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);
        // Percorre linhas e colunas desenhando só as células ainda ativas
        for (int y = 0; y < LINHAS; y++) {
            for (int x = 0; x < COLUNAS; x++) {
                const Celula &c = grade[y][x];
                if (c.ativa)
                    desenharRetangulo(x * CEL_W + ESPACO / 2, y * CEL_H + ESPACO / 2,
                                      CEL_W - ESPACO, CEL_H - ESPACO, glm::vec4(c.cor, 1.0f));
            }
        }
        // Quando não sobra nenhum retângulo, mostra a tela de fim de jogo por cima
        if (restantes == 0) desenharTelaFinal();
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    // Libera os recursos da GPU e fecha o GLFW
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderID);
    glfwTerminate();
    return 0;
}
