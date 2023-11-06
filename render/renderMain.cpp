#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <iostream>
#include <chrono>
#include "math.cpp"
#include <list>

using namespace std;

void drawTriangle(const Vec3d& vertex1, const Vec3d& vertex2, const Vec3d& vertex3, const Vec3d& color) {
    glBegin(GL_TRIANGLES);
    glColor3f(color.x, color.y, color.z);  // Red color

    glVertex2f(vertex1.x, vertex1.y);
    glVertex2f(vertex2.x, vertex2.y);
    glVertex2f(vertex3.x, vertex3.y);
    
    glEnd();
}

class Camera {
public:
    Vec3d vCameraPosition;
    Vec3d vLookDir;
    Vec3d vUp = { 0,1,0 };
    Vec3d vTarget = { 0,0,1 };

    float fYaw;
    float fPitch;

    Camera() {
		vCameraPosition = { 0.0f, 0.0f, 0.0f };
		vLookDir = { 0.0f, 0.0f, 1.0f };

		fYaw = 0.0f;
		fPitch = 0.0f;
	}

    Camera(Vec3d vCamera, Vec3d vLookDir) {
		this->vCameraPosition = vCamera;
		this->vLookDir = vLookDir;

		fYaw = 0.0f;
		fPitch = 0.0f;
	}
};

class Renderer3d {
    vector<Mesh> meshes;

    Mat4x4 worldMatrix;
    Mat4x4 viewMatrix;
    Mat4x4 projectionMatrix;

    float screenWidth;
    float screenHeight;

public:
    Camera camera;

    Renderer3d(float ffov, float width, float height)
        : screenWidth(width), screenHeight(height) {

        Mesh mesh;
        mesh.LoadFromObjectFile("mountains.obj");
        meshes.push_back(mesh);

        projectionMatrix = Mat4x4::Matrix_MakeProjection(ffov, width / height, 0.1f, 1000.0f);
    }

