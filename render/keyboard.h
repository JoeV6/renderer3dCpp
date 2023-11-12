#pragma once

#include <GLFW/glfw3.h>
#include "math.h"

class Keyboard {

protected:
    bool keys[GLFW_KEY_LAST];
    bool buttons[GLFW_MOUSE_BUTTON_LAST];
    Vec2f mousePosition;

public:

    static void mouseButtonCallbackStatic(GLFWwindow* window, int button, int action, int mods) {
        Keyboard* instance = static_cast<Keyboard*>(glfwGetWindowUserPointer(window));
        if (instance) {
            instance->mouseButtonCallback(window, button, action, mods);
        }
    }

    static void cursorPosCallbackStatic(GLFWwindow* window, double xpos, double ypos) {
        Keyboard* instance = static_cast<Keyboard*>(glfwGetWindowUserPointer(window));
        if (instance) {
            instance->cursorPosCallback(window, xpos, ypos);
        }
    }

    static void keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods) {
        Keyboard* instance = static_cast<Keyboard*>(glfwGetWindowUserPointer(window));
        if (instance) {
            instance->keyCallback(window, key, scancode, action, mods);
        }
    }

    void setCallbacks(GLFWwindow* window) {
        glfwSetWindowUserPointer(window, this);
        glfwSetMouseButtonCallback(window, Keyboard::mouseButtonCallbackStatic);
        glfwSetCursorPosCallback(window, Keyboard::cursorPosCallbackStatic);
        glfwSetKeyCallback(window, Keyboard::keyCallbackStatic);
    }

    void resetMousePosition(float width, float height) {
        mousePosition = { width / 2, height / 2 };
    }

    void setMousePosition(Vec2f pos) {
        mousePosition = pos;
    }

    Vec2f getMousePosition() {
        return mousePosition;
    }

    bool isKeyPressed(int key) {
        return keys[key];
    }

    bool isButtonPressed(int button) {
        return buttons[button];
    }

    void setKeyPressed(int key, bool pressed) {
        keys[key] = pressed;
    }

    void setButtonPressed(int button, bool pressed) {
        buttons[button] = pressed;
    }

    virtual ~Keyboard() = default;
    virtual void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) = 0;
    virtual void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) = 0;
    virtual void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) = 0;
};