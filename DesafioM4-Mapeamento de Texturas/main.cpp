/*
 * Desafio M4 - Mapeamento de Texturas
 *
 * Um retângulo do tamanho da janela recebe uma textura de fundo (terra) e,
 * sobre ele, o usuário cola "adesivos" (frutas em PNG com transparência).
 * O fragment shader descarta (discard) os fragmentos transparentes dos
 * adesivos, aplica filtros de imagem e faz o "flip" horizontal/vertical
 * alterando as coordenadas de textura.
 *
 * Controles:
 *   Botão esquerdo         - cola um adesivo novo / seleciona e arrasta um existente
 *   Botão direito          - remove o adesivo sob o cursor
 *   1, 2, 3, 4             - escolhe a fruta do próximo adesivo
 *   TAB                    - alterna a seleção (fundo -> adesivos)
 *   Q / E (segurar)        - gira o adesivo selecionado
 *   Z / X (segurar), roda  - diminui / aumenta o adesivo selecionado
 *   H / V                  - flip horizontal / vertical do objeto selecionado
 *   F                      - troca o filtro do objeto selecionado
 *   DELETE                 - remove o adesivo selecionado
 *   C                      - remove todos os adesivos
 *   ESC                    - sai
 *
 * Implementado com OpenGL 3.3 core: um único VAO/VBO (quadrado unitário
 * centralizado na origem), matriz de modelo por objeto (translação + rotação
 * + escala) e texturas carregadas com stb_image.
 */

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <glad/glad.h>  // carrega as funções do OpenGL moderno
#include <GLFW/glfw3.h> // cria a janela e trata mouse/teclado

#include <glm/glm.hpp>                  // vetores e matrizes
#include <glm/gtc/matrix_transform.hpp> // translate, rotate, scale, ortho
#include <glm/gtc/type_ptr.hpp>         // value_ptr (envia matrizes para o shader)

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> // leitura de imagens PNG/JPG

// ---------------- Configurações ----------------
const GLuint WIDTH = 800, HEIGHT = 600;
const float TAMANHO_BASE = 150.0f;      // maior lado (px) de um adesivo com escala 1
const float VEL_ROTACAO = 90.0f;        // graus por segundo
const float VEL_ESCALA = 1.0f;          // fator por segundo
const float ESCALA_MIN = 0.2f, ESCALA_MAX = 5.0f;
const float REPETICOES_FUNDO = 2.0f;    // quantas vezes a textura de fundo se repete na horizontal

const std::string FUNDO = "Ground070_1K-JPG/Ground070_1K-JPG_Color.jpg";
const std::vector<std::string> ARQUIVOS_ADESIVOS = {"apple.png", "banana.png", "grape.png", "orange.png"};
const std::vector<std::string> NOMES_ADESIVOS = {"Maca", "Banana", "Uva", "Laranja"};
const std::vector<std::string> NOMES_FILTROS = {"Nenhum", "Tons de cinza", "Sepia", "Negativo",
                                                "Binarizacao", "Desfoque", "Bordas (Sobel)", "Pixelizacao"};

// Uma textura carregada na GPU e suas dimensões (para manter a proporção)
struct Textura {
    GLuint id;
    int largura, altura;
};

// Propriedades de mapeamento comuns ao fundo e aos adesivos
struct Mapeamento {
    bool flipH = false, flipV = false;
    int filtro = 0;
};

// Um adesivo colado na cena
struct Adesivo {
    int tipo;          // índice da textura em "texAdesivos"
    glm::vec2 posicao; // centro, em pixels
    float angulo;      // graus
    float escala;
    Mapeamento map;
};

// ---------------- Estado da aplicação ----------------
Textura texFundo;
std::vector<Textura> texAdesivos;
Mapeamento mapFundo;
std::vector<Adesivo> adesivos; // o último da lista é desenhado por cima
int tipoAtual = 0;             // fruta usada no próximo adesivo
int selecionado = -1;          // -1 = fundo, >= 0 = índice do adesivo
bool arrastando = false;
glm::vec2 offsetArraste;       // distância entre o cursor e o centro do adesivo arrastado

// Vertex Shader: posiciona o vértice e gera a coordenada de textura (0..1)
// a partir da posição do quadrado unitário (-0.5..0.5)
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

// Fragment Shader: flip, filtros, descarte de fragmentos transparentes e contorno de seleção
const GLchar *fragmentShaderSource = R"(
#version 330 core
in vec2 uv;
uniform sampler2D tex;
uniform bool flipH;
uniform bool flipV;
uniform int filtro;
uniform vec2 repeticoes; // > 1 repete a textura (usado no fundo)
uniform bool destaque;   // desenha uma moldura no objeto selecionado
out vec4 frg;

