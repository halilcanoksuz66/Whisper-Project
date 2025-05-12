#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "whisper.h"
#include "whisper_transcribe.h"

// WAV dosyasının header yapısı
struct WAVHeader {
    char riff[4];              // "RIFF" sabiti
    uint32_t chunk_size;       // Chunk boyutu
    char wave[4];              // "WAVE" sabiti
    char fmt[4];               // "fmt " sabiti
    uint32_t fmt_chunk_size;   // Format chunk boyutu
    uint16_t audio_format;     // Ses formatı (örneğin PCM)
    uint16_t num_channels;     // Kanal sayısı (mono, stereo)
    uint32_t sample_rate;      // Örnekleme hızı (Hz)
    uint32_t byte_rate;        // Byte başına hız
    uint16_t block_align;      // Blok hizalama
    uint16_t bits_per_sample;  // Örnek başına bit sayısı
};

// Global değişkenler
HMODULE WhisperTranscribe::hModule = nullptr;  // DLL modül handle'ı
struct whisper_context* WhisperTranscribe::ctx = nullptr; // Whisper context objesi

// DLL fonksiyon işaretçileri
WhisperInitFromFileFunc WhisperTranscribe::whisper_init_from_file = nullptr;
WhisperFreeFunc WhisperTranscribe::whisper_free = nullptr;
WhisperFullFunc WhisperTranscribe::whisper_full = nullptr;
WhisperFullNSegmentsFunc WhisperTranscribe::whisper_full_n_segments = nullptr;
WhisperFullGetSegmentTextFunc WhisperTranscribe::whisper_full_get_segment_text = nullptr;
WhisperFullDefaultParamsFunc WhisperTranscribe::whisper_full_default_params = nullptr;

// Yapıcı fonksiyon: Model yükleme işlemi
WhisperTranscribe::WhisperTranscribe(MainWindow* mainWindowRef, QObject *parent)
    : QObject(parent),
    mainWindow(mainWindowRef)
{
    // Modeli yüklemeye çalış
    if (!load_model()) {
        std::cerr << "Model yüklenemedi!" << std::endl;
    }
}

// Yıkıcı fonksiyon: Bellek ve kaynakları serbest bırak
WhisperTranscribe::~WhisperTranscribe() {
    // Whisper context belleği serbest bırak
    if (ctx) {
        whisper_free(ctx);
    }

    // DLL modülünü serbest bırak
    if (hModule) {
        FreeLibrary(hModule);
    }
}

// Modeli yüklemek için fonksiyon
bool WhisperTranscribe::load_model() {
    // DLL dosyasını yükle
    hModule = LoadLibrary(L"C:\\Users\\halil\\Documents\\GitHub\\Whisper-Project\\lib\\whisper.dll");
    if (!hModule) {
        std::cerr << "DLL yüklenemedi!" << std::endl;
        return false;
    }

    // DLL fonksiyonlarını al
    whisper_init_from_file = (WhisperInitFromFileFunc)GetProcAddress(hModule, "whisper_init_from_file");
    whisper_free = (WhisperFreeFunc)GetProcAddress(hModule, "whisper_free");

    // Modeli başlat
    ctx = whisper_init_from_file("C:\\Users\\halil\\Documents\\GitHub\\Whisper-Project\\models\\ggml-large-v3.bin");
    if (!ctx) {
        std::cerr << "Model yüklenemedi!" << std::endl;
        return false;
    }

    return true;
}

