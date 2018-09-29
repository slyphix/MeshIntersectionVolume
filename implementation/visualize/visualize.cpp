#include <QApplication>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QVector3D>
#include <QSvgGenerator>
#include <QPainter>

#include <fstream>
#include <iostream>
#include <sstream>

#include "visualize.h"


const float tangent_scale = 1.0f;
const float surface_scale = 0.3f;
const float normal_scale = 0.2f;

const QVector3D tangent_color(0.8f, 0, 0);
const QVector3D surface_color(0, 0.8f, 0);
const QVector3D normal_color(0.5f, 0.5f, 0.8f);

MainWindow::MainWindow(QWidget* parent) : QMainWindow (parent) {
    resize(900, 900);
    setWindowTitle("MI Visualizer");

    QFrame* f = new QFrame(this);
    f->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    f->setLineWidth(2);

    shader_widget = new ShaderWidget(this, f);

    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(shader_widget);
    layout->setMargin(0);
    f->setLayout(layout);

    QMenu *file = new QMenu("&File",this);
    file->addAction("Load view state", shader_widget, SLOT(load_view_state()), Qt::CTRL+Qt::Key_V);
    file->addAction("Save view state", shader_widget, SLOT(save_view_state()), Qt::CTRL+Qt::SHIFT+Qt::Key_V);
    file->addAction("Save Framebuffer", this, SLOT(save_framebuffer()), Qt::CTRL+Qt::Key_S);
    file->addAction("Export scene to SVG", shader_widget, SLOT(save_svg()), Qt::CTRL+Qt::Key_X);
    file->addAction("Quit", qApp, SLOT(quit()), Qt::CTRL+Qt::SHIFT+Qt::Key_Q);
    menuBar()->addMenu(file);

    QMenu *mesh = new QMenu("&Options",this);
    mesh->addAction("Increase Line Extent", shader_widget, SLOT(increase_extent()), Qt::Key_Plus);
    mesh->addAction("Decrease Line Extent", shader_widget, SLOT(decrease_extent()), Qt::Key_Minus);
    mesh->addAction("Reset Line Extent", shader_widget, SLOT(reset_extent()), Qt::Key_R);
    mesh->addAction("Reset View", shader_widget, SLOT(reset_view()));
    mesh->addAction("Toggle Mesh", shader_widget, SLOT(toggle_mesh()), Qt::Key_M);
    mesh->addAction("Toggle Intersection Structure", shader_widget, SLOT(toggle_structure()), Qt::Key_I);
    menuBar()->addMenu(mesh);

    setCentralWidget(f);
}

ShaderWidget::ShaderWidget(MainWindow *mw, QWidget* parent) : QOpenGLWidget(parent), main_window(mw) {
    setFocusPolicy(Qt::ClickFocus);
}

constexpr float extent_factor = 1.1f;

void ShaderWidget::increase_extent() {
    line_scale *=  extent_factor;
    update();
}

void ShaderWidget::decrease_extent() {
    line_scale /=  extent_factor;
    update();
}

void ShaderWidget::reset_extent() {
    line_scale = 1.0f;
    update();
}

void ShaderWidget::reset_view() {
    camera_position = QVector3D(0, 0, -50);
    pitch = 0.0f;
    yaw = 0.0f;
    update_view();
    update();
}

void ShaderWidget::toggle_mesh() {
    current_mode ^= render_mesh;
    update();
}

void ShaderWidget::toggle_structure() {
    current_mode ^= render_structure;
    update();
}