float luminancia(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

void main() {
    // Moldura tracejada de seleção (antes do discard, para aparecer mesmo nas áreas transparentes)
    vec2 borda = min(uv, 1.0 - uv) / fwidth(uv); // distância até a borda, em pixels
    if (destaque && min(borda.x, borda.y) < 2.0) {
        float t = (gl_FragCoord.x + gl_FragCoord.y) / 8.0;
        frg = mod(t, 2.0) < 1.0 ? vec4(1.0) : vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Flip: espelha a coordenada de textura no eixo escolhido
    vec2 st = uv;
    if (flipH) st.x = 1.0 - st.x;
    if (flipV) st.y = 1.0 - st.y;
    st *= repeticoes;

    // Tamanho de 1 pixel da tela em coordenadas de textura (para filtros de vizinhança)
    vec2 px = fwidth(st);
    vec4 cor = texture(tex, st);

    if (filtro == 1) {                       // tons de cinza
        cor.rgb = vec3(luminancia(cor.rgb));
    } else if (filtro == 2) {                // sépia
        cor.rgb = clamp(vec3(dot(cor.rgb, vec3(0.393, 0.769, 0.189)),
                             dot(cor.rgb, vec3(0.349, 0.686, 0.168)),
                             dot(cor.rgb, vec3(0.272, 0.534, 0.131))), 0.0, 1.0);
    } else if (filtro == 3) {                // negativo
        cor.rgb = 1.0 - cor.rgb;
    } else if (filtro == 4) {                // binarização
        cor.rgb = vec3(step(0.5, luminancia(cor.rgb)));
    } else if (filtro == 5) {                // desfoque (média 9x9)
        vec4 soma = vec4(0.0);
        for (int i = -4; i <= 4; i++)
            for (int j = -4; j <= 4; j++)
                soma += texture(tex, st + vec2(i, j) * px);
        cor = soma / 81.0;
    } else if (filtro == 6) {                // bordas (operador de Sobel na luminância)
        float l[9];
        for (int i = 0; i < 9; i++)
            l[i] = luminancia(texture(tex, st + vec2(i % 3 - 1, i / 3 - 1) * px * 1.5).rgb);
        float gx = -l[0] - 2.0 * l[3] - l[6] + l[2] + 2.0 * l[5] + l[8];
        float gy = -l[0] - 2.0 * l[1] - l[2] + l[6] + 2.0 * l[7] + l[8];
        cor.rgb = vec3(clamp(length(vec2(gx, gy)) * 2.0, 0.0, 1.0));
    } else if (filtro == 7) {                // pixelização (blocos de 12 pixels da tela)
        vec2 bloco = px * 12.0;
        cor = texture(tex, (floor(st / bloco) + 0.5) * bloco);
    }

    // Descarta os fragmentos transparentes: o fundo do adesivo não é desenhado
    if (cor.a < 0.1) discard;
    frg = cor;
}
)";

// Localização dos uniforms no shader
GLint modelLoc, flipHLoc, flipVLoc, filtroLoc, repeticoesLoc, destaqueLoc;

// Carrega uma imagem do disco e cria a textura na GPU (com mipmaps)
Textura carregarTextura(const std::string &arquivo, GLint wrap) {
    std::string caminho = std::string(ASSETS_DIR) + arquivo;
    int w, h, canais;
    // Forçamos 4 canais (RGBA) para tratar JPG e PNG do mesmo jeito.
    // Não invertemos a imagem: a linha 0 (topo) fica em t = 0, que é o topo do
    // quadrado, pois a projeção usa y para baixo.
    unsigned char *dados = stbi_load(caminho.c_str(), &w, &h, &canais, 4);
    if (!dados) {
        std::cerr << "Falha ao carregar textura: " << caminho << std::endl;
        return {0, 1, 1};
    }
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dados);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(dados);
    glBindTexture(GL_TEXTURE_2D, 0);
    std::cout << "Textura carregada: " << arquivo << " (" << w << "x" << h << ")" << std::endl;
    return {id, w, h};
}

// Matriz de modelo de um adesivo: translação * rotação * escala (mantendo a proporção da imagem)
glm::mat4 matrizAdesivo(const Adesivo &a) {
    const Textura &t = texAdesivos[a.tipo];
    float maior = (float)std::max(t.largura, t.altura);
    glm::vec2 tamanho = TAMANHO_BASE * a.escala * glm::vec2(t.largura / maior, t.altura / maior);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(a.posicao, 0.0f));
    model = glm::rotate(model, glm::radians(a.angulo), glm::vec3(0.0f, 0.0f, 1.0f));
    return glm::scale(model, glm::vec3(tamanho, 1.0f));
}

