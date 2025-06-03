#define GLM_FORCE_RADIANS

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "constants.h"
#include "allmodels.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "ObjModel.h"

Models::ObjModel tree("apple_tree.obj");
Models::ObjModel apple("apple.obj");
Models::ObjModel grass("grass.obj");

GLuint apple_text;
GLuint tree_text;
GLuint grass_text;

float scale_tree = 0.1274;
float scale_apple = 0.0398;

float speed;

//kamera
glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 10.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 250.0f; 
float lastY = 250.0f; 
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

std::vector<glm::vec3> treePositions;

struct FallingApple {
	glm::vec3 position;
	float startY;
	float velocity;
	float timeStarted;
	float delay;
	bool onGround = false;
	float groundTime = 0.0f;
};
std::vector<FallingApple> fallingApples;

void initTrees() {
	int numTrees = 20;
	float areaMin = -5.0f + 0.5f;
	float areaMax = 5.0f - 0.5f;

	for (int i = 0; i < numTrees; i++) {
		float x = areaMin + static_cast<float>(rand()) / RAND_MAX * (areaMax - areaMin);
		float z = areaMin + static_cast<float>(rand()) / RAND_MAX * (areaMax - areaMin);
		glm::vec3 treePos = glm::vec3(x, 0.0f, z);
		treePositions.push_back(treePos);

		if (rand() % 100 < 40) {
			float offsetX = ((rand() % 100) / 100.0f - 0.5f) * 1.0f;
			float offsetZ = ((rand() % 100) / 100.0f - 0.5f) * 1.0f;

			glm::vec3 applePos = glm::vec3(treePos.x + offsetX, 1.5f, treePos.z + offsetZ);
			FallingApple apple;
			apple.position = applePos;
			apple.startY = applePos.y;
			apple.velocity = 0.0f;
			apple.timeStarted = glfwGetTime();
			apple.delay = static_cast<float>(rand()) / RAND_MAX * 10.0f;
			fallingApples.push_back(apple);
		}
	}
}

//Procedura obsługi błędów
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	
	float cameraSpeed = 2.5f * deltaTime;

	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		if (key == GLFW_KEY_W)
			cameraPos += cameraSpeed * cameraFront;
		if (key == GLFW_KEY_S)
			cameraPos -= cameraSpeed * cameraFront;
		if (key == GLFW_KEY_A)
			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		if (key == GLFW_KEY_D)
			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		if (key == GLFW_KEY_P)
			return;
	}
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // odwrócone: y rośnie w górę
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(direction);
}

GLuint readTexture2(const char* filename) {
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);

	//Wczytanie do pamięci komputera
	std::vector<unsigned char> image;   //Alokuj wektor do wczytania obrazka
	unsigned width, height;   //Zmienne do których wczytamy wymiary obrazka
	//Wczytaj obrazek
	unsigned error = lodepng::decode(image, width, height, filename);

	//Import do pamięci karty graficznej
	glGenTextures(1, &tex); //Zainicjuj jeden uchwyt
	glBindTexture(GL_TEXTURE_2D, tex); //Uaktywnij uchwyt
	//Wczytaj obrazek do pamięci KG skojarzonej z uchwytem

	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // powtarzanie w osi S (X)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  // powtarzanie w osi T (Y)

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return tex;
}


//Procedura inicjująca
void initOpenGLProgram(GLFWwindow* window) {
	initShaders();
	//************Tutaj umieszczaj kod, który należy wykonać raz, na początku programu************	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	apple_text = readTexture2("apple_text.png");
	tree_text = readTexture2("tree_text.png");
	grass_text = readTexture2("grass2.png");

}

//Zwolnienie zasobów zajętych przez program
void freeOpenGLProgram(GLFWwindow* window) {
	freeShaders();
	//************Tutaj umieszczaj kod, który należy wykonać po zakończeniu pętli głównej************
	glDeleteTextures(1, &apple_text);
	glDeleteTextures(1, &tree_text);
	glDeleteTextures(1, &grass_text);
}

void drawModel(Models::ObjModel& model, GLuint tex, glm::mat4 P, glm::mat4 V, glm::mat4 M) {

	spTextured->use(); //Aktywuj program cieniujący

	glUniformMatrix4fv(spTextured->u("P"), 1, false, glm::value_ptr(P)); //Załaduj do programu cieniującego macierz rzutowania
	glUniformMatrix4fv(spTextured->u("V"), 1, false, glm::value_ptr(V)); //Załaduj do programu cieniującego macierz widoku
	glUniformMatrix4fv(spTextured->u("M"), 1, false, glm::value_ptr(M)); //Załaduj do programu cieniującego macierz modelu


	glEnableVertexAttribArray(spTextured->a("vertex"));
	glVertexAttribPointer(spTextured->a("vertex"), 4, GL_FLOAT, false, 0, model.vertices); //Współrzędne wierzchołków bierz z tablicy myCubeVertices

	glEnableVertexAttribArray(spTextured->a("texCoord"));
	glVertexAttribPointer(spTextured->a("texCoord"), 2, GL_FLOAT, false, 0, model.texCoords); //Współrzędne teksturowania bierz z tablicy myCubeTexCoords

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(spTextured->u("tex"), 0);

	glDrawArrays(GL_TRIANGLES, 0, model.vertexCount);

	glDisableVertexAttribArray(spTextured->a("vertex"));
	glDisableVertexAttribArray(spTextured->a("color"));

}

