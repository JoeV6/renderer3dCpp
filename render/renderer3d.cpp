#include "math.h"
#include "keyboard.h"
#include "camera.h"
#include <list>

using namespace std;

class Renderer3d {
    vector<Mesh> meshes;

    Mat4x4 worldMatrix;
    Mat4x4 viewMatrix;
    Mat4x4 projectionMatrix;

public:
    Camera camera;

    float screenWidth;
    float screenHeight;

    Renderer3d(float ffov, float width, float height)
        : screenWidth(width), screenHeight(height) {

        Mesh mesh;
        mesh.LoadFromObjectFile("mountains.obj");
        mesh.increaseSize(5.0f);
        meshes.push_back(mesh);

        projectionMatrix = Mat4x4::MakeProjection(ffov, width / height, 0.01f, 1000.0f);
    }

    void drawEvent() {
        drawMeshes();

        //draw downwards trig in middle of screen with edge at the middle with size of x 
        float x = 0.005f;
        drawTriangle({ -x, -x, 0.0f }, { x, -x, 0.0f }, { 0.0f, x, 0.0f }, { 1.0f, 0.0f, 0.0f });
    }

private:
    void drawMeshes() {
        setupMatrices();

        vector<Triangle> vecTrianglesToRaster;

        for (auto mesh : meshes) {
            for (auto tri : mesh.tris) {
                // Will be rendering in 3 stages
                Triangle triProjected, triTransformed, triViewed;

                // Apply world matrix to each vertex
                for (int i = 0; i < 3; i++) {
                    triTransformed.p[i] = Mat4x4::MultiplyVector(worldMatrix, tri.p[i]);
                }

                Vec3d normal = triTransformed.getNormal();
                Vec3d vCameraRay = triTransformed.p[0] - camera.vCameraPosition;

                // Only draw triangles that face the camera (backface culling)
                float dotProduct = normal.dot(vCameraRay);

                if (dotProduct >= 0.0f) continue;


                // Get shading of triangle
                Vec3d light_direction = { 0.0f, 1.0f, -1.0f };
                light_direction = light_direction.normalize();
                float dp = max(0.1f, light_direction.dot(normal));

                triTransformed.color = { dp, dp, dp };

                // Apply view matrix to each vertex
                for (int i = 0; i < 3; i++) {
                    triViewed.p[i] = Mat4x4::MultiplyVector(viewMatrix, triTransformed.p[i]);
                }

                triViewed.color = triTransformed.color;

                // Clip triangles against near plane
                int nClippedTriangles = 0;
                Triangle clipped[2];
                nClippedTriangles = Triangle::clipAgainstPlane({ 0.0f, 0.0f, 0.01f }, { 0.0f, 0.0f, 1.0f }, triViewed, clipped[0], clipped[1]);

                for (int n = 0; n < nClippedTriangles; n++) {
                    Triangle clippedTriangle = clipped[n];
                    Triangle triProjected;

                    // Apply projection matrix to each vertex
                    for (int i = 0; i < 3; i++) {
                        triProjected.p[i] = Mat4x4::MultiplyVector(projectionMatrix, clippedTriangle.p[i]);
                        triProjected.p[i] = triProjected.p[i] / triProjected.p[i].w;
                    }

                    // Scale and shift to screen space
                    for (int i = 0; i < 3; i++) {
                        triProjected.p[i].x *= 0.5f;
                        triProjected.p[i].y *= 0.5f;
                    }

                    triProjected.color = clippedTriangle.color;

                    // Add to the list
                    vecTrianglesToRaster.push_back(triProjected);
                }

            }
        }

        sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](Triangle& t1, Triangle& t2) {
            float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
            float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
            return z1 > z2;
        });

        clipAndRasterizeTriangles(vecTrianglesToRaster);

        for (auto& triToRaster : vecTrianglesToRaster) {
            drawTriangle(triToRaster.p[0], triToRaster.p[1], triToRaster.p[2], triToRaster.color);
        }
    }

    void clipAndRasterizeTriangles(const vector<Triangle>& vecTrianglesToRaster) {
        for (auto& originalTriangle : vecTrianglesToRaster) {
            Triangle clipped[2];
            list<Triangle> listTriangles;

            listTriangles.push_back(originalTriangle);
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
    }


    void setupMatrices() {
        Mat4x4 translationMatrix;
        translationMatrix = Mat4x4::MakeIdentity();
        translationMatrix = Mat4x4::MakeTranslation(0.0f, 0.0f, 0.0f);

        worldMatrix = Mat4x4::MakeIdentity();
        worldMatrix = Mat4x4::MultiplyMatrix(worldMatrix, translationMatrix);

        Mat4x4 cameraRotationMatrix, cameraRotationMatrixX, cameraRotationMatrixY;
        cameraRotationMatrixY = Mat4x4::MakeRotationY(camera.fYaw);
        cameraRotationMatrixX = Mat4x4::MakeRotationX(camera.fPitch);

        cameraRotationMatrix = Mat4x4::MultiplyMatrix(cameraRotationMatrixX, cameraRotationMatrixY);

        camera.vUp = { 0,1,0 };
        camera.vTarget = { 0,0,1 };

        camera.vLookDir = Mat4x4::MultiplyVector(cameraRotationMatrix, camera.vTarget);
        camera.vTarget = camera.vCameraPosition + camera.vLookDir;

        Mat4x4 cameraMatrix = Mat4x4::PointAt(camera.vCameraPosition, camera.vTarget, camera.vUp);

        viewMatrix = Mat4x4::QuickInverse(cameraMatrix);
    }

    void drawTriangle(const Vec3d& vertex1, const Vec3d& vertex2, const Vec3d& vertex3, const Vec3d& color) {
        glBegin(GL_TRIANGLES);
        glColor3f(color.x, color.y, color.z); 

        glVertex2f(vertex1.x, vertex1.y);
        glVertex2f(vertex2.x, vertex2.y);
        glVertex2f(vertex3.x, vertex3.y);

        glEnd();
    }
};