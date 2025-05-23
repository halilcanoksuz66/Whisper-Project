#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "audiocapture.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void appendMessageToDisplay(const QString& message);

private slots:
    void onSaveButtonClicked();
    void onStartButtonClicked();
    void onStopButtonClicked();


private:
    Ui::MainWindow *ui;
    AudioCapture *audioCapture;
};

#endif // MAINWINDOW_H
