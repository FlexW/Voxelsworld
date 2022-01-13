#include "gl_shader.hpp"

#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
std::string read_text_file(const std::filesystem::path &file_path)
{
  std::ifstream ifs(file_path);
  if (!ifs.is_open())
  {
    throw std::runtime_error(std::string("Could not open file ") +
                             file_path.string());
  }
  std::string str(
      static_cast<std::stringstream const &>(std::stringstream() << ifs.rdbuf())
          .str());

  return str;
}

void check_for_shader_compile_errors(const std::string &file_name,
                                     GLuint             shader_id)
{
  GLint     success       = false;
  const int info_log_size = 1024;
  GLchar    info_log[info_log_size];

  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(shader_id, info_log_size, nullptr, info_log);

    const std::string message =
        " Shader compile error in " + file_name + ":\n" + info_log;
    throw std::runtime_error(message);
    }
}

void check_for_program_link_errors(GLuint program_id)
{
  GLint     success       = false;
  const int info_log_size = 1024;
  GLchar    info_log[info_log_size];
  glGetProgramiv(program_id, GL_LINK_STATUS, &success);
  if (!success)
  {
    glGetProgramInfoLog(program_id, info_log_size, nullptr, info_log);

    const std::string message = std::string("Shader link error:\n ") + info_log;
    throw std::runtime_error(message);
  }
}

} // namespace

GlShader::~GlShader()
{
  if (program_id_)
  {
    glDeleteProgram(program_id_);
  }

  if (vertex_array_id_)
  {
    glDeleteVertexArrays(1, &vertex_array_id_);
  }
}

void GlShader::init(const std::filesystem::path &vertex_shader_file_path,
                    const std::filesystem::path &fragment_shader_file_path)
{

  // Compile vertex shader
  const auto vertex_shader_code       = read_text_file(vertex_shader_file_path);
  const auto vertex_shader_id         = glCreateShader(GL_VERTEX_SHADER);
  auto       vertex_shader_code_c_str = vertex_shader_code.c_str();
  glShaderSource(vertex_shader_id, 1, &vertex_shader_code_c_str, nullptr);
  glCompileShader(vertex_shader_id);
  check_for_shader_compile_errors(vertex_shader_file_path, vertex_shader_id);

  // Compile fragment shader
  const auto fragment_shader_code = read_text_file(fragment_shader_file_path);
  const auto fragment_shader_id   = glCreateShader(GL_FRAGMENT_SHADER);
  auto       fragment_shader_code_c_str = fragment_shader_code.c_str();
  glShaderSource(fragment_shader_id, 1, &fragment_shader_code_c_str, nullptr);
  glCompileShader(fragment_shader_id);
  check_for_shader_compile_errors(fragment_shader_file_path,
                                  fragment_shader_id);

  // Link the shaders together
  program_id_ = glCreateProgram();
  glAttachShader(program_id_, vertex_shader_id);
  glAttachShader(program_id_, fragment_shader_id);
  glLinkProgram(program_id_);
  check_for_program_link_errors(program_id_);

  glDeleteShader(vertex_shader_id);
  glDeleteShader(fragment_shader_id);

  std::cout << "Compiled vertex shader " << vertex_shader_file_path.string()
            << std::endl;
  std::cout << "Compiled fragment shader " << fragment_shader_file_path.string()
            << std::endl;

  // Query informations
  GLint attribs_count = 0;
  glGetProgramiv(program_id_, GL_ACTIVE_ATTRIBUTES, &attribs_count);
  GLint max_attrib_name_length = 0;
  glGetProgramiv(program_id_,
                 GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
                 &max_attrib_name_length);

  glGenVertexArrays(1, &vertex_array_id_);
  glBindVertexArray(vertex_array_id_);

  std::cout << "Active attributes: " << attribs_count << std::endl;
  std::vector<GLchar> attrib_name_data(max_attrib_name_length);
  for (GLint i = 0; i < attribs_count; ++i)
  {
    GLint   array_size    = 0;
    GLenum  type          = 0;
    GLsizei actual_length = 0;
    glGetActiveAttrib(program_id_,
                      i,
                      attrib_name_data.size(),
                      &actual_length,
                      &array_size,
                      &type,
                      attrib_name_data.data());
    std::string attrib_name(attrib_name_data.data(), actual_length);
    std::cout << "Attribute " << i << " " << attrib_name << " type: " << type
              << " size: " << array_size << std::endl;
    glEnableVertexAttribArray(i);
    switch (type)
    {
    case GL_FLOAT_VEC2:
      glVertexAttribFormat(i, 2, GL_FLOAT, GL_FALSE, 0);
      break;
    case GL_FLOAT_VEC3:
      glVertexAttribFormat(i, 3, GL_FLOAT, GL_FALSE, 0);
      break;
    case GL_FLOAT_VEC4:
      glVertexAttribFormat(i, 4, GL_FLOAT, GL_FALSE, 0);
      break;
    case GL_INT_VEC4:
      glVertexAttribIFormat(i, 4, GL_INT, 0);
      break;
    default:
      assert(0 && "Can not handle Glsl type");
    }
    glVertexAttribBinding(i, i);
  }
  glBindVertexArray(0);

  GLint uniforms_count = 0;
  glGetProgramInterfaceiv(program_id_,
                          GL_UNIFORM,
                          GL_ACTIVE_RESOURCES,
                          &uniforms_count);
  std::cout << "Active uniforms: " << uniforms_count << std::endl;
  for (GLint i = 0; i < uniforms_count; ++i)
  {
    std::array<GLenum, 3> properties = {GL_NAME_LENGTH, GL_TYPE, GL_LOCATION};
    std::array<GLint, 3>  results{};
    glGetProgramResourceiv(program_id_,
                           GL_UNIFORM,
                           i,
                           properties.size(),
                           properties.data(),
                           results.size(),
                           nullptr,
                           results.data());
    std::string name;
    const auto  name_length = results[0];
    name.resize(name_length);
    glGetProgramResourceName(program_id_,
                             GL_UNIFORM,
                             i,
                             name_length,
                             nullptr,
                             name.data());
    const auto type     = results[1];
    const auto location = results[2];
    std::cout << "Uniform " << location << " " << name << std::endl;
    uniform_locations_[name] = location;
  }
}

