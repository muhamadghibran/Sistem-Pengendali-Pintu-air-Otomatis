```markdown
# 🌊 Prototipe Sistem Pengendali Pintu Air Otomatis

Selamat datang di repositori ini! Di sini, Anda akan menemukan kode dan dokumentasi lengkap untuk prototipe sistem kendali otomatis pintu air di bendungan yang menggunakan Arduino. Sistem ini dirancang untuk mensimulasikan skenario banjir, di mana air akan terus naik hingga ketiga pintu terbuka, dan kemudian mulai turun kembali. Prototipe ini bertujuan untuk menjaga ketinggian air tetap aman selama kondisi darurat.

---

## 📋 Daftar Isi

1. [Deskripsi Sistem](#deskripsi-sistem)  
2. [Komponen & Fungsinya](#komponen--fungsinya)  
3. [Skema Blok](#skema-blok)  
4. [Penjelasan Kode](#penjelasan-kode)  
   - [1. Inklusi Library & Prototipe](#1-inklusi-library--prototipe)  
   - [2. Konstanta & Variabel Global](#2-konstanta--variabel-global)  
   - [3. Fungsi `setup()`](#3-fungsi-setup)  
   - [4. Fungsi `loop()`](#4-fungsi-loop)  
   - [5. Fungsi Bantu](#5-fungsi-bantu)  
5. [Opsi Pengembangan Tambahan](#opsi-pengembangan-tambahan)  
6. [Referensi](#referensi)  

---

## 📝 Deskripsi Sistem

### **Tujuan Utama**  
- Membangun prototipe kendali otomatis untuk tiga pintu air di bendungan.  
- Menyalurkan kelebihan air saat banjir untuk menjaga ketinggian air tetap aman.

### **Cara Kerja**  
1. **Pengukuran Level**  
   - Sensor HC‑SR04 mengukur jarak ke permukaan air dan mengonversinya menjadi nilai `levelAir`.  
2. **Logika Kontrol Pintu**  
   - Jika `levelAir > 10 cm`, maka Pintu 1 akan terbuka.  
   - Jika `levelAir > 15 cm`, maka Pintu 2 akan terbuka.  
   - Jika `levelAir > 20 cm`, maka Pintu 3 akan terbuka dan sistem akan masuk ke fase **draining**.  
   - Saat `levelAir ≤ 10 cm`, semua pintu akan ditutup dan sistem kembali ke fase **rising**.  
3. **Perubahan Level**  
   - **Rising**: Level air meningkat akibat hujan.  
   - **Draining**: Level air menurun karena semua pintu terbuka.  
4. **Feedback**  
   - **LCD I²C 16×2**: Menampilkan level air dan status pintu.  
   - **Buzzer**: Mengeluarkan alarm saat pintu mulai membuka.  
   - **Serial Monitor**: Mencatat level, arah, dan debit aliran air.

---

## 🛠️ Komponen & Fungsinya

| Komponen               | Fungsi                                                         |
|------------------------|---------------------------------------------------------------|
| **Arduino Uno/Nano**   | Mikrokontroler yang menjalankan logika dan kendali I/O        |
| **HC‑SR04 Ultrasonik** | Sensor jarak yang memberikan data ketinggian air              |
| **3× Servo SG‑90**     | Aktuator yang membuka dan menutup pintu air                   |
| **LCD 16×2 (I²C)**     | Tampilan real-time untuk level air dan status pintu           |
| **Buzzer Piezo**       | Alarm audio yang berbunyi saat pintu bergerak                  |
| **Breadboard & Kabel** | Media untuk menyambungkan sinyal dan daya antar komponen      |
| **Power Supply 5 V**   | Sumber daya stabil untuk Arduino, sensor, servo, dan LCD      |

---
```


---

## 🔍 Penjelasan Kode

```cpp
#include <LiquidCrystal_I2C.h>
#include <NewPing.h>
#include <Servo.h>
#include <math.h>
```

### 1. Inklusi Library & Prototipe

* **LiquidCrystal_I2C**: Kontrol LCD via I²C
* **NewPing**: Pembacaan sensor ultrasonik HC‑SR04
* **Servo**: Kendali sudut servo SG‑90
* **math.h**: Fungsi `sqrt()` untuk perhitungan debit

```cpp
void kontrolPintu();
void hitungPerubahanLevelAir();
void tampilkanStatus();
void bukaPintu(int id, bool buka);
float hitungOutflow();
```

---

