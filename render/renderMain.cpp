#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <iostream>
#include <chrono>
#include <list>
#include "math.h"
#include "keyboard.h"
#include "renderer3d.cpp"
#include "camera.h"
#include "fps.h"

using namespace std;

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

    static void cleanup() {
        if (instance) {
            delete instance;
            instance = nullptr;
        }
    }
};


// Initialize static instance
KeyboardE* KeyboardE::instance = nullptr;


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
    renderer.camera.fPitch += sensitivity *(mouseY - screenHeight / 2);

    if (renderer.camera.fPitch < -1.5f)
        renderer.camera.fPitch = -1.5f;

    if (renderer.camera.fPitch > 1.5f)
        renderer.camera.fPitch = 1.5f;

    keyboard->setMousePosition({ (float)screenWidth / 2, (float)screenHeight / 2 });
    glfwSetCursorPos(glfwGetCurrentContext(), (float)screenWidth / 2, (float)screenHeight / 2);
}

void handleTick(Renderer3d& renderer, float fElapsedTime) {
    renderer.drawEvent();
    handleKeyboardInput(renderer, fElapsedTime);
    handleMouseInput(renderer, fElapsedTime);
}

int main() {
    if (!glfwInit()) {
        return -1;
    }

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    int screenWidth = mode->width;
    int screenHeight = mode->height;

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Renderer3d", NULL, NULL);
   
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwGetWindowSize(window, &screenWidth, &screenHeight);

    Renderer3d renderer = Renderer3d(60.0f, (float)screenWidth, (float)screenHeight);   
    KeyboardE* keyboard = KeyboardE::getInstance();
    keyboard->setCallbacks(window);
    Fps fpsCounter = Fps();

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    while (!glfwWindowShouldClose(window)) {

        double deltaTime = fpsCounter.currentTime - fpsCounter.lastTime;
        if (deltaTime > 1.0) {
            float fps = fpsCounter.fps;

            char title[256];
            snprintf(title, sizeof(title), "Render3d - FPS: %.2f", fps);
            glfwSetWindowTitle(window, title);
        }

        glfwMakeContextCurrent(window);
        glClear(GL_COLOR_BUFFER_BIT);

        handleTick(renderer, (float)deltaTime);
        fpsCounter.update();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    KeyboardE::cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}







