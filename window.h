#pragma once

#include "openglwidget.h"

#include <QElapsedTimer>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QWindow>

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
    QElapsedTimer timer_;
    GLint colorUniform_ = -1;
    float hue_ = 0;

    GLint mvpUniform_ = -1;

    QMatrix4x4 model_;
    QMatrix4x4 view_;
    QMatrix4x4 projection_;

    QOpenGLBuffer vbo_{QOpenGLBuffer::Type::VertexBuffer};
    QOpenGLBuffer ibo_{QOpenGLBuffer::Type::IndexBuffer};
    QOpenGLVertexArrayObject vao_;

    std::unique_ptr<QOpenGLTexture> texture_;
    std::unique_ptr<QOpenGLShaderProgram> program_;

    bool animated_ = true;
    bool сolorAnimated_ = animated_;

    float a_ = 0.f;
    float b_ = 0.f;
    float c_ = 0.f;
    float d_ = 0.f;
    float e_ = 0.f;
};