void ShaderWidget::save_svg() {
    const QString file = QFileDialog::getSaveFileName(this, "Save SVG...", "rendering.svg", "SVG files (*.svg);;All files (*.*)");
    if (file.isEmpty()) return;

    const int w = width();
    const int h = height();
    const QMatrix4x4 mvp = projection * view * model;

    QSvgGenerator gen;
    gen.setFileName(file);
    gen.setSize(QSize(w, h));
    gen.setViewBox(QRect(0, 0, w, h));
    gen.setTitle("3D Scene Rendering");
    gen.setDescription("");

    auto svg_project = [&](const QVector3D &v) {
        const QVector3D ndc = mvp * v;
        return QPointF((ndc.x() + 1) * 0.5f * w, (1 - (ndc.y() + 1) * 0.5f) * h);
    };

    constexpr int narrow_pen = 5;
    constexpr int wide_pen = 7;
    Qt::PenCapStyle capstyle = Qt::RoundCap;

    QPainter painter;
    painter.begin(&gen);

    if ((current_mode & render_mesh) && mesh_count > 0) {
        const QColor default_color(0, 0, 0);

        if (!mesh_color)
            painter.setPen(QPen(QBrush(default_color), narrow_pen, Qt::SolidLine, capstyle));


        struct triangle_structure {
            QVector3D a, b, c;
        };

        float *current_offset = mesh_buffer;

        for (int mesh = 0; mesh < mesh_count; ++mesh) {

            if (mesh_color) {
                const QColor color = QColor::fromRgbF(mesh_color[3 * mesh], mesh_color[3 * mesh + 1], mesh_color[3 * mesh + 2]);
                // const QColor inverse = QColor::fromHslF(color.hslHueF(), color.hslSaturationF(), 1 - color.lightnessF());
                painter.setPen(QPen(QBrush(color), narrow_pen, Qt::SolidLine, capstyle));
            }

            for (int tri = 0; tri < mesh_triangle_count[mesh]; ++tri) {
                triangle_structure &term = reinterpret_cast<triangle_structure *>(current_offset)[tri];

                const QPointF a_f = svg_project(term.a);
                const QPointF b_f = svg_project(term.b);
                const QPointF c_f = svg_project(term.c);

                painter.drawLine(a_f, b_f);
                painter.drawLine(b_f, c_f);
                painter.drawLine(c_f, a_f);
            }
            current_offset += mesh_triangle_count[mesh] * 9;
        }
    }

    if ((current_mode & render_structure) && terms_count > 0) {
        painter.setPen(wide_pen);

        const QBrush tb(QColor::fromRgbF(tangent_color[0], tangent_color[1], tangent_color[2]));
        const QBrush sb(QColor::fromRgbF(surface_color[0], surface_color[1], surface_color[2]));
        const QBrush nb(QColor::fromRgbF(normal_color[0], normal_color[1], normal_color[2]));

        struct term_structure {
            QVector3D p, t, s, n;
        };

        for (int i = 0; i < terms_count; ++i) {
            term_structure &term = reinterpret_cast<term_structure *>(terms_buffer)[i];

            const QVector3D base = term.p;
            const QVector3D with_tangent = base + term.t * tangent_scale * line_scale;
            const QVector3D with_surface = with_tangent + term.s * surface_scale * line_scale;
            const QVector3D with_normal = with_surface + term.n * normal_scale * line_scale;

            const QPointF base_f = svg_project(base);
            const QPointF with_tangent_f = svg_project(with_tangent);
            const QPointF with_surface_f = svg_project(with_surface);
            const QPointF with_normal_f = svg_project(with_normal);

            painter.setPen(QPen(tb, wide_pen, Qt::SolidLine, capstyle));
            painter.drawLine(base_f, with_tangent_f);
            painter.setPen(QPen(sb, wide_pen, Qt::SolidLine, capstyle));
            painter.drawLine(with_tangent_f, with_surface_f);
            painter.setPen(QPen(nb, wide_pen, Qt::SolidLine, capstyle));
            painter.drawLine(with_surface_f, with_normal_f);
        }
    }

    painter.end();
}

void ShaderWidget::load_view_state() {
    const QString filename =
            QFileDialog::getOpenFileName(this, "Load State...", "", "MI view state (*.mivs);;All files (*.*)");
    if (filename.isEmpty()) return;

    QFile file(filename);
    file.open(QIODevice::ReadOnly);

    QDataStream in(&file);
    in >> camera_position >> pitch >> yaw >> line_scale;
    update_view();
    update();
}

void ShaderWidget::save_view_state() {
    const QString filename =
            QFileDialog::getSaveFileName(this, "Save State...", "view.mivs", "MI view state (*.mivs);;All files (*.*)");
    if (filename.isEmpty()) return;

    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    QDataStream out(&file);
    out << camera_position << pitch << yaw << line_scale;
}

void MainWindow::save_framebuffer() {
    const QImage screen = shader_widget->grabFramebuffer();

    const QString file = QFileDialog::getSaveFileName(this, "Save View...", "rendering.png", "PNG files (*.png);;All files (*.*)");

    if (file.isEmpty() || !screen.save(file)) {
        statusBar()->showMessage("ERROR: Could not save view.", 3000);
    }
}