// Envia os uniforms e desenha o quadrado com a textura indicada
void desenhar(const glm::mat4 &model, const Textura &tex, const Mapeamento &map,
              const glm::vec2 &repeticoes, bool destaque) {
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(flipHLoc, map.flipH);
    glUniform1i(flipVLoc, map.flipV);
    glUniform1i(filtroLoc, map.filtro);
    glUniform2fv(repeticoesLoc, 1, glm::value_ptr(repeticoes));
    glUniform1i(destaqueLoc, destaque);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Mapeamento do objeto selecionado (fundo ou adesivo)
Mapeamento &mapeamentoSelecionado() {
    return selecionado < 0 ? mapFundo : adesivos[selecionado].map;
}

// Converte a posição do cursor (janela) para o sistema do mundo (WIDTH x HEIGHT)
glm::vec2 cursorNoMundo(GLFWwindow *window) {
    double xpos, ypos;
    int winW, winH;
    glfwGetCursorPos(window, &xpos, &ypos);
    glfwGetWindowSize(window, &winW, &winH);
    if (winW == 0 || winH == 0) return glm::vec2(0.0f);
    return glm::vec2((float)xpos * WIDTH / winW, (float)ypos * HEIGHT / winH);
}

// Retorna o adesivo mais acima sob o ponto p (ou -1).
// O ponto é levado para o espaço local do adesivo com a inversa da matriz de modelo;
// se cair dentro do quadrado unitário (-0.5..0.5), houve acerto.
int adesivoEm(const glm::vec2 &p) {
    for (int i = (int)adesivos.size() - 1; i >= 0; i--) {
        glm::vec4 local = glm::inverse(matrizAdesivo(adesivos[i])) * glm::vec4(p, 0.0f, 1.0f);
        if (std::abs(local.x) <= 0.5f && std::abs(local.y) <= 0.5f) return i;
    }
    return -1;
}

// Mostra o estado atual no título da janela (só atualiza se mudou)
void atualizarTitulo(GLFWwindow *window) {
    static std::string anterior;
    std::string titulo = "Mapeamento de Texturas | Proximo: " + NOMES_ADESIVOS[tipoAtual] + " | Selecionado: ";
    if (selecionado < 0) titulo += "Fundo";
    else titulo += NOMES_ADESIVOS[adesivos[selecionado].tipo] + " #" + std::to_string(selecionado + 1);
    const Mapeamento &m = mapeamentoSelecionado();
    titulo += " | Filtro: " + NOMES_FILTROS[m.filtro];
    if (m.flipH) titulo += " | Flip H";
    if (m.flipV) titulo += " | Flip V";
    if (titulo != anterior) glfwSetWindowTitle(window, titulo.c_str());
    anterior = titulo;
}

void removerAdesivo(int i) {
    if (i < 0) return;
    adesivos.erase(adesivos.begin() + i);
    selecionado = -1;
    arrastando = false;
}

// Botão esquerdo: seleciona/arrasta o adesivo clicado ou cola um novo; direito: remove
void mouse_button_callback(GLFWwindow *window, int button, int action, int) {
    glm::vec2 p = cursorNoMundo(window);
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        int i = adesivoEm(p);
        if (i < 0) {
            adesivos.push_back({tipoAtual, p, 0.0f, 1.0f, {}});
            i = (int)adesivos.size() - 1;
        } else {
            // Traz o adesivo clicado para a frente (fim da lista)
            Adesivo a = adesivos[i];
            adesivos.erase(adesivos.begin() + i);
            adesivos.push_back(a);
            i = (int)adesivos.size() - 1;
        }
        selecionado = i;
        arrastando = true;
        offsetArraste = adesivos[i].posicao - p;
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        arrastando = false;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        removerAdesivo(adesivoEm(p));
    }
}

// Move o adesivo enquanto o botão esquerdo está pressionado
void cursor_pos_callback(GLFWwindow *window, double, double) {
    if (arrastando && selecionado >= 0) adesivos[selecionado].posicao = cursorNoMundo(window) + offsetArraste;
}

// Roda do mouse: escala o adesivo selecionado
void scroll_callback(GLFWwindow *, double, double yoffset) {
    if (selecionado < 0) return;
    float &e = adesivos[selecionado].escala;
    e = glm::clamp(e * std::pow(1.1f, (float)yoffset), ESCALA_MIN, ESCALA_MAX);
}

