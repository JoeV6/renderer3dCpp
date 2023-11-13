#include "math.h"
#include <list>

using namespace std;

class PhysicsObject {
	Mesh& mesh;
	Mesh collidingMesh;
	Vec3d previousPosition;
	Vec3d position;

	Vec3d velocity = { 0,0,0 };
	bool collidable = true;
	bool invisible = false;

public:
	PhysicsObject(Mesh& mesh, Vec3d position) : mesh(mesh), position(position) {
		previousPosition = position;
	}

	void collide(PhysicsObject& p) {
		if (!collidable || !p.isCollidable()) {
			return;
		}

		for (auto& tri : collidingMesh.tris) {
			if (p.isCollidingWithTri(tri)) {
				// Calculate the normal of the triangle
				Vec3d normal = tri.getNormal();

				// Calculate the distance between the two objects
				Vec3d distance = position - p.getPosition();

				// Calculate the dot product of the distance and the normal
				float dotProduct = distance.dot(normal);

				// Calculate the new position of the object
				Vec3d newPosition = position - normal * dotProduct;

				// Calculate the new velocity of the object
				Vec3d newVelocity = velocity - normal * velocity.dot(normal) * 2;

				// Set the new position and velocity of the object
				position = newPosition;
				velocity = newVelocity;
			}
		}
	}

	bool isColliding(PhysicsObject& p) {
		if (!collidable || !p.isCollidable()) {
			return false;
		}

		for (auto& tri : collidingMesh.tris) {
			if (p.isCollidingWithTri(tri)) {
				return true;
			}
		}
	}

	bool isCollidingWithTri(const Triangle& tri) const {
		// Check if the distance between the object and the triangle is less than a threshold
		const float collisionThreshold = 1.0f;

		for (Triangle triColliding : collidingMesh.tris) {
			for (int i = 0; i < 3; ++i) {
				float distance = calculateDistanceToTriangle(triColliding, tri.p[i]);
				if (distance < collisionThreshold) {
					return true;
				}
			}
		}

		return false;
	}

	float calculateDistanceToTriangle(const Triangle& tri, const Vec3d& point) const {
		// Replace this with your actual distance calculation logic
		// For the sake of this example, using a simple Euclidean distance
		float dx = point.x - tri.p[0].x;
		float dy = point.y - tri.p[0].y;
		float dz = point.z - tri.p[0].z;
		return std::sqrt(dx * dx + dy * dy + dz * dz);
	}

	void update() {
		position = position + velocity;

		Vec3d positionChange = position - previousPosition;
		mesh.moveMesh(positionChange);
		moveVertices(positionChange);
		previousPosition = position;
	}

	void moveVertices(Vec3d positionChange) {
		for (auto& tri : collidingMesh.tris) {
			for (auto& vertex : tri.p) {
				vertex = vertex + positionChange;
			}
		}
	}

	void addGravity() {
		velocity.y -= 0.981f;
	}

	void setVelocity(Vec3d velocity) {
		this->velocity = velocity;
	}

	void setInvisible(bool invisible) {
		this->invisible = invisible;
	}

	bool isInvisible() {
		return invisible;
	}

	bool isCollidable() {
		return collidable;
	}

	void setCollidable(bool collidable) {
		this->collidable = collidable;
	}

	Mesh getMesh() {
		return mesh;
	}

	Vec3d getPosition() {
		return position;
	}
};


class Physics3d {
	vector<PhysicsObject>& physicsObjects;
	vector<Mesh>& meshes;

public:
	Physics3d(vector<Mesh>& meshes, vector<PhysicsObject>& physicsObjects) : meshes(meshes), physicsObjects(physicsObjects){
		
	}

	void addPhysicsObject(PhysicsObject& physicsObject) {
		physicsObjects.push_back(physicsObject);
	}

	void update() {
		for (auto& physicsObject : physicsObjects) {
			physicsObject.update();
		}

		for (auto& physicsObject : physicsObjects) {
			for (auto& physicsObject2 : physicsObjects) {
				if (&physicsObject != &physicsObject2) {
					physicsObject.collide(physicsObject2);
				}
			}
		}
	}
};