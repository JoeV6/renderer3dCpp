#include <vector>
#include <string>
#include <strstream>
#include <fstream>
#include <strstream>
#include <algorithm>

using namespace std;

struct Vec2f {
	float x = 0;
	float y = 0;
};

struct Vec3d {
	float x = 0;
	float y = 0;
	float z = 0;
    float w = 1;

    Vec3d() = default;

    Vec3d(float x, float y, float z) : x(x), y(y), z(z) {}
    Vec3d(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    
    Vec3d operator+(const Vec3d& other) const {
        return Vec3d(x + other.x, y + other.y, z + other.z);
    }

    
    Vec3d operator-(const Vec3d& other) const {
        return Vec3d(x - other.x, y - other.y, z - other.z);
    }

    Vec3d operator*(float scalar) const {
        return Vec3d(x * scalar, y * scalar, z * scalar);
    }

    Vec3d operator/(float scalar) const {
        if (scalar != 0) {
            return Vec3d(x / scalar, y / scalar, z / scalar);
        }
        // Handle division by zero
        throw std::runtime_error("Division by zero");
    }



    float dot(const Vec3d& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    float length() const {
        return std::sqrt(this->dot(*this));
    }

    Vec3d normalize() const {
        float len = length();
        if (len != 0) {
            return *this / len;
        }
        // Handle zero vector
        throw std::runtime_error("Normalization of a zero-length vector");
    }

    Vec3d cross(const Vec3d& other) const {
        return Vec3d(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    static Vec3d intersectPlane(const Vec3d& plane_p, const Vec3d& plane_n, const Vec3d& lineStart, const Vec3d& lineEnd) {
        Vec3d plane_n_normalized = plane_n.normalize();
        float plane_d = -plane_n_normalized.dot(plane_p);
        float ad = lineStart.dot(plane_n_normalized);
        float bd = lineEnd.dot(plane_n_normalized);
        float t = (-plane_d - ad) / (bd - ad);
        Vec3d lineStartToEnd = lineEnd - lineStart;
        Vec3d lineToIntersect = lineStartToEnd * t;
        return lineStart + lineToIntersect;
    }
};

struct Triangle {
	Vec3d p[3];
    Vec3d color;

    static int clipAgainstPlane(Vec3d plane_p, Vec3d plane_n, Triangle& in_tri, Triangle& out_tri1, Triangle& out_tri2)
    {
        // Make sure plane normal is indeed normal
        plane_n = plane_n.normalize();

        // Return signed shortest distance from point to plane, plane normal must be normalised
        auto dist = [&](Vec3d& p)
            {
                Vec3d n = p.normalize();
                return (plane_n.x * p.x + plane_n.y * p.y + plane_n.z * p.z - plane_n.dot(plane_p));
            };

        // Create two temporary storage arrays to classify points either side of plane
        // If distance sign is positive, point lies on "inside" of plane
        Vec3d* inside_points[3];  int nInsidePointCount = 0;
        Vec3d* outside_points[3]; int nOutsidePointCount = 0;

        // Get signed distance of each point in triangle to plane
        float d0 = dist(in_tri.p[0]);
        float d1 = dist(in_tri.p[1]);
        float d2 = dist(in_tri.p[2]);

        if (d0 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[0]; }
        else { outside_points[nOutsidePointCount++] = &in_tri.p[0]; }
        if (d1 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[1]; }
        else { outside_points[nOutsidePointCount++] = &in_tri.p[1]; }
        if (d2 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[2]; }
        else { outside_points[nOutsidePointCount++] = &in_tri.p[2]; }

        // Now classify triangle points, and break the input triangle into 
        // smaller output triangles if required. There are four possible
        // outcomes...

        if (nInsidePointCount == 0)
        {
            // All points lie on the outside of plane, so clip whole triangle
            // It ceases to exist

            return 0; // No returned triangles are valid
        }

        if (nInsidePointCount == 3)
        {
            // All points lie on the inside of plane, so do nothing
            // and allow the triangle to simply pass through
            out_tri1 = in_tri;

            return 1; // Just the one returned original triangle is valid
        }

        if (nInsidePointCount == 1 && nOutsidePointCount == 2)
        {
            // Triangle should be clipped. As two points lie outside
            // the plane, the triangle simply becomes a smaller triangle


            // The inside point is valid, so keep that...
            out_tri1.p[0] = *inside_points[0];

            // but the two new points are at the locations where the 
            // original sides of the triangle (lines) intersect with the plane
            out_tri1.p[1] = Vec3d::intersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);
            out_tri1.p[2] = Vec3d::intersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[1]);

            return 1; // Return the newly formed single triangle
        }

        if (nInsidePointCount == 2 && nOutsidePointCount == 1)
        {
            // Triangle should be clipped. As two points lie inside the plane,
            // the clipped triangle becomes a "quad". Fortunately, we can
            // represent a quad with two new triangles

            // Copy appearance info to new triangles


            // The first triangle consists of the two inside points and a new
            // point determined by the location where one side of the triangle
            // intersects with the plane
            out_tri1.p[0] = *inside_points[0];
            out_tri1.p[1] = *inside_points[1];
            out_tri1.p[2] = Vec3d::intersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);

            // The second triangle is composed of one of he inside points, a
            // new point determined by the intersection of the other side of the 
            // triangle and the plane, and the newly created point above
            out_tri2.p[0] = *inside_points[1];
            out_tri2.p[1] = out_tri1.p[2];
            out_tri2.p[2] = Vec3d::intersectPlane(plane_p, plane_n, *inside_points[1], *outside_points[0]);

            return 2; // Return two newly formed triangles which form a quad
        }
    }
};

struct Mesh {
    vector<Triangle> tris;

