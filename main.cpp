#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

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

    GLFWwindow* window = glfwCreateWindow(800, 600, "finestra GLFW", nullptr, nullptr);
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
    
    GLuint textures[2];
    glGenTextures(2, textures);

    std::vector<float> zeroBuffer(800 * 600 * 4, 0.0f);

    for (int i=0; i<2; i++){
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 800, 600, 0, GL_RGBA, GL_FLOAT, nullptr);

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 800, 600, GL_RGBA, GL_FLOAT, zeroBuffer.data());
    }


    glBindTexture(GL_TEXTURE_2D, 0);

    bool usaTextureA = true;
    
    // COMPILAZIONE SHADER
    std::string computeCode = loadShaderSource("shaders/simulate.glsl");
    const char* cShaderCode = computeCode.c_str();

    GLuint computeShader = compileShader(GL_COMPUTE_SHADER, cShaderCode);
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
    std::string screenFragSourceRaw = loadShaderSource("shaders/screen/screen.frag");

    const char* screenVertSource = screenVertSourceRaw.c_str();
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

        GLuint readTex = usaTextureA ? textures[0] : textures[1];
        GLuint writeTex = usaTextureA ? textures[1] : textures[0];

        glUseProgram(computeProgram);

        float currentTime = (float)glfwGetTime();
        glUniform1f(glGetUniformLocation(computeProgram, "u_time"), currentTime);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, readTex);
        glUniform1i(glGetUniformLocation(computeProgram, "u_readTex"), 0);

        glBindImageTexture(0, writeTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        glDispatchCompute(50, 37, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(screenProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, writeTex);
        
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        usaTextureA = !usaTextureA;
    }

    glDeleteTextures(2, textures);
    glDeleteProgram(computeProgram);
    glDeleteProgram(screenProgram);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}