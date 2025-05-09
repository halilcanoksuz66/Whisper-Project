#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "whisper.h"
#include "whisper_transcribe.h"

// WAV dosyası header yapısı
struct WAVHeader {
    char riff[4];
    uint32_t chunk_size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_chunk_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

HMODULE WhisperTranscribe::hModule = nullptr;
struct whisper_context* WhisperTranscribe::ctx = nullptr;

WhisperInitFromFileFunc WhisperTranscribe::whisper_init_from_file = nullptr;
WhisperFreeFunc WhisperTranscribe::whisper_free = nullptr;
WhisperFullFunc WhisperTranscribe::whisper_full = nullptr;
WhisperFullNSegmentsFunc WhisperTranscribe::whisper_full_n_segments = nullptr;
WhisperFullGetSegmentTextFunc WhisperTranscribe::whisper_full_get_segment_text = nullptr;
WhisperFullDefaultParamsFunc WhisperTranscribe::whisper_full_default_params = nullptr;

WhisperTranscribe::WhisperTranscribe(MainWindow* mainWindowRef, QObject *parent)
    : QObject(parent),
    mainWindow(mainWindowRef)
{
    if (!load_model()) {
        std::cerr << "Model yüklenemedi!" << std::endl;
    }
}

WhisperTranscribe::~WhisperTranscribe() {
    // Belleği serbest bırak
    if (ctx) {
        whisper_free(ctx);
    }

    if (hModule) {
        FreeLibrary(hModule);
    }
}

bool WhisperTranscribe::load_model() {
    hModule = LoadLibrary(L"C:\\Users\\halil\\Documents\\GitHub\\Whisper-Project\\lib\\whisper.dll");
    if (!hModule) {
        std::cerr << "DLL yüklenemedi!" << std::endl;
        return false;
    }

    whisper_init_from_file = (WhisperInitFromFileFunc)GetProcAddress(hModule, "whisper_init_from_file");
    whisper_free = (WhisperFreeFunc)GetProcAddress(hModule, "whisper_free");

    ctx = whisper_init_from_file("C:\\Users\\halil\\Documents\\GitHub\\Whisper-Project\\models\\ggml-small-q8_0.bin");
    if (!ctx) {
        std::cerr << "Model yüklenemedi!" << std::endl;
        return false;
    }

    return true;
}

// WAV dosyasını oku ve gerekirse dönüştür
bool WhisperTranscribe::read_wav_file(const char* filename, std::vector<float>& samples) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "WAV dosyası açılamadı: " << filename << std::endl;
        return false;
    }

    // WAV header'ı oku
    WAVHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WAVHeader));

    // Header kontrolü
    if (std::string(header.riff, 4) != "RIFF" || std::string(header.wave, 4) != "WAVE") {
        std::cerr << "Geçersiz WAV dosyası!" << std::endl;
        return false;
    }

    // Data chunk'ı bul
    char chunk_id[4];
    uint32_t chunk_size;
    while (file.read(chunk_id, 4)) {
        file.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size));
        if (std::string(chunk_id, 4) == "data") {
            break;
        }
        file.seekg(chunk_size, std::ios::cur);
    }

    // Ses verilerini oku
    std::vector<int16_t> raw_samples(chunk_size / sizeof(int16_t));
    file.read(reinterpret_cast<char*>(raw_samples.data()), chunk_size);

    // 16-bit PCM'den float'a dönüştür
    samples.resize(raw_samples.size());
    for (size_t i = 0; i < raw_samples.size(); i++) {
        samples[i] = raw_samples[i] / 32768.0f;
    }

    // Stereo ise mono'ya dönüştür
    if (header.num_channels == 2) {
        std::vector<float> mono_samples(samples.size() / 2);
        for (size_t i = 0; i < mono_samples.size(); i++) {
            mono_samples[i] = (samples[i * 2] + samples[i * 2 + 1]) / 2.0f;
        }
        samples = std::move(mono_samples);
        std::cout << "Stereo ses mono'ya dönüştürüldü." << std::endl;
    }

    // Örnekleme hızını 16kHz'e dönüştür
    if (header.sample_rate != WHISPER_SAMPLE_RATE) {
        std::vector<float> resampled_samples;
        float ratio = (float)WHISPER_SAMPLE_RATE / header.sample_rate;
        resampled_samples.resize(samples.size() * ratio);

        for (size_t i = 0; i < resampled_samples.size(); i++) {
            float pos = i / ratio;
            size_t pos0 = (size_t)pos;
            size_t pos1 = pos0 + 1;
            float frac = pos - pos0;

            if (pos1 >= samples.size()) {
                resampled_samples[i] = samples[pos0];
            } else {
                resampled_samples[i] = samples[pos0] * (1.0f - frac) + samples[pos1] * frac;
            }
        }

        samples = std::move(resampled_samples);
        std::cout << "Örnekleme hızı 16kHz'e dönüştürüldü." << std::endl;
    }

    return true;
}

