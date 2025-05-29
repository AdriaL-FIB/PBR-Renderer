// Author: Imanol Munoz-Pandiella 2023 based on Marc Comino 2020

#include <glwidget.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>

#include "./mesh_io.h"
#include "./triangle_mesh.h"

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <QDir>

#define SHADER_GBUFFER 5
#define SHADER_GBUFFER_VIEW 6

namespace {

// Función para mostrar errores de OpenGL
void checkOpenGLError(const char* location) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        switch (err) {
            case GL_INVALID_ENUM:
                qDebug() << "OpenGL Error at" << location << ": GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                qDebug() << "OpenGL Error at" << location << ": GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                qDebug() << "OpenGL Error at" << location << ": GL_INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                qDebug() << "OpenGL Error at" << location << ": GL_STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                qDebug() << "OpenGL Error at" << location << ": GL_STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                qDebug() << "OpenGL Error at" << location << ": GL_OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                qDebug() << "OpenGL Error at" << location << ": GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            default:
                qDebug() << "OpenGL Error at" << location << ": Unknown error (" << err << ")";
                break;
        }
    }
}


const double kFieldOfView = 60;
const double kZNear = 0.0001;
const double kZFar = 5;

const std::vector<std::vector<std::string>> kShaderFiles = {
                {"../shaders/phong.vert",        "../shaders/phong.frag"},
                {"../shaders/texMap.vert",       "../shaders/texMap.frag"},
                {"../shaders/reflection.vert",   "../shaders/reflection.frag"},
                {"../shaders/pbs.vert",          "../shaders/pbs.frag"},
                {"../shaders/ibl-pbs.vert",      "../shaders/ibl-pbs.frag"},
                {"../shaders/gbuffer.vert",      "../shaders/gbuffer.frag"},
                {"../shaders/gbuffer-view.vert", "../shaders/gbuffer-view.frag"},


                {"../shaders/precomputeBRDF.vert",         "../shaders/precomputeBRDF.frag"},
                {"../shaders/prefiltered-env.vert",        "../shaders/prefiltered-env.frag"},
                {"../shaders/computeIrradiance.vert",      "../shaders/computeIrradiance.frag"},
                {"../shaders/sky.vert",          "../shaders/sky.frag"}
              };//sky needs to be the last one

const int kVertexAttributeIdx = 0;
const int kNormalAttributeIdx = 1;
const int kTexCoordAttributeIdx = 2;


bool ReadFile(const std::string filename, std::string *shader_source) {
  std::ifstream infile(filename.c_str());

  if (!infile.is_open() || !infile.good()) {
    std::cerr << "Error " + filename + " not found." << std::endl;
    return false;
  }

  std::stringstream stream;
  stream << infile.rdbuf();
  infile.close();

  *shader_source = stream.str();
  return true;
}

bool LoadImage(const std::string& path, GLuint cube_map_pos) {
    QImage image;
    bool res = image.load(path.c_str());
    if (res) {
        // Asegurar formato consistente
        QImage gl_image = image.convertToFormat(QImage::Format_RGBA8888);

        glTexImage2D(cube_map_pos, 0, GL_RGBA, gl_image.width(), gl_image.height(), 0,
            GL_RGBA, GL_UNSIGNED_BYTE, gl_image.bits());
    }
    return res;
}

