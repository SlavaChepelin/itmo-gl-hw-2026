#pragma once

#include "openglwidget.h"

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QPoint>
#include <QVector2D>

class Window : public OpenGLWidget
{
    Q_OBJECT

public:
    Window();
    ~Window();

    void onInit() override;
    void onRender() override;
    void onResize(size_t width, size_t height) override;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QVector2D screenToWorld(const QPointF& screen) const;

    GLint colorUniform_ = -1;
    GLint mvpUniform_ = -1;

    QMatrix4x4 projection_;

    QOpenGLBuffer vbo_{QOpenGLBuffer::Type::VertexBuffer};
    QOpenGLVertexArrayObject vao_;

    std::unique_ptr<QOpenGLShaderProgram> program_;

    float a_ = 0.f;
    float b_ = 0.f;
    float c_ = 0.f;
    float d_ = 0.f;
    float e_ = 0.f;
    float xMin_ = -10.f;
    float xMax_ = 10.f;
    float yMin_ = -10.f;
    float yMax_ = 10.f;

    bool dragging_ = false;
    QPoint lastMousePos_;
};
