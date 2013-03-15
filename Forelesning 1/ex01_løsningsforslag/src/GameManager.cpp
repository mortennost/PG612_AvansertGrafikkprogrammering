#include "GameManager.h"
#include "GeometryManager.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <assert.h>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GlTools.h"
#include "GameException.h"

using std::cerr;
using std::endl;

GameManager::GameManager() {
	my_timer.restart();
}

GameManager::~GameManager() {
}

void GameManager::createOpenGLContext() {
	//Set OpenGL major an minor versions
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

	// Set OpenGL attributes
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // Use double buffering
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16); // Use framebuffer with 16 bit depth buffer
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8); // Use framebuffer with 8 bit for red
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8); // Use framebuffer with 8 bit for green
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8); // Use framebuffer with 8 bit for blue
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); // Use framebuffer with 8 bit for alpha
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	// Initalize video
	main_window = SDL_CreateWindow("NITH - PG612 Example OpenGL Program", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		window_width, window_height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!main_window) {
		THROW_EXCEPTION("SDL_CreateWindow failed");
	}

	//Create OpenGL context
	main_context = SDL_GL_CreateContext(main_window);
	
	// Init glew
	// glewExperimental is required in openGL 3.1 
	// to create forward compatible contexts 
	glewExperimental = GL_TRUE;
	GLenum glewErr = glewInit();
	if (glewErr != GLEW_OK) {
		std::stringstream err;
		err << "Error initializing GLEW: " << glewGetErrorString(glewErr);
		THROW_EXCEPTION(err.str());
	}

	// Unfortunately glewInit generates an OpenGL error, but does what it's
	// supposed to (setting function pointers for core functionality).
	// Lets do the ugly thing of swallowing the error....
	glGetError();
}

void GameManager::setOpenGLStates() {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0, 0.0, 0.5, 1.0);
}

void GameManager::createMatrices() {
	projection_matrix = glm::perspective(45.0f, window_width / (float) window_height, 0.8f, 10.f);
	model_matrix = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(3.0f)), glm::vec3(-0.5f));
	view_matrix  = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));
}

void GameManager::createSimpleProgram() {
	std::string fs_src = "#version 130\n"
		"in vec3 ex_Color;\n"
		"in vec3 ex_Normal;\n"
		"in vec3 ex_View;\n"
		"in vec3 ex_Light;\n"
		"out vec4 res_Color;\n"
		"void main() {\n"
		"	vec3 v = normalize(ex_View);\n"
		"	vec3 l = normalize(ex_Light);\n"
		"	vec3 n = normalize(ex_Normal);\n"
		"	vec3 h = normalize(v+l);\n"
		"	float diff = max(0.0f, dot(l, n));\n"
		"	float spec = pow(max(0.0f, dot(h, n)), 128.0f);\n"
		"	res_Color = diff*vec4(ex_Color, 1.0f) + vec4(spec);\n"
		"}";

	std::string vs_src = "#version 130\n"
		"uniform mat4 projection_matrix;\n"
		"uniform mat4 model_matrix;\n"
		"uniform mat4 view_matrix;\n"
		"uniform mat3 normal_matrix;\n"
		"in  vec3 in_Position;\n"
		"in  vec3 in_Normal;\n"
		"out vec3 ex_Color;\n"
		"out vec3 ex_Normal;\n"
		"out vec3 ex_View;\n"
		"out vec3 ex_Light;\n"
		"void main() {\n"
		"	vec4 pos = view_matrix * model_matrix * vec4(in_Position, 1.0);\n"
		"	gl_Position = projection_matrix * pos;\n"
		"	ex_Color = vec3(0.5f, 0.5f, 1.0f);\n"
		"	ex_Normal = normal_matrix*in_Normal;\n"
		"	ex_View = -pos.rgb;\n"
		"	ex_Light = vec3(0.0f, 0.0f, 0.0f) - pos.rgb;\n"
		"}\n";

	//Compile shaders, attach to program object, and link
	GLuint vs = GlTools::compileShader(vs_src, GL_VERTEX_SHADER);
	GLuint fs = GlTools::compileShader(fs_src, GL_FRAGMENT_SHADER);
	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	GlTools::linkProgram(program);

	//Set uniforms for the program.
	glUseProgram(program);
	GLint projectionLocation = glGetUniformLocation(program, "projection_matrix");
	assert(projectionLocation >= 0);
	glUniformMatrix4fv(projectionLocation, 1, 0, glm::value_ptr(projection_matrix));
	
	GLint modelLocation = glGetUniformLocation(program, "model_matrix");
	assert(modelLocation >= 0);
	glUniformMatrix4fv(modelLocation, 1, 0, glm::value_ptr(model_matrix));
	
	GLint viewLocation = glGetUniformLocation(program, "view_matrix");
	assert(viewLocation >= 0);
	glUniformMatrix4fv(viewLocation, 1, 0, glm::value_ptr(view_matrix));
	glUseProgram(0);
}