bool LoadCubeMap(const QString &dir) {
  std::string path = dir.toUtf8().constData();
  bool res = LoadImage(path + "/right.png", GL_TEXTURE_CUBE_MAP_POSITIVE_X);
  res = res && LoadImage(path + "/left.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
  res = res && LoadImage(path + "/top.png", GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
  res = res && LoadImage(path + "/bottom.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
  res = res && LoadImage(path + "/back.png", GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
  res = res && LoadImage(path + "/front.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

  if (res) {
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  }

  return res;
}

bool LoadProgram(const std::string &vertex, const std::string &fragment,
                 QOpenGLShaderProgram *program) {
  std::string vertex_shader, fragment_shader;
  bool res =
      ReadFile(vertex, &vertex_shader) && ReadFile(fragment, &fragment_shader);

  if (res) {
    program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                     vertex_shader.c_str());
    program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                     fragment_shader.c_str());
    program->bindAttributeLocation("vertex", kVertexAttributeIdx);
    program->bindAttributeLocation("normal", kNormalAttributeIdx);
    program->bindAttributeLocation("texCoord", kTexCoordAttributeIdx);
    program->link();
  }

  return res;
}

}  // namespace


void GLWidget::DrawCube(){
  static GLuint cubeVAO = 0;
  static GLuint cubeVBO = 0;

  if (cubeVAO == 0) {
      float vertices[] = {
          -1.0f,  1.0f, -1.0f,
          -1.0f, -1.0f, -1.0f,
          1.0f, -1.0f, -1.0f,
          1.0f, -1.0f, -1.0f,
          1.0f,  1.0f, -1.0f,
          -1.0f,  1.0f, -1.0f,

          -1.0f, -1.0f,  1.0f,
          -1.0f, -1.0f, -1.0f,
          -1.0f,  1.0f, -1.0f,
          -1.0f,  1.0f, -1.0f,
          -1.0f,  1.0f,  1.0f,
          -1.0f, -1.0f,  1.0f,

          1.0f, -1.0f, -1.0f,
          1.0f, -1.0f,  1.0f,
          1.0f,  1.0f,  1.0f,
          1.0f,  1.0f,  1.0f,
          1.0f,  1.0f, -1.0f,
          1.0f, -1.0f, -1.0f,

          -1.0f, -1.0f,  1.0f,
          -1.0f,  1.0f,  1.0f,
          1.0f,  1.0f,  1.0f,
          1.0f,  1.0f,  1.0f,
          1.0f, -1.0f,  1.0f,
          -1.0f, -1.0f,  1.0f,

          -1.0f,  1.0f, -1.0f,
          1.0f,  1.0f, -1.0f,
          1.0f,  1.0f,  1.0f,
          1.0f,  1.0f,  1.0f,
          -1.0f,  1.0f,  1.0f,
          -1.0f,  1.0f, -1.0f,

          -1.0f, -1.0f, -1.0f,
          -1.0f, -1.0f,  1.0f,
          1.0f, -1.0f, -1.0f,
          1.0f, -1.0f, -1.0f,
          -1.0f, -1.0f,  1.0f,
          1.0f, -1.0f,  1.0f
      };

      glGenVertexArrays(1, &cubeVAO);
      glGenBuffers(1, &cubeVBO);

      glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

      glBindVertexArray(cubeVAO);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  }

  glBindVertexArray(cubeVAO);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}

void GLWidget::DrawQuad() {
  static GLuint quadVAO = 0;
  static GLuint quadVBO_v = 0; // Vertex buffer
  static GLuint quadVBO_t = 0; // TexCoord buffer

  if (quadVAO == 0) {
      float vertices[] = {
          // Positions (x, y)
          -1.0f,  1.0f,
          -1.0f, -1.0f,
           1.0f, -1.0f,
           
          -1.0f,  1.0f,
           1.0f, -1.0f,
           1.0f,  1.0f
      };

      float uvs[] = {
          // Texture Coords (u, v)
          0.0f, 1.0f,
          0.0f, 0.0f,
          1.0f, 0.0f,
          
          0.0f, 1.0f,
          1.0f, 0.0f,
          1.0f, 1.0f
      };

      glGenVertexArrays(1, &quadVAO);
      glGenBuffers(1, &quadVBO_v);
      glGenBuffers(1, &quadVBO_t);

      glBindVertexArray(quadVAO);

      // Vertex positions
      glBindBuffer(GL_ARRAY_BUFFER, quadVBO_v);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
      glEnableVertexAttribArray(0); // Layout location 0
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

      // Texture coordinates
      glBindBuffer(GL_ARRAY_BUFFER, quadVBO_t);
      glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);
      glEnableVertexAttribArray(1); // Layout location 1
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0);
  }

  // Render the quad
  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}




glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
glm::mat4 captureViews[] = {
  glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
  glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
  glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
  glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
  glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
  glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};