// WAV dosyasını okuyup, gerekirse dönüştürme işlemi
bool WhisperTranscribe::read_wav_file(const char* filename, std::vector<float>& samples) {
    std::ifstream file(filename, std::ios::binary);  // Dosyayı ikili modda aç
    if (!file.is_open()) {
        std::cerr << "WAV dosyası açılamadı: " << filename << std::endl;
        return false;
    }

    // WAV header'ını oku
    WAVHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WAVHeader));

    // Header geçerliliğini kontrol et
    if (std::string(header.riff, 4) != "RIFF" || std::string(header.wave, 4) != "WAVE") {
        std::cerr << "Geçersiz WAV dosyası!" << std::endl;
        return false;
    }

    // "data" chunk'ını bulana kadar okuma işlemi
    char chunk_id[4];
    uint32_t chunk_size;
    while (file.read(chunk_id, 4)) {
        file.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size));
        if (std::string(chunk_id, 4) == "data") {
            break;
        }
        file.seekg(chunk_size, std::ios::cur); // Chunk'ı atla
    }

    // Ses verisini oku
    std::vector<int16_t> raw_samples(chunk_size / sizeof(int16_t));
    file.read(reinterpret_cast<char*>(raw_samples.data()), chunk_size);

    // 16-bit PCM'den float'a dönüştür
    samples.resize(raw_samples.size());
    for (size_t i = 0; i < raw_samples.size(); i++) {
        samples[i] = raw_samples[i] / 32768.0f;  // PCM verilerini float'a normalize et
    }

    // Stereo ise mono'ya dönüştür
    if (header.num_channels == 2) {
        std::vector<float> mono_samples(samples.size() / 2);
        for (size_t i = 0; i < mono_samples.size(); i++) {
            mono_samples[i] = (samples[i * 2] + samples[i * 2 + 1]) / 2.0f;
        }
        samples = std::move(mono_samples);  // Mono ses verisini kullan
        std::cout << "Stereo ses mono'ya dönüştürüldü." << std::endl;
    }

    // Örnekleme hızını 16kHz'e dönüştür
    if (header.sample_rate != WHISPER_SAMPLE_RATE) {
        std::vector<float> resampled_samples;
        float ratio = (float)WHISPER_SAMPLE_RATE / header.sample_rate;
        resampled_samples.resize(samples.size() * ratio);

        // Yeni örnekleme hızına göre veriyi yeniden örnekle
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

        samples = std::move(resampled_samples);  // Yeniden örneklenmiş ses verisini kullan
        std::cout << "Örnekleme hızı 16kHz'e dönüştürüldü." << std::endl;
    }

    return true;
}

// Ses verisini metne dönüştürme işlemi
void WhisperTranscribe::transcribe_audio(const std::vector<float> audio) {
    timer.start();  // Zamanlayıcıyı başlat

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

        // Fonksiyon işaretçileri kontrol et
        if (!whisper_init_from_file || !whisper_free || !whisper_full || !whisper_full_n_segments ||
            !whisper_full_get_segment_text || !whisper_full_default_params) {
            std::cerr << "DLL fonksiyonları yüklenemedi! Hata kodu: " << GetLastError() << std::endl;
            FreeLibrary(hModule);
            hModule = nullptr;
            return;
        }
    }

    // Transcribe parametrelerini ayarla
    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress = false;
    params.print_realtime = false;
    params.print_timestamps = true;
    params.language = "tr";  // Dil Türkçe
    params.translate = false;
    params.no_speech_thold = 0.6f;

    // Transcribe işlemi başlat
    if (whisper_full(ctx, params, audio.data(), audio.size()) != 0) {
        std::cerr << "Transcribe işlemi başarısız oldu!" << std::endl;
        return;
    }

    qint64 elapsedTime = timer.elapsed();  // Geçen zamanı al

    const int n_segments = whisper_full_n_segments(ctx);
    std::ofstream outfile("C:\\Users\\halil\\Documents\\GitHub\\Whisper-Project\\output.txt");
    if (!outfile.is_open()) {
        std::cerr << "Çıktı dosyası açılamadı!" << std::endl;
        return;
    }

    // Her bir segmentin metnini dosyaya yaz
    for (int i = 0; i < n_segments; ++i) {
        const char* text = whisper_full_get_segment_text(ctx, i);
        if (text) {
            std::cout << text << std::endl;
            mainWindow->appendMessageToDisplay(QString::fromUtf8(text));  // Arayüze yaz
            mainWindow->appendMessageToDisplay(QString("Geçen süre: %1 saniye").arg(elapsedTime / 1000.0));
            outfile << text << std::endl;  // Dosyaya yaz
        }
    }

    outfile.close();  // Dosyayı kapat
    std::cout << "Transkript dosyaya yazıldı." << std::endl;
}
