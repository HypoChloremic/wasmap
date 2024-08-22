#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <emscripten/emscripten.h>
#define GLM_FORCE_PURE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include "shader.hpp"
#include "file_handling.hpp"
#include "buffering.hpp"
#include "cleaning.hpp"
#include "controls.hpp"
#include "setup.hpp"

using namespace glm;

const GLfloat BASE_LAT = 0.0;
const GLfloat BASE_LON = 0.0;

GLFWwindow* window;
GLuint vertexbuffer, VertexArrayID;
GLuint programID, MatrixID, UniformColorID;
std::vector<GLfloat> coordinates;

void main_loop() {
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(programID);

    // Compute the MVP matrix from keyboard and mouse input
    computeMatricesFromInputs(window);
    glm::mat4 ProjectionMatrix = getProjectionMatrix();
    glm::mat4 ViewMatrix = getViewMatrix();
    glm::mat4 RotationMatrix = getRotationMatrix();
    glm::mat4 MVP = RotationMatrix * ProjectionMatrix * ViewMatrix;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            std::cout << MVP[i][j] << " ";
        }
        std::cout << std::endl;
    }

    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)0
    );

    glUniform3f(UniformColorID, 1.0f, 0.0f, 0.0f);
    glDrawArrays(GL_LINES, 0, coordinates.size() / 2);

    glDisableVertexAttribArray(0);
    glfwSwapBuffers(window);
    glfwPollEvents();

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwWindowShouldClose(window) != 0) {
        emscripten_cancel_main_loop();
    }
}

extern "C" {
    void updateCoordinates(float* newCoords, int length) {
        coordinates.clear();
        for (int i = 0; i < length; ++i) {
            coordinates.push_back(newCoords[i]);
        }
        // Normalize the new coordinates
        normalize_coords(coordinates, BASE_LAT, BASE_LON);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Missing input file flags" << std::endl;
        return -1;
    }

    window = windowSetup();
    if (window == nullptr) {
        std::cerr << "Window setup error" << std::endl;
        return -1;
    }

    std::string filename = get_filename(argc, argv);
    std::ifstream file(filename);
    if (file) {
        GLfloat iVal;
        while (file >> iVal) {
            coordinates.push_back(iVal);
        }
    } else {
        std::cerr << "Could not open file: " << filename << std::endl;
        return -1;
    }

    normalize_coords(coordinates, BASE_LAT, BASE_LON);

    draw_triangle(&VertexArrayID, &vertexbuffer, coordinates);

    programID = LoadShaders("./shaders/vertex_shader.vert", "./shaders/fragment_shader.frag");
    MatrixID = glGetUniformLocation(programID, "MVP");
    UniformColorID = glGetUniformLocation(programID, "uniformcolor");

    // Start the main loop
    emscripten_set_main_loop(main_loop, 0, 1);

    return 0;
}
