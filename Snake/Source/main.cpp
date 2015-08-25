#define GLEW_STATIC
#define _GLIBCXX_USE_NANOSLEEP
#define GLSL(src) "#version 130\n" #src

#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <list>
#include <iostream>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SOIL/SOIL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using std::string;
using std::list;

void initialiseWindow(int, int, string);
void initialiseGL();
void initialiseGame();
void destroyDisplay();
void takeInput();
void gameOver();
void updateGame();
void updateDisplay();
void drawSquare(int, int);
void drawSnake();
void drawTitle();
void debugShaders(GLint, GLint);

struct Coord
{
	int x, y;
};

enum Direction
{
	UP, DOWN, LEFT, RIGHT
};

Coord goal;
list<Coord> snake;
bool running = true, alive = false, inMenu = true;
Direction dir = UP;
int score = 0;
SDL_Window* window;
SDL_GLContext glContext;
GLuint vbo, vao, ebo, tex, vertexShader, fragmentShader, shaderProgram, posAttrib, colAttrib, texAttrib;
GLuint textures[2];
GLint uniSquareTrans;
unsigned char* image;
glm::mat4 squareTrans;
const int WIDTH = 600, HEIGHT = 600, SQUARE_COUNT = 60, FPS = 30, WAIT_TIME_MILLIS = 1000 / FPS, START_X = 1, START_Y = 1;
const float SQUARE_SIZE = (2.0f / SQUARE_COUNT);
float white[] = {1.0f, 1.0f, 1.0f};
const string TITLE = "Snake";
GLfloat squareVertices[] =
{
//	 x		y	  r		g	  b     tX   tY
	-0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
	-0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
	 0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
	 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
};
GLuint squareElements[] = {0, 1, 2, 0, 2, 3};

const GLchar* vertexShaderSource = GLSL
(
	in vec2 position;
	in vec2 texcoord;
	in vec3 color;

	out vec3 Color;
	out vec2 Texcoord;

	uniform mat4 square;

    void main()
    {
    	Texcoord = texcoord;
        Color = color;
        gl_Position = square * vec4(position, 0.0, 1.0);
    }
);

const GLchar* fragmentShaderSource = GLSL
(
    in vec3 Color;
	in vec2 Texcoord;

	out vec4 outColor;

	uniform sampler2D tex;

    void main()
    {
        outColor = vec4(Color, 1.0) * texture(tex, Texcoord);
    }
);

int main()
{
	initialiseWindow(WIDTH, HEIGHT, TITLE);
	initialiseGL();

	while (running)
	{
		drawTitle();
		while (inMenu)
		{
			takeInput();
		}

		initialiseGame();

		while(alive)
		{
			takeInput();
			updateGame();
			updateDisplay();
			std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_MILLIS));
		}
	}

	destroyDisplay();

	return 0;
}

void takeInput()
{
	SDL_Event windowEvent;

	if (SDL_PollEvent(&windowEvent))
	{
		if (windowEvent.type == SDL_QUIT ||
		   (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE))
			{
				running = false;
				alive = false;
				inMenu = false;
			}

		if (windowEvent.type == SDL_KEYDOWN)
		{
			switch(windowEvent.key.keysym.sym)
			{
			case SDLK_UP:
				if (dir != DOWN)
					dir = UP;
			break;
			case SDLK_LEFT:
				if (dir != RIGHT)
					dir = LEFT;
			break;
			case SDLK_DOWN:
				if (dir != UP)
					dir = DOWN;
			break;
			case SDLK_RIGHT:
				if (dir != LEFT)
					dir = RIGHT;
			break;
			case SDLK_RETURN:
				inMenu = false;
				alive = true;
			break;
			}
		}
	}
}

void gameOver()
{
	std::cout << "GAME OVER" << std::endl;
	std::cout << "FINAL SCORE: " << score << std::endl;
	alive = false;
	inMenu = true;
}

