#include "window.h"

#include <QLabel>
#include <QSlider>



constexpr std::array<GLfloat, 21u> vertices = {
    0.0f, 0.5f, 1.f,  0.f,  0.f,   0.5f, 1.0f, -0.5f, -0.5f, 0.f,  1.f,
    0.f,  0.0f, 0.0f, 0.5f, -0.5f, 0.f,  0.f,  1.f,   1.0f,  0.0f,
};

constexpr std::array<GLuint, 3u> indices = {0, 1, 2};



Window::Window()
{
    // Add label for FPS
    auto* fpsLabel = new QLabel("FPS: 0", this);
    fpsLabel->setStyleSheet("QLabel { color: white; }");
    fpsLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    fpsLabel->adjustSize();
    fpsLabel->move(0, 0);
    fpsLabel->raise();
    connect(this, &Window::fpsUpdated, [fpsLabel](double fps) {
        fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 0));
        fpsLabel->adjustSize();
    });

    //https://doc.qt.io/qt-6/qslider.html
    auto makeSlider = [this](float& coeff, int y, int def, const QString& name) {

      auto* label = new QLabel(name, this);
      label->setStyleSheet("QLabel { color: white; }");
      label->move(10, y);

      auto* valueLabel = new QLabel(QString::number(def * 0.1f, 'f', 1), this);
      valueLabel->setStyleSheet("QLabel { color: white; }");
      valueLabel->move(240, y);

      auto* slider = new QSlider(Qt::Horizontal, this);
      slider->setRange(-50, 50);
      slider->setSingleStep(1);
      slider->setPageStep(20);
      slider->setValue(def);
      slider->setGeometry(30, y, 200, 20);
      connect(slider, &QSlider::valueChanged, this, [this, &coeff, valueLabel](int value) {
        coeff = value * 0.1;
        valueLabel->setText(QString::number(coeff, 'f', 1));
        valueLabel->adjustSize();
        update();
      });
      return slider;
    };

    makeSlider(a_, 30, 10, "a"); // :ASK: такой костылинг пикселей это норм?
    makeSlider(b_, 55, 0, "b") ;
    makeSlider(c_, 80, 0, "c");
    makeSlider(d_, 105, 0, "d");
    makeSlider(e_, 130, 0, "e");

}

Window::~Window()
{
    texture_.reset();
    program_.reset();
}

void Window::onInit()
{
    // Configure shaders
    program_ = std::make_unique<QOpenGLShaderProgram>(this);
    program_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/template.vs");
    program_->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/template.fs");
    if (!program_->link()) {
        qDebug() << "Ошибка линковки:" << program_->log();
    }
    program_->bind();

    // Configure textures
    texture_ = std::make_unique<QOpenGLTexture>(QImage(":/textures/cat.jpg"));
    texture_->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    texture_->setWrapMode(QOpenGLTexture::WrapMode::Repeat);
    program_->setUniformValue("tex_2d", 0);

    // Configure uniforms
    mvpUniform_ = program_->uniformLocation("mvp");
    colorUniform_ = program_->uniformLocation("color");
    program_->setUniformValue(colorUniform_, QColor::fromHsv(0, 0, 255));

    // Create VAO object
    vao_.create();
    vao_.bind();

    // Create VBO
    vbo_.create();
    vbo_.bind();
    vbo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo_.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(GLfloat)));

    // Create IBO
    ibo_.create();
    ibo_.bind();
    ibo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ibo_.allocate(indices.data(), static_cast<int>(indices.size() * sizeof(GLuint)));

    // Bind attributes
    program_->enableAttributeArray(0);
    program_->setAttributeBuffer(0, GL_FLOAT, 0, 2, static_cast<int>(7 * sizeof(GLfloat)));

    program_->enableAttributeArray(1);
    program_->setAttributeBuffer(1,
                                 GL_FLOAT,
                                 static_cast<int>(2 * sizeof(GLfloat)),
                                 3,
                                 static_cast<int>(7 * sizeof(GLfloat)));

    program_->enableAttributeArray(2);
    program_->setAttributeBuffer(2,
                                 GL_FLOAT,
                                 static_cast<int>(5 * sizeof(GLfloat)),
                                 2,
                                 static_cast<int>(7 * sizeof(GLfloat)));

    // Release all
    program_->release();

    vao_.release();
    vbo_.release();
    ibo_.release();

    // Start timer for animation (if needed)
    timer_.start();

    // Еnable depth test and face culling
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Opaque clear color (Wayland alpha compositing)
    glClearColor(0.f, 0.f, 0.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::onRender()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Calculate MVP matrix
    model_.setToIdentity();
    model_.translate(0, 0, -1.5);
    view_.setToIdentity();
    const auto mvp = projection_ * view_ * model_;

    // Bind shader program
    program_->bind();

    // Update uniform values
    program_->setUniformValue(mvpUniform_, mvp);

    if (animated_ && сolorAnimated_) {
        const float dt = timer_.restart() / 1000.0f;
        hue_ = std::fmod(hue_ + dt * (60.0f / 360.0f), 1.0f);
        program_->setUniformValue(colorUniform_, QColor::fromHsvF(hue_, 1.0, 1.0));
    }

    // Add textures
    glActiveTexture(GL_TEXTURE0);
    texture_->bind();

    // Bind VAO
    vao_.bind();

    // Draw
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);

    // Release all
    vao_.release();
    program_->release();
    texture_->release();

    if (animated_)
        update();
}

void Window::onResize(const size_t width, const size_t height)
{
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
