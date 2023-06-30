#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <cstdint>
#include <QString>
#include <QValidator>
#include <random>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_mutateIncrement_clicked();
    void on_mutateDecrement_clicked();
    void on_mutateRandom_clicked();
    void on_clearGen_clicked();
    void on_addList_clicked();
    void on_remList_clicked();

private:
    Ui::MainWindow *ui;
    uint64_t min;
    uint64_t max;
    uint32_t step;
    uint32_t repeat;
    uint64_t mask;
    uint64_t cur_value;
    uint64_t gen_value;
    uint32_t incr_cycle;

    void reset_params();
    void init_vars_from_gui();
};
#endif // MAINWINDOW_H
