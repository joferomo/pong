#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE "PONG"

GLFWwindow* window;
int windowWidth, windowHeight;

typedef float mat4[16];

float paddle1[2], paddle2[2];
float paddleSize[2] = { 8.0f, 80.0f };
float paddleSpeed = 600.0f;
int p1Score = 0, p2Score = 0;

float ball[2];
float ballSize[2] = { 8.0f, 8.0f };
float ballDirection[2] = { 0.0f, 0.0f };
float ballSpeed = 800.0f;
int ballServed = 0;
float serveTime = 3.0f;
float timeElapsed = 0.0f;
float hitConditions[2];

float deltaTime;
float t = 0.0f;
float t0 = 0.0f;

int initGL()
{
	if(glfwInit() == GLFW_FALSE)
	{
		fprintf(stderr, "Failed to initialize GLFW.\n");
		return GL_FALSE;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if(window == NULL)
	{
		fprintf(stderr, "Failed to create the main window.\n");
		glfwTerminate();
		return GL_FALSE;
	}
	glfwMakeContextCurrent(window);
	const GLFWvidmode* screen = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwGetWindowSize(window, &windowWidth, &windowHeight);
	glfwSetWindowPos(window, (screen->width - windowWidth) / 2, (screen->height - windowHeight) / 2);

	if(gladLoaderLoadGL() == GL_FALSE)
	{
		fprintf(stderr, "Failed to initialize GLAD.\n");
		glfwTerminate();
		return GL_FALSE;
	}

	return GL_TRUE;
}

int loadShader(unsigned int id, const char* file)
{
	FILE* shaderFile = fopen(file, "r");
	if(shaderFile == NULL)
	{
		fprintf(stderr, "Failed to load shader file %s\n", file);
		return GL_FALSE;
	}

	char buffer[128];
	char* source = malloc(sizeof (char));
	*source = '\0';
	int n = 0;
	while(fgets(buffer, 128, shaderFile))
	{
		source = realloc(source, 128 * sizeof (char) * (++n));
		strncat(source, buffer, 128);
	}
	const char* sourcePtr = source;
	glShaderSource(id, 1, &sourcePtr, NULL);

	glCompileShader(id);
	int success;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);
	if(success == GL_FALSE)
	{
		char infoLog[512];
		glGetShaderInfoLog(id, 512, 0, infoLog);
		fprintf(stderr, "Failed to compile shader %d:\n%s\n", id, infoLog);
		return GL_FALSE;
	}

	return GL_TRUE;
}

int linkProgram(unsigned int id, unsigned int* shadersToAttach)
{
	for(int i = 0; shadersToAttach[i] != '\0'; i++)
	{
		glAttachShader(id, shadersToAttach[i]);
	}

	glLinkProgram(id);
	int success;
	glGetProgramiv(id, GL_LINK_STATUS, &success);
	if(success == GL_FALSE)
	{
		char infoLog[512];
		glGetProgramInfoLog(id, 512, 0, infoLog);
		fprintf(stderr, "Failed to link program %d:\n%s\n", id, infoLog);
		return GL_FALSE;
	}

	return GL_TRUE;
}

void setScreenCoordinates(float* pos, float x, float y)
{
	pos[0] = -1.0f + x * 2 / windowWidth;
	pos[1] = -1.0f + y * 2 / windowHeight;
}

void restartGame()
{
	setScreenCoordinates(paddle1, 32.0f, 300.0f);
	setScreenCoordinates(paddle2, 768.0f, 300.0f);
	ball[0] = 0.0f;
	ball[1] = 0.0f;
	ballDirection[0] = 0.0f;
	ballDirection[1] = 0.0f;
	timeElapsed = 0.0f;
	ballServed = 0;
	p1Score = 0;
	p2Score = 0;
}