void GLWidget::ComputeIrradianceMaps() {
  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);

  glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
  glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
  // glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);


  // Resize diffuse map to 32x32
  glBindTexture(GL_TEXTURE_CUBE_MAP, diffuse_map_);
  for (unsigned int i = 0; i < 6; ++i) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, 
                   GL_RGB, GL_FLOAT, nullptr);
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
  glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

  GLint projection_location, view_location, specular_map_location;

  int programIndex = programs_.size()-2;

  programs_[programIndex]->bind();

  projection_location     = programs_[programIndex]->uniformLocation("projection");
  view_location           = programs_[programIndex]->uniformLocation("view");
  specular_map_location   = programs_[programIndex]->uniformLocation("specular_map");
  
  glUniformMatrix4fv(projection_location, 1, GL_FALSE, &captureProjection[0][0]);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, specular_map_);
  glUniform1i(specular_map_location, 0);

  glViewport(0, 0, 32, 32);
  for (unsigned int i = 0; i < 6; ++i) {
    glUniformMatrix4fv(view_location, 1, GL_FALSE, &captureViews[i][0][0]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, diffuse_map_, 0);
                        
    // glDrawBuffer(GL_COLOR_ATTACHMENT0);
    // glClearColor(0.f, 1.f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render 1x1x1 cube
    DrawCube();

  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(vp[0], vp[1], vp[2], vp[3]);
}

void GLWidget::ComputePrefilteredMap() {
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    int programIndex = programs_.size() - 3;

    programs_[programIndex]->bind();

    GLint projection_location, view_location, specular_map_location, roughness_location;

    projection_location = programs_[programIndex]->uniformLocation("projection");
    view_location = programs_[programIndex]->uniformLocation("view");
    specular_map_location = programs_[programIndex]->uniformLocation("specular_map");
    roughness_location = programs_[programIndex]->uniformLocation("roughness");

    glUniformMatrix4fv(projection_location, 1, GL_FALSE, &captureProjection[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, specular_map_);
    glUniform1i(specular_map_location, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        // reisze framebuffer according to mip-level size.
        unsigned int mipWidth = 128 >> mip;
        unsigned int mipHeight = 128 >> mip;
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        glUniform1f(roughness_location, roughness);

        for (unsigned int i = 0; i < 6; ++i)
        {
            glUniformMatrix4fv(view_location, 1, GL_FALSE, &captureViews[i][0][0]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            DrawCube();
        }

        //SaveCubeMap(prefilterMap, "prefilterMap" + QString::number(mip), mipWidth, mip);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(vp[0], vp[1], vp[2], vp[3]);
}

void GLWidget::ComputeBRDFLUTTexture() {
  glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
  glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

  glViewport(0, 0, 512, 512);
  programs_[programs_.size() - 4]->bind();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  DrawQuad();

  // Save image
  // QImage image(512, 512, QImage::Format_RGBA8888);
  // glReadBuffer(GL_COLOR_ATTACHMENT0);
  // glReadPixels(0, 0, 512, 512, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
  // image = image.mirrored();
  // image.save("brdfLUT.png");

  glBindFramebuffer(GL_FRAMEBUFFER, 0); 
}

void GLWidget::InitCaptureFBO() {
  std::cout << "Init FBO" << std::endl;
  glGenFramebuffers(1, &captureFBO);
  glGenRenderbuffers(1, &captureRBO);

  
  glGenTextures(1, &prefilterMap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
  for (unsigned int i = 0; i < 6; ++i) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);


  glGenTextures(1, &brdfLUTTexture);
  // pre-allocate enough memory for the LUT texture.
  glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 

}

void GLWidget::InitGBufferTextures() {
    glGenFramebuffers(1, &gBufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);

    glGenTextures(1, &gAlbedoTexture);
    glBindTexture(GL_TEXTURE_2D, gAlbedoTexture);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->width_, this->height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gAlbedoTexture, 0);

    glGenTextures(1, &gNormalTexture);
    glBindTexture(GL_TEXTURE_2D, gNormalTexture);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, this->width_, this->height_, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormalTexture, 0);

    glGenTextures(1, &gDepthTexture);
    glBindTexture(GL_TEXTURE_2D, gDepthTexture);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, this->width_, this->height_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepthTexture, 0);

    GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLWidget::renderGBuffer() {
    if (!initialized_)
        return;

    camera_.SetViewport();

    glm::mat4x4 projection = camera_.SetProjection();
    glm::mat4x4 view = camera_.SetView();
    glm::mat4x4 model = camera_.SetModel();

    //compute normal matrix
    glm::mat4x4 t = view * model;
    glm::mat3x3 normal;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            normal[i][j] = t[i][j];
    normal = glm::transpose(glm::inverse(normal));

    if (mesh_ != nullptr) {
        GLint projection_location, view_location, model_location, normal_matrix_location;

        glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Framebuffer not complete. Status: 0x" << std::hex << status << std::endl;
        }

        programs_[SHADER_GBUFFER]->bind();

        projection_location = programs_[SHADER_GBUFFER]->uniformLocation("projection");
        view_location = programs_[SHADER_GBUFFER]->uniformLocation("view");
        model_location = programs_[SHADER_GBUFFER]->uniformLocation("model");
        normal_matrix_location = programs_[SHADER_GBUFFER]->uniformLocation("normal_matrix");
        //current_text_location = programs_[SHADER_GBUFFER]->uniformLocation("current_texture");

        glUniformMatrix4fv(projection_location, 1, GL_FALSE, &projection[0][0]);
        glUniformMatrix4fv(view_location, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(model_location, 1, GL_FALSE, &model[0][0]);
        glUniformMatrix3fv(normal_matrix_location, 1, GL_FALSE, &normal[0][0]);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, mesh_->faces_.size(), GL_UNSIGNED_INT, nullptr);

        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    }
}


