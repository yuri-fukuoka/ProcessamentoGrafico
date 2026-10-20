# Atividade Vivencial 1 - Criando Triângulos a partir do Clique do Mouse

**Integrante:** Yuri Fukuoka de Oliveira

Versão modificada do Exercício 3 do Desafio M2 (Parte 2): agora **cada clique cria um vértice**, e **a cada 3 vértices um triângulo é formado**, cada um com uma **cor nova**.

## Como compilar e executar

Na raiz do repositório (`ProcessamentoGrafico/`):

```bash
cmake -S . -B build
cmake --build build --target Vivencial1
./build/Vivencial1
```

## Controles

| Tecla / mouse   | Ação                                                        |
|-----------------|-------------------------------------------------------------|
| Clique esquerdo | cria um vértice na posição clicada                          |
| Clique direito  | desfaz o último vértice (também "desmonta" o último triângulo) |
| `C`             | limpa a tela                                                |
| `ESC`           | sai                                                         |

Os vértices que ainda não formam um triângulo aparecem como **pontos brancos**. O título da janela mostra quantos triângulos existem e quantos vértices estão pendentes (x/3).

## Como funciona

- **Struct Triangle:** diferente do M2, agora guarda a **geometria** (os 3 vértices) e a cor, já que cada triângulo tem um formato diferente.
- **Projeção:** ortográfica com a janela do mundo do mesmo tamanho da tela (800x600, y para baixo), então **1 unidade = 1 pixel** e a posição do mouse é usada direto como vértice.
- **Um único VAO/VBO dinâmico:** o VBO é criado com capacidade para 3000 vértices (`GL_DYNAMIC_DRAW`). Quando algo muda, os vértices de todos os triângulos e os pendentes são reenviados com `glBufferSubData`. Cada triângulo ocupa 3 posições seguidas do buffer e é desenhado com `glDrawArrays(GL_TRIANGLES, i * 3, 3)`. Os pendentes são desenhados com `GL_POINTS`.
- **Cores novas:** o matiz (HSV) de cada triângulo avança pela **razão áurea** (≈ 0.618), o que garante cores bem diferentes entre triângulos seguidos. Depois a cor é convertida para RGB.
