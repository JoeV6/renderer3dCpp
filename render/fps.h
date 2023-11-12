#pragma once

#include <GLFW/glfw3.h>
#include <stdio.h>

class Fps {
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
			printf("%f ms/frame\n", 1000.0 / double(nbFrames));
			fps = float(nbFrames);
			nbFrames = 0;
			lastTime += 1.0;
		}
	}
};