void getPlayersInput(GLFWwindow* window)
{
	if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		paddle1[1] += paddleSpeed * deltaTime / windowHeight;
	}
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		paddle1[1] -= paddleSpeed * deltaTime / windowHeight;
	}
	if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
	{
		paddle2[1] += paddleSpeed * deltaTime / windowHeight;
	}
	if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		paddle2[1] -= paddleSpeed * deltaTime / windowHeight;
	}
	if(glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
	{
		restartGame();
	}
	if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

float getDistance(float n)
{
	if(n < 0.0f)
	{
		return n * -1.0f;
	}
	return n;
}

int main()
{
	srand(time(NULL));

	if(initGL() == GL_FALSE) return 1;

	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	loadShader(vertexShader, "vertexShader.glsl");

	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	loadShader(fragmentShader, "fragmentShader.glsl");

	unsigned int shadersToAttach[] = { vertexShader, fragmentShader };

	unsigned int shaderProgram = glCreateProgram();
	linkProgram(shaderProgram, &shadersToAttach[0]);
	glUseProgram(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	float size[2] = { 1.0f / windowWidth, 1.0f / windowHeight};

	float vertices[] = {
		-size[0],  size[1],
		size[0],  size[1],
		size[0], -size[1],
		-size[0], -size[1]
	};

	unsigned int indices[] = {
		0, 1, 2,
		0, 3, 2
	};

	unsigned int vao, vbo, ebo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof (float), (void*) 0);
	glEnableVertexAttribArray(0);

	setScreenCoordinates(paddle1, 32.0f, 300.0f);
	setScreenCoordinates(paddle2, 768.0f, 300.0f);

	hitConditions[0] = (paddleSize[0] + ballSize[0]) / windowWidth;
	hitConditions[1] = (paddleSize[1] + ballSize[1]) / windowHeight;

	do
	{
		t = glfwGetTime();
		deltaTime = t - t0;

		timeElapsed += deltaTime;
		if(timeElapsed >= serveTime && ballServed == 0)
		{
			int directionX = rand() % 2;
			switch(directionX)
			{
				case 0:
					ballDirection[0] = -1.0f;
					break;
				case 1:
					ballDirection[0] = 1.0f;
					break;
			}
			ballDirection[1] = -1.0f + rand() % 3;
			ballServed = 1;
		}

		glClear(GL_COLOR_BUFFER_BIT);

		// Move paddles
		getPlayersInput(window);

		if(paddle1[1] > 1.0f - paddleSize[1] / windowHeight)
		{
			paddle1[1] = 1.0f - paddleSize[1] / windowHeight;
		}
		else if(paddle1[1] < -1.0f + paddleSize[1] / windowHeight)
		{
			paddle1[1] = -1.0f + paddleSize[1] / windowHeight;
		}

		if(paddle2[1] > 1.0f - paddleSize[1] / windowHeight)
		{
			paddle2[1] = 1.0f - paddleSize[1] / windowHeight;
		}
		else if(paddle2[1] < -1.0f + paddleSize[1] / windowHeight)
		{
			paddle2[1] = -1.0f + paddleSize[1] / windowHeight;
		}

		// Move ball
		if(ballServed == 1)
		{
			float hyp = sqrt(ballDirection[0] * ballDirection[0] + ballDirection[1] * ballDirection[1]);
			float scoreZone = 1.0f + ballSize[0] / windowWidth;
			float reflectZone = 1.0f - ballSize[1] / windowHeight;
			ball[0] += ballDirection[0] * ballSpeed * deltaTime / hyp / windowWidth;
			ball[1] += ballDirection[1] * ballSpeed * deltaTime / hyp / windowHeight;

			if(ball[0] >= scoreZone || ball[0] <= -scoreZone)
			{
				if(ballDirection[0] > 0.0f)
				{
					p1Score++;
				}
				else
				{
					p2Score++;
				}
				printf("P1's score: %d\n", p1Score);
				printf("P2's score: %d\n", p2Score);
				ball[0] = 0.0f;
				ball[1] = 0.0f;
				ballDirection[0] = 0.0f;
				ballDirection[1] = 0.0f;
				timeElapsed = 0.0f;
				ballServed = 0;
			}

			if((ball[1] >= reflectZone && ballDirection[1] > 0.0f) || (ball[1] <= -reflectZone && ballDirection[1] <= 0.0f))
			{
				ballDirection[1] *= -1.0f;
			}

			if(getDistance(ball[0] - paddle1[0]) <= hitConditions[0] && getDistance(ball[1] - paddle1[1]) <= hitConditions[1] && ballDirection[0] < 0.0f)
			{
				ballDirection[0] *= -1.0f;
				ballDirection[1] = (ball[1] - paddle1[1]) / hitConditions[1];
			}
			else if(getDistance(ball[0] - paddle2[0]) <= hitConditions[0] && getDistance(ball[1] - paddle2[1]) <= hitConditions[1] && ballDirection[0] > 0.0f)
			{
				ballDirection[0] *= -1.0f;
				ballDirection[1] = (ball[1] - paddle2[1]) / hitConditions[1];
			}
		}

		// Draw left paddle
		{
			mat4 transform = {
				paddleSize[0],	0.0f,			0.0f,	0.0f,
				0.0f,			paddleSize[1],	0.0f,	0.0f,
				0.0f,			0.0f,			1.0f,	0.0f,
				paddle1[0],		paddle1[1],		0.0f,	1.0f
			};
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "transform"), 1, GL_FALSE, transform);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		}

		// Draw right paddle
		{
			mat4 transform = {
				paddleSize[0],	0.0f,			0.0f,	0.0f,
				0.0f,			paddleSize[1],	0.0f,	0.0f,
				0.0f,			0.0f,			1.0f,	0.0f,
				paddle2[0],		paddle2[1],		0.0f,	1.0f
			};
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "transform"), 1, GL_FALSE, transform);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		}

		// Draw ball
		{
			mat4 transform = {
				ballSize[0],	0.0f,			0.0f,	0.0f,
				0.0f,			ballSize[1],	0.0f,	0.0f,
				0.0f,			0.0f,			1.0f,	0.0f,
				ball[0],		ball[1],		0.0f,	1.0f
			};
			glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "transform"), 1, GL_FALSE, transform);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		}

		glfwSwapBuffers(window);

		glfwPollEvents();

		t0 = t;
	} while(glfwWindowShouldClose(window) == GL_FALSE);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(shaderProgram);
	glfwTerminate();
	return 0;
}
