#include "audiocapture.h"
#include <iostream>
#include <fstream>
#include <QDateTime>

AudioCapture::AudioCapture(QObject *parent)
    : QObject(parent)
    , stream(nullptr)
    , inputDeviceIndex(-1)
    , sampleRate(44100.0) {  // Varsayılan örnekleme hızı

    // PortAudio başlatılıyor
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio başlatılamadı: " << Pa_GetErrorText(err) << std::endl;
    }

    // Varsayılan giriş cihazını seçiyoruz
    inputDeviceIndex = Pa_GetDefaultInputDevice();
    if (inputDeviceIndex == paNoDevice) {
        std::cerr << "Giriş cihazı bulunamadı!" << std::endl;
    }
}

AudioCapture::~AudioCapture() {
    // PortAudio'yu sonlandırıyoruz
    if (stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
    }
    Pa_Terminate();
}

void AudioCapture::startCapture() {
    // Ses akışını açıyoruz
    PaStreamParameters inputParameters;
    inputParameters.device = inputDeviceIndex;
    inputParameters.channelCount = 1;       // Mono ses
    inputParameters.sampleFormat = paInt16; // 16-bit integer format
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputDeviceIndex)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;

    // Cihazın varsayılan örnekleme hızını alıyoruz
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(inputDeviceIndex);
    sampleRate = deviceInfo->defaultSampleRate;

    PaError err = Pa_OpenStream(&stream, &inputParameters, nullptr, sampleRate, 256, paClipOff, audioCallback, this);
    if (err != paNoError) {
        std::cerr << "Stream açılmadı: " << Pa_GetErrorText(err) << std::endl;
        return;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "Stream başlatılamadı: " << Pa_GetErrorText(err) << std::endl;
    }
}

int AudioCapture::audioCallback(const void* inputBuffer, void* outputBuffer,
                                unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags, void* userData) {
    // Ses verisi yakalandığında bu callback fonksiyonu çağrılır

    AudioCapture* audioCapture = static_cast<AudioCapture*>(userData);
    const short* input = static_cast<const short*>(inputBuffer);
    std::vector<char> data(reinterpret_cast<const char*>(input), reinterpret_cast<const char*>(input + framesPerBuffer));

    // Veriyi saklıyoruz
    audioCapture->capturedData.insert(audioCapture->capturedData.end(), data.begin(), data.end());

    return paContinue;
}

void AudioCapture::stopCapture() {
    // Ses yakalamayı durduruyoruz
    Pa_StopStream(stream);
    Pa_CloseStream(stream);

    emit saveRequested(); // Kapatırsak programı sesi yine de klasöre kaydetsin .
}

std::vector<char> AudioCapture::getCapturedData() {
    return capturedData;
}

bool AudioCapture::saveToWav(const QString& filename) {
    if (capturedData.empty()) {
        std::cerr << "Kaydedilecek ses verisi yok!" << std::endl;
        return false;
    }

    std::ofstream outFile(filename.toStdString(), std::ios::binary);
    if (!outFile) {
        std::cerr << "Dosya açılamadı: " << filename.toStdString() << std::endl;
        return false;
    }

    // WAV header yazılıyor
    // RIFF header
    outFile.write("RIFF", 4);
    uint32_t fileSize = 36 + capturedData.size();
    outFile.write(reinterpret_cast<char*>(&fileSize), 4);
    outFile.write("WAVE", 4);

    // fmt chunk
    outFile.write("fmt ", 4);
    uint32_t fmtSize = 16;
    outFile.write(reinterpret_cast<char*>(&fmtSize), 4);
    uint16_t audioFormat = 1; // PCM
    outFile.write(reinterpret_cast<char*>(&audioFormat), 2);
    uint16_t numChannels = 1; // Mono
    outFile.write(reinterpret_cast<char*>(&numChannels), 2);
    uint32_t sampleRateInt = static_cast<uint32_t>(sampleRate);
    outFile.write(reinterpret_cast<char*>(&sampleRateInt), 4);
    uint32_t byteRate = sampleRateInt * numChannels * 2; // 2 bytes per sample
    outFile.write(reinterpret_cast<char*>(&byteRate), 4);
    uint16_t blockAlign = numChannels * 2;
    outFile.write(reinterpret_cast<char*>(&blockAlign), 2);
    uint16_t bitsPerSample = 16;
    outFile.write(reinterpret_cast<char*>(&bitsPerSample), 2);

    // data chunk
    outFile.write("data", 4);
    uint32_t dataSize = capturedData.size();
    outFile.write(reinterpret_cast<char*>(&dataSize), 4);

    // Ses verisini yazıyoruz
    outFile.write(capturedData.data(), capturedData.size());

    outFile.close();
    return true;
}


