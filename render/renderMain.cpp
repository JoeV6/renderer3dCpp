#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <iostream>
#include <chrono>
#include <windows.h>
#include <list>
#include "math.h"
#include "keyboard.h"
#include "camera.h"
#include "fps.h"
#include "renderer3d.cpp"

using namespace std;

// Put console window on second monitor (if available)
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFOEX);
    if (GetMonitorInfo(hMonitor, &monitorInfo)) {
        int monitorCount = reinterpret_cast<int*>(dwData)[0];
        if (monitorCount == 1) {
            // Set the position of the console window to the second monitor
            HWND consoleWindow = GetConsoleWindow();
            SetWindowPos(consoleWindow, 0, monitorInfo.rcWork.left, monitorInfo.rcWork.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            return FALSE;  // Stop enumeration
        }
        reinterpret_cast<int*>(dwData)[0]++;
    }
    return TRUE;  // Continue enumeration
}

class KeyboardE : public Keyboard {
    static KeyboardE* instance;

    KeyboardE() {
        for (int i = 0; i < GLFW_KEY_LAST; i++) {
            keys[i] = false;
        }

        for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++) {
            buttons[i] = false;
        }
    };

public:
    static KeyboardE* getInstance() { 
        if (!instance) {
            instance = new KeyboardE;
        }
        return instance;
    }

    void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) override {
        KeyboardE* keyboard = KeyboardE::getInstance();
        if (action == GLFW_PRESS) {
            keyboard->setButtonPressed(button, true);
        }
        else if (action == GLFW_RELEASE) {
            keyboard->setButtonPressed(button, false);
        }
    }

    void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) override {
        KeyboardE* keyboard = KeyboardE::getInstance();

        keyboard->setMousePosition({ (float)xpos, (float)ypos });
    }

    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        KeyboardE* keyboard = KeyboardE::getInstance();
        if (action == GLFW_PRESS) {
            keyboard->setKeyPressed(key, true);
        }
        else if (action == GLFW_RELEASE) {
            keyboard->setKeyPressed(key, false);
        }
    }

    void handleKeyboardInput(Renderer3d& renderer, float fElapsedTime) {
        KeyboardE* keyboard = KeyboardE::getInstance();

        Vec3d vForward = renderer.camera.vLookDir * 0.2f;

        if (keyboard->isKeyPressed(GLFW_KEY_W)) {
            renderer.camera.vCameraPosition = renderer.camera.vCameraPosition + vForward;
        }

        if (keyboard->isKeyPressed(GLFW_KEY_S)) {
            renderer.camera.vCameraPosition = renderer.camera.vCameraPosition - vForward;
        }

        if (keyboard->isKeyPressed(GLFW_KEY_A)) {
            renderer.camera.vCameraPosition = renderer.camera.vCameraPosition + vForward.cross(renderer.camera.vUp).normalize() * 0.2f;
        }

        if (keyboard->isKeyPressed(GLFW_KEY_D)) {
            renderer.camera.vCameraPosition = renderer.camera.vCameraPosition - vForward.cross(renderer.camera.vUp).normalize() * 0.2f;
        }

        if (keyboard->isKeyPressed(GLFW_KEY_SPACE)) {
            renderer.camera.vCameraPosition.y += 0.2f;
        }

        if (keyboard->isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
            renderer.camera.vCameraPosition.y -= 0.2f;
        }

        if (keyboard->isKeyPressed(GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
            KeyboardE::cleanup();
            glfwDestroyWindow(glfwGetCurrentContext());
            glfwTerminate();
            FreeConsole();
            HWND consoleWindow = GetConsoleWindow();
            PostMessage(consoleWindow, WM_CLOSE, 0, 0);
            ExitProcess(0);
        }
    }

    void handleMouseInput(Renderer3d& renderer, float fElapsedTime) {
        KeyboardE* keyboard = KeyboardE::getInstance();

        Vec2f mousePosition = keyboard->getMousePosition();

        int screenWidth = renderer.screenWidth;
        int screenHeight = renderer.screenHeight;
        float sensitivity = 0.003f;
        int mouseX = mousePosition.x;
        int mouseY = mousePosition.y;

        renderer.camera.fYaw -= sensitivity * (mouseX - screenWidth / 2);
        renderer.camera.fPitch += sensitivity * (mouseY - screenHeight / 2);

        if (renderer.camera.fPitch < -1.5f)
            renderer.camera.fPitch = -1.5f;

        if (renderer.camera.fPitch > 1.5f)
            renderer.camera.fPitch = 1.5f;

        keyboard->setMousePosition({ (float)screenWidth / 2, (float)screenHeight / 2 });
        glfwSetCursorPos(glfwGetCurrentContext(), (float)screenWidth / 2, (float)screenHeight / 2);
    }

    static void cleanup() {
        if (instance) {
            delete instance;
            instance = nullptr;
        }
    }
};

