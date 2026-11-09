#pragma once

#include "openglwidget.h"

#include <QElapsedTimer>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QPoint>
#include <QVector2D>

class Window : public OpenGLWidget {
  Q_OBJECT

public:
  Window();
  ~Window();

  void onInit() override;
  void onRender() override;
  void onResize(size_t width, size_t height) override;

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  QVector2D screenToWorld(const QPointF &screen) const;

  GLint resolutionUniform_ = -1;
  GLint viewMinUniform_ = -1;
  GLint viewMaxUniform_ = -1;
  GLint nUniform_ = -1;
  GLint timeUniform_ = -1;
  GLint rotSpeedUniform_ = -1;

  QOpenGLBuffer vbo_{QOpenGLBuffer::Type::VertexBuffer};
  QOpenGLVertexArrayObject vao_;
  std::unique_ptr<QOpenGLTexture> texture_;
  std::unique_ptr<QOpenGLShaderProgram> program_;
  QElapsedTimer timer_;

  float n_ = 6.f;
  float rotSpeed_ = 0.4f;

  float xMin_ = -1.35f;
  float xMax_ = 1.35f;
  float yMin_ = -1.35f;
  float yMax_ = 1.35f;

  bool dragging_ = false;
  QPoint lastMousePos_;
};
