/*
 * Desafio M2 - Triângulos | Parte 1 (sem matriz de transformação)
 *
 * Exercício 1: função createTriangle, que cria um VAO para um triângulo a
 *              partir das coordenadas (em NDC) dos vértices v0, v1 e v2.
 * Exercício 2: instancia 5 triângulos com essa função, guarda os VAOs em um
 *              vector e desenha todos iterando sobre ele no loop de renderização.
 *
 * Controles:
 *   ESC - sai
 */

#include <iostream>
#include <vector>

#include <glad/glad.h>  // carrega as funções do OpenGL moderno
#include <GLFW/glfw3.h> // cria a janela e trata o teclado

const GLuint WIDTH = 800, HEIGHT = 600;

// Vertex Shader: sem matriz, a posição já está em coordenadas normalizadas (NDC)
const GLchar *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 vp;
void main() {
    gl_Position = vec4(vp, 1.0);
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

// Chamada pelo GLFW quando uma tecla é pressionada: ESC fecha
void key_callback(GLFWwindow *window, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
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

// Exercício 1: cria um triângulo com os vértices v0, v1 e v2 e retorna o VAO
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

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Desafio M2 - Parte 1: 5 triangulos", nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
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
    GLint colorLoc = glGetUniformLocation(shaderID, "fc");

    // Exercício 2: 5 triângulos, cada VAO guardado em um vector
    std::vector<GLuint> triangulos = {
        createTriangle(-0.9f, 0.3f, -0.5f, 0.3f, -0.7f, 0.8f),   // superior esquerdo
        createTriangle(-0.2f, 0.3f, 0.2f, 0.3f, 0.0f, 0.8f),     // superior central
        createTriangle(0.5f, 0.3f, 0.9f, 0.3f, 0.7f, 0.8f),      // superior direito
        createTriangle(-0.6f, -0.8f, -0.1f, -0.8f, -0.35f, -0.2f), // inferior esquerdo
        createTriangle(0.1f, -0.8f, 0.6f, -0.8f, 0.35f, -0.2f),  // inferior direito
    };
    // Uma cor para cada triângulo, apenas para diferenciá-los
    const GLfloat cores[][4] = {
        {1.0f, 0.3f, 0.3f, 1.0f},
        {0.3f, 1.0f, 0.3f, 1.0f},
        {0.3f, 0.5f, 1.0f, 1.0f},
        {1.0f, 0.9f, 0.2f, 1.0f},
        {0.9f, 0.4f, 1.0f, 1.0f},
    };

    // Loop principal: trata eventos, limpa a tela e desenha os triângulos
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        for (size_t i = 0; i < triangulos.size(); i++) {
            glUniform4fv(colorLoc, 1, cores[i % 5]);
            glBindVertexArray(triangulos[i]);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    // Libera os recursos da GPU e fecha o GLFW
    glDeleteVertexArrays((GLsizei)triangulos.size(), triangulos.data());
    glDeleteProgram(shaderID);
    glfwTerminate();
    return 0;
}
