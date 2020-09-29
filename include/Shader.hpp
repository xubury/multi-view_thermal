#ifndef _SHADER_HPP
#define _SHADER_HPP

#include <string>
#include <memory>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    typedef std::unique_ptr<Shader> Ptr;
    static Shader::Ptr Create(const std::string &vertex_path, const std::string &fragment_path);
public:
    // the program ID
    unsigned int ID;

    // constructor reads and builds the shader
    Shader(const std::string &vertexPath, const std::string &fragmentPath);

    // use/activate the shader
    void use() const;

    // utility uniform functions
    void setBool(const std::string &name, bool value) const;

    void setInt(const std::string &name, int value) const;

    void setFloat(const std::string &name, float value) const;

    void setVec3f(const std::string &name, const glm::vec3 &value) const;

    void setVec4f(const std::string &name, const glm::vec4 &value) const;

    void setMat4f(const std::string &name, const glm::mat4 &value) const;
};

inline Shader::Ptr Shader::Create(const std::string &vertex_path, const std::string &fragment_path) {
    Shader::Ptr shader(new Shader(vertex_path, fragment_path));
    return shader;
}

#endif //_SHADER_HPP