void key_callback(GLFWwindow *window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GL_TRUE);
    if (key >= GLFW_KEY_1 && key < GLFW_KEY_1 + (int)texAdesivos.size()) tipoAtual = key - GLFW_KEY_1;
    if (key == GLFW_KEY_TAB) selecionado = selecionado + 1 < (int)adesivos.size() ? selecionado + 1 : -1;
    if (key == GLFW_KEY_H) mapeamentoSelecionado().flipH ^= true;
    if (key == GLFW_KEY_V) mapeamentoSelecionado().flipV ^= true;
    if (key == GLFW_KEY_F) mapeamentoSelecionado().filtro = (mapeamentoSelecionado().filtro + 1) % NOMES_FILTROS.size();
    if (key == GLFW_KEY_DELETE || key == GLFW_KEY_BACKSPACE) removerAdesivo(selecionado);
    if (key == GLFW_KEY_C) {
        adesivos.clear();
        selecionado = -1;
        arrastando = false;
    }
}

// Rotação e escala contínuas enquanto as teclas estão pressionadas
void processarTeclasContinuas(GLFWwindow *window, float dt) {
    if (selecionado < 0) return;
    Adesivo &a = adesivos[selecionado];
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) a.angulo -= VEL_ROTACAO * dt;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) a.angulo += VEL_ROTACAO * dt;
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) a.escala /= 1.0f + VEL_ESCALA * dt;
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) a.escala *= 1.0f + VEL_ESCALA * dt;
    a.escala = glm::clamp(a.escala, ESCALA_MIN, ESCALA_MAX);
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
        GLchar log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
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
        GLchar log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "Erro ao linkar programa:\n" << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// Quadrado unitário centralizado na origem (-0.5..0.5), para girar em torno do centro.
// A coordenada de textura é calculada no vertex shader (uv = posição + 0.5).
GLuint setupGeometry() {
    GLfloat vertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
        -0.5f,  0.5f,
         0.5f,  0.5f,
    };
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return VAO;
}

int main() {
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

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Mapeamento de Texturas", nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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

    // Fundo com GL_REPEAT (textura "tileable"); adesivos com CLAMP para não vazar nas bordas
    texFundo = carregarTextura(FUNDO, GL_REPEAT);
    for (const auto &arq : ARQUIVOS_ADESIVOS) texAdesivos.push_back(carregarTextura(arq, GL_CLAMP_TO_EDGE));

    glUseProgram(shaderID);
    modelLoc = glGetUniformLocation(shaderID, "model");
    flipHLoc = glGetUniformLocation(shaderID, "flipH");
    flipVLoc = glGetUniformLocation(shaderID, "flipV");
    filtroLoc = glGetUniformLocation(shaderID, "filtro");
    repeticoesLoc = glGetUniformLocation(shaderID, "repeticoes");
    destaqueLoc = glGetUniformLocation(shaderID, "destaque");
    glUniform1i(glGetUniformLocation(shaderID, "tex"), 0);
    glActiveTexture(GL_TEXTURE0);

    // Transparência: as bordas suaves das frutas se misturam com o fundo
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Projeção ortográfica em pixels, com origem no canto superior esquerdo (y para baixo)
    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // O fundo é um retângulo do tamanho da janela; a textura se repete mantendo a proporção
    glm::mat4 modelFundo = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(WIDTH / 2.0f, HEIGHT / 2.0f, 0.0f)),
                                      glm::vec3(WIDTH, HEIGHT, 1.0f));
    glm::vec2 repeticoesFundo(REPETICOES_FUNDO,
                              REPETICOES_FUNDO * HEIGHT / WIDTH * texFundo.largura / texFundo.altura);

    std::cout << "\nControles:\n"
                 "  Clique esquerdo: cola adesivo / seleciona e arrasta\n"
                 "  Clique direito: remove adesivo\n"
                 "  1-4: escolhe a fruta (Maca, Banana, Uva, Laranja)\n"
                 "  TAB: alterna selecao (fundo -> adesivos)\n"
                 "  Q/E: gira | Z/X ou roda do mouse: escala\n"
                 "  H/V: flip horizontal/vertical | F: troca filtro\n"
                 "  DELETE: remove selecionado | C: limpa tudo | ESC: sai\n";

    double ultimoTempo = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        double agora = glfwGetTime();
        processarTeclasContinuas(window, (float)(agora - ultimoTempo));
        ultimoTempo = agora;
        atualizarTitulo(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);
        desenhar(modelFundo, texFundo, mapFundo, repeticoesFundo, false);
        for (size_t i = 0; i < adesivos.size(); i++) {
            const Adesivo &a = adesivos[i];
            desenhar(matrizAdesivo(a), texAdesivos[a.tipo], a.map, glm::vec2(1.0f), (int)i == selecionado);
        }
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteTextures(1, &texFundo.id);
    for (auto &t : texAdesivos) glDeleteTextures(1, &t.id);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderID);
    glfwTerminate();
    return 0;
}