void GameManager::createVAO() {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	ASSERT_GL;

	//Generate VBO for the vertex data and upload our data.
	//We have n vertices, each with three coordinates (x, y, z), and
	//each coordinate is sizeof(float) bytes.
	glGenBuffers(1, &vertex_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
	glBufferData(GL_ARRAY_BUFFER,
			GeometryManager::getCubeNVertices() * 3 * sizeof(float),
			GeometryManager::getCubeVertices(), GL_STATIC_DRAW);
	GLint vertexAttribLocation = glGetAttribLocation(program, "in_Position");
	assert(vertexAttribLocation >= 0);
	glVertexAttribPointer(vertexAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vertexAttribLocation);
	ASSERT_GL;

	//Generate VBO for normal data and upload
	glGenBuffers(1, &normal_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, normal_vbo);
	glBufferData(GL_ARRAY_BUFFER,
			GeometryManager::getCubeNVertices() * 3 * sizeof(float),
			GeometryManager::getCubeNormals(), GL_STATIC_DRAW);
	GLint normalAttribLocation = glGetAttribLocation(program, "in_Normal");
	assert(normalAttribLocation >= 0);
	glVertexAttribPointer(normalAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(normalAttribLocation);
	ASSERT_GL;

	//Unbind VBOs and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	ASSERT_GL;
}

void GameManager::init() {
	// Initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		std::stringstream err;
		err << "Could not initialize SDL: " << SDL_GetError();
		THROW_EXCEPTION(err.str());
	}
	atexit(SDL_Quit);

	createOpenGLContext();
	setOpenGLStates();
	createMatrices();
	createSimpleProgram();
	createVAO();
}

void GameManager::render() {
	//Get elapsed time
	double elapsed = my_timer.elapsedAndRestart();
	float rotate_degrees = elapsed * 20.0;

	//Clear screen, and set the correct program
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);

	//Rotate cube model
	glm::mat4 rotate_matrix = glm::rotate(glm::mat4(1.0), rotate_degrees, glm::vec3(1.0f, 1.0f, 0.0f));
	model_matrix = rotate_matrix * model_matrix;
	GLint modelLocation = glGetUniformLocation(program, "model_matrix");
	assert(modelLocation >= 0);
	glUniformMatrix4fv(modelLocation, 1, 0, glm::value_ptr(model_matrix));

	//Create normal matrix, the transpose of the inverse
	//3x3 leading submatrix of the modelview matrix
	glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_matrix*view_matrix)));
	GLint normalLocation = glGetUniformLocation(program, "normal_matrix");
	assert(normalLocation >= 0);
	glUniformMatrix3fv(normalLocation, 1, 0, glm::value_ptr(normal_matrix));

	//Render geometry
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, GeometryManager::getCubeNVertices());
	glUseProgram(0);
	ASSERT_GL;
}

void GameManager::play() {
	bool doExit = false;

	//SDL main loop
	while (!doExit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {// poll for pending events
			switch (event.type) {
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) //Esc
					doExit = true;
				if (event.key.keysym.sym == SDLK_q
						&& event.key.keysym.mod & KMOD_CTRL) //Ctrl+q
					doExit = true;
				break;
			case SDL_QUIT: //e.g., user clicks the upper right x
				doExit = true;
				break;
			}
		}

		//Render, and swap front and back buffers
		render();
		SDL_GL_SwapWindow(main_window);
	}
	quit();
}

void GameManager::quit() {
	std::cout << "Bye bye..." << std::endl;
}
