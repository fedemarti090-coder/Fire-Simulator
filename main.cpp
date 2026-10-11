#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <fstream>
#include <sstream>

std::string loadShaderSource(const std::string& filepath){
    std::ifstream file(filepath);
    if(!file.is_open()){
        std::cerr << "Impossibile aprire lo shader" << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compileShader(GLenum type, const char* source){
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Errore compilazione shader grafico: " << infoLog << std::endl;
    }
    return shader;
}

int main(){

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "fistra GLFW", nullptr, nullptr);
    if(!window){
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Errore nell'inizializzazione di GLAD!" << std::endl;
        return -1;
    }

    // CREAZIONE DELLE TEXTURE
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 800, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // COMPILAZIONE SHADER
    std::string computeCode = loadShaderSource("shaders/simulate.glsl");
    const char* cShaderCode = computeCode.c_str();

    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, &cShaderCode, NULL);
    glCompileShader(computeShader);

    int success;
    char infoLog[512];
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(computeShader, 512, NULL, infoLog);
        std::cerr << "Errore compilazione Compute Shader:\n" << infoLog << std::endl;
    }

    GLuint computeProgram = glCreateProgram();
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);
    glDeleteShader(computeShader);

    // FULL SCREEN QUAD
    float quadVertices[] = {
        -1.0f,  1.0f,    0.0f, 1.0f,
        -1.0f, -1.0f,    0.0f, 0.0f,
         1.0f, -1.0f,    1.0f, 0.0f,

        -1.0f,  1.0f,    0.0f, 1.0f,
         1.0f, -1.0f,    1.0f, 0.0f,
         1.0f,  1.0f,    1.0f, 1.0f
    };

    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // FRAGMENT E VERTEX SHADER
    std::string screenVertSourceRaw = loadShaderSource("shaders/screen/screen.vert");
    const char* screenVertSource = screenVertSourceRaw.c_str();

    std::string screenFragSourceRaw = loadShaderSource("shaders/screen/screen.frag");
    const char* screenFragSource = screenFragSourceRaw.c_str();

    GLuint vertShader = compileShader(GL_VERTEX_SHADER, screenVertSource);
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, screenFragSource);
    GLuint screenProgram = glCreateProgram();
    glAttachShader(screenProgram, vertShader);
    glAttachShader(screenProgram, fragShader);
    glLinkProgram(screenProgram);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);


    glUseProgram(screenProgram);
    glUniform1i(glGetUniformLocation(screenProgram, "screenTexture"), 0);



    while (!glfwWindowShouldClose(window)){

        glUseProgram(computeProgram);

        float currentTime = (float)glfwGetTime();
        glUniform1f(glGetUniformLocation(computeProgram, "u_time"), currentTime);

        
        glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
        glDispatchCompute(50, 38, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(screenProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteTextures(1, &texture);
    glDeleteProgram(computeProgram);
    glDeleteProgram(screenProgram);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}