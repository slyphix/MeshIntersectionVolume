#ifndef PRIVATE_VISUALIZE_H
#define PRIVATE_VISUALIZE_H

#include <array>

#include <QMainWindow>
#include <QGridLayout>
#include <QFrame>
#include <QtCore>
#include <QLocale>
#include <QInputDialog>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

#include "include/visualize.h"

using render_mode = uint64_t;
constexpr render_mode render_mesh = 1;
constexpr render_mode render_structure = 2;

class ShaderWidget;

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow() = default;

    ShaderWidget *shader_widget;

public slots:
    void save_framebuffer();
};

class ShaderWidget : public QOpenGLWidget, public QOpenGLFunctions {
Q_OBJECT

    friend class MainWindow;

public:

    ShaderWidget(MainWindow*, QWidget*);
    void initializeGL() override;

    void init(int num_terms, float *terms_buffer, int num_meshes, int *len_meshes, float *mesh_buffer, float *colors);

protected:

    void paintGL() override;
    void resizeGL(int, int) override;

    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;

    MainWindow *main_window;

private slots:
    void increase_extent();
    void decrease_extent();
    void reset_extent();
    void reset_view();

    void save_svg();
    void toggle_mesh();
    void toggle_structure();

    void load_view_state();
    void save_view_state();

    void movement_tick();

private:

    QOpenGLShaderProgram line_shader, triangle_shader_wireframe;

    float *mesh_buffer, *terms_buffer;

    float *mesh_color;
    int *mesh_triangle_count;
    int terms_count, mesh_count;

    GLuint mesh_vbo_id;
    GLuint terms_vbo_id;

    float line_scale = 1.0f;

    QMatrix4x4 projection, model, view;
    QVector3D camera_position;
    float pitch = 0.0f;
    float yaw = 0.0f;

    void update_view();

    render_mode current_mode = render_mesh | render_structure;

    int click_start_position_x, click_start_position_y;
    int mouse_button;

    bool move_forward = false;
    bool move_back = false;
    bool move_right = false;
    bool move_left = false;
    bool move_up = false;
    bool move_down = false;
    bool move_slow = false;

    void draw_model();
};

#endif