    void drawEvent() {
        drawMeshes();
    }

private:
    void drawMeshes() {
        setupMatrices();

        vector<Triangle> vecTrianglesToRaster;

        for (auto mesh : meshes) {
            for (auto tri : mesh.tris) {
                Triangle triProjected, triTransformed, triViewed;

                triTransformed.p[0] = Mat4x4::Matrix_MultiplyVector(worldMatrix, tri.p[0]);
                triTransformed.p[1] = Mat4x4::Matrix_MultiplyVector(worldMatrix, tri.p[1]);
                triTransformed.p[2] = Mat4x4::Matrix_MultiplyVector(worldMatrix, tri.p[2]);


                Vec3d normal, line1, line2;

                line1 = triTransformed.p[1] - triTransformed.p[0];
                line2 = triTransformed.p[2] - triTransformed.p[0];

                normal = line1.cross(line2);

                if (normal.length() == 0.0f) continue;

                normal = normal.normalize();

                Vec3d vCameraRay = triTransformed.p[0] - camera.vCameraPosition;

                float dotProduct = normal.dot(vCameraRay);

                if (dotProduct >= 0.0f) continue;

                Vec3d light_direction = { 0.0f, 1.0f, -1.0f };
                light_direction = light_direction.normalize();

                float dp = max(0.1f, light_direction.dot(normal));

                triTransformed.color = { dp, dp, dp };

                triViewed.p[0] = Mat4x4::Matrix_MultiplyVector(viewMatrix, triTransformed.p[0]);
                triViewed.p[1] = Mat4x4::Matrix_MultiplyVector(viewMatrix, triTransformed.p[1]);
                triViewed.p[2] = Mat4x4::Matrix_MultiplyVector(viewMatrix, triTransformed.p[2]);

                triViewed.color = triTransformed.color;

                int nClippedTriangles = 0;
                Triangle clipped[2];
                nClippedTriangles = Triangle::clipAgainstPlane({ 0.0f, 0.0f, 0.1f }, { 0.0f, 0.0f, 1.0f }, triViewed, clipped[0], clipped[1]);
                
                for (int n = 0; n < nClippedTriangles; n++) {

                    triProjected.p[0] = Mat4x4::Matrix_MultiplyVector(projectionMatrix, clipped[n].p[0]);
                    triProjected.p[1] = Mat4x4::Matrix_MultiplyVector(projectionMatrix, clipped[n].p[1]);
                    triProjected.p[2] = Mat4x4::Matrix_MultiplyVector(projectionMatrix, clipped[n].p[2]);

                    triProjected.color = clipped[n].color;

                    triProjected.p[0] = triProjected.p[0] / triProjected.p[0].w;
                    triProjected.p[1] = triProjected.p[1] / triProjected.p[1].w;
                    triProjected.p[2] = triProjected.p[2] / triProjected.p[2].w;

                    triProjected.p[0].x *= 1.0f;
                    triProjected.p[1].x *= 1.0f;
                    triProjected.p[2].x *= 1.0f;
                    triProjected.p[0].y *= 1.0f;
                    triProjected.p[1].y *= 1.0f;
                    triProjected.p[2].y *= 1.0f;

                    Vec3d vOffsetView = { 1,1,0 };
                    triProjected.p[0] = triProjected.p[0] + vOffsetView;
                    triProjected.p[1] = triProjected.p[1] + vOffsetView;
                    triProjected.p[2] = triProjected.p[2] + vOffsetView;

                    triProjected.p[0].x *= 0.5f;
                    triProjected.p[0].y *= 0.5f;
                    triProjected.p[1].x *= 0.5f;
                    triProjected.p[1].y *= 0.5f;
                    triProjected.p[2].x *= 0.5f;
                    triProjected.p[2].y *= 0.5f;

                    vecTrianglesToRaster.push_back(triProjected);
                }
            }
        }

        sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](Triangle& t1, Triangle& t2) {
			float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
			float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
			return z1 > z2;
		});

        for (auto& vecTrianglesToRaster : vecTrianglesToRaster) {
            Triangle clipped[2];
            list<Triangle> listTriangles;

            listTriangles.push_back(vecTrianglesToRaster);
            int nNewTriangles = 1;

            for (int p = 0; p < 4; p++) {
                int nTrisToAdd = 0;

                while (nNewTriangles > 0) {

                    Triangle test = listTriangles.front();
					listTriangles.pop_front();
					nNewTriangles--;

                    switch (p) {
					case 0:
						nTrisToAdd = Triangle::clipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, test, clipped[0], clipped[1]);
						break;
					case 1:
						nTrisToAdd = Triangle::clipAgainstPlane({ 0.0f, (float)screenHeight - 1, 0.0f }, { 0.0f, -1.0f, 0.0f }, test, clipped[0], clipped[1]);
						break;
					case 2:
						nTrisToAdd = Triangle::clipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]);
						break;
					case 3:
						nTrisToAdd = Triangle::clipAgainstPlane({ (float)screenWidth - 1, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]);
						break;
					}

                    for (int w = 0; w < nTrisToAdd; w++) {
						listTriangles.push_back(clipped[w]);
                    }
                }
            }
        }



        for (auto& triToRaster : vecTrianglesToRaster) {
			drawTriangle(triToRaster.p[0], triToRaster.p[1], triToRaster.p[2], triToRaster.color);
		}
    }

    void setupMatrices() {
        Mat4x4 translationMatrix;
        translationMatrix = Mat4x4::Matrix_MakeIdentity();
        translationMatrix = Mat4x4::Matrix_MakeTranslation(0.0f, 0.0f, 5.0f); //move obj

        worldMatrix = Mat4x4::Matrix_MakeIdentity();
        worldMatrix = Mat4x4::Matrix_MultiplyMatrix(worldMatrix, translationMatrix);

        Mat4x4 cameraRotationMatrix, cameraRotationMatrixX, cameraRotationMatrixY;
        cameraRotationMatrixY = Mat4x4::Matrix_MakeRotationY(camera.fYaw);
        cameraRotationMatrixX = Mat4x4::Matrix_MakeRotationX(camera.fPitch);

        cameraRotationMatrix = Mat4x4::Matrix_MultiplyMatrix(cameraRotationMatrixX, cameraRotationMatrixY);

        
        camera.vUp = { 0,1,0 };
        camera.vTarget = { 0,0,1 };
        camera.vLookDir = Mat4x4::Matrix_MultiplyVector(cameraRotationMatrixY, camera.vTarget);
        
        camera.vTarget = camera.vCameraPosition + camera.vLookDir;
       

        Mat4x4 cameraMatrix = Mat4x4::Matrix_PointAt(camera.vCameraPosition, camera.vTarget, camera.vUp);

        viewMatrix = Mat4x4::Matrix_QuickInverse(cameraMatrix);
    }
};