    bool LoadFromObjectFile(string sFilename)
    {
        ifstream f(sFilename);
        if (!f.is_open())
            return false;


        vector<Vec3d> verts;

        while (!f.eof())
        {
            char line[128];
            f.getline(line, 128);

            strstream s;
            s << line;

            char junk;

            if (line[0] == 'v')
            {
                Vec3d v;
                s >> junk >> v.x >> v.y >> v.z;
                verts.push_back(v);
            }

            if (line[0] == 'f')
            {
                int f[3];
                s >> junk >> f[0] >> f[1] >> f[2];
                tris.push_back({ verts[f[0] - 1], verts[f[1] - 1], verts[f[2] - 1] });
            }
        }

        return true;
    }
};

struct Mat4x4 {
	float m[4][4] = { 0 };

    static Vec3d Matrix_MultiplyVector(Mat4x4& m, Vec3d& i)
    {
        Vec3d v;
        v.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + i.w * m.m[3][0];
        v.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + i.w * m.m[3][1];
        v.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + i.w * m.m[3][2];
        v.w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + i.w * m.m[3][3];
        return v;
    }

    static Mat4x4 Matrix_MakeIdentity()
    {
        Mat4x4 matrix;
        matrix.m[0][0] = 1.0f;
        matrix.m[1][1] = 1.0f;
        matrix.m[2][2] = 1.0f;
        matrix.m[3][3] = 1.0f;
        return matrix;
    }

    static Mat4x4 Matrix_MakeRotationX(float fAngleRad)
    {
        Mat4x4 matrix;
        matrix.m[0][0] = 1.0f;
        matrix.m[1][1] = cosf(fAngleRad);
        matrix.m[1][2] = sinf(fAngleRad);
        matrix.m[2][1] = -sinf(fAngleRad);
        matrix.m[2][2] = cosf(fAngleRad);
        matrix.m[3][3] = 1.0f;
        return matrix;
    }

    static Mat4x4 Matrix_MakeRotationY(float fAngleRad)
    {
        Mat4x4 matrix;
        matrix.m[0][0] = cosf(fAngleRad);
        matrix.m[0][2] = sinf(fAngleRad);
        matrix.m[2][0] = -sinf(fAngleRad);
        matrix.m[1][1] = 1.0f;
        matrix.m[2][2] = cosf(fAngleRad);
        matrix.m[3][3] = 1.0f;
        return matrix;
    }

    static Mat4x4 Matrix_MakeRotationZ(float fAngleRad)
    {
        Mat4x4 matrix;
        matrix.m[0][0] = cosf(fAngleRad);
        matrix.m[0][1] = sinf(fAngleRad);
        matrix.m[1][0] = -sinf(fAngleRad);
        matrix.m[1][1] = cosf(fAngleRad);
        matrix.m[2][2] = 1.0f;
        matrix.m[3][3] = 1.0f;
        return matrix;
    }

