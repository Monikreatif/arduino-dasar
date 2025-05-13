#define AUTO_GAIN 1        // Penyesuaian volume otomatis (fungsi eksperimental)
#define VOL_THR 35         // Ambang batas keheningan (tidak ada tampilan pada matriks di bawahnya)
#define LOW_PASS 30        // Ambang batas sensitivitas kebisingan yang lebih rendah (tidak ada lonjakan saat tidak ada suara)
#define DEF_GAIN 80        // Ambang batas maksimum secara default (diabaikan dengan GAIN_CONTROL)
#define FHT_N 256          // Lebar spektrum x2
// Array nada yang diisi secara manual, mulainya halus, kemudian semakin curam
byte posOffset[16] = { 2 , 3 , 4 , 6 , 8 , 10 , 12 , 14 , 16 , 20 , 25 , 30 , 35 , 60 , 80 , 100 };

#define LOG_OUT 1
#include <FHT.h>          
#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 10, 5, 4, 3, 2);
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

byte v1[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11011};
byte v2[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11011, 0b11011};
byte v3[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11011, 0b11011, 0b11011};
byte v4[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b11011, 0b11011, 0b11011, 0b11011};
byte v5[8] = {0b00000, 0b00000, 0b00000, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011};
byte v6[8] = {0b00000, 0b00000, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011};
byte v7[8] = {0b00000, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011};
byte v8[8] = {0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011, 0b11011};

byte gain = DEF_GAIN;
unsigned long gainTimer;
byte maxValue, maxValue_f;
float k =  0.1;

void setup() {
  lcd.begin(16, 2); // LCD 16x2
  loadingAnimation();
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);
  analogReference(INTERNAL);
  lcd.createChar(0, v1);
  lcd.createChar(1, v2);
  lcd.createChar(2, v3);
  lcd.createChar(3, v4);
  lcd.createChar(4, v5);
  lcd.createChar(5, v6);
  lcd.createChar(6, v7);
  lcd.createChar(7, v8);
}

void loop() {
  analyzeAudio();    // Fungsi FHT, mengisi array fht_log_out[] dengan nilai berdasarkan spektrum

  for (int pos = 0; pos < 16; pos++) {    // Untuk menampilkan window 0 sampai 15
    // Cari nilai maksimum dari paket nada
    if (fht_log_out[posOffset[pos]] > maxValue) maxValue = fht_log_out[posOffset[pos]];

    lcd.setCursor(pos, 0);

    // Konversi nilai spektrum ke rentang 0..15 dengan mempertimbangkan pengaturan
    int posLevel = map(fht_log_out[posOffset[pos]], LOW_PASS, gain, 0, 15);
    posLevel = constrain(posLevel, 0, 15);

    if (posLevel > 7) {                // Jika nilai lebih besar dari 7 (maka kotak bawah akan penuh)
      lcd.write((uint8_t)posLevel - 8);     // Isi kotak atas dengan sisa yang ada
      lcd.setCursor(pos, 1);           // Pindah ke kotak bawah
      lcd.write((uint8_t)7);           // Isi kotak bawah sepenuhnya
    }
    else {                           // Jika nilai kurang dari 8
      lcd.print(" ");                  // Kotak atas kosong
      lcd.setCursor(pos, 1);           // Kotak bawah
      lcd.write((uint8_t)posLevel);    // Isi dengan garis-garis
    }
  }

  if (AUTO_GAIN) {
    maxValue_f = maxValue * k + maxValue_f * (1 - k);
    if (millis() - gainTimer > 1500) {       // Setiap 1500 ms
      // Jika nilai maksimum lebih besar dari ambang batas, ambil sebagai nilai maksimum untuk ditampilkan
      if (maxValue_f > VOL_THR) gain = maxValue_f;

      // Jika tidak, ambil ambang batas yang lebih tinggi agar kebisingan tidak lolos
      else gain = 100;
      gainTimer = millis();
    }
  }
}

void analyzeAudio() {
  for (int i = 0; i < FHT_N; i++) {
    int sample = analogRead(A0);
    fht_input[i] = sample; // Masukkan data asli ke dalam bin
  }
  fht_window();   // Menambahkan window pada data untuk respons frekuensi yang lebih baik
  fht_reorder();  // Mengurutkan data sebelum memproses FHT
  fht_run();      // Proses data dalam FHT
  fht_mag_log();  // Ambil output dari FHT
}

void loadingAnimation() {
  byte loadingChar[8] = {
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111
  };
  lcd.createChar(8, loadingChar); // Membuat karakter loading
  lcd.setCursor(0, 0);
  lcd.print("Loading...");        // Teks loading

  for (int i = 0; i < 16; i++) {
    lcd.setCursor(i, 1);          // Geser posisi karakter di baris bawah
    lcd.write((uint8_t)8);        // Menampilkan karakter loading
    delay(200);                   // Delay untuk kecepatan animasi
    lcd.setCursor(i, 1);
    lcd.print(" ");               // Hapus karakter sebelumnya untuk efek animasi
  }
  lcd.clear();                    // Bersihkan layar setelah animasi selesai
}
