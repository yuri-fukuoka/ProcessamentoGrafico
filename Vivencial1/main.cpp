/*
 * Atividade Vivencial 1 - Criando Triângulos a partir do Clique do Mouse
 *
 * Versão modificada do Exercício 3 (Desafio M2 - Parte 2):
 *   1) Cada clique cria apenas 1 vértice
 *   2) A cada 3 vértices criados, um triângulo é formado
 *   3) Cada novo triângulo recebe uma cor nova
 *
 * A estrutura Triangle agora armazena a geometria (3 vértices) e a cor.
 * Usa um único VAO/VBO dinâmico com todos os vértices, e projeção ortográfica
 * com a janela do mundo do mesmo tamanho da tela (800 x 600): 1 unidade = 1 pixel.
 *
 * Controles:
 *   Botão esquerdo do mouse - cria um vértice na posição clicada
 *   Botão direito do mouse  - desfaz o último vértice
 *   C                        - limpa a tela
 *   ESC                      - sai
 */

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <glad/glad.h>  // carrega as funções do OpenGL moderno
#include <GLFW/glfw3.h> // cria a janela e trata mouse/teclado

#include <glm/glm.hpp>                  // vetores e matrizes
#include <glm/gtc/matrix_transform.hpp> // ortho
#include <glm/gtc/type_ptr.hpp>         // value_ptr (envia matrizes para o shader)

const GLuint WIDTH = 800, HEIGHT = 600;
const size_t MAX_VERTICES = 3 * 1000; // capacidade do VBO

// Cada triângulo guarda sua geometria (vértices em coordenadas do mundo) e sua cor
struct Triangle {
    glm::vec2 vertices[3];
    glm::vec3 cor;
};

// ---------------- Estado ----------------
std::vector<Triangle> triangulos;
std::vector<glm::vec2> pendentes; // vértices clicados que ainda não formam um triângulo
bool bufferDesatualizado = true;  // indica que o VBO precisa ser reenviado à GPU

// Vertex Shader: aplica a projeção ortográfica a cada vértice
const GLchar *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 vp;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(vp, 0.0, 1.0);
}
)";

// Fragment Shader: pinta com a cor recebida pelo uniform "fc"
const GLchar *fragmentShaderSource = R"(
#version 330 core
uniform vec4 fc;
out vec4 frg;
void main() {
    frg = fc;
}
)";

// Converte HSV (h em [0,1)) para RGB
glm::vec3 hsv2rgb(float h, float s, float v) {
    glm::vec3 k = glm::mod(glm::vec3(5.0f, 3.0f, 1.0f) + h * 6.0f, 6.0f);
    return v - v * s * glm::clamp(glm::min(k, 4.0f - k), 0.0f, 1.0f);
}

// Gera uma cor nova para cada triângulo: o matiz avança pela razão áurea,
// garantindo cores bem distintas entre triângulos consecutivos
glm::vec3 novaCor(size_t indice) {
    const float phi = 0.618033988749895f;
    float h = std::fmod(0.1f + indice * phi, 1.0f);
    return hsv2rgb(h, 0.75f, 0.95f);
}

// Mostra a quantidade de triângulos e vértices pendentes no título da janela
void atualizarTitulo(GLFWwindow *window) {
    std::string titulo = "Vivencial 1 | Triangulos: " + std::to_string(triangulos.size()) +
                         " | Vertices pendentes: " + std::to_string(pendentes.size()) +
                         "/3 | Clique para criar vertice, C para limpar";
    glfwSetWindowTitle(window, titulo.c_str());
}