void updateGame()
{
	if (snake.begin()->x >= 60 || snake.begin()->y >= 60 || snake.begin()->x < 0 || snake.begin()->y < 0)
	{
		gameOver();
	}
	if ((snake.begin()->x == goal.x) && (snake.begin()->y == goal.y))
	{
		score++;

		std::ostringstream oss;
		oss << TITLE << " - Score: " << score;
		SDL_SetWindowTitle(window, oss.str().c_str());

		Coord newSeg;
		newSeg.x = snake.back().x - 1;
		newSeg.y = snake.back().y - 1;
		snake.push_back(newSeg);

		goal.x = rand() % 59 + 1;
		goal.y = rand() % 59 + 1;
	}

	for (list<Coord>::iterator t = ++snake.begin(); t != snake.end(); ++t)
	{
		if ((snake.begin()->x == t->x) && (snake.begin()->y == t->y))
		{
			gameOver();
		}
	}

	Coord newHead;
	switch (dir)
	{
	case UP:
		newHead.x = snake.begin()->x;
		newHead.y = snake.begin()->y + 1;
	break;
	case LEFT:
		newHead.x = snake.begin()->x - 1;
		newHead.y = snake.begin()->y;
	break;
	case DOWN:
		newHead.x = snake.begin()->x;
		newHead.y = snake.begin()->y - 1;
	break;
	case RIGHT:
		newHead.x = snake.begin()->x + 1;
		newHead.y = snake.begin()->y;
	break;
	}
	snake.push_front(newHead);
	snake.pop_back();

}

void drawSnake()
{
	for (list<Coord>::iterator t = snake.begin(); t != snake.end(); ++t)
	{
		drawSquare(t->x, t->y);
	}
}

void drawTitle()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	squareTrans = glm::mat4();
	glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);
	squareTrans = glm::scale(squareTrans, glm::vec3(1.0f, 1.0f, 1.0f));
	glUniformMatrix4fv(uniSquareTrans, 1, GL_FALSE, glm::value_ptr(squareTrans));
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	SDL_GL_SwapWindow(window);
}

void drawSquare(int x, int y)
{
	squareTrans = glm::mat4();
	squareTrans = glm::translate(squareTrans, glm::vec3(SQUARE_SIZE * x - 1.0f + SQUARE_SIZE * 0.5f, SQUARE_SIZE * y - 1.0f + SQUARE_SIZE * 0.5f, 0.0f));
	squareTrans = glm::scale(squareTrans, SQUARE_SIZE * glm::vec3(1.0f, 1.0f, 1.0f));
	glUniformMatrix4fv(uniSquareTrans, 1, GL_FALSE, glm::value_ptr(squareTrans));
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void initialiseGame()
{
	snake.clear();

	Coord start;
	start.x = START_X;
	start.y = START_Y;
	snake.push_back(start);

	goal.x = rand() % 60;
	goal.y = rand() % 60;
	dir = UP;

	SDL_SetWindowTitle(window, "Snake");
	score = 0;

	glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 1);

}

void updateDisplay()
{
    SDL_GL_SwapWindow(window);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawSnake();
	drawSquare(goal.x, goal.y);
}

void initialiseWindow(int width, int height, string title)
{
	SDL_Init(SDL_INIT_EVERYTHING);

	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
	glContext = SDL_GL_CreateContext(window);
}

void destroyDisplay()
{
	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
}

void initialiseGL()
{
	glewExperimental = GL_TRUE;
	glewInit();

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	//debugShaders(vertexShader, fragmentShader);

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	glGenTextures(2, textures);

	int width, height;
	unsigned char* image;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	image = SOIL_load_image("snakeTitle.png", &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, white);
	glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 0);

	colAttrib = glGetAttribLocation(shaderProgram, "color");
	glEnableVertexAttribArray(colAttrib);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(2 * sizeof(float)));

	texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(5 * sizeof(float)));

	glGenBuffers(1, &ebo);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareElements), squareElements, GL_STATIC_DRAW);

	uniSquareTrans = glGetUniformLocation(shaderProgram, "square");

}

void debugShaders(GLint vertexShaderIn, GLint fragmentShaderIn)
{
	GLint status;
	glGetShaderiv(vertexShaderIn, GL_COMPILE_STATUS, &status);

	char buffer[512];
	glGetShaderInfoLog(vertexShaderIn, 512, NULL, buffer);

	std::cout << "VERTEX" << std::endl;
	std::cout << buffer << std::endl;

	glGetShaderiv(fragmentShaderIn, GL_COMPILE_STATUS, &status);

	glGetShaderInfoLog(fragmentShaderIn, 512, NULL, buffer);

	std::cout << "FRAGMENT" << std::endl;
	std::cout << buffer << std::endl;
}
