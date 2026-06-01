# Smart Thermal Management and Real-Time Monitoring System

## Informasi Proyek

**Mata Pelajaran:** Informatika
**Kelas:** 10
**Semester:** Genap (Semester 2)
**Sekolah:** SMA Negeri 3 Yogyakarta
**Tahun Ajaran:** 2025/2026
### Kelompok

| No | Nama                             | Presensi |
| -- | -------------------------------- | -------- |
| 1  | Adhyatma Aji Wistara             | 01       |
| 2  | Ahmad Isa Mujahidin Alfadani     | 02       |
| 3  | Devika Ayu Bening Ragitsaputri   | 11       |
| 4  | Firzana Muyassarah Syakira       | 13       |
| 5  | Kayla Putri Aribowo              | 16       |
| 6  | Wilbert Reyhan Susabda           | 36       |

---

## Deskripsi Proyek

**Smart Thermal Management and Real-Time Monitoring System** merupakan sistem manajemen termal berbasis Internet of Things (IoT) yang dirancang untuk memantau suhu dan kelembapan lingkungan secara waktu nyata (*real-time*) serta mengendalikan kecepatan kipas secara otomatis berdasarkan kondisi suhu yang terukur.

Sistem menggunakan mikrokontroler ESP8266 sebagai pusat kendali, sensor DHT11 untuk pengukuran suhu dan kelembapan, layar OLED sebagai media tampilan lokal, serta antarmuka web yang dapat diakses melalui jaringan Wi-Fi yang disediakan oleh perangkat.

Selain mode otomatis, sistem juga menyediakan mode manual yang memungkinkan pengguna mengatur kecepatan kipas secara langsung melalui dashboard web.

---

## Tujuan Proyek

1. Menerapkan konsep Internet of Things (IoT) dalam sistem pemantauan lingkungan.
2. Mengintegrasikan sensor, aktuator, dan antarmuka pengguna dalam satu sistem.
3. Mengendalikan kecepatan kipas secara otomatis berdasarkan suhu lingkungan.
4. Menampilkan data suhu dan kelembapan secara waktu nyata.
5. Mempelajari komunikasi data menggunakan WebSocket pada jaringan lokal.

---

## Fitur Utama

### Monitoring Lingkungan

* Pembacaan suhu menggunakan sensor DHT11.
* Pembacaan kelembapan udara menggunakan sensor DHT11.
* Pembaruan data secara waktu nyata.

### Pengendalian Kipas

* Mode otomatis berdasarkan suhu.
* Mode manual melalui dashboard web.
* Pengaturan kecepatan menggunakan PWM (*Pulse Width Modulation*).

### Dashboard Web

* Menampilkan suhu dan kelembapan secara langsung.
* Menampilkan status mode operasi.
* Mengatur kecepatan kipas pada mode manual.
* Komunikasi data menggunakan WebSocket.

### Tampilan OLED

* Menampilkan suhu saat ini.
* Menampilkan kelembapan saat ini.
* Menampilkan persentase daya kipas.
* Menampilkan grafik perubahan suhu.
* Menampilkan animasi aliran udara.

### Sistem Peringatan

* Alarm buzzer aktif ketika suhu melebihi batas tertentu.
* Pola bunyi dirancang sebagai indikator suhu tinggi.

---

## Komponen yang Digunakan

| Komponen            | Fungsi                       |
| ------------------- | ---------------------------- |
| ESP8266             | Mikrokontroler utama         |
| DHT11               | Sensor suhu dan kelembapan   |
| OLED SSD1306 128×64 | Menampilkan informasi sistem |
| Kipas DC            | Sistem pendingin             |
| MOSFET Driver       | Pengendali daya kipas        |
| Buzzer              | Alarm suhu tinggi            |
| Catu Daya           | Sumber daya sistem           |

---

## Diagram Blok Sistem

```text
+-----------+
|  DHT11    |
+-----+-----+
      |
      v
+------------------+
|     ESP8266      |
| Sistem Kendali   |
+---+----------+---+
    |          |
    |          |
    v          v
+-------+  +--------+
| OLED  |  | Web UI |
+-------+  +--------+
    |
    v
+----------+
| PWM Fan  |
+----------+
    |
    v
 Pendinginan

+----------+
| Buzzer   |
+----------+
```

---

## Cara Kerja Sistem

1. Sensor DHT11 membaca suhu dan kelembapan lingkungan.
2. Data diproses oleh ESP8266.
3. Jika mode otomatis aktif, sistem menentukan kecepatan kipas berdasarkan suhu yang terbaca.
4. Nilai PWM dikirim ke kipas sehingga kecepatan dapat berubah secara bertahap.
5. Informasi sistem ditampilkan pada layar OLED.
6. Data juga dikirim ke dashboard web menggunakan WebSocket.
7. Jika suhu melebihi batas yang ditentukan, buzzer akan aktif sebagai peringatan.

---

## Logika Pengendalian Kipas

| Suhu        | Kecepatan Kipas              |
| ----------- | ---------------------------- |
| < 28°C      | Mati                         |
| 28°C – 35°C | Menyesuaikan secara bertahap |
| > 35°C      | Maksimum                     |

---

## Hasil yang Diharapkan

* Sistem mampu membaca suhu dan kelembapan secara akurat.
* Dashboard web dapat menampilkan data secara waktu nyata.
* Kecepatan kipas berubah sesuai kondisi suhu.
* Alarm aktif saat suhu melebihi batas yang ditentukan.
* Sistem dapat dioperasikan dalam mode otomatis maupun manual.

## Kesimpulan

Proyek ini berhasil menggabungkan teknologi sensor, mikrokontroler, komunikasi jaringan, dan sistem kendali dalam sebuah sistem pemantauan serta pengendalian suhu berbasis IoT. Melalui proyek ini, anggota kelompok dapat mempelajari penerapan konsep pemrograman, elektronika, jaringan komputer, dan sistem tertanam (*embedded system*) dalam satu perangkat yang terintegrasi.
