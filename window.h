#pragma once

#include "openglwidget.h"

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

class Window : public OpenGLWidget
{
    Q_OBJECT

public:
    Window();
    ~Window();

    void onInit() override;
    void onRender() override;
    void onResize(size_t width, size_t height) override;

private:
    GLint colorUniform_ = -1;
    GLint mvpUniform_ = -1;

    QMatrix4x4 model_;
    QMatrix4x4 view_;
    QMatrix4x4 projection_;

    QOpenGLBuffer vbo_{QOpenGLBuffer::Type::VertexBuffer};
    QOpenGLVertexArrayObject vao_;

    std::unique_ptr<QOpenGLShaderProgram> program_;

    float a_ = 0.f;
    float b_ = 0.f;
    float c_ = 0.f;
    float d_ = 0.f;
    float e_ = 0.f;
};
