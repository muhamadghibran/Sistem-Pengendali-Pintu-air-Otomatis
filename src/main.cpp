#include <LiquidCrystal_I2C.h>
#include <NewPing.h>
#include <Servo.h>
#include <math.h>  

void bacaSensorUltrasonik();
void kendalikanPintuAir();
void hitungPerubahanLevelAir();
void tampilkanStatus();
void bukaSemuaPintu(int sudut);
void tutupSemuaPintu();
void aktifkanAlarm();
float hitungDebitKeluar();

const float BATAS_PINTU1  = 10.0;
const float BATAS_PINTU2  = 15.0;
const float BATAS_PINTU3  = 20.0;  
const float LEVEL_AMAN    =  8.0;
const float BATAS_BUKA    = BATAS_PINTU3; 
const float BATAS_TUTUP   = LEVEL_AMAN;  
LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo servoPintu1;
Servo servoPintu2;
Servo servoPintu3;

#define PIN_TRIG    9
#define PIN_ECHO   10
#define JARAK_MAX 200   
NewPing sonar(PIN_TRIG, PIN_ECHO, JARAK_MAX);

#define PIN_BUZZER 8

float levelAir    = 5.0;   
const float MAX_LEVEL = 25.0;
const float MIN_LEVEL = 0.0;
float lajuNaik    = 0.3;  
float lajuTurun   = 0.5; 

bool pintu1Terbuka = false;
bool pintu2Terbuka = false;
bool pintu3Terbuka = false;

void setup() {
  Serial.begin(9600);

  Serial.println("=== SISTEM PENGENDALI PINTU AIR OTOMATIS ===");
  Serial.println("Konfigurasi Sistem:");
  Serial.print("- Batas Pintu 1       : "); Serial.print(BATAS_PINTU1); Serial.println(" cm");
  Serial.print("- Batas Pintu 2       : "); Serial.print(BATAS_PINTU2); Serial.println(" cm");
  Serial.print("- Batas Pintu 3       : "); Serial.print(BATAS_PINTU3); Serial.println(" cm");
  Serial.print("- Level Aman          : "); Serial.print(LEVEL_AMAN);   Serial.println(" cm");
  Serial.println("============================================");

  lcd.init();
  lcd.backlight();
  servoPintu1.attach(5);
  servoPintu2.attach(6);
  servoPintu3.attach(7);

  pinMode(PIN_BUZZER, OUTPUT);

  tutupSemuaPintu();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Kendali");
  lcd.setCursor(0, 1);
  lcd.print("Pintu Air Otomatis");
  delay(2000);
}

void loop() {
  bacaSensorUltrasonik();
  kendalikanPintuAir();
  hitungPerubahanLevelAir();
  tampilkanStatus();
  delay(300);
}

void bacaSensorUltrasonik() {
  unsigned int uS = sonar.ping();
  float jarak = uS / US_ROUNDTRIP_CM;
  // Jika ingin pakai jarak nyata, ubah levelAir di sini:
  // levelAir = TEGAK - jarak;
}

void kendalikanPintuAir() {
  // Buka semua pintu ketika levelAir > BATAS_BUKA
  if (!pintu1Terbuka && levelAir > BATAS_BUKA) {
    bukaSemuaPintu(90);
    aktifkanAlarm();
  }
  else if (pintu1Terbuka && levelAir <= BATAS_TUTUP) {
    tutupSemuaPintu();
  }
}

void hitungPerubahanLevelAir() {
  if (pintu1Terbuka || pintu2Terbuka || pintu3Terbuka) {
    levelAir -= lajuTurun;
  } else {
    levelAir += lajuNaik;
  }
  levelAir = constrain(levelAir, MIN_LEVEL, MAX_LEVEL);
}

void tampilkanStatus() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Air: ");
  lcd.print(levelAir, 1);
  lcd.print(" cm");

  lcd.setCursor(0, 1);
  lcd.print(pintu1Terbuka ? "BUKA " : "TUTUP ");
  lcd.print(pintu2Terbuka ? "BUKA " : "TUTUP ");
  lcd.print(pintu3Terbuka ? "BUKA"  : "TUTUP");

  const char* arah = pintu1Terbuka ? "TURUN" : "NAIK";
  float debit = hitungDebitKeluar();

  Serial.print("Status: Ketinggian Air - ");
  Serial.print(levelAir, 1);
  Serial.print(" cm | Arah: ");
  Serial.print(arah);
  Serial.print(" | Pintu 1: ");
  Serial.print(pintu1Terbuka ? "TERBUKA(90)" : "TERTUTUP(0)");
  Serial.print(" | Pintu 2: ");
  Serial.print(pintu2Terbuka ? "TERBUKA(90)" : "TERTUTUP(0)");
  Serial.print(" | Pintu 3: ");
  Serial.print(pintu3Terbuka ? "TERBUKA(90)" : "TERTUTUP(0)");
  Serial.println();

  Serial.print("Debit keluar: ");
  Serial.print(debit, 2);
  Serial.println(" cm/s");
}

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
    digitalWrite(PIN_BUZZER, HIGH);
    delay(200);
    digitalWrite(PIN_BUZZER, LOW);
    delay(100);
  }
}

float hitungDebitKeluar() {
  float debit = 0.0;
  if (pintu1Terbuka) debit += 90 * 0.02 * sqrt(levelAir);
  if (pintu2Terbuka) debit += 90 * 0.02 * sqrt(levelAir);
  if (pintu3Terbuka) debit += 90 * 0.02 * sqrt(levelAir);
  return debit;
}
