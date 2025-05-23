// whisper_transcribe_worker.h
#ifndef WHISPER_TRANSCRIBE_WORKER_H
#define WHISPER_TRANSCRIBE_WORKER_H

#include <QObject>
#include "mainwindow.h"

class WhisperTranscribeWorker : public QObject {
    Q_OBJECT

public:
    WhisperTranscribeWorker(MainWindow* mainWindowRef);
    ~WhisperTranscribeWorker();

public slots:
    void doTranscription(const QString& filePath);  // Asıl işi yapacak
    void transcribeFile(const QString& filePath);

signals:
    void finished();         // İşlem bittiğinde
    void error(QString err); // Hata olursa

private:
    MainWindow* mainWindow;
};

#endif // WHISPER_TRANSCRIBE_WORKER_H
