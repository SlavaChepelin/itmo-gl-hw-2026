#include "window.h"

#include <QLabel>
#include <QMouseEvent>
#include <QSlider>
#include <QWheelEvent>

#include <algorithm>

Window::Window() {
  // Add label for FPS
  auto *fpsLabel = new QLabel("FPS: 0", this);
  fpsLabel->setStyleSheet("QLabel { color: white; }");
  fpsLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  fpsLabel->adjustSize();
  fpsLabel->move(0, 0);
  fpsLabel->raise();
  connect(this, &Window::fpsUpdated, [fpsLabel](double fps) {
    fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 0));
    fpsLabel->adjustSize();
  });

  // https://doc.qt.io/qt-6/qslider.html
  auto makeSlider = [this](float &coeff, int y, int def, const QString &name) {
    auto *label = new QLabel(name, this);
    label->setStyleSheet("QLabel { color: white; }");
    label->move(10, y);

    auto *valueLabel = new QLabel(QString::number(def * 0.1f, 'f', 1), this);
    valueLabel->setStyleSheet("QLabel { color: white; }");
    valueLabel->move(240, y);

    auto *slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(-50, 50);
    slider->setSingleStep(1);
    slider->setPageStep(20);
    slider->setGeometry(30, y, 200, 20);
    connect(slider, &QSlider::valueChanged, this,
            [this, &coeff, valueLabel](int value) {
              coeff = value * 0.1f;
              valueLabel->setText(QString::number(coeff, 'f', 1));
              valueLabel->adjustSize();
              update();
            });
    slider->setValue(def);
    return slider;
  };

  makeSlider(a_, 30, 20, "a");
  makeSlider(b_, 55, 10, "b");
  makeSlider(c_, 80, -20, "c");
  makeSlider(d_, 105, -20, "d");
  makeSlider(e_, 130, -10, "e");
}

Window::~Window() { program_.reset(); }

QVector2D Window::screenToWorld(const QPointF &screen) const {
  const float w = (float)(std::max(1, width()));
  const float h = (float)(std::max(1, height()));
  const float u = (float)(screen.x()) / w;
  const float v = (float)(screen.y()) / h;
  const float wx = xMin_ + u * (xMax_ - xMin_);
  const float wy = yMax_ - v * (yMax_ - yMin_);
  return {wx, wy};
}

void Window::onInit() {
  program_ = std::make_unique<QOpenGLShaderProgram>(this);
  program_->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                    ":/shaders/template.vs");
  program_->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                    ":/shaders/template.fs");
  if (!program_->link()) {
    qDebug() << "Ошибка линковки:" << program_->log();
  }
  program_->bind();

  mvpUniform_ = program_->uniformLocation("mvp");
  colorUniform_ = program_->uniformLocation("color");
  program_->setUniformValue(colorUniform_, QColor(255, 255, 255));

  vao_.create();
  vao_.bind();

  vbo_.create();
  vbo_.bind();
  vbo_.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  vbo_.allocate(nullptr, 0);

  program_->enableAttributeArray(0);
  program_->setAttributeBuffer(0, GL_FLOAT, 0, 2,
                               static_cast<int>(2 * sizeof(GLfloat)));

  program_->release();
  vao_.release();
  vbo_.release();

  glClearColor(0.f, 0.f, 0.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::onRender() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  projection_.setToIdentity();
  projection_.ortho(xMin_, xMax_, yMin_, yMax_, -1.f, 1.f);

  program_->bind();
  program_->setUniformValue(mvpUniform_, projection_);
  program_->setUniformValue(colorUniform_, QColor(255, 255, 255));

  vao_.bind();

  float begin = xMin_;
  float end = xMax_;

  constexpr int n = 1000;

  float x1[n];
  float y1[n];
  for (int i = 0; i < n; i++) {
    float x = (end - begin) * i / (n - 1) + begin;
    x1[i] = x;
    y1[i] = a_ * x * x * x * x + b_ * x * x * x + c_ * x * x + d_ * x + e_;
  }

  float vertices[2 * n];
  for (int i = 0; i < n; i++) {
    vertices[2 * i] = x1[i];
    vertices[2 * i + 1] = y1[i];
  }

  vbo_.bind();
  vbo_.allocate(vertices, static_cast<int>(sizeof(vertices)));
  vbo_.release();

  glDrawArrays(GL_LINE_STRIP, 0, n);

  vao_.release();
  program_->release();
}

void Window::onResize(const size_t width, const size_t height) {
  glViewport(0, 0, static_cast<GLint>(width), static_cast<GLint>(height));

  float safeHeight = (float)std::max<size_t>(1, height);
  float aspect = (float)(width) / safeHeight;

  float centerX = (xMin_ + xMax_) / 2;
  float centerY = (yMin_ + yMax_) / 2;

  float visibleHeight = yMax_ - yMin_;
  float visibleWidth = visibleHeight * aspect;

  float halfWidth = visibleWidth / 2;
  float halfHeight = visibleHeight / 2;

  xMin_ = centerX - halfWidth;
  xMax_ = centerX + halfWidth;
  yMin_ = centerY - halfHeight;
  yMax_ = centerY + halfHeight;

  projection_.setToIdentity();
  projection_.ortho(xMin_, xMax_, yMin_, yMax_, -1.f, 1.f);
}

void Window::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    dragging_ = true;
    lastMousePos_ = event->pos();
  }
  QOpenGLWidget::mousePressEvent(event);
}

void Window::mouseMoveEvent(QMouseEvent *event) {
  if (dragging_) {
    QPoint delta = event->pos() - lastMousePos_;
    lastMousePos_ = event->pos();

    float w = (float)(std::max(1, width()));
    float h = (float)std::max(1, height());
    float worldDx = delta.x() / w * (xMax_ - xMin_);
    float worldDy = delta.y() / h * (yMax_ - yMin_);
    xMin_ -= worldDx;
    xMax_ -= worldDx;
    yMin_ += worldDy;
    yMax_ += worldDy;

    update();
  }
  QOpenGLWidget::mouseMoveEvent(event);
}

void Window::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    dragging_ = false;
  }
  QOpenGLWidget::mouseReleaseEvent(event);
}

void Window::wheelEvent(QWheelEvent *event) {
  float factor = event->angleDelta().y() > 0 ? 0.9 : 1.1;

  QPointF cursor = event->position();
  QVector2D world = screenToWorld(cursor);

  float u = cursor.x() / float(width());
  float v = cursor.y() / float(height());

  float newW = (xMax_ - xMin_) * factor;
  float newH = (yMax_ - yMin_) * factor;

  xMin_ = world.x() - u * newW;
  xMax_ = world.x() + (1.f - u) * newW;
  yMax_ = world.y() + v * newH;
  yMin_ = world.y() - (1.f - v) * newH;

  update();
}
