#include "window.h"

#include <QLabel>
#include <QMouseEvent>
#include <QSlider>
#include <QWheelEvent>

#include <algorithm>

constexpr float kQuad[] = {
    -1.f, -1.f, 1.f, -1.f, 1.f, 1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f,
};

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
  auto makeSlider = [this](int y, int minV, int maxV, int def, const QString &name, auto onChange) {
    auto *label = new QLabel(name, this);
    label->setStyleSheet("QLabel { color: white; }");
    label->move(10, y);

    auto *valueLabel = new QLabel(QString::number(def), this);
    valueLabel->setStyleSheet("QLabel { color: white; }");
    valueLabel->move(240, y);
    auto *slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(minV, maxV);
    slider->setGeometry(30, y, 200, 20);
    connect(slider, &QSlider::valueChanged, this,
            [valueLabel, onChange](int value) {
              onChange(value);
              valueLabel->setText(QString::number(value));
              valueLabel->adjustSize();
            });
    slider->setValue(def);
  };

  makeSlider(30, 2, 24, 6, "n", [this](int v) {
    n_ = v;
    update();
  });
  makeSlider(55, 0, 20, 4, "rot", [this](int v) {
    rotSpeed_ = v * 0.1f;
    update();
  });
  makeSlider(80, 0, 10, 7, "col", [this](int v) {
    color_ = v * 0.1f;
    update();
  });
}

Window::~Window() {
  texture_.reset();
  program_.reset();
}

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

  texture_ = std::make_unique<QOpenGLTexture>(QImage(":/textures/kolei.jpg"));
  texture_->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
  texture_->setWrapMode(QOpenGLTexture::Repeat);
  program_->setUniformValue("tex_2d", 0);

  resolutionUniform_ = program_->uniformLocation("uResolution");
  viewMinUniform_ = program_->uniformLocation("uViewMin");
  viewMaxUniform_ = program_->uniformLocation("uViewMax");
  nUniform_ = program_->uniformLocation("uN");
  timeUniform_ = program_->uniformLocation("uTime");
  rotSpeedUniform_ = program_->uniformLocation("uRotSpeed");
  colorUniform_ = program_->uniformLocation("uColor");

  vao_.create();
  vao_.bind();
  vbo_.create();
  vbo_.bind();
  vbo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
  vbo_.allocate(kQuad, sizeof(kQuad));
  program_->enableAttributeArray(0);
  program_->setAttributeBuffer(0, GL_FLOAT, 0, 2,
                               static_cast<int>(2 * sizeof(float)));

  program_->release();
  vao_.release();
  vbo_.release();

  glClearColor(0.f, 0.f, 0.f, 1.f);
  timer_.start();
}

void Window::onRender() {
  glClear(GL_COLOR_BUFFER_BIT);

  float time = timer_.elapsed() / 1000.f;

  program_->bind();
  program_->setUniformValue(resolutionUniform_, QVector2D(width(), height()));
  program_->setUniformValue(viewMinUniform_, QVector2D(xMin_, yMin_));
  program_->setUniformValue(viewMaxUniform_, QVector2D(xMax_, yMax_));
  program_->setUniformValue(nUniform_, n_);
  program_->setUniformValue(timeUniform_, time);
  program_->setUniformValue(rotSpeedUniform_, rotSpeed_);
  program_->setUniformValue(colorUniform_, color_);

  glActiveTexture(GL_TEXTURE0);
  texture_->bind();

  vao_.bind();
  glDrawArrays(GL_TRIANGLES, 0, 6);
  vao_.release();

  texture_->release();
  program_->release();

  update();
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
  if (event->button() == Qt::LeftButton)
    dragging_ = false;
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