void GlShader::bind() { glUseProgram(program_id_); }

void GlShader::unbind() { glUseProgram(0); }

void GlShader::draw(const std::vector<const GlVertexBuffer *> &vertex_buffers,
                    const GlIndexBuffer                       &index_buffer,
                    GLenum                                     mode)
{
  bind_vertex_array(vertex_buffers);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer.id());

  glDrawElements(mode, index_buffer.count(), GL_UNSIGNED_INT, nullptr);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void GlShader::draw(const std::vector<const GlVertexBuffer *> &vertex_buffers,
                    GLenum                                     mode)
{
  assert(vertex_buffers.size() > 0);
  bind_vertex_array(vertex_buffers);

  glDrawArrays(mode, 0, vertex_buffers[0]->count());

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

GLint GlShader::uniform_location(const std::string &name)
{
  const auto iter = uniform_locations_.find(name);
  if (iter == uniform_locations_.end())
  {
    const auto location = glGetUniformLocation(program_id_, name.c_str());
    if (location == -1)
    {
      std::cerr << "Could not find uniform " << name << std::endl;
    }
    uniform_locations_[name] = location;
    return location;
  }
  return iter->second;
}

#define GET_UNIFORM_OR_RETURN(name, location)                                  \
  const auto location = uniform_location(name);                                \
  if (location == -1)                                                          \
  {                                                                            \
    return;                                                                    \
  }

void GlShader::set_uniform(const std::string &name, bool value)
{
  GET_UNIFORM_OR_RETURN(name, location)
  glUniform1i(location, static_cast<int>(value));
}

void GlShader::set_uniform(const std::string &name, int value)
{
  GET_UNIFORM_OR_RETURN(name, location)
  glUniform1i(location, static_cast<int>(value));
}

void GlShader::set_uniform(const std::string &name, const glm::vec3 &value)
{

  GET_UNIFORM_OR_RETURN(name, location)
  glUniform3fv(location, 1, glm::value_ptr(value));
}

void GlShader::set_uniform(const std::string &name, const glm::mat4 &value)
{
  GET_UNIFORM_OR_RETURN(name, location)
  glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void GlShader::bind_vertex_array(
    const std::vector<const GlVertexBuffer *> &vertex_buffers)
{
  glBindVertexArray(vertex_array_id_);

  GLuint binding_index = 0;
  for (int i = 0; i < vertex_buffers.size(); ++i)
  {
    const auto &vertex_buffer        = vertex_buffers[i];
    const auto &vertex_buffer_layout = vertex_buffer->layout();
    GLintptr    offset               = 0;
    for (const auto &vertex_buffer_layout_element :
         vertex_buffer_layout.elements())
    {
      glBindVertexBuffer(binding_index,
                         vertex_buffer->id(),
                         offset,
                         vertex_buffer_layout_element.stride);
      ++binding_index;
      offset += vertex_buffer_layout_element.stride;
    }
  }
}