//void GLWidget::initializeSSAOTex() {
//    glGenFramebuffers(1, &ssaoFBO);
//    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
//
//    glGenTextures(1, &ssaoTex);
//    glBindTexture(GL_TEXTURE_2D, ssaoTex);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width_, height_, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTex, 0);
//
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//}

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
    initialized_(false),
    width_(0.0),
    height_(0.0),
    currentShader_(0),
    fresnel_(0.2, 0.2, 0.2),
    currentTexture_(0),
    skyVisible_(true),
    metalness_(0),
    roughness_(0),
    renderGBuffer_(true),
    showGBuffer(-1),
    VAO(0), VAO_sky(0), VBO_i(0), VBO_n(0), VBO_tc(0), VBO_v(0), VBO_i_sky(0), VBO_v_sky(0)
        {
  setFocusPolicy(Qt::StrongFocus);
}

GLWidget::~GLWidget() {
  if (initialized_) {
    glDeleteTextures(1, &specular_map_);
    glDeleteTextures(1, &diffuse_map_);
  }
}

bool GLWidget::LoadModel(const QString &filename) {
  std::string file = filename.toUtf8().constData();
  size_t pos = file.find_last_of(".");
  std::string type = file.substr(pos + 1);

  std::unique_ptr<data_representation::TriangleMesh> mesh =
      std::make_unique<data_representation::TriangleMesh>();

  bool res = false;
  if (type.compare("ply") == 0) {
    res = data_representation::ReadFromPly(file, mesh.get());
  } else if (type.compare("obj") == 0) {
    res = data_representation::ReadFromObj(file, mesh.get());
  } else if(type.compare("null") == 0) {
    res = data_representation::CreateSphere(mesh.get());
    std::cout << "Creating Sphere" << std::endl;
  }

  if (res) {
    makeCurrent();
    mesh_.reset(mesh.release());
    camera_.UpdateModel(mesh_->min_, mesh_->max_);
    //mesh_->computeNormals();

    // T0DO(students): Create / Initialize buffers.
    //MESH: You need to create 1 VAO and 4 VBO
    //mesh_->vertices -> attrib location 0
    //mesh_->normals -> attrib location 1
    //mesh_->texCoords -> attrib location 2
    //mesh_->faces -> elements

    //if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    //glGenVertexArrays(1, &VAO);

    //checkOpenGLError("glGenVertexArrays VAO");


    //if (VBO_v != 0) glDeleteBuffers(1, &VBO_v);
    //if (VBO_n != 0) glDeleteBuffers(1, &VBO_n);
    //if (VBO_tc != 0) glDeleteBuffers(1, &VBO_tc);
    //if (VBO_i != 0) glDeleteBuffers(1, &VBO_i);
    //glGenBuffers(1, &VBO_v);
    //glGenBuffers(1, &VBO_n);
    //glGenBuffers(1, &VBO_tc);
    //glGenBuffers(1, &VBO_i);
    //checkOpenGLError("glGenBuffers VBO");


    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_v);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*mesh_->vertices_.size(), &mesh_->vertices_[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_n);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*mesh_->normals_.size(), &mesh_->normals_[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_tc);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*mesh_->texCoords_.size(), &mesh_->texCoords_[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_i);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*mesh_->faces_.size(), &mesh_->faces_[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    //SKY BOX: You need to create 1 VAO and 2 VBO:
    // vertices -> attrib location 0
    //faces -> elements
    
    if (VAO_sky == 0) {
        glGenVertexArrays(1, &VAO_sky);

        glGenBuffers(1, &VBO_v_sky);
        glGenBuffers(1, &VBO_i_sky);

        glBindVertexArray(VAO_sky);


        /*
         *
         *
         *          4           5
         *      6           7
         *
         *
         *          0           1
         *      2           3
         */

        skyVertices_ = {
          -1.f, -1.f, -1.f,
          +1.f, -1.f, -1.f,
          -1.f, -1.f, +1.f,
          +1.f, -1.f, +1.f,
          -1.f, +1.f, -1.f,
          +1.f, +1.f, -1.f,
          -1.f, +1.f, +1.f,
          +1.f, +1.f, +1.f,
          };

        skyFaces_ = {
          0, 1, 2,
          1, 3, 2,

          2, 7, 6,
          2, 3, 7,

          0, 5, 4,
          0, 1, 5,

          6, 4, 5,
          6, 5, 7,
      
          3, 5, 7,
          3, 1, 5,

          0, 6, 4,
          0, 2, 6
        };

        glBindBuffer(GL_ARRAY_BUFFER, VBO_v_sky);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*skyVertices_.size(), &skyVertices_[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_i_sky);  
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*skyFaces_.size(), &skyFaces_[0], GL_STATIC_DRAW);

        glBindVertexArray(0);
        // T0DO END.
    }

    checkOpenGLError("Load model");
    doneCurrent();

    emit SetFaces(QString(std::to_string(mesh_->faces_.size() / 3).c_str()));
    emit SetVertices(
        QString(std::to_string(mesh_->vertices_.size() / 3).c_str()));
    return true;
  }

  return false;
}