// Initialize static instance
KeyboardE* KeyboardE::instance = nullptr;

class RenderApp {
private:
    vector<Mesh> renderedMeshes;
    vector<PhysicsObject> physicsObjects;

    unique_ptr<Renderer3d> renderer;
    unique_ptr <Physics3d> physics;

    Fps fpsCounter;

    GLFWwindow* window;

    int screenWidth;
    int screenHeight;

public:
    RenderApp() : window(nullptr), screenWidth(0), screenHeight(0), renderer(nullptr), physics(nullptr) {}

    int run() {
        if (!initializeGLFW()) {
            return -1;
        }

        if (!createWindow()) {
            glfwTerminate();
            return -1;
        }

        initialize();

        while (!glfwWindowShouldClose(window)) {
            double deltaTime = fpsCounter.currentTime - fpsCounter.lastTime;

            glfwMakeContextCurrent(window);
            glClear(GL_COLOR_BUFFER_BIT);

            handleTick((float)deltaTime);
            fpsCounter.update();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        cleanup();
        return 0;
    }

private:
    bool initializeGLFW() {
        if (!glfwInit()) {
            return false;
        }

        return true;
    }

    bool createWindow() {
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

        screenWidth = mode->width;
        screenHeight = mode->height;

        int monitorCount = 0;
        EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitorCount));

        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

        window = glfwCreateWindow(screenWidth, screenHeight, "Renderer3d", NULL, NULL);

        return window != nullptr;
    }

    void initialize() {
        glfwGetWindowSize(window, &screenWidth, &screenHeight);

        // Initialize renderer
        renderer = std::make_unique<Renderer3d>(60.0f, screenWidth, screenHeight, renderedMeshes);
        physics = std::make_unique<Physics3d>(renderedMeshes, physicsObjects);

        // Initialize meshes and objects
        Mesh mesh;
        mesh.LoadFromObjectFile("mountains.obj");
        mesh.increaseSize(5.0f);
        renderedMeshes.push_back(mesh);

        // Initialize other components
        KeyboardE* keyboard = KeyboardE::getInstance();
        keyboard->setCallbacks(window);

        glfwMakeContextCurrent(window);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void handleTick(float fElapsedTime) {
        KeyboardE* keyboard = KeyboardE::getInstance();

        renderer->drawEvent();
        keyboard->handleKeyboardInput(*renderer, fElapsedTime);
        keyboard->handleMouseInput(*renderer, fElapsedTime);
    }

    void cleanup() {
        glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
        KeyboardE::cleanup();
        glfwDestroyWindow(glfwGetCurrentContext());
        glfwTerminate();
        FreeConsole();
        HWND consoleWindow = GetConsoleWindow();
        PostMessage(consoleWindow, WM_CLOSE, 0, 0);
        ExitProcess(0);
    }
};

int main() {
    RenderApp app;

    return app.run();
}