// Chamada pelo GLFW quando um botão do mouse é pressionado
void mouse_button_callback(GLFWwindow *window, int button, int action, int) {
    if (action != GLFW_PRESS) return;

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        // Desfaz o último vértice (se não houver pendentes, "desmonta" o último triângulo)
        if (pendentes.empty() && !triangulos.empty()) {
            Triangle t = triangulos.back();
            triangulos.pop_back();
            pendentes = {t.vertices[0], t.vertices[1]};
        } else if (!pendentes.empty()) {
            pendentes.pop_back();
        }
        bufferDesatualizado = true;
        atualizarTitulo(window);
        return;
    }

    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    if ((triangulos.size() + 1) * 3 > MAX_VERTICES) return;

    double xpos, ypos;
    int winW, winH;
    glfwGetCursorPos(window, &xpos, &ypos);
    glfwGetWindowSize(window, &winW, &winH);
    if (winW == 0 || winH == 0) return;

    // Mundo = 800 x 600 com origem no canto superior esquerdo (igual à tela).
    // A escala mantém o mapeamento correto mesmo se a janela for redimensionada.
    float x = (float)xpos * WIDTH / winW;
    float y = (float)ypos * HEIGHT / winH;
    pendentes.push_back(glm::vec2(x, y));

    // A cada 3 vértices, forma-se um novo triângulo com uma cor nova
    if (pendentes.size() == 3) {
        triangulos.push_back({{pendentes[0], pendentes[1], pendentes[2]}, novaCor(triangulos.size())});
        pendentes.clear();
    }
    bufferDesatualizado = true;
    atualizarTitulo(window);
}

// Chamada pelo GLFW quando uma tecla é pressionada: ESC fecha, C limpa a tela
void key_callback(GLFWwindow *window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GL_TRUE);
    if (key == GLFW_KEY_C) {
        triangulos.clear();
        pendentes.clear();
        bufferDesatualizado = true;
        atualizarTitulo(window);
    }
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

// Cria um VAO único com um VBO dinâmico com capacidade para MAX_VERTICES vértices 2D
GLuint setupGeometry(GLuint &VBO) {
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    // VBO: reserva memória na GPU; os dados são enviados conforme os cliques
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);
    // VAO: atributo 0 = posição "vp", 2 floats por vértice
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (GLvoid *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return VAO;
}

// Envia ao VBO os vértices dos triângulos seguidos dos vértices pendentes
void atualizarBuffer(GLuint VBO) {
    std::vector<glm::vec2> dados;
    dados.reserve(triangulos.size() * 3 + pendentes.size());
    for (const Triangle &t : triangulos)
        dados.insert(dados.end(), t.vertices, t.vertices + 3);
    dados.insert(dados.end(), pendentes.begin(), pendentes.end());
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, dados.size() * sizeof(glm::vec2), dados.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    bufferDesatualizado = false;
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

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Vivencial 1", nullptr, nullptr);
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
    glUseProgram(shaderID);
    GLint projLoc = glGetUniformLocation(shaderID, "projection");
    GLint colorLoc = glGetUniformLocation(shaderID, "fc");

    // Projeção ortográfica: janela do mundo 800 x 600, com y crescendo para baixo
    // (mesma orientação das coordenadas do mouse), então 1 unidade = 1 pixel
    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    GLuint VBO;
    GLuint VAO = setupGeometry(VBO);
    glPointSize(8.0f);

    atualizarTitulo(window);

    // Loop principal: trata eventos, limpa a tela e desenha triângulos e vértices pendentes
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (bufferDesatualizado) atualizarBuffer(VBO);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);
        // Cada triângulo ocupa 3 vértices consecutivos no VBO e é desenhado com sua cor
        for (size_t i = 0; i < triangulos.size(); i++) {
            glUniform4fv(colorLoc, 1, glm::value_ptr(glm::vec4(triangulos[i].cor, 1.0f)));
            glDrawArrays(GL_TRIANGLES, (GLint)(i * 3), 3);
        }
        // Vértices pendentes (ainda sem triângulo) aparecem como pontos brancos
        if (!pendentes.empty()) {
            glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
            glDrawArrays(GL_POINTS, (GLint)(triangulos.size() * 3), (GLsizei)pendentes.size());
        }
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    // Libera os recursos da GPU e fecha o GLFW
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderID);
    glfwTerminate();
    return 0;
}