void ShaderWidget::initializeGL() {
    initializeOpenGLFunctions();

    glGenBuffers(1, &terms_vbo_id);
    glGenBuffers(1, &mesh_vbo_id);

    setlocale(LC_NUMERIC, "C");

    Q_INIT_RESOURCE(shaders);

    if (!line_shader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/line_vertex.glsl")) QApplication::exit(1);
    if (!line_shader.addShaderFromSourceFile(QOpenGLShader::Geometry, ":/line_geometry.glsl")) QApplication::exit(1);
    if (!line_shader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/line_fragment.glsl")) QApplication::exit(1);
    if (!line_shader.link()) QApplication::exit(1);
    if (!line_shader.bind()) QApplication::exit(1);

    if (!triangle_shader_wireframe.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/wireframe_vertex.glsl")) QApplication::exit(1);
    if (!triangle_shader_wireframe.addShaderFromSourceFile(QOpenGLShader::Geometry, ":/wireframe_geometry.glsl")) QApplication::exit(1);
    if (!triangle_shader_wireframe.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/wireframe_fragment.glsl")) QApplication::exit(1);
    if (!triangle_shader_wireframe.link()) QApplication::exit(1);
    if (!triangle_shader_wireframe.bind()) QApplication::exit(1);

    setlocale(LC_ALL, "");

    const QVector3D background(250, 250, 250);
    glClearColor(background[0], background[1], background[2], 1);

    glEnable(GL_DEPTH_TEST);
    glLineWidth(2.0);

    int total_length = mesh_count == 0 ? 0 : std::accumulate(mesh_triangle_count, mesh_triangle_count + mesh_count, 0);

    if (total_length > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, mesh_vbo_id);
        glBufferData(GL_ARRAY_BUFFER, total_length * 9 * sizeof(float), mesh_buffer, GL_STATIC_DRAW);
    }

    if (terms_count > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, terms_vbo_id);
        glBufferData(GL_ARRAY_BUFFER, terms_count * 12 * sizeof(float), terms_buffer, GL_STATIC_DRAW);
    }

    reset_view();

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(movement_tick()));
    timer->setInterval(20);
    timer->start();
}

void ShaderWidget::draw_model() {

    QMatrix4x4 modelview = view * model;

    const QVector3D default_color(1, 1, 1);

    if (current_mode & render_mesh) {

        int offset = 0;
        for (int mesh = 0; mesh < mesh_count; ++mesh) {

            int vertex_count = mesh_triangle_count[mesh] * 3;
            const QVector3D color = mesh_color
                                    ? QVector3D(mesh_color[3 * mesh], mesh_color[3 * mesh + 1], mesh_color[3 * mesh + 2])
                                    : default_color;

            glBindBuffer(GL_ARRAY_BUFFER, mesh_vbo_id);

            triangle_shader_wireframe.bind();
            triangle_shader_wireframe.setUniformValue("projection", projection);
            triangle_shader_wireframe.setUniformValue("modelview", modelview);
            triangle_shader_wireframe.setUniformValue("normal_transform", modelview.normalMatrix());
            triangle_shader_wireframe.setUniformValue("color", color);

            int vertex_location = triangle_shader_wireframe.attributeLocation("a_position");
            triangle_shader_wireframe.enableAttributeArray(vertex_location);
            glVertexAttribPointer((GLuint) vertex_location, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), nullptr);

            glDrawArrays(GL_TRIANGLES, offset, vertex_count);

            triangle_shader_wireframe.disableAttributeArray(vertex_location);

            offset += vertex_count;
        }
    }

    if ((current_mode & render_structure) && terms_count > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, terms_vbo_id);

        line_shader.bind();
        line_shader.setUniformValue("projection", projection);
        line_shader.setUniformValue("modelview", modelview);
        line_shader.setUniformValue("normal_transform", modelview.normalMatrix());
        line_shader.setUniformValue("tangent_scale", tangent_scale * line_scale);
        line_shader.setUniformValue("surface_scale", surface_scale * line_scale);
        line_shader.setUniformValue("normal_scale", normal_scale * line_scale);
        line_shader.setUniformValue("tangent_color", tangent_color);
        line_shader.setUniformValue("surface_color", surface_color);
        line_shader.setUniformValue("normal_color", normal_color);

        int vertex_location = line_shader.attributeLocation("a_position");
        line_shader.enableAttributeArray(vertex_location);
        glVertexAttribPointer((GLuint) vertex_location, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(QVector3D), nullptr);

        int vertex_tangent = line_shader.attributeLocation("a_tangent");
        line_shader.enableAttributeArray(vertex_tangent);
        glVertexAttribPointer((GLuint) vertex_tangent, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(QVector3D),
                              (void *) sizeof(QVector3D));

        int vertex_surface = line_shader.attributeLocation("a_surface");
        line_shader.enableAttributeArray(vertex_surface);
        glVertexAttribPointer((GLuint) vertex_surface, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(QVector3D),
                              (void *) (2 * sizeof(QVector3D)));

        int vertex_normal = line_shader.attributeLocation("a_normal");
        line_shader.enableAttributeArray(vertex_normal);
        glVertexAttribPointer((GLuint) vertex_normal, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(QVector3D),
                              (void *) (3 * sizeof(QVector3D)));

        glDrawArrays(GL_POINTS, 0, terms_count);

        line_shader.disableAttributeArray(vertex_location);
        line_shader.disableAttributeArray(vertex_tangent);
        line_shader.disableAttributeArray(vertex_surface);
        line_shader.disableAttributeArray(vertex_normal);
    }
}

