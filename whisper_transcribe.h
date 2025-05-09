#ifndef WHISPER_TRANSCRIBE_H
#define WHISPER_TRANSCRIBE_H

#include <QObject>
#include <vector>
#include <QString>
#include "whisper.h"  // Eğer whisper.h başlığına sahipseniz, burada kullanabilirsiniz.
#include "mainwindow.h"

// Whisper fonksiyon pointer typedef'lerini buraya koy
typedef struct whisper_context* (*WhisperInitFromFileFunc)(const char* path_model);
typedef void (*WhisperFreeFunc)(struct whisper_context* ctx);
typedef int (*WhisperFullFunc)(struct whisper_context* ctx, struct whisper_full_params params, const float* samples, int n_samples);
typedef int (*WhisperFullNSegmentsFunc)(struct whisper_context* ctx);
typedef const char* (*WhisperFullGetSegmentTextFunc)(struct whisper_context* ctx, int i_segment);
typedef struct whisper_full_params (*WhisperFullDefaultParamsFunc)(enum whisper_sampling_strategy strategy);

class WhisperTranscribe : public QObject {
    Q_OBJECT

public:
    WhisperTranscribe(MainWindow* mainWindowRef,QObject *parent = nullptr);
    ~WhisperTranscribe();

public:
    void transcribe_audio(const std::vector<float> audio);
    bool read_wav_file(const char* filename, std::vector<float>& samples);

private:
    MainWindow* mainWindow;
    bool load_model();

    static HMODULE hModule;
    static struct whisper_context* ctx;

    static WhisperInitFromFileFunc whisper_init_from_file;
    static WhisperFreeFunc whisper_free;
    static WhisperFullFunc whisper_full;
    static WhisperFullNSegmentsFunc whisper_full_n_segments;
    static WhisperFullGetSegmentTextFunc whisper_full_get_segment_text;
    static WhisperFullDefaultParamsFunc whisper_full_default_params;
};

#endif // WHISPER_TRANSCRIBE_H
