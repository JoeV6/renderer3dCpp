#pragma once

#include <GLFW/glfw3.h>
#include <stdio.h>

class Fps {
	bool printFPS = true;

public:
	double currentTime;
	double lastTime;
	int nbFrames;
	float fps;

	Fps() {
		lastTime = glfwGetTime();
		nbFrames = 0;
		fps = 0.0f;
	}

	void update() {
		currentTime = glfwGetTime();
		nbFrames++;
		if (currentTime - lastTime >= 1.0) {
			if (printFPS) {
				printf("%f ms/frame |", 1000.0 / double(nbFrames));
				printf("%f fps\n", (float)nbFrames);
			}
			fps = float(nbFrames);
			nbFrames = 0;
			lastTime += 1.0;
		}
	}
};