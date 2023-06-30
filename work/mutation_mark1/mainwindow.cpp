#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace std;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    reset_params();

    ui->mMin->setValidator(new QIntValidator);
    ui->mMax->setValidator(new QIntValidator);
    ui->mStep->setValidator(new QIntValidator);
    ui->mRepeat->setValidator(new QIntValidator);
    ui->mMask->setValidator(new QIntValidator);

    // initial test parameters
    ui->mMin->setText("0");
    ui->mMax->setText("8");
    ui->mStep->setText("1");
    ui->mRepeat->setText("1");
    ui->mMask->setText("0");
}


MainWindow::~MainWindow() {
    delete ui;
}


void MainWindow::on_mutateIncrement_clicked() {
    reset_params();
    init_vars_from_gui();

    cur_value = min;

    while (true) {
        gen_value = cur_value; // & mask;
        ui->genValues->addItem(QString("%1").arg(gen_value));
        cur_value += step;
        if (cur_value > max) {
            incr_cycle++;
            cur_value = min;
            if (incr_cycle >= repeat) {
                break;
            }
        }
    }
}


void MainWindow::on_mutateDecrement_clicked() {
    reset_params();
    init_vars_from_gui();

    cur_value = max;

    while (true) {
        gen_value = cur_value; // & mask;
        ui->genValues->addItem(QString("%1").arg(gen_value));
        cur_value -= step;
        if (cur_value < min || cur_value > max) {
            incr_cycle++;
            cur_value = max;
            if (incr_cycle >= repeat) {
                break;
            }
        }
    }
}


void MainWindow::on_mutateRandom_clicked() {
    // reset_params();
    init_vars_from_gui();

    // generate random values between min and max
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distr(min, max);
    uint64_t v = distr(gen);
    ui->genValues->addItem(QString("%1").arg(v));
}


void MainWindow::reset_params() {
    ui->genValues->clear();
    min = 0;
    max = 0;
    step = 0;
    repeat = 0;
    mask = 0;
    cur_value = 0;
    gen_value = 0;
    incr_cycle = 0;

}


void MainWindow::init_vars_from_gui() {
    min = ui->mMin->text().toULongLong();
    max = ui->mMax->text().toULongLong();
    step = ui->mStep->text().toUInt();
    repeat = ui->mRepeat->text().toUInt();
    mask = ui->mMask->text().toULongLong();
}


void MainWindow::on_clearGen_clicked() {
    ui->genValues->clear();
}


void MainWindow::on_addList_clicked() {

}


void MainWindow::on_remList_clicked() {

}