void ShaderWidget::paintGL() {
    const float mynear = 1;
    const float myfar = 200;
    const float aspectratio = width() / (float) height();
    const float display_height = 30;
    const float display_width = aspectratio*display_height;

    constexpr float cam_x = 0;
    constexpr float cam_y = 0;
    constexpr float cam_z = 50;
    const float left   = mynear * (-display_width / 2 - cam_x) / cam_z;
    const float right  = mynear * ( display_width / 2 - cam_x) / cam_z;
    const float bottom = mynear * (-display_height / 2 - cam_y) / cam_z;
    const float top    = mynear * ( display_height / 2 - cam_y) / cam_z;

    projection.setToIdentity();
    projection.frustum(left, right, bottom, top, mynear, myfar);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_model();
}

void ShaderWidget::resizeGL(int width, int height) {
    glViewport(0, 0, width, height);
    update();
}

void ShaderWidget::mousePressEvent(QMouseEvent *event) {
    click_start_position_x = event->x();
    click_start_position_y = event->y();

    mouse_button = event->button();

    update();
}

void ShaderWidget::mouseReleaseEvent(QMouseEvent*) {}

void ShaderWidget::mouseMoveEvent(QMouseEvent* event) {

    if (mouse_button == Qt::LeftButton) {
        yaw += (event->x() - click_start_position_x) * 0.1f;
        pitch += (event->y() - click_start_position_y) * 0.1f;

        update_view();
    }

    click_start_position_x = event->x();
    click_start_position_y = event->y();

    update();
}

void ShaderWidget::update_view() {
    view.setToIdentity();
    view.rotate(pitch, QVector3D(1, 0, 0));
    view.rotate(yaw, QVector3D(0, 1, 0));
    view.translate(camera_position);
}

void ShaderWidget::keyPressEvent(QKeyEvent* event) {

    switch (event->key()) {
        case Qt::Key_W:
            move_forward = true;
            break;
        case Qt::Key_S:
            move_back = true;
            break;
        case Qt::Key_D:
            move_right = true;
            break;
        case Qt::Key_A:
            move_left = true;
            break;
        case Qt::Key_Q:
            move_up = true;
            break;
        case Qt::Key_E:
            move_down = true;
            break;
        case Qt::Key_Shift:
            move_slow = true;
        default:
            QWidget::keyPressEvent(event);
    }
}

void ShaderWidget::keyReleaseEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_W:
            move_forward = false;
            break;
        case Qt::Key_S:
            move_back = false;
            break;
        case Qt::Key_D:
            move_right = false;
            break;
        case Qt::Key_A:
            move_left = false;
            break;
        case Qt::Key_Q:
            move_up = false;
            break;
        case Qt::Key_E:
            move_down = false;
            break;
        case Qt::Key_Shift:
            move_slow = false;
        default:
            QWidget::keyReleaseEvent(event);
    }
}

void ShaderWidget::movement_tick() {
    constexpr float step_size = 0.3f;
    constexpr float forward_factor = 2.0f;

    if (move_forward || move_back || move_left || move_right || move_up || move_down) {
        QVector3D up = view.row(1).toVector3D();
        QVector3D strafe = view.row(0).toVector3D();
        QVector3D forward = view.row(2).toVector3D();

        float slow_factor = move_slow ? 0.1f : 1.0f;

        camera_position -= step_size * slow_factor * (
                  up * (move_up - move_down)
                + strafe * (move_right - move_left)
                - forward * (move_forward - move_back) * forward_factor);

        update_view();
        update();
    }
}

void ShaderWidget::init(int num_terms, float *terms_buffer, int num_meshes, int *mesh_triangle_count, float *mesh_buffer, float *colors) {
    this->terms_count = num_terms;
    this->terms_buffer = terms_buffer;
    this->mesh_count = num_meshes;
    this->mesh_triangle_count = mesh_triangle_count;
    this->mesh_buffer = mesh_buffer;
    this->mesh_color = colors;
}

namespace visualize {

void init(int &argc, char **argv) {
    static QApplication app(argc, argv);
}

int launch(int num_terms, float *terms_buffer, int num_meshes, int *mesh_triangle_count, float *mesh_buffer, float *colors) {
    MainWindow w;
    // inject values (there has to be a cleaner way)
    w.shader_widget->init(num_terms, terms_buffer, num_meshes, mesh_triangle_count, mesh_buffer, colors);
    w.show();

    return QApplication::exec();
}

}