#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "lib/stb_image/stb_image.h"
#include "lib/imgui/imgui.h"
#include "lib/imgui/imgui_impl_glfw.h"
#include "lib/imgui/imgui_impl_opengl3.h"
#include "shader.h"
#include "camera.h"
#include "model.h"
#include "skybox.h"

// shaders
std::string vertexShaderPath = "shaders/shader.vert";
std::string fragmentShaderPath = "shaders/shader.frag";

std::string postVertexShaderPath = "shaders/post.vert";
std::string postFragmentShaderPath = "shaders/post.frag";

std::string skyboxVertexShaderPath = "shaders/skybox.vert";
std::string skyboxFragmentShaderPath = "shaders/skybox.frag";

// viewport
const int INITIAL_VIEWPORT_WIDTH = 800;
const int INITIAL_VIEWPORT_HEIGHT = 600;
const float IMGUI_FONT_SCALE = 1.0f;

// camera
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraInitialPosition = glm::vec3(0.0f, 0.0f, 3.0f);
float cameraInitialYaw = 0;
float cameraInitialPitch = 0;
Camera camera(
    cameraUp, 
    cameraInitialPosition, 
    cameraInitialYaw, 
    cameraInitialPitch, 
    INITIAL_VIEWPORT_WIDTH, 
    INITIAL_VIEWPORT_HEIGHT
);

// time
float frameTimeDelta = 0.0f; // time between current frame and last frame
float lastFrameTime = 0.0f; // time of last frame

// invoked on window resizes, update gl window to the current size
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    camera.setWindowDimensions(width, height);
}

bool mouseCameraEnabled = true;

// handle input such as keyboard
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        mouseCameraEnabled = true;
    }

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        mouseCameraEnabled = false;
    }
}

void processMouseMovement(GLFWwindow* window, double xPos, double yPos) {
    if (mouseCameraEnabled) {
        camera.processMouseMovement(window, xPos, yPos);
    }
}

// GOAL: split out main shader from post-processing shader
int main()
{
    glfwInit();

    // see window creation options here: https://www.glfw.org/docs/latest/window.html#window_hints
    const char* glslVersion = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // core profile doesn't include unneeded backwards compat features

    GLFWwindow* window = glfwCreateWindow(INITIAL_VIEWPORT_WIDTH, INITIAL_VIEWPORT_HEIGHT, "TinyEngine", NULL, NULL);

    if (window == NULL) {
        std::cout << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // GLAD manages the function pointers for OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // OpenGL options 

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, INITIAL_VIEWPORT_WIDTH, INITIAL_VIEWPORT_HEIGHT); // set initial viewport size
    
    // GLFW options
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, processMouseMovement);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // stb options
    stbi_set_flip_vertically_on_load(true);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = IMGUI_FONT_SCALE;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    // Shader
    Shader shader(vertexShaderPath.c_str(), fragmentShaderPath.c_str());
    Shader postShader(postVertexShaderPath.c_str(), postFragmentShaderPath.c_str());
    Shader skyboxShader(skyboxVertexShaderPath.c_str(), skyboxFragmentShaderPath.c_str());

    // Model
    Skybox skybox("resources/skybox");
    //Model backpack("resources/backpack/backpack.obj");
    Model sphere("resources/sphere/sphere.gltf");

    // Lights
    std::vector<glm::vec3> lightPositions = {
        glm::vec3(0.0f, 0.0f, 10.0f)
    };

    std::vector<glm::vec3> lightColors = {
        glm::vec3(150.0f, 150.0f, 150.0f)
    };

    while (!glfwWindowShouldClose(window)) {
        // calculate frame time
        float currentTime = glfwGetTime();
        frameTimeDelta = lastFrameTime - currentTime;
        lastFrameTime = currentTime;

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("TinyEngine");                          // Create a window called "Hello, world!" and append into it.
        ImGui::CollapsingHeader("General");
        ImGui::Text("Average FPS %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        camera.drawDebugPanel();

        ImGui::CollapsingHeader("Model");
        float scale = 1.0;
        ImGui::SliderFloat("scale", &scale, 0.0, 100);

        ImGui::End();
        
        // ImGui::ShowDemoWindow();

        ImGuiIO& io = ImGui::GetIO();

        if (mouseCameraEnabled) {
            io.ConfigFlags = io.ConfigFlags | ImGuiConfigFlags_NoMouse;
        }
        else {
            io.ConfigFlags = io.ConfigFlags & !ImGuiConfigFlags_NoMouse;
        }

        // input
        processInput(window);
        glfwSetInputMode(window, GLFW_CURSOR, mouseCameraEnabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL); // don't show cursor on the windo
        camera.processKeyboard(window, frameTimeDelta);
        glm::vec3 cameraPosition = camera.getPosition();

        // rendering commands
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw our models 
        glm::mat4 projection = camera.getProjectionMatrix();
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 model;

        // backpack
        shader.use();
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(scale, scale, scale));
        shader.setModelViewProjectionMatrices(model, view, projection);

        // sphere
        shader.setFloat("ambientOcclusion", 0.5f);
        shader.setVec3Array("lightPositions", lightPositions);
        shader.setVec3Array("lightColors", lightColors);
        shader.setVec3("cameraPosition", cameraPosition);

        sphere.Draw(shader);


        // skybox (draw this last to avoid running fragment shader in places where objects are present
        skyboxShader.use();
        model = glm::mat4(1.0f);
        glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); // remove translation so skybox is always surrounding camera

        skyboxShader.setModelViewProjectionMatrices(model, skyboxView, projection);
        skybox.Draw(shader);



        // draw ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // check events and swap back buffer to display it in the window
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}