// Ana fonksiyon
void WhisperTranscribe::transcribe_audio(const std::vector<float> audio) {
    // Audio verisi boşsa, işlem yapma
    if (audio.empty()) {
        std::cerr << "Audio verisi boş!" << std::endl;
        return;
    }

    if (!ctx) {
        std::cerr << "Model yüklenmemiş!" << std::endl;
        return;
    }

    // DLL ve context sadece ilk seferde yüklensin
    if (hModule) {

        whisper_init_from_file = reinterpret_cast<WhisperInitFromFileFunc>(GetProcAddress(hModule, "whisper_init_from_file"));
        whisper_free = reinterpret_cast<WhisperFreeFunc>(GetProcAddress(hModule, "whisper_free"));
        whisper_full = reinterpret_cast<WhisperFullFunc>(GetProcAddress(hModule, "whisper_full"));
        whisper_full_n_segments = reinterpret_cast<WhisperFullNSegmentsFunc>(GetProcAddress(hModule, "whisper_full_n_segments"));
        whisper_full_get_segment_text = reinterpret_cast<WhisperFullGetSegmentTextFunc>(GetProcAddress(hModule, "whisper_full_get_segment_text"));
        whisper_full_default_params = reinterpret_cast<WhisperFullDefaultParamsFunc>(GetProcAddress(hModule, "whisper_full_default_params"));

        if (!whisper_init_from_file || !whisper_free || !whisper_full || !whisper_full_n_segments ||
            !whisper_full_get_segment_text || !whisper_full_default_params) {
            std::cerr << "DLL fonksiyonları yüklenemedi! Hata kodu: " << GetLastError() << std::endl;
            FreeLibrary(hModule);
            hModule = nullptr;
            return;
        }
    }

    // Parametreleri hazırla
    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress = false;
    params.print_realtime = false;
    params.print_timestamps = true;
    params.language = "tr";
    params.translate = false;
    params.no_speech_thold = 0.6f;

    // Transcribe işlemi
    if (whisper_full(ctx, params, audio.data(), audio.size()) != 0) {
        std::cerr << "Transcribe işlemi başarısız oldu!" << std::endl;
        return;
    }

    const int n_segments = whisper_full_n_segments(ctx);
    std::ofstream outfile("C:\\Users\\halil\\Documents\\GitHub\\Whisper-Project\\output.txt");
    if (!outfile.is_open()) {
        std::cerr << "Çıktı dosyası açılamadı!" << std::endl;
        return;
    }

    for (int i = 0; i < n_segments; ++i) {
        const char* text = whisper_full_get_segment_text(ctx, i);
        if (text) {
            std::cout << text << std::endl;
            mainWindow->appendMessageToDisplay(QString::fromUtf8(text));
            outfile << text << std::endl;
        }
    }

    outfile.close();
    std::cout << "Transkript dosyaya yazıldı." << std::endl;
}