    static Mat4x4 Matrix_MakeTranslation(float x, float y, float z)
    {
        Mat4x4 matrix;
        matrix.m[0][0] = 1.0f;
        matrix.m[1][1] = 1.0f;
        matrix.m[2][2] = 1.0f;
        matrix.m[3][3] = 1.0f;
        matrix.m[3][0] = x;
        matrix.m[3][1] = y;
        matrix.m[3][2] = z;
        return matrix;
    }

    static Mat4x4 Matrix_MakeProjection(float fFovDegrees, float fAspectRatio, float fNear, float fFar)
    {
        float fFovRad = 1.0f / tanf(fFovDegrees * 0.5f / 180.0f * 3.14159265358979323846f);
        Mat4x4 matrix;
        matrix.m[0][0] = fAspectRatio * fFovRad;
        matrix.m[1][1] = fFovRad;
        matrix.m[2][2] = fFar / (fFar - fNear);
        matrix.m[3][2] = (-fFar * fNear) / (fFar - fNear);
        matrix.m[2][3] = 1.0f;
        matrix.m[3][3] = 0.0f;
        return matrix;
    }

    static Mat4x4 Matrix_MultiplyMatrix(Mat4x4& m1, Mat4x4& m2)
    {
        Mat4x4 matrix;
        for (int c = 0; c < 4; c++)
            for (int r = 0; r < 4; r++)
                matrix.m[r][c] = m1.m[r][0] * m2.m[0][c] + m1.m[r][1] * m2.m[1][c] + m1.m[r][2] * m2.m[2][c] + m1.m[r][3] * m2.m[3][c];
        return matrix;
    }

    static Mat4x4 Matrix_PointAt(Vec3d& pos, Vec3d& target, Vec3d& up)
    {
        // Calculate new forward direction
        Vec3d newForward = target - pos;
        newForward = newForward.normalize();

        // Calculate new Up direction
        Vec3d a = (newForward * (up.dot(newForward)));
        Vec3d newUp = up - a;
        newUp = newUp.normalize();

        Vec3d newRight = newUp.cross(newForward);

        // Construct Dimensioning and Translation Matrix	
        Mat4x4 matrix;
        matrix.m[0][0] = newRight.x;	matrix.m[0][1] = newRight.y;	matrix.m[0][2] = newRight.z;	matrix.m[0][3] = 0.0f;
        matrix.m[1][0] = newUp.x;		matrix.m[1][1] = newUp.y;		matrix.m[1][2] = newUp.z;		matrix.m[1][3] = 0.0f;
        matrix.m[2][0] = newForward.x;	matrix.m[2][1] = newForward.y;	matrix.m[2][2] = newForward.z;	matrix.m[2][3] = 0.0f;
        matrix.m[3][0] = pos.x;			matrix.m[3][1] = pos.y;			matrix.m[3][2] = pos.z;			matrix.m[3][3] = 1.0f;
        return matrix;
    }

    static Mat4x4 Matrix_QuickInverse(Mat4x4& m) // Only for Rotation/Translation Matrices
    {
        Mat4x4 matrix;
        matrix.m[0][0] = m.m[0][0]; matrix.m[0][1] = m.m[1][0]; matrix.m[0][2] = m.m[2][0]; matrix.m[0][3] = 0.0f;
        matrix.m[1][0] = m.m[0][1]; matrix.m[1][1] = m.m[1][1]; matrix.m[1][2] = m.m[2][1]; matrix.m[1][3] = 0.0f;
        matrix.m[2][0] = m.m[0][2]; matrix.m[2][1] = m.m[1][2]; matrix.m[2][2] = m.m[2][2]; matrix.m[2][3] = 0.0f;
        matrix.m[3][0] = -(m.m[3][0] * matrix.m[0][0] + m.m[3][1] * matrix.m[1][0] + m.m[3][2] * matrix.m[2][0]);
        matrix.m[3][1] = -(m.m[3][0] * matrix.m[0][1] + m.m[3][1] * matrix.m[1][1] + m.m[3][2] * matrix.m[2][1]);
        matrix.m[3][2] = -(m.m[3][0] * matrix.m[0][2] + m.m[3][1] * matrix.m[1][2] + m.m[3][2] * matrix.m[2][2]);
        matrix.m[3][3] = 1.0f;
        return matrix;
    }
};