bool GLWidget::LoadSpecularMap(const QString &dir) {
  glBindTexture(GL_TEXTURE_CUBE_MAP, specular_map_);
  bool res = LoadCubeMap(dir);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

  ComputeIrradianceMaps();
  ComputePrefilteredMap();
  ComputeBRDFLUTTexture();
  // test_tt();
  // GenerateIrradianceAndPrefilteredMaps(32, 32, 3);

  update();
  return res;
}

bool GLWidget::LoadDiffuseMap(const QString &dir) {
  glBindTexture(GL_TEXTURE_CUBE_MAP, diffuse_map_);
  bool res = LoadCubeMap(dir);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  update();
  return res;
}

bool GLWidget::LoadColorMap(const QString &filename)
{
    //TODO Students
    //Configure the texture with identifier color_map_. Take advantage of LoadImage("path", GL_TEXTURE_2D).
    //Remember to configure the texture parameters.
    bool res;
    glBindTexture(GL_TEXTURE_2D, color_map_);
    res = LoadImage(filename.toStdString(), GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    //TODO END
    update();
    return res;

}

bool GLWidget::LoadRoughnessMap(const QString &filename)
{
    //TODO Students
    //Configure the texture with identifier roughness_map_. Take advantage of LoadImage("path", GL_TEXTURE_2D)
    //Remember to configure the texture parameters.
    bool res;
    glBindTexture(GL_TEXTURE_2D, roughness_map_);
    res = LoadImage(filename.toStdString(), GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    //TODO END
    update();
    return res;
}

bool GLWidget::LoadMetalnessMap(const QString &filename)
{
    //TODO Students
    //Configure the texture with identifier metalness_map_. Take advantage of LoadImage("path", GL_TEXTURE_2D)
    //Remember to configure the texture parameters.
    bool res;
    glBindTexture(GL_TEXTURE_2D, metalness_map_);
    res = LoadImage(filename.toStdString(), GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    //TODO END
    update();
    return res;
}

void GLWidget::initializeGL()
{
  // Cal inicialitzar l'ús de les funcions d'OpenGL
  initializeOpenGLFunctions();
  checkOpenGLError("initializeOpenGLFunctions");

  //initializing opengl state
  //glEnable(GL_NORMALIZE);
  glDisable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); 
  checkOpenGLError("OpenGL State Setup");

  //glEnable(GL_TEXTURE_2D);

  //generating needed textures
  glGenTextures(1, &specular_map_);
  glGenTextures(1, &diffuse_map_);
  glGenTextures(1, &color_map_);
  glGenTextures(1, &roughness_map_);
  glGenTextures(1, &metalness_map_);

  InitCaptureFBO();

  InitGBufferTextures();


  //create shader programs
  for (unsigned int k = 0; k < kShaderFiles.size(); ++k) {
      programs_.push_back(std::make_unique<QOpenGLShaderProgram>());
  }

  //load vertex and fragment shader files
  bool res = true;
  for (unsigned int k = 0; k < programs_.size(); ++k) {
    res &= LoadProgram(kShaderFiles[k][0],   kShaderFiles[k][1],    programs_[k].get());
  }

  if (!res) exit(0);

  glGenVertexArrays(1, &VAO);

  glGenBuffers(1, &VBO_v);
  glGenBuffers(1, &VBO_n);
  glGenBuffers(1, &VBO_tc);
  glGenBuffers(1, &VBO_i);

  //LoadModel("../models/Armadillo.ply");//create an sphere
  LoadModel(".null");//create an sphere

  initialized_ = true;
}

void GLWidget::resizeGL(int w, int h)
{
    if (h == 0) h = 1;
    width_ = w;
    height_ = h;

    std::cout << "resizeGL to w:" << w << " h:" << h << std::endl;

    // Resize GBuffer textures
    glBindTexture(GL_TEXTURE_2D, gAlbedoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, gNormalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width_, height_, 0, GL_RGB, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, gDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width_, height_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    camera_.SetViewport(0, 0, w, h);
    camera_.SetProjection(kFieldOfView, kZNear, kZFar);
}

void GLWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    camera_.StartRotating(event->x(), event->y());
  }
  if (event->button() == Qt::RightButton) {
    camera_.StartZooming(event->x(), event->y());
  }
  update();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event) {
  camera_.SetRotationX(event->y());
  camera_.SetRotationY(event->x());
  camera_.SafeZoom(event->y());
  update();
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    camera_.StopRotating(event->x(), event->y());
  }
  if (event->button() == Qt::RightButton) {
    camera_.StopZooming(event->x(), event->y());
  }
  update();
}

