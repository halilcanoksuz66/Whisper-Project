#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "audiocapture.h"
#include <QDateTime>
#include <QDir>
#include <QThread>
#include "whispertranscribeworker.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , audioCapture(new AudioCapture())
{
    ui->setupUi(this);

    // Start ve Stop butonlarına bağlantı ekle
    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);

    // Save butonuna bağlantı ekle
    connect(ui->saveButton, &QPushButton::clicked, this, &MainWindow::onSaveButtonClicked);
    
    // Connect the saveRequested signal from AudioCapture
    connect(audioCapture, &AudioCapture::saveRequested, this, &MainWindow::onSaveButtonClicked);

}

MainWindow::~MainWindow()
{
    delete ui;
    delete audioCapture;
}



void MainWindow::onSaveButtonClicked() {
    // Proje klasöründe recordings alt klasörü oluştur
    QDir dir;
    if (!dir.exists("recordings")) {
        dir.mkdir("recordings");
    }

    // Dosya adını tarih ve saat ile oluştur
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString filename = QString("recordings/recording_%1.wav").arg(timestamp);

    // Ses verisini kaydet
    if (audioCapture->saveToWav(filename)) {
        ui->messageDisplay->setText("Ses kaydedildi: " + filename);

        // Ses kaydedildikten sonra, dosya yolunu transcribe_audio fonksiyonuna gönder
        QString filePath = dir.absoluteFilePath(filename);  // Dosya yolunu tam olarak al

        QThread* thread = new QThread;
        WhisperTranscribeWorker* worker = new WhisperTranscribeWorker();

        worker->moveToThread(thread);
        // Thread başladığında transkripsiyon fonksiyonunu çalıştır
        QObject::connect(thread, &QThread::started, worker, [worker, filePath]() {
            worker->doTranscription(filePath);
        });        QObject::connect(worker, &WhisperTranscribeWorker::finished, thread, &QThread::quit);
        QObject::connect(worker, &WhisperTranscribeWorker::finished, worker, &QObject::deleteLater);
        QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);

        thread->start();
    } else {
        ui->messageDisplay->setText("Ses kaydedilemedi!");

    }
}

void MainWindow::onStartButtonClicked()
{
    // Ses kaydını başlat
    audioCapture->startCapture();

    // Kullanıcıya mesaj göster
    QMessageBox::information(this, "Bilgi", "Ses kaydı başladı.");

    // Start butonunu devre dışı bırak
    ui->startButton->setEnabled(false);

    // Stop butonunu aktif et (opsiyonel)
    ui->stopButton->setEnabled(true);
}

void MainWindow::onStopButtonClicked()
{
    audioCapture->stopCapture();
    ui->startButton->setEnabled(true);  // start butonunu yeniden aktif et
    ui->stopButton->setEnabled(false);  // stop butonunu devre dışı bırak
}
