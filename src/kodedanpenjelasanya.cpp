#include <LiquidCrystal_I2C.h>   // Kendali LCD I²C 16×2
#include <NewPing.h>             // Kendali sensor ultrasonik HC‑SR04
#include <Servo.h>               // Kendali servo SG‑90
#include <math.h>                // Fungsi sqrt() untuk perhitungan debit

//LiquidCrystal_I2C: Memudahkan komunikasi I²C ke modul LCD, sehingga hanya dua kabel data yang diperlukan.
//NewPing: Menyederhanakan pembacaan jarak waktu-tempuh pulsa ultrasonik, lebih efisien daripada menulis interrupt manual.
//Servo: Mengendalikan sudut servo SG‑90 untuk membuka atau menutup pintu air.
//math.h: Menyediakan fungsi matematika dasar, di sini dipakai sqrt().


const float BATAS_PINTU1 = 10.0;   // Pintu 1 akan membuka saat ketinggian > 10 cm
const float BATAS_PINTU2 = 15.0;   // Pintu 2 akan membuka saat ketinggian > 15 cm
const float BATAS_PINTU3 = 20.0;   // Pintu 3 akan membuka saat ketinggian > 20 cm
const float LEVEL_AMAN   =  8.0;   // Semua pintu menutup saat ketinggian ≤ 8 cm

// Histeresis: mencegah buka/tutup terlalu sering
const float BATAS_BUKA  = BATAS_PINTU3;  
const float BATAS_TUTUP = LEVEL_AMAN;  

// Objek LCD dan servo
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servoPintu1, servoPintu2, servoPintu3;

// Konfigurasi sensor ultrasonik
#define PIN_TRIG   9
#define PIN_ECHO  10
#define JARAK_MAX 200
NewPing sonar(PIN_TRIG, PIN_ECHO, JARAK_MAX);

// Buzzer sebagai alarm
#define PIN_BUZZER 8

// Simulasi level air dan laju naik/turun
float levelAir   = 5.0;     // Starting level (cm)
const float MAX_LEVEL = 25.0;
const float MIN_LEVEL = 0.0;
float lajuNaik   = 0.3;     // cm per siklus saat pintu tertutup
float lajuTurun  = 0.5;     // cm per siklus saat ada pintu terbuka

// Status tiap pintu (true = terbuka)
bool pintu1Terbuka = false;
bool pintu2Terbuka = false;
bool pintu3Terbuka = false;

//BATAS_PINTU1–3 menentukan ketinggian di mana masing‑masing pintu akan membuka.

//LEVEL_AMAN (histeresis) memastikan pintu hanya menutup kembali saat benar‑benar turun jauh.

//levelAir, lajuNaik, lajuTurun: untuk kebutuhan simulasi, diimplementasikan sebagai nilai CM per siklus loop.

void setup() {
  Serial.begin(9600);
  // Cetak konfigurasi ke Serial Monitor
  Serial.println("=== SISTEM PENGENDALI PINTU AIR OTOMATIS ===");
  Serial.print("- Batas Pintu 1: "); Serial.print(BATAS_PINTU1); Serial.println(" cm");
  // … serupa untuk pintu 2, 3, dan level aman …
  Serial.println("============================================");

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();

  // Pasang servo di pin 5,6,7
  servoPintu1.attach(5);
  servoPintu2.attach(6);
  servoPintu3.attach(7);

  // Siapkan buzzer
  pinMode(PIN_BUZZER, OUTPUT);

  // Pastikan awalnya semua pintu tertutup
  tutupSemuaPintu();

  // Splash screen di LCD selama 2 detik
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Sistem Kendali");
  lcd.setCursor(0,1); lcd.print("Pintu Air Otomatis");
  delay(2000);
}

//Serial header membantu verifikasi parameter saat debugging.

//lcd.init() dan lcd.backlight() mengaktifkan modul.

//Pintu dipastikan tertutup sebelum loop utama berjalan.

//Splash screen memberi waktu sistem stabil dan memberi tahu pengguna.


void loop() {
  bacaSensorUltrasonik();     // (Simulasi) pembacaan level air
  kendalikanPintuAir();       // Logika buka/tutup pintu
  hitungPerubahanLevelAir();  // Simulasi kenaikan/turunan
  tampilkanStatus();          // Update LCD & Serial
  delay(300);                 // jeda 300 ms
}

//Sensor di–read (untuk implementasi nyata).

//Pintu dikontrol berdasar level.

//Level diperbarui dengan simulasi.

//Status termonitor di layar dan Serial.

void bacaSensorUltrasonik() {
  unsigned int uS = sonar.ping();
  float jarak = uS / US_ROUNDTRIP_CM;  // cm
  // Untuk implementasi nyata:
  // levelAir = sensorMountHeight - jarak;
}
//sonar.ping() mengembalikan waktu pulsa kembali, dibagi konstanta US_ROUNDTRIP_CM menghasilkan jarak.

