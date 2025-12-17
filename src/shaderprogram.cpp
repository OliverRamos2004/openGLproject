#include "shaderprogram.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>


std::string ShaderProgram::loadShaderSource(const std::string &filePath) {
    //OpenGL requires the shader to be stored as a const string.
    //Create an input file stream to read from the file.
    std::ifstream shaderFile;
    //bitwise OR operation combining two error state flags used with
    //input/output streams, particularly std::ifstream for file input.
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        shaderFile.open(filePath);
        std::stringstream shaderStream;
        //Reads the entire contents of the file into the string stream using the file buffer.
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        return shaderStream.str();
    } catch (std::ifstream::failure &e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << filePath << "\n";
        return "";
    }
}

GLuint ShaderProgram::compileShader(const std::string &source, GLenum shaderType) {
    //Converts the std::string source code into a C-style string (const char*)
    //because OpenGL expects shader source code in this format.
    const char *shaderCode = source.c_str();
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderCode, nullptr);
    glCompileShader(shader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << "\n";
    }

    return shader;
}

void ShaderProgram::linkProgram(GLuint vertexShader, GLuint fragmentShader) {
    //Creates a new shader program and returns its ID.
    ID = glCreateProgram();
    //Attach the compiled vertex and fragment shaders to the program.
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);
    //After linking, the shaders are no longer needed separately, so it's safe to delete them.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, nullptr, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << "\n";
    }
}

ShaderProgram::ShaderProgram(const std::string &vertexPath, const std::string &fragmentPath) {
    std::string vertexCode = loadShaderSource(vertexPath);
    std::string fragmentCode = loadShaderSource(fragmentPath);

    //we call compileShader function by supplying different second parameter to indicate different shaders.
    GLuint vertexShader = compileShader(vertexCode, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentCode, GL_FRAGMENT_SHADER);

    linkProgram(vertexShader, fragmentShader);
}

//This is the destructor of the ShaderProgram class.
//It is automatically called when a ShaderProgram object goes out of scope or is deleted.
//It calls the destroy() method to clean up the OpenGL shader program.
ShaderProgram::~ShaderProgram() {
    destroy();
}

//This method activates the shader program so that OpenGL uses it for rendering.
void ShaderProgram::use() const {
    glUseProgram(ID);
}

//This is a getter method that returns the shader program's ID.
GLuint ShaderProgram::getID() const {
    return ID;
}

//This method cleans up the shader program by deleting it from OpenGL.
//!isDeleted: to avoid deleting the program multiple times.
//ID != 0: to ensure there's a valid program to delete.

void ShaderProgram::destroy() {
    if (!isDeleted && ID != 0) {
        glDeleteProgram(ID);
        ID = 0;
        isDeleted = true;
    }
}

void ShaderProgram::setUniform(const std::string &name, int value) const {
    GLint location = glGetUniformLocation(ID, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: uniform '" << name << "' not found in shader.\n";
        return;
    }
    glUniform1i(location, value);
}

void ShaderProgram::setUniform(const std::string &name, float value) const {
    GLint location = glGetUniformLocation(ID, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: uniform '" << name << "' not found in shader.\n";
        return;
    }
    glUniform1f(location, value);
}

void ShaderProgram::setUniform(const std::string &name, const glm::vec3 &value) const {
    GLint location = glGetUniformLocation(ID, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: uniform '" << name << "' not found in shader.\n";
        return;
    }
    glUniform3fv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::setUniform(const std::string &name, const glm::mat4 &value) const {
    GLint location = glGetUniformLocation(ID, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: uniform '" << name << "' not found in shader.\n";
        return;
    }
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

GLuint ShaderProgram::bindTexture2D(const std::string &samplerName,
                                    const std::string &filePath,
                                    GLint textureUnit,
                                    bool flipVertical,
                                    GLint wrap,
                                    GLint minFilter,
                                    GLint magFilter,
                                    bool generateMipmaps) const {
    if (samplerName.empty() || filePath.empty()) {
        std::cerr << "bindTexture2D: empty samplerName or filePath\n";
        return 0;
    }

    // Ensure we set uniforms on the correct program
    use();

    stbi_set_flip_vertically_on_load(flipVertical ? 1 : 0);
    int w = 0, h = 0, ch = 0;
    unsigned char *data = stbi_load(filePath.c_str(), &w, &h, &ch, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << filePath << "\n";
        return 0;
    }

    GLenum srcFormat = GL_RGB;
    GLint internal = GL_RGB;
    if (ch == 4) {
        srcFormat = GL_RGBA;
        internal = GL_RGBA;
    } else if (ch == 3) {
        srcFormat = GL_RGB;
        internal = GL_RGB;
    } else if (ch == 1) {
        srcFormat = GL_RED;
        internal = GL_RED;
    }

    GLuint texID = 0;
    glGenTextures(1, &texID);

    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

    glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, srcFormat, GL_UNSIGNED_BYTE, data);
    if (generateMipmaps)
        glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    // bind sampler uniform to this unit
    GLint loc = glGetUniformLocation(ID, samplerName.c_str());
    if (loc >= 0) {
        glUniform1i(loc, textureUnit);
    } else {
        std::cerr << "Warning: sampler uniform not found: " << samplerName << "\n";
    }

    return texID;
}

GLuint ShaderProgram::bindCubeMap(const std::string &samplerName,
                                  const std::vector<std::string> &faces,
                                  GLint textureUnit,
                                  bool flipVertical,
                                  GLint wrap,
                                  GLint minFilter,
                                  GLint magFilter) const {
    use();
    stbi_set_flip_vertically_on_load(flipVertical ? 1 : 0);

    GLuint texID = 0;
    glGenTextures(1, &texID);

    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    int w = 0, h = 0, ch = 0;
    for (int i = 0; i < faces.size(); ++i) {
        unsigned char *data = stbi_load(faces[i].c_str(), &w, &h, &ch, 0);
        if (!data) {
            std::cerr << "Failed to load cubmap face: " << faces[i] << std::endl;
            break;
        }
        GLenum srcFormat = GL_RGB;
        GLint internal = GL_RGB;
        if (ch == 4) {
            srcFormat = GL_RGBA;
            internal = GL_RGBA;
        } else if (ch == 3) {
            srcFormat = GL_RGB;
            internal = GL_RGB;
        } else if (ch == 1) {
            srcFormat = GL_RED;
            internal = GL_RED;
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(i),
                     0, internal, w, h, 0,
                     srcFormat, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, magFilter);

    GLint loc = glGetUniformLocation(ID, samplerName.c_str());
    if (loc >= 0) {
        glUniform1i(loc, textureUnit);
    } else {
        std::cerr << "Warning: samplercube uniform not found!" << std::endl;
    }

    return texID;
}