void drawModelGrass(Models::ObjModel& model, glm::mat4 P, glm::mat4 V, glm::mat4 M, GLuint grassTextureID) {
	spGrass->use();

	glUniformMatrix4fv(spGrass->u("P"), 1, GL_FALSE, glm::value_ptr(P));
	glUniformMatrix4fv(spGrass->u("V"), 1, GL_FALSE, glm::value_ptr(V));
	glUniformMatrix4fv(spGrass->u("M"), 1, GL_FALSE, glm::value_ptr(M));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, grassTextureID);
	glUniform1i(spGrass->u("grassTexture"), 0);

	glEnableVertexAttribArray(spGrass->a("vertex"));
	glVertexAttribPointer(spGrass->a("vertex"), 4, GL_FLOAT, GL_FALSE, 0, model.vertices);

	glEnableVertexAttribArray(spGrass->a("texCoord"));
	glVertexAttribPointer(spGrass->a("texCoord"), 2, GL_FLOAT, GL_FALSE, 16, model.texCoords);

	float timeValue = glfwGetTime();
	glUniform1f(spGrass->u("time"), timeValue);

	glDrawArrays(GL_TRIANGLES, 0, model.vertexCount);

	glDisableVertexAttribArray(spGrass->a("vertex"));
	glDisableVertexAttribArray(spGrass->a("texCoord"));
}

//Procedura rysująca zawartość sceny
void drawScene(GLFWwindow* window) {
	//************Tutaj umieszczaj kod rysujący obraz******************l
	glClearColor(0.53f, 0.81f, 0.98f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 M = glm::mat4(1.0f);

	glm::mat4 M_tree = glm::mat4(1.0f);
	M_tree = glm::scale(M_tree, glm::vec3(scale_tree));

	glm::mat4 M_apple = glm::mat4(1.0f);
	M_apple = glm::translate(M_apple, glm::vec3(0.5f, 0.0f, 0.5f));
	M_apple = glm::scale(M_apple, glm::vec3(scale_apple));

	glm::mat4 V = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);


	glm::mat4 P = glm::perspective(glm::radians(50.0f), 1.0f, 0.1f, 100.0f);

	// oswietlenie do zrobienia
	
	drawModelGrass(grass, P, V, M, grass_text);

	//rysowanie drzew
	for (auto& treePos : treePositions) {
		glm::mat4 M_tree_instance = glm::mat4(1.0f);
		M_tree_instance = glm::translate(M_tree_instance, treePos);
		M_tree_instance = glm::scale(M_tree_instance, glm::vec3(scale_tree));
		drawModel(tree, tree_text, P, V, M_tree_instance);
	}

	//rysowanie jablek
	float groundLevel = scale_apple / 2.0f;

	for (auto& a : fallingApples) {
		float currentTime = glfwGetTime();

		if (!a.onGround) {
			float t = currentTime - a.timeStarted;
			float g = 9.81f;
			float y = a.startY - 0.5f * g * t * t;

			if (y < groundLevel) {
				y = groundLevel;
				a.onGround = true;
				a.groundTime = currentTime;
			}

			a.position.y = y;
		}
		else {
			if (currentTime - a.groundTime > 2.0f) {
				a.onGround = false;
				a.timeStarted = currentTime;

				if (!treePositions.empty()) {
					int treeIndex = rand() % treePositions.size();
					glm::vec3 treePos = treePositions[treeIndex];
					float offsetX = ((rand() % 100) / 100.0f - 0.5f) * 1.0f;
					float offsetZ = ((rand() % 100) / 100.0f - 0.5f) * 1.0f;
					glm::vec3 newPos = glm::vec3(treePos.x + offsetX, 1.5f, treePos.z + offsetZ);

					a.position.x = newPos.x;
					a.position.z = newPos.z;
					a.startY = newPos.y;
					a.position.y = newPos.y;
				}
			}
		}

		glm::mat4 M_apple_instance = glm::mat4(1.0f);
		M_apple_instance = glm::translate(M_apple_instance, glm::vec3(a.position.x, a.position.y, a.position.z));
		M_apple_instance = glm::scale(M_apple_instance, glm::vec3(scale_apple));
		drawModel(apple, apple_text, P, V, M_apple_instance);
	}

	glfwSwapBuffers(window);
}


int main(void)
{
	srand(time(NULL));
	initTrees();

	GLFWwindow* window; //Wskaźnik na obiekt reprezentujący okno

	glfwSetErrorCallback(error_callback);//Zarejestruj procedurę obsługi błędów

	if (!glfwInit()) { //Zainicjuj bibliotekę GLFW
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(500, 500, "OpenGL", NULL, NULL);

	if (!window) //Jeżeli okna nie udało się utworzyć, to zamknij program
	{
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	//kamera
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;


	if (glewInit() != GLEW_OK) { //Zainicjuj bibliotekę GLEW
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window); //Operacje inicjujące


	//Główna pętla	
	while (!glfwWindowShouldClose(window)) //Tak długo jak okno nie powinno zostać zamknięte
	{
		drawScene(window); //Wykonaj procedurę rysującą
		glfwSetKeyCallback(window, key_callback);
		glfwSetCursorPosCallback(window, mouse_callback);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // ukryj i zablokuj kursor
		glfwPollEvents(); //Wykonaj procedury callback w zalezności od zdarzeń jakie zaszły.
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window); //Usuń kontekst OpenGL i okno
	glfwTerminate(); //Zwolnij zasoby zajęte przez GLFW
	exit(EXIT_SUCCESS);
}