void GLWidget::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Up) camera_.Zoom(-1);
  if (event->key() == Qt::Key_Down) camera_.Zoom(1);

  if (event->key() == Qt::Key_Left) camera_.Rotate(-1);
  if (event->key() == Qt::Key_Right) camera_.Rotate(1);

  if (event->key() == Qt::Key_W) camera_.Zoom(-1);
  if (event->key() == Qt::Key_S) camera_.Zoom(1);

  if (event->key() == Qt::Key_A) camera_.Rotate(-1);
  if (event->key() == Qt::Key_D) camera_.Rotate(1);

  if (event->key() == Qt::Key_R) {
      for(auto i = 0; i < programs_.size(); ++i) {
          programs_[i].reset();
          programs_[i] = std::make_unique<QOpenGLShaderProgram>();
          LoadProgram(kShaderFiles[i][0], kShaderFiles[i][1], programs_[i].get());
      }
  }

  update();
}

void GLWidget::paintGL()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (initialized_) {
        camera_.SetViewport();

        glm::mat4x4 projection = camera_.SetProjection();
        glm::mat4x4 view = camera_.SetView();
        glm::mat4x4 model = camera_.SetModel();

        //compute normal matrix
        glm::mat4x4 t = view * model;
        glm::mat3x3 normal;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                normal[i][j] = t[i][j];
         normal = glm::transpose(glm::inverse(normal));

        if (mesh_ != nullptr) {
            GLint projection_location, view_location, model_location,
            normal_matrix_location, specular_map_location, diffuse_map_location,
            fresnel_location, color_map_location, roughness_map_location, metalness_map_location,
            current_text_location, light_location, roughness_location, metalness_location;

            GLint prefilter_map_location, brdfLUT_location;

            GLint zNear_location, zFar_location;


            checkOpenGLError("Before renderGBuffer()");
            renderGBuffer();
            checkOpenGLError("After renderGBuffer()");

            //std::cout << "showGBuffer = " << showGBuffer << std::endl;
            if (showGBuffer > 0) {

                glBindFramebuffer(GL_READ_FRAMEBUFFER, gBufferFBO);
                checkOpenGLError("glBindFramebuffer GL_READ_FRAMEBUFFER");


                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebufferObject());
                checkOpenGLError("glBindFramebuffer GL_DRAW_FRAMEBUFFER");


                glBlitFramebuffer(
                    0, 0, width_, height_,
                    0, 0, width_, height_,
                    GL_DEPTH_BUFFER_BIT,
                    GL_NEAREST
                );
                checkOpenGLError("glBlitFramebuffer");

                programs_[SHADER_GBUFFER_VIEW]->bind();

                GLint albedo_sampler_loc = programs_[SHADER_GBUFFER_VIEW]->uniformLocation("gAlbedoSampler");
                GLint normal_sampler_loc = programs_[SHADER_GBUFFER_VIEW]->uniformLocation("gNormalSampler");
                GLint depth_sampler_loc = programs_[SHADER_GBUFFER_VIEW]->uniformLocation("gDepthSampler");
                zNear_location = programs_[SHADER_GBUFFER_VIEW]->uniformLocation("zNear");
                zFar_location = programs_[SHADER_GBUFFER_VIEW]->uniformLocation("zFar");
                current_text_location = programs_[SHADER_GBUFFER_VIEW]->uniformLocation("current_texture");

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, gAlbedoTexture);
                glUniform1i(albedo_sampler_loc, 0);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, gNormalTexture);
                glUniform1i(normal_sampler_loc, 1);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, gDepthTexture);
                glUniform1i(depth_sampler_loc, 2);

                glUniform1i(current_text_location, showGBuffer-1);
                glUniform1f(zNear_location, kZNear);
                glUniform1f(zFar_location, kZFar);

                glDisable(GL_DEPTH_TEST);
                glDepthMask(GL_FALSE);
                DrawQuad();
                glDepthMask(GL_TRUE);
                glEnable(GL_DEPTH_TEST);

            }
            else {
                //MESH-----------------------------------------------------------------------------------------
                //general shader setting

                programs_[currentShader_]->bind();

                projection_location       = programs_[currentShader_]->uniformLocation("projection");
                view_location             = programs_[currentShader_]->uniformLocation("view");
                model_location            = programs_[currentShader_]->uniformLocation("model");
                normal_matrix_location    = programs_[currentShader_]->uniformLocation("normal_matrix");
                specular_map_location     = programs_[currentShader_]->uniformLocation("specular_map");
                prefilter_map_location    = programs_[currentShader_]->uniformLocation("prefilter_map");
                brdfLUT_location          = programs_[currentShader_]->uniformLocation("brdfLUT");
                diffuse_map_location      = programs_[currentShader_]->uniformLocation("diffuse_map");
                color_map_location        = programs_[currentShader_]->uniformLocation("color_map");
                roughness_map_location    = programs_[currentShader_]->uniformLocation("roughness_map");
                metalness_map_location    = programs_[currentShader_]->uniformLocation("metalness_map");
                current_text_location     = programs_[currentShader_]->uniformLocation("current_texture");
                fresnel_location          = programs_[currentShader_]->uniformLocation("fresnel");
                light_location            = programs_[currentShader_]->uniformLocation("light");
                roughness_location        = programs_[currentShader_]->uniformLocation("roughness");
                metalness_location        = programs_[currentShader_]->uniformLocation("metalness");



                glUniformMatrix4fv(projection_location, 1, GL_FALSE, &projection[0][0]);
                glUniformMatrix4fv(view_location, 1, GL_FALSE, &view[0][0]);
                glUniformMatrix4fv(model_location, 1, GL_FALSE, &model[0][0]);
                glUniformMatrix3fv(normal_matrix_location, 1, GL_FALSE, &normal[0][0]);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, specular_map_);
                glUniform1i(specular_map_location, 0);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_CUBE_MAP, diffuse_map_);
                glUniform1i(diffuse_map_location, 1);

                //T0DO(students): active texture location for the following textures:
                //Texture unit 3 color_map_
                //Texture unit 4 roughness_map_
                //Texture unit 5 metalness_map_

                glActiveTexture(GL_TEXTURE2);
                //glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, color_map_);
                glUniform1i(color_map_location, 2);

                glActiveTexture(GL_TEXTURE3);
                // glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, roughness_map_);
                glUniform1i(roughness_map_location, 3);

                glActiveTexture(GL_TEXTURE4);
                // glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, metalness_map_);
                glUniform1i(metalness_map_location, 4);

                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
                glUniform1i(prefilter_map_location, 5);

                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
                glUniform1i(brdfLUT_location, 6);


                //T0DO END

                glUniform1i(current_text_location, currentTexture_ + 2); // TODO: chapuza ?
                glUniform3f(fresnel_location, fresnel_[0], fresnel_[1], fresnel_[2]);
                glUniform3f(light_location, 0, 2, 0);
                glUniform1f(roughness_location, roughness_);
                glUniform1f(metalness_location, metalness_);


                // T0DO(students): Implement draw call of the mesh
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, mesh_->faces_.size(), GL_UNSIGNED_INT, nullptr);

                // T0DO END.

            }
            //SKY-----------------------------------------------------------------------------------------
            if(skyVisible_) {
                //model = camera_.SetIdentity();

                programs_[programs_.size()-1]->bind();

                projection_location     = programs_[programs_.size()-1]->uniformLocation("projection");
                view_location           = programs_[programs_.size()-1]->uniformLocation("view");
                model_location          = programs_[programs_.size()-1]->uniformLocation("model");
                normal_matrix_location  = programs_[programs_.size()-1]->uniformLocation("normal_matrix");
                specular_map_location   = programs_[programs_.size()-1]->uniformLocation("specular_map");



                glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view)); 

                glUniformMatrix4fv(projection_location, 1, GL_FALSE, &projection[0][0]);
                glUniformMatrix4fv(view_location, 1, GL_FALSE, &viewNoTranslation[0][0]);
                glUniformMatrix4fv(model_location, 1, GL_FALSE, &model[0][0]);
                glUniformMatrix3fv(normal_matrix_location, 1, GL_FALSE, &normal[0][0]);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, specular_map_);
                glUniform1i(specular_map_location, 0);

                // T0DO(students): implement the draw call of the sky box
                // unsigned int texId;
                // glGenTextures(1, &texId);
                // glBindTexture()

                glDepthFunc(GL_LEQUAL);
                glBindVertexArray(VAO_sky);
                glDrawElements(GL_TRIANGLES, skyFaces_.size(), GL_UNSIGNED_INT, nullptr);
                glDepthFunc(GL_LESS);

                // T0DO END.
            }
        }
    }
}

