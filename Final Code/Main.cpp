#include "Header.h"
#include "ShaderBuilder.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

double  mouse_old_x, 
        mouse_old_y;

float   aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
        uInc = 0.01f,
        uDisplay = 0.45f;
int order = 2, 
    splineSize = 0, 
    pointToMove = -1, 
    window_width = WINDOW_WIDTH,
    window_height = WINDOW_HEIGHT;
bool movePoint = false, geometric = false;

std::vector<glm::vec2> controls;
std::vector<float> weights;
std::vector<std::vector<glm::vec2>> geom;

const GLfloat clearColor[] = { 0.f, 0.f, 0.f };

GLuint  splineVertexArray = -1,
        controlsVertexArray = -1,
        geomVertexArray = -1,

        splineProgram = -1, 
        controlsProgram = -1,
        geometryProgram = -1;

void errorCallback(int error, const char* description);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void window_size_callback(GLFWwindow* window, int width, int height);
void printOpenGLVersion();

// takes in the order of the spline and returns the number of points in the resulting curve
int generateSplineBuffers(int order)
{
    GLuint vertexBuffer = 0;

    glGenVertexArrays(1, &splineVertexArray);
    glBindVertexArray(splineVertexArray);

    std::vector<glm::vec2> spline;

    nurbsSpline(controls, weights, spline, order, uInc);
    if(geometric)
        generateGeometric(controls, weights, geom, order, uDisplay);

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(spline[0]) * spline.size(), &spline[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);


    glBindVertexArray(0);
    return spline.size();
}