### 2. Konstanta & Variabel Global

```cpp
const float TH1 = 10.0, TH2 = 15.0, TH3 = 20.0;

float levelAir      = 5.0;
float waterRiseRate = 0.5;
bool gate1Open=false, gate2Open=false, gate3Open=false;
bool isDraining=false;

LiquidCrystal_I2C lcd(0x27,16,2);
NewPing sonar(9,10,200);
Servo gate1, gate2, gate3;
#define BUZZER 8
```

* **TH1–TH3**: Ambang level untuk membuka masing‑masing pintu
* **`isDraining`**: Menandai fase penurunan air

---

### 3. Fungsi `setup()`

```cpp
void setup() {
  Serial.begin(9600);
  lcd.init(); lcd.backlight();
  gate1.attach(5); gate2.attach(6); gate3.attach(7);
  pinMode(BUZZER, OUTPUT);
  bukaPintu(1,false); bukaPintu(2,false); bukaPintu(3,false);
}
```

* Inisialisasi Serial, LCD, servo, dan kondisi awal pintu tertutup

---

### 4. Fungsi `loop()`

```cpp
void loop() {
  kontrolPintu();
  hitungPerubahanLevelAir();
  tampilkanStatus();
  delay(300);
}
```

* Siklus: kontrol → update level → tampilkan → jeda

---

### 5. Fungsi Bantu

#### `kontrolPintu()`

```cpp
void kontrolPintu() {
  gate1Open = levelAir > TH1;
  gate2Open = levelAir > TH2;
  gate3Open = levelAir > TH3;

  if (gate1Open && gate2Open && gate3Open)
    isDraining = true;

  if (isDraining && levelAir <= TH1) {
    isDraining = false;
    gate1Open = gate2Open = gate3Open = false;
  }

  bukaPintu(1, gate1Open);
  bukaPintu(2, gate2Open);
  bukaPintu(3, gate3Open);

  if (gate1Open || gate2Open || gate3Open) {
    digitalWrite(BUZZER, HIGH);
    delay(150);
    digitalWrite(BUZZER, LOW);
  }
}
```

#### `hitungPerubahanLevelAir()`

```cpp
void hitungPerubahanLevelAir() {
  if (isDraining)
    levelAir -= hitungOutflow();
  else
    levelAir += waterRiseRate;

  levelAir = constrain(levelAir, 0, 30);
}
```

#### `hitungOutflow()`

```cpp
float hitungOutflow() {
  return (TH1 + TH2 + TH3) * 0.02 * sqrt(levelAir);
}
```

#### `tampilkanStatus()`

```cpp
void tampilkanStatus() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Air:"); lcd.print(levelAir,1); lcd.print("cm ");
  lcd.print(isDraining?"TURUN":"NAIK");
  lcd.setCursor(0,1);
  lcd.print(gate1Open?"1:O ":"1:C ");
  lcd.print(gate2Open?"2:O ":"2:C ");
  lcd.print(gate3Open?"3:O":"3:C");

  Serial.print("Lvl="); Serial.print(levelAir,1);
  Serial.print("cm | P1="); Serial.print(gate1Open?"O":"C");
  Serial.print(" P2="); Serial.print(gate2Open?"O":"C");
  Serial.print(" P3="); Serial.print(gate3Open?"O":"C");
  Serial.print(" | Debit="); Serial.print(isDraining? hitungOutflow():0,2);
  Serial.println("cm/s");
}
```

#### `bukaPintu()`

```cpp
void bukaPintu(int id, bool buka) {
  int sudut = buka ? 90 : 0;
  if (id == 1) gate1.write(sudut);
  if (id == 2) gate2.write(sudut);
  if (id == 3) gate3.write(sudut);
}
```

---

## 🚀 Opsi Pengembangan Tambahan

* **Sensor Nyata**: langsung konversi jarak HC‑SR04 → `levelAir`
* **Kontrol Proporsional**: sudut servo variabel (0–90°) sesuai level
* **PID Controller**: smoothing buka/tutup pintu sesuai debit
* **Logging & Dashboard**: simpan data ke SD card atau tampilkan via web/Bluetooth

---

## 📚 Referensi

* HC‑SR04 Ultrasonic Sensor Datasheet
* SG‑90 Servo Motor Tutorial
* [LiquidCrystal_I2C Library](https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library) (404 Client Error: Not Found)
* [NewPing Library](https://bitbucket.org/teckel12/arduino-new-ping)

```
```