class Keyboard {
    static Keyboard* instance;
    bool keys[GLFW_KEY_LAST];
    bool buttons[GLFW_MOUSE_BUTTON_LAST];
    Vec2f mousePosition;

public:
    static Keyboard* getInstance() {
        if (!instance) {
            instance = new Keyboard();
        }
        return instance;
    }

    void setMousePosition(Vec2f pos) {
		mousePosition = pos;
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

private:
    Keyboard() {
        for (int i = 0; i < GLFW_KEY_LAST; i++) {
            keys[i] = false;
        }
    }

    Keyboard(const Keyboard& other) = delete;
    Keyboard& operator=(const Keyboard& other) = delete;
};

Keyboard* Keyboard::instance = nullptr;


void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Keyboard* keyboard = Keyboard::getInstance();
    if (action == GLFW_PRESS) {
		keyboard->setButtonPressed(button, true);
	}
    else if (action == GLFW_RELEASE) {
		keyboard->setButtonPressed(button, false);
	}
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    Keyboard* keyboard = Keyboard::getInstance();

    keyboard->setMousePosition({ (float)xpos, (float)ypos });
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Keyboard* keyboard = Keyboard::getInstance();
    if (action == GLFW_PRESS) {
		keyboard->setKeyPressed(key, true);
	}
    else if (action == GLFW_RELEASE) {
		keyboard->setKeyPressed(key, false);
	}
}

void handleTick(Renderer3d& renderer, float fElapsedTime) {
    renderer.drawEvent();

    Keyboard* keyboard = Keyboard::getInstance();
    
    Vec3d vForward = renderer.camera.vLookDir * 0.2f;
    
    if (keyboard->isKeyPressed(GLFW_KEY_W)) {
        renderer.camera.vCameraPosition = renderer.camera.vCameraPosition + vForward;
	}

    if (keyboard->isKeyPressed(GLFW_KEY_S)) {
        renderer.camera.vCameraPosition = renderer.camera.vCameraPosition - vForward;
    }

    if (keyboard->isKeyPressed(GLFW_KEY_A)) {
		renderer.camera.fYaw += 0.01f;
	}

    if (keyboard->isKeyPressed(GLFW_KEY_D)) {
		renderer.camera.fYaw -= 0.01f;
	}
    /*
    if (keyboard->isKeyPressed(GLFW_KEY_UP)) {
		renderer.camera.fPitch += 0.05f;
	}

    if (keyboard->isKeyPressed(GLFW_KEY_DOWN)) {
		renderer.camera.fPitch -= 0.05f;
	}
    */

    if (keyboard->isKeyPressed(GLFW_KEY_UP)) {
		renderer.camera.vCameraPosition.y += 0.1f;
	}

    if (keyboard->isKeyPressed(GLFW_KEY_DOWN)) {
		renderer.camera.vCameraPosition.y -= 0.1f;
	}
    
}

int main() {
    if (!glfwInit()) {
        return -1;
    }

    int width = 1000;
    int height = 1000;

    GLFWwindow* window = glfwCreateWindow(width, height, "Render3d", NULL, NULL);

    if (!window) {
        glfwTerminate();
        return -1;
    }


    double lastTime = 0;
    int frameCount = 0;
    glfwGetWindowSize(window, &width, &height);

    Renderer3d renderer = Renderer3d(50.0f, (float) width, (float) height);
    
    glfwMakeContextCurrent(window); 

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetKeyCallback(window, keyCallback);

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;

        frameCount++;
        if (deltaTime >= 1.0) {
            lastTime = currentTime;
            double fps = static_cast<double>(frameCount) / deltaTime;
            frameCount = 0;

            char title[256];
            snprintf(title, sizeof(title), "Render3d - FPS: %.2f", fps);
            glfwSetWindowTitle(window, title);
        }


        glfwMakeContextCurrent(window);

        glClear(GL_COLOR_BUFFER_BIT);

        handleTick(renderer, (float)deltaTime);

        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}