void generateControlsBuffer()
{
    GLuint vertexBuffer = 0;

    glGenVertexArrays(1, &controlsVertexArray);
    glBindVertexArray(controlsVertexArray);
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(controls[0]) * controls.size(), &controls[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void generateGeometricBuffer(std::vector<glm::vec2> geometry)
{
    GLuint vertexBuffer = 0;

    glGenVertexArrays(1, &geomVertexArray);
    glBindVertexArray(geomVertexArray);
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(geometry[0]) * geometry.size(), &geometry[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void generateShaders()
{
    splineProgram = generateProgram("shaders/general.vert", "shaders/general.frag");
    controlsProgram = generateProgram("shaders/general.vert", "shaders/controls.frag");
    geometryProgram = generateProgram("shaders/general.vert", "shaders/geometry.frag");
}

void renderSpline(int numOfVertices)
{
    glBindVertexArray(splineVertexArray);
    glUseProgram(splineProgram);

    glDrawArrays(GL_LINE_STRIP, 0, numOfVertices);

    glBindVertexArray(0);
}

void renderControls()
{
    glBindVertexArray(controlsVertexArray);
    glUseProgram(controlsProgram);

    for (int i = 0; i < controls.size(); i++)
    {
        glPointSize(15 * weights[i]);
        if (weights[i] == 0.f)
            glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
        else
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDrawArrays(GL_POINTS, i, 1);
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glBindVertexArray(0);
}

void renderGeometric()
{

    for (int i = 0; i < geom.size(); i++)
    {
        generateGeometricBuffer(geom[i]);

        glBindVertexArray(geomVertexArray);
        glUseProgram(geometryProgram);

        glPointSize(10);
        glDrawArrays(GL_POINTS, 0, geom[i].size());
        glDrawArrays(GL_LINE_STRIP, 0, geom[i].size());

        glBindVertexArray(0);
    }
}

void pointMove(GLFWwindow *window)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    float x = xpos * 2.f / window_width - 1;
    float y = -(ypos * 2.f / window_height - 1);

    controls[pointToMove] = glm::vec2(x, y);

    if (controls.size() >= 1)
        generateControlsBuffer();

    if (controls.size() >= 2)
        splineSize = generateSplineBuffers(order);
}

int main()
{
	if (!glfwInit())
	{
		std::cout << "Failed to initialize GLFW" << std::endl;
		return -1;
	}
	glfwSetErrorCallback(errorCallback);

	glfwWindowHint(GLFW_DOUBLEBUFFER, true);
	glfwWindowHint(GLFW_SAMPLES, 32);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "NURBS Curve", NULL, NULL);

	if (!window) {
		std::cout << "Failed to create window" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwSetKeyCallback          (window, key_callback); 
    glfwSetMouseButtonCallback  (window, mouse_button_callback);
    glfwSetScrollCallback       (window, scroll_callback);
    glfwSetWindowSizeCallback   (window, window_size_callback);
	glfwMakeContextCurrent      (window);

	if (!gladLoadGL())
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		exit(EXIT_FAILURE);
	}
	printOpenGLVersion();

    generateShaders();

    glfwSwapInterval(1);

	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearBufferfv(GL_COLOR, 0, clearColor);


        if (controls.size() > 0)
            renderControls();

        if (controls.size() > 1)
        {
            renderSpline(splineSize);
            if (geometric)
                renderGeometric();
        }

		glfwSwapBuffers(window);
		glfwWaitEvents();

        if (movePoint) pointMove(window);
	}

	// Shutdow the program
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case(GLFW_KEY_N):
			std::cout << "Recompiling Shaders... ";
            generateShaders();
            std::cout << "Done" << std::endl;
			break;
        case(GLFW_KEY_W):
            if (order + 1 <= (int)controls.size())
            {
                order++;
                splineSize = generateSplineBuffers(order);
            }
            std::cout << "order = " << order << std::endl;
            break;
        case(GLFW_KEY_S):
            if (order - 1 > 1)  // dont allow an order of 1, rather a degree 0 curve
            {
                order--;
                splineSize = generateSplineBuffers(order);
            }
            std::cout << "order = " << order << std::endl;
            break;
        case(GLFW_KEY_G):
            geometric = !geometric;
            if (geometric)
                generateGeometric(controls, weights, geom, order, uDisplay);
            break;
		default:
			break;
		}
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    float x = xpos * 2.f / window_width - 1;
    float y = -(ypos * 2.f / window_height - 1);

    float pointSize = 0.04f;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        int i = 0;
        for (i; i < controls.size(); i++)
        {
            if (glm::distance(glm::vec2(x, y), controls[i]) <= pointSize * std::max(0.5f, weights[i]))
            {
                movePoint = true;
                pointToMove = i;
                break;
            }
        }
        if (i == controls.size())
        {
            controls.push_back(glm::vec2(x, y));
            weights.push_back(1.f);
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        for (int i = 0; i < controls.size(); i++)
        {
            if (glm::distance(glm::vec2(x, y), controls[i]) <= pointSize * std::max(0.5f, weights[i]))
            {
                controls.erase(controls.begin() + i);
                break;
            }
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        movePoint = false;
        pointToMove = -1;
	}

    if (controls.size() >= 1)
        generateControlsBuffer();

    if(controls.size() >= 2)
        splineSize = generateSplineBuffers(order);
}

void incU(bool inc)
{
    float   uIncStep    = 0.005f, 
            uIncLimit   = 0.001f, // tried 0.0001 but this slowed the program considerably
            uInc2       = 0.f;
    if (!inc)
        uInc2 = std::min(1.f - uIncLimit, uInc + uIncStep);
    else
        uInc2 = std::max(uIncLimit, uInc - uIncStep);
    if (uInc2 != uInc)
    {
        uInc = uInc2;
        std::cout << "u Increment = " << uInc << std::endl;
        splineSize = generateSplineBuffers(order);
    }
}

void incUDisplay(bool inc)
{
    float       uIncStep = 0.005f,
                uIncLimit = 0.001f,
                uDisp2 = 0.f;
    if (!inc)
        uDisp2 = std::min(1.f - uIncLimit, uDisplay + uIncStep);
    else
        uDisp2 = std::max(uIncLimit, uDisplay - uIncStep);
    if (uDisp2 != uDisplay)
    {
        uDisplay = uDisp2;
        if (geometric)
            generateGeometric(controls, weights, geom, order, uDisplay);
        std::cout << "u Displayed = " << uDisplay << std::endl;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Scroll Down
    if (yoffset < 0)
    {
        if (glfwGetKey(window, GLFW_KEY_I))
            incU(true);
        else if (glfwGetKey(window, GLFW_KEY_U))
            incUDisplay(true);
        else if (pointToMove != -1)
            weights[pointToMove] = std::max(0.f, weights[pointToMove] - 0.1f);
	}
    // scroll Up
    else if (yoffset > 0)
    {
        if (glfwGetKey(window, GLFW_KEY_I))
            incU(false);
        else if (glfwGetKey(window, GLFW_KEY_U))
            incUDisplay(false);
        else if (pointToMove != -1)
            weights[pointToMove] += 0.1f;
	}
}

void printOpenGLVersion()
{
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	printf("OpenGL on %s %s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER));
	printf("OpenGL version supported %s\n", glGetString(GL_VERSION));
	printf("GLSL version supported %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("GL version major, minor: %i.%i\n", major, minor);
}

void errorCallback(int error, const char* description)
{
	std::cout << "GLFW ERROR " << error << ": " << description << std::endl;
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    aspectRatio = (float)width / (float)height;
    window_width = width;
    window_height = height;
    glViewport(0, 0, width, height);
}