void GLWidget::SetReflection(bool set) {
    if(set) currentShader_ = 2;
    update();
}

void GLWidget::SetPBS(bool set) {
    if(set) currentShader_ = 3;
    update();
}

void GLWidget::SetIBLPBS(bool set) {
    if(set) currentShader_ = 4;
    update();
}

void GLWidget::SetPhong(bool set)
{
    if(set) currentShader_ = 0;
    update();
}

void GLWidget::SetTexMap(bool set)
{
    if(set) currentShader_ = 1;
    update();
}

void GLWidget::SetFresnelR(double r) {
    fresnel_[0] = r;
    update();
}

void GLWidget::SetFresnelG(double g) {
    fresnel_[1] = g;
    update();
}

void GLWidget::SetCurrentTexture(int i)
{
    currentTexture_ = i;
    update();
}

void GLWidget::SetSkyVisible(bool set)
{
    skyVisible_ = set;
    update();
}

void GLWidget::SetFresnelB(double b) {
    fresnel_[2] = b;
    update();
}

void GLWidget::SetMetalness(double d) {
    metalness_ = d;
    update();
}

void GLWidget::SetRoughness(double d) {
    roughness_ = d;
    update();
}

void GLWidget::SetShowGBuffer(bool b) {
    showGBuffer *= -1;
    update();
}

void GLWidget::SetShowGBuffer(int i) {
    int sign;
    if (showGBuffer >= 0)
        sign = 1;
    else
        sign = -1;
    showGBuffer = (i+1) * sign;
    update();
}


