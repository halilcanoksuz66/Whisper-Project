#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <portaudio.h>
#include <vector>
#include <QObject>
#include <QString>

class AudioCapture : public QObject
{
    Q_OBJECT

public:
    AudioCapture(QObject *parent = nullptr);
    ~AudioCapture();

signals:
    void saveRequested();

public slots:
    void startCapture();
    void stopCapture();
    std::vector<char> getCapturedData();
    bool saveToWav(const QString& filename);


private:
    PaStream *stream;                  // Ses akışını yöneten PortAudio stream
    int inputDeviceIndex;              // Giriş cihazının indeks numarası
    static int audioCallback(
        const void *inputBuffer,                    // Mikrofon verisi burada gelir
        void *outputBuffer,                         // Çıkış (kullanılmıyor, çünkü sadece kayıt yapıyoruz)
        unsigned long framesPerBuffer,              // Kaç tane "frame" geldiğini söyler
        const PaStreamCallbackTimeInfo *timeInfo,   // Zaman bilgileri (önemli değil şu an)
        PaStreamCallbackFlags statusFlags,          // Sistem durumu (gecikme vs.)
        void *userData                              // Bizim sınıfımıza (AudioCapture*) erişmek için
        );

    std::vector<char> capturedData;    // Yakalanan tüm ses verisi burda
    double sampleRate;                 // Örnekleme hızı
};

#endif // AUDIOCAPTURE_H
