# Whisper-Project — Real-Time Voice-to-Text Transcription with GPU Acceleration 🎙️🧠

Bu proje, **PortAudio** ile mikrofondan gelen sesi alır ve **Whisper.cpp** kütüphanesi ile bu sesi **anlık olarak yazıya döker**. Uygulama, Whisper modelini doğrudan çalıştırmak yerine, **Whisper.dll** dinamik kütüphanesi üzerinden kullanılarak modüler ve performanslı bir yapı kurulmuştur.

## 🔧 Özellikler

- 🎤 Gerçek zamanlı ses kaydı (PortAudio)
- 🧠 Whisper modeli ile metne dönüştürme (DLL üzerinden)
- ⚡ CUDA desteği ile 7-8 kat daha hızlı inference
- 📂 Kayıt ve çıktı yönetimi otomatik klasörleme ile
- 🪟 Qt GUI — kullanıcı dostu arayüz
---

## 🔊 Ses Kayıt Konumu
PortAudio ile alınan ses, aşağıdaki klasöre kaydedilir:

```
C:\Users\halil\Documents\GitHub\Whisper-Project\build\Desktop_Qt_6_8_2_MinGW_64_bit-Debug\recordings
```

> Sizin sisteminizde ses, QT'de **kitin yolunu ne olarak ayarladıysanız oraya** kaydedilecektir.

## 📝 Çıktı Metin
Whisper ile çıkarılan metin, proje dizininde `output.txt` dosyası olarak kaydedilir.

## 📦 Model Dosyaları
Modeller büyük olduğu için GitHub’a yüklenmedi. Onun yerine görünürde `bin` dosyaları eklendi.

Ana `bin` dosyalarına ulaşmak için aşağıdaki bağlantıdan modelleri indirebilirsiniz:  
🔗 [HuggingFace Whisper.cpp Modelleri](https://huggingface.co/ggerganov/whisper.cpp/tree/main)

## ⚙️ Whisper.dll CUDA Derlemesi
`Whisper.dll`, **CUDA**'ya göre derlendi. Böylece Whisper modeli **GPU** üzerinde çalışır.

> GPU ile çalıştırıldığında, CPU'ya göre 7–8 kat daha hızlı performans sunar.

### CUDA Gereksinimi
Bilgisayarınızın GPU’sunu model çalıştırmada kullanmak için **CUDA** indirmeniz ve sistem değişkenlerine eklemeniz gerekir.

🔽 **CUDA İndirme Linki**  
[Download CUDA 12.9.0](https://developer.download.nvidia.com/compute/cuda/12.9.0/local_installers/cuda_12.9.0_576.02_windows.exe)

📂 **Sistem Path'e Eklenecek Yol**
```
C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.9\bin
```

🖥️ **Kurulum Testi**
1. Bilgisayarı yeniden başlatın.
2. Komut satırına `nvcc --version` yazın.
   - Versiyon bilgisi çıkıyorsa kurulum başarılıdır.

---

## 🐢 Yavaş Çalışıyorsa?
Uygulamayı **debug modunda** çalıştırın ve aşağıdaki gibi bir çıktı bekleyin:

```
ggml_cuda_init : found 1 CUDA devices
```

> Eğer CUDA cihazı bulunamıyorsa, ekran kartı sürücünüzü güncelleyin ve tekrar `nvcc --version` komutunu çalıştırarak doğrulayın.

---

## 📊 Model Karşılaştırmaları (Test Sistemi: RTX 3050 Ti)

| Model Adı                        | Örnek Cümle                                              | Geçen Süre |
|----------------------------------|----------------------------------------------------------|------------|
| `ggml-tiny.bin`                 | "Bir deneme yapmakta yemeysesin böyle geliyor."          | 0.22 sn    |
| `ggml-small-q8_0.bin`           | "Bir derime yapmakta yine sesim böyle geliyor."         | 0.336 sn   |
| `ggml-small.bin`               | "Birilerime yapmakta imdesesim de öyle geliyor."        | 0.368 sn   |
| `ggml-large-v3-turbo-q8_0_2.bin`| "Bir deneme yapmaktayım ve sesim böyle geliyor."        | 0.753 sn   |
| `ggml-large-v3-turbo.bin`       | "Bir deneme yapmaktayım ve sesim böyle geliyor."        | 0.817 sn   |
| `ggml-large-v3.bin`            | "Bir deneme yapmaktayım ve sesim böyle geliyor."        | 1.014 sn   |

---

📌 **Hazırlayan:** Halil Öksüz
