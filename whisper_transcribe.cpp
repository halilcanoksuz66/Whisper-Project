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

WhisperTranscribe::WhisperTranscribe(QObject *parent)
    : QObject(parent)
{
    // Buraya gerekiyorsa başlatma kodları yazılır
}

WhisperTranscribe::~WhisperTranscribe() {
    // Temizlik işlemleri
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
    // DLL dosyasını yükle
    HMODULE hModule = LoadLibrary(L"C:\\Users\\halil\\Documents\\GitHub\\Whisper-Project\\lib\\whisper.dll");
    if (hModule == NULL) {
        DWORD dwError = GetLastError();
        std::cerr << "DLL yüklenemedi! Hata kodu: " << dwError << std::endl;
        return;
    }
    else {
        std::cout << "DLL başarıyla yüklendi!" << std::endl;
    }

    // Fonksiyonları tanımla
    typedef struct whisper_context* (*WhisperInitFromFileFunc)(const char* path_model);
    typedef void (*WhisperFreeFunc)(struct whisper_context* ctx);
    typedef int (*WhisperFullFunc)(struct whisper_context* ctx, struct whisper_full_params params, const float* samples, int n_samples);
    typedef int (*WhisperFullNSegmentsFunc)(struct whisper_context* ctx);
    typedef const char* (*WhisperFullGetSegmentTextFunc)(struct whisper_context* ctx, int i_segment);
    typedef struct whisper_full_params (*WhisperFullDefaultParamsFunc)(enum whisper_sampling_strategy strategy);

    // Fonksiyonları yükle
    WhisperInitFromFileFunc whisper_init_from_file = (WhisperInitFromFileFunc)GetProcAddress(hModule, "whisper_init_from_file");
    WhisperFreeFunc whisper_free = (WhisperFreeFunc)GetProcAddress(hModule, "whisper_free");
    WhisperFullFunc whisper_full = (WhisperFullFunc)GetProcAddress(hModule, "whisper_full");
    WhisperFullNSegmentsFunc whisper_full_n_segments = (WhisperFullNSegmentsFunc)GetProcAddress(hModule, "whisper_full_n_segments");
    WhisperFullGetSegmentTextFunc whisper_full_get_segment_text = (WhisperFullGetSegmentTextFunc)GetProcAddress(hModule, "whisper_full_get_segment_text");
    WhisperFullDefaultParamsFunc whisper_full_default_params = (WhisperFullDefaultParamsFunc)GetProcAddress(hModule, "whisper_full_default_params");

    if (!whisper_init_from_file || !whisper_free || !whisper_full || !whisper_full_n_segments ||
        !whisper_full_get_segment_text || !whisper_full_default_params) {
        DWORD dwError = GetLastError();
        std::cerr << "Fonksiyonlar DLL içinde bulunamadı! Hata kodu: " << dwError << std::endl;
        FreeLibrary(hModule);
        return;
    }

    // Whisper context'i oluştur
    struct whisper_context* ctx = whisper_init_from_file("C:\\Users\\halil\\Documents\\GitHub\\Whisper-Project\\models\\ggml-large-v3.bin");
    if (!ctx) {
        std::cerr << "Whisper context oluşturulamadı!" << std::endl;
        FreeLibrary(hModule);
        return;
    }



    // Transcribe parametrelerini ayarla
    struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress = true;
    params.print_realtime = true;
    params.print_timestamps = true;
    params.language = "tr"; // Türkçe
    params.translate = false; // Çeviri yapma
    params.no_speech_thold = 0.6f;

    // Transcribe işlemini başlat
    if (whisper_full(ctx, params, audio.data(), audio.size()) != 0) {
        std::cerr << "Transcribe işlemi başarısız oldu!" << std::endl;
        whisper_free(ctx);
        FreeLibrary(hModule);
        return;
    }

    // Sonuçları yazdır
    const int n_segments = whisper_full_n_segments(ctx);

    // TXT dosyasını aç
    std::ofstream outfile("C:\\Users\\halil\\Documents\\GitHub\\Whisper-Project\\output.txt");
    if (!outfile.is_open()) {
        std::cerr << "Çıktı dosyası açılamadı!" << std::endl;
        whisper_free(ctx);  // Kaynakları serbest bırak
        FreeLibrary(hModule);  // DLL'i serbest bırak
        return;
    } else {
        // Ekrana ve dosyaya yaz
        for (int i = 0; i < n_segments; ++i) {
            const char* text = whisper_full_get_segment_text(ctx, i);
            std::cout << text << std::endl;
            outfile << text << std::endl; // Dosyaya da yaz
        }
        outfile.close();
        std::cout << "Transkript output.txt dosyasına yazıldı." << std::endl;
    }

    // Temizlik
    whisper_free(ctx);
    FreeLibrary(hModule);
}
