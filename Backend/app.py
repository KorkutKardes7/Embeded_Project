from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import sqlite3

app = FastAPI(title="Frost Prevention API")


#Dışarıdan gelen verilerin tipini kontrol et

class TelemetryData(BaseModel):
    air_temperature: float
    soil_temperature: float
    soil_moisture: float

class ValveControl(BaseModel):
    command: str

#veritabanı bağlantı fonksiyonu

def get_db_connection():
    conn = sqlite3.connect('tarim_otomasyon.db')
    conn.row_factory = sqlite3.Row
    return conn


# ENDPOINT 1: ESP'DEN SENSÖR VERİSİ ALMA (HTTP POST)

@app.post("/api/esp/telemetry")
def receive_telemetry(data: TelemetryData):

    #Eğer ESP eksik veya hatalı veri gönderirse FastAPI otomatik olarak hata fırlatır.
    
    hava_sicakligi = data.air_temperature
    toprak_sicakligi = data.soil_temperature
    toprak_nemi = data.soil_moisture
    
    conn = get_db_connection()
    cursor = conn.cursor()
    
    #Sensör verilerini tabloya kaydet
    cursor.execute('''
        INSERT INTO sensor_verileri (hava_sicakligi, toprak_sicakligi, toprak_nemi)
        VALUES (?, ?, ?)
    ''', (hava_sicakligi, toprak_sicakligi, toprak_nemi))
    
    # En son vana emrini veri tabanından oku
    cursor.execute('SELECT vana_durumu FROM sistem_kontrol ORDER BY id DESC LIMIT 1')
    kontrol_kaydi = cursor.fetchone()
    mevcut_emir = kontrol_kaydi['vana_durumu'] if kontrol_kaydi else 'CLOSE'
    
    conn.commit()
    conn.close()
    
    print(f"-> ESP Verisi Kaydedildi. Hava: {hava_sicakligi}°C | Vana Emri: {mevcut_emir}")
    
    return {
        "status": "success",
        "valve_command": mevcut_emir
    }


#HTTP GET İLE WEB SİTESİNE VERİ GÖNDERME

@app.get("/api/web/data")
def get_web_data():
    conn = get_db_connection()
    cursor = conn.cursor()
    
    # Son 30 sensör verisini çek
    cursor.execute('SELECT * FROM sensor_verileri ORDER BY id DESC LIMIT 30')
    kayitlar = cursor.fetchall()
    
    # Güncel vana durumunu çek
    cursor.execute('SELECT vana_durumu FROM sistem_kontrol ORDER BY id DESC LIMIT 1')
    kontrol_kaydi = cursor.fetchone()
    mevcut_emir = kontrol_kaydi['vana_durumu'] if kontrol_kaydi else 'CLOSE'
    conn.close()
    
    veri_listesi = [dict(row) for row in kayitlar]
    
    return {
        "valve_status": mevcut_emir,
        "telemetry_history": veri_listesi
    }

#HTTP POST İLE WEB SİTESİNDEN VANA EMRİ ALMA

@app.post("/api/web/control")
def control_valve(data: ValveControl):
    yeni_komut = data.command
    
    if yeni_komut not in ['OPEN', 'CLOSE']:
        #HTTP 400 hatası döndürerek işlemi güvenle iptal eder
        raise HTTPException(status_code=400, detail="Gecersiz vana komutu")
        
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute('''
        UPDATE sistem_kontrol 
        SET vana_durumu = ?, son_guncelleme = CURRENT_TIMESTAMP 
        WHERE id = 1
    ''', (yeni_komut,))
    
    conn.commit()
    conn.close()
    
    print(f"-> Siteden yeni vana komutu alındı. Yeni Vana Durumu: {yeni_komut}")
    return {
        "status": "success", 
        "updated_valve_status": yeni_komut
    }

if __name__ == '__main__':
    import uvicorn
    uvicorn.run("app:app", host="0.0.0.0", port=5000, reload=True)