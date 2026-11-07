#include "window.h"

#include <QLabel>
#include <QSlider>

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

  makeSlider(a_, 30,  20, "a"); // :ASK: такой костылинг пикселей это норм?
  makeSlider(b_, 55, 10, "b");
  makeSlider(c_, 80, -20, "c");
  makeSlider(d_, 105, -20, "d");
  makeSlider(e_, 130, -10, "e");
}

Window::~Window() { program_.reset(); }

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
  // Points will be filled in onRender from a..e
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

  model_.setToIdentity();
  model_.translate(0, 0, -1.5);
  view_.setToIdentity();
  const auto mvp = projection_ * view_ * model_;

  program_->bind();
  program_->setUniformValue(mvpUniform_, mvp);
  program_->setUniformValue(colorUniform_, QColor(255, 255, 255));

  vao_.bind();

  float begin = -10;
  float end = 10;

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
  // Configure viewport
  glViewport(0, 0, static_cast<GLint>(width), static_cast<GLint>(height));

  // Configure matrix
  const auto aspect = static_cast<float>(width) / static_cast<float>(height);
  const auto zNear = 0.1f;
  const auto zFar = 100.0f;
  const auto fov = 60.0f;
  projection_.setToIdentity();
  projection_.perspective(fov, aspect, zNear, zFar);
}
