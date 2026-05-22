import sqlite3

def kur_veritabanı():
    conn = sqlite3.connect('tarim_otomasyon.db')
    cursor = conn.cursor()
    
    # 1.(Sensör Verileri) Tablosu
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS sensor_verileri (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            hava_sicakligi REAL,
            hava_nemi REAL,
            toprak_sicakligi REAL,
            toprak_nemi REAL,
            hava_basinci REAL
        )
    ''')
    
    # 2. (Vana Durumu ve Emirler) Tablosu
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS sistem_kontrol (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            vana_durumu TEXT DEFAULT 'CLOSE',
            mod TEXT DEFAULT 'AUTO',
            son_guncelleme DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    
    # Başlangıçta vana durumunu 'CLOSE' ve modu 'AUTO' olarak ekleyelim (Eğer tablo boşsa)
    cursor.execute('SELECT COUNT(*) FROM sistem_kontrol')
    if cursor.fetchone()[0] == 0:
        cursor.execute("INSERT INTO sistem_kontrol (vana_durumu, mod) VALUES ('CLOSE', 'AUTO')")
    
    conn.commit()
    conn.close()
    print("Veritabanı ve tablolar başarıyla hazırlandı!")

if __name__ == '__main__':
    kur_veritabanı()