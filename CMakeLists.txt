cmake_minimum_required(VERSION 3.16)
project(ProcessamentoGrafico LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1)
FetchContent_MakeAvailable(glfw glm)

add_library(glad STATIC common/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Desafio M2 - Triangulos
add_executable(M2_Parte1 "DesafioM2-Triangulos/Parte1.cpp")
target_link_libraries(M2_Parte1 PRIVATE glad glfw)
add_executable(M2_Parte2 "DesafioM2-Triangulos/Parte2.cpp")
target_link_libraries(M2_Parte2 PRIVATE glad glfw glm::glm)

# Desafio M3 - Jogo das Cores
add_executable(JogoDasCores "DesafioM3-Jogo das Cores/main.cpp")
target_link_libraries(JogoDasCores PRIVATE glad glfw glm::glm)

# Desafio M4 - Mapeamento de Texturas
add_executable(M4_Texturas "DesafioM4-Mapeamento de Texturas/main.cpp")
target_link_libraries(M4_Texturas PRIVATE glad glfw glm::glm)
target_compile_definitions(M4_Texturas PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/")

# Desafio M5 - Navegacao em Camadas (Tappy Plane com parallax)
add_executable(M5_Camadas "DesafioM5-Navegacao em Camadas/main.cpp")
target_link_libraries(M5_Camadas PRIVATE glad glfw glm::glm)
target_compile_definitions(M5_Camadas PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/kenney_tappy-plane/PNG/")

# Desafio M6 - Jogo em Tilemaps (Dungeon Escape)
add_executable(M6_Dungeon "DesafioM6-Jogo em Tilemaps/main.cpp")
target_link_libraries(M6_Dungeon PRIVATE glad glfw glm::glm)
target_compile_definitions(M6_Dungeon PRIVATE
    ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/kenney_tinyDungeon/"
    MAPA_ARQUIVO="${CMAKE_SOURCE_DIR}/DesafioM6-Jogo em Tilemaps/mapa.txt")

# Atividade Vivencial 1 - Triangulos com clique do mouse
add_executable(Vivencial1 "Vivencial1/main.cpp")
target_link_libraries(Vivencial1 PRIVATE glad glfw glm::glm)

# Atividade Vivencial 2 - Parallax Scrolling (foguete no espaço)
add_executable(Vivencial2 "Vivencial2/main.cpp")
target_link_libraries(Vivencial2 PRIVATE glad glfw glm::glm)
target_compile_definitions(Vivencial2 PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/")
