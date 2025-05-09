#ifndef WHISPER_TRANSCRIBE_H
#define WHISPER_TRANSCRIBE_H

#include <QObject>
#include <vector>
#include <QString>
#include "whisper.h"  // Eğer whisper.h başlığına sahipseniz, burada kullanabilirsiniz.


class WhisperTranscribe : public QObject {
    Q_OBJECT

public:
    WhisperTranscribe(QObject *parent = nullptr);
    ~WhisperTranscribe();

public:
    void transcribe_audio(const std::vector<float> audio);
    bool read_wav_file(const char* filename, std::vector<float>& samples);

};

#endif // WHISPER_TRANSCRIBE_H
