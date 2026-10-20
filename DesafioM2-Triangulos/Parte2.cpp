/*
 * Desafio M2 - Triângulos | Parte 2 (com matriz de transformação)
 *
 * Exercício 3: um único VAO com o triângulo padrão
 *              v0 (-0.1, -0.1), v1 (0.1, -0.1), v2 (0.0, 0.1)
 *              é reutilizado para desenhar vários triângulos (struct Triangle),
 *              cada um com sua posição e cor. A cada clique do mouse um novo
 *              triângulo é criado na posição clicada, com cor RGB sorteada.
 *
 * Controles:
 *   Botão esquerdo do mouse - cria um triângulo na posição clicada
 *   C                        - limpa a tela
 *   ESC                      - sai
 */

#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <glad/glad.h>  // carrega as funções do OpenGL moderno
#include <GLFW/glfw3.h> // cria a janela e trata mouse/teclado

#include <glm/glm.hpp>                  // vetores e matrizes
#include <glm/gtc/matrix_transform.hpp> // translate
#include <glm/gtc/type_ptr.hpp>         // value_ptr (envia matrizes para o shader)

const GLuint WIDTH = 800, HEIGHT = 600;

// Cada triângulo guarda apenas sua posição e sua cor; a geometria é compartilhada
struct Triangle {
    glm::vec2 posicao;
    glm::vec3 cor;
};

// ---------------- Estado ----------------
std::vector<Triangle> triangulos;
std::mt19937 rng{std::random_device{}()}; // gerador de números aleatórios

// Vertex Shader: aplica a matriz de modelo (translação) a cada vértice
const GLchar *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 vp;
uniform mat4 model;
void main() {
    gl_Position = model * vec4(vp, 1.0);
}
)";

// Fragment Shader: pinta o triângulo com a cor recebida pelo uniform "fc"
const GLchar *fragmentShaderSource = R"(
#version 330 core
uniform vec4 fc;
out vec4 frg;
void main() {
    frg = fc;
}
)";

// Mostra a quantidade de triângulos no título da janela
void atualizarTitulo(GLFWwindow *window) {
    std::string titulo = "Desafio M2 - Parte 2 | Triangulos: " + std::to_string(triangulos.size()) +
                         " | Clique para criar, C para limpar";
    glfwSetWindowTitle(window, titulo.c_str());
}

// Chamada pelo GLFW quando um botão do mouse é pressionado: cria um novo triângulo
void mouse_button_callback(GLFWwindow *window, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
    double xpos, ypos;
    int winW, winH;
    glfwGetCursorPos(window, &xpos, &ypos);
    glfwGetWindowSize(window, &winW, &winH);
    if (winW == 0 || winH == 0) return;
    // Converte de coordenadas da janela (pixels, y para baixo) para NDC (-1..1, y para cima)
    float x = 2.0f * (float)xpos / winW - 1.0f;
    float y = 1.0f - 2.0f * (float)ypos / winH;
    // Sorteia as componentes RGB da cor
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    triangulos.push_back({glm::vec2(x, y), glm::vec3(dist(rng), dist(rng), dist(rng))});
    atualizarTitulo(window);
}

// Chamada pelo GLFW quando uma tecla é pressionada: ESC fecha, C limpa a tela
void key_callback(GLFWwindow *window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GL_TRUE);
    if (key == GLFW_KEY_C) {
        triangulos.clear();
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

// Cria um triângulo com os vértices v0, v1 e v2 e retorna o VAO (mesma função do Exercício 1)
GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2) {
    GLfloat vertices[] = {
        x0, y0, 0.0f, // v0
        x1, y1, 0.0f, // v1
        x2, y2, 0.0f, // v2
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

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Desafio M2 - Parte 2", nullptr, nullptr);
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
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint colorLoc = glGetUniformLocation(shaderID, "fc");

    // Um único VAO com o triângulo padrão, reutilizado por todos os triângulos
    GLuint VAO = createTriangle(-0.1f, -0.1f, 0.1f, -0.1f, 0.0f, 0.1f);

    atualizarTitulo(window);

    // Loop principal: trata eventos, limpa a tela e desenha todos os triângulos
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);
        for (const Triangle &t : triangulos) {
            // Matriz de modelo: move o triângulo padrão para a posição do clique
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(t.posicao, 0.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform4fv(colorLoc, 1, glm::value_ptr(glm::vec4(t.cor, 1.0f)));
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    // Libera os recursos da GPU e fecha o GLFW
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderID);
    glfwTerminate();
    return 0;
}