//Di prototipe ini hanya simulasi—levelAir tetap diupdate lewat fungsi lain.

void kendalikanPintuAir() {
  // Buka semua pintu jika belum terbuka dan levelAir melewati BATAS_BUKA (20 cm)
  if (!pintu1Terbuka && levelAir > BATAS_BUKA) {
    bukaSemuaPintu(90);
    aktifkanAlarm();
  }
  // Tutup semua pintu jika sudah terbuka dan turun di bawah BATAS_TUTUP (8 cm)
  else if (pintu1Terbuka && levelAir <= BATAS_TUTUP) {
    tutupSemuaPintu();
  }
}
//Histeresis mencegah pintu buka/tutup terlalu sering:

//Hanya buka saat > BATAS_BUKA.

//Hanya tutup kembali saat ≤ BATAS_TUTUP.

//pintu1Terbuka dipakai sebagai flag agar alarm & perintah servo dipanggil sekali saja saat status berubah.

void hitungPerubahanLevelAir() {
  if (pintu1Terbuka || pintu2Terbuka || pintu3Terbuka) {
    levelAir -= lajuTurun;   // air turun lebih cepat kalau ada pintu terbuka
  } else {
    levelAir += lajuNaik;    // air naik kalau semua pintu tertutup (hujan)
  }
  levelAir = constrain(levelAir, MIN_LEVEL, MAX_LEVEL);
}
//Skenario “hujan lebat”: level naik selama pintu tertutup.

//Outflow: saat pintu terbuka, level air turun.

void tampilkanStatus() {
  // Baris 1 LCD: ketinggian air
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Air: "); lcd.print(levelAir,1); lcd.print(" cm");

  // Baris 2 LCD: status 3 pintu
  lcd.setCursor(0,1);
  lcd.print(pintu1Terbuka ? "BUKA " : "TUTUP ");
  lcd.print(pintu2Terbuka ? "BUKA " : "TUTUP ");
  lcd.print(pintu3Terbuka ? "BUKA"  : "TUTUP");

  // Serial: detail ketinggian, arah, status pintu, dan debit
  const char* arah = pintu1Terbuka ? "TURUN" : "NAIK";
  float debit = hitungDebitKeluar();

  Serial.print("Status: Ketinggian Air - ");
  Serial.print(levelAir,1);
  Serial.print(" cm | Arah: "); Serial.print(arah);
  Serial.print(" | Pintu 1: "); Serial.print(pintu1Terbuka?"TERBUKA(90)":"TERTUTUP(0)");
  Serial.print(" | Pintu 2: "); Serial.print(pintu2Terbuka?"TERBUKA(90)":"TERTUTUP(0)");
  Serial.print(" | Pintu 3: "); Serial.print(pintu3Terbuka?"TERBUKA(90)":"TERTUTUP(0)");
  Serial.print(" | Debit keluar: "); Serial.print(debit,2); Serial.println(" cm/s");
}
//LCD menampilkan ringkas level air dan status buka/tutup tiap pintu.

//Serial menampilkan data lebih lengkap untuk log dan debugging, berguna saat presentasi real‑time.

void bukaSemuaPintu(int sudut) {
  servoPintu1.write(sudut); pintu1Terbuka = true;
  servoPintu2.write(sudut); pintu2Terbuka = true;
  servoPintu3.write(sudut); pintu3Terbuka = true;
}

void tutupSemuaPintu() {
  servoPintu1.write(0); pintu1Terbuka = false;
  servoPintu2.write(0); pintu2Terbuka = false;
  servoPintu3.write(0); pintu3Terbuka = false;
}

void aktifkanAlarm() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_BUZZER, HIGH); delay(200);
    digitalWrite(PIN_BUZZER, LOW);  delay(100);
  }
}
//bukaSemuaPintu(90): memundurkan semua servo ke 90° → pintu terbuka penuh.

//tutupSemuaPintu(): mengembalikan ke 0° → pintu tertutup.

//Alarm: mikrofon piezo berbunyi 3 kali untuk memberi tanda saat pintu mulai bergerak.

float hitungDebitKeluar() {
  float debit = 0.0;
  if (pintu1Terbuka) debit += 90 * 0.02 * sqrt(levelAir);
  if (pintu2Terbuka) debit += 90 * 0.02 * sqrt(levelAir);
  if (pintu3Terbuka) debit += 90 * 0.02 * sqrt(levelAir);
  return debit;
}
//Debit diasumsikan sebanding dengan sudut buka servo × √(level air), skala 0.02.

//Ini hanya ilustrasi; di implementasi nyata Anda dapat mengukur debit dengan sensor aliran (flow meter).