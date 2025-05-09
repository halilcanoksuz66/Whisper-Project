// whisper_transcribe_worker.cpp
#include "whispertranscribeworker.h"
#include "whisper_transcribe.h"
#include <QDebug>

WhisperTranscribeWorker::WhisperTranscribeWorker(MainWindow* mainWindowRef) : mainWindow(mainWindowRef) {
    // Constructor implementation
}

WhisperTranscribeWorker::~WhisperTranscribeWorker() {
    // Destructor implementation
}

void WhisperTranscribeWorker::doTranscription(const QString& filePath) {

    // transcribeFile fonksiyonunu çağır
    transcribeFile(filePath);
}

void WhisperTranscribeWorker::transcribeFile(const QString& filePath) {
    WhisperTranscribe t(mainWindow);
    std::vector<float> samples;

    if (!t.read_wav_file(filePath.toStdString().c_str(), samples)) {
        qDebug() << "WAV dosyası okunamadı.";
        emit finished();
        return;
    }

    t.transcribe_audio(samples);
    emit finished();
}
