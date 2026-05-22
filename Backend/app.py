from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from typing import Optional
import sqlite3
import os
from dotenv import load_dotenv  
import chromadb
from chromadb.utils.embedding_functions import SentenceTransformerEmbeddingFunction

load_dotenv()

# Hugging Face anonim istek uyarısını engellemek için çevre değişkeni ayarı
os.environ["HF_TOKEN"] = "ANONYMOUS"

from google import genai

app = FastAPI(title="Frost Prevention AI Powered API")

# -------------------------------------------------------------
#  RAG ve LLM KURULUMLARI 
# -------------------------------------------------------------

# ADIM 3: API Anahtarını elden yazmak yerine .env dosyasından gizlice çekiyoruz
GEMINI_API_KEY = os.getenv("GEMINI_API_KEY")

if not GEMINI_API_KEY:
    print("⚠️ UYARI: GEMINI_API_KEY bulunamadı! .env dosyanızı ve anahtarınızı kontrol edin.")

# Yeni GenAI Client Kurulumu
ai_client = genai.Client(api_key=GEMINI_API_KEY)

# 2. ChromaDB Vektör Veritabanı Bağlantısı
chromadb_path = "./chroma_db"
chromadb_client = chromadb.PersistentClient(path=chromadb_path)

# Türkçe performansı yüksek olan E5 embedding fonksiyonumuzu tanımlıyoruz
embedding_fn = SentenceTransformerEmbeddingFunction(model_name="intfloat/multilingual-e5-base")

# Koleksiyon bağlantısı
collection = chromadb_client.get_or_create_collection(
    name="tarim_ve_don_rehberi", 
    embedding_function=embedding_fn
)

print("-> RAG Vektör Veritabanı ve Gemini Yapay Zekası Başarıyla Bağlandı.")

# -------------------------------------------------------------
#  VERI ŞABLONLARI
# -------------------------------------------------------------
class TelemetryData(BaseModel):
    temperature: Optional[float] = None
    humidity: Optional[float] = None
    pressure: Optional[float] = None
    air_temperature: Optional[float] = None
    soil_temperature: Optional[float] = None
    soil_moisture: Optional[float] = None

class ValveControl(BaseModel):
    command: str
    mode: Optional[str] = None

class ChatRequest(BaseModel):
    user_message: str

# Veritabanı bağlantı fonksiyonu
def get_db_connection():
    conn = sqlite3.connect('tarim_otomasyon.db')
    conn.row_factory = sqlite3.Row
    return conn

# -------------------------------------------------------------
#  1. ENDPOINT: ESP'DEN SENSÖR VERİSİ ALMA (ARIZA ÖNLEMELİ)
# -------------------------------------------------------------
@app.post("/api/esp/telemetry")
def receive_telemetry(data: TelemetryData):
    hava_sicakligi = data.temperature if data.temperature is not None else (data.air_temperature if data.air_temperature is not None else 0.0)
    toprak_sicakligi = data.soil_temperature if data.soil_temperature is not None else 0.0
    hava_nemi = data.humidity if data.humidity is not None else 0.0
    hava_basinci = data.pressure if data.pressure is not None else 0.0
        
    conn = get_db_connection()
    cursor = conn.cursor()
    
    cursor.execute('''
        INSERT INTO sensor_verileri (hava_sicakligi, toprak_sicakligi, hava_nemi, hava_basinci)
        VALUES (?, ?, ?, ?)
    ''', (hava_sicakligi, toprak_sicakligi, hava_nemi, hava_basinci))
    
    cursor.execute('SELECT vana_durumu, mod FROM sistem_kontrol ORDER BY id DESC LIMIT 1')
    kontrol_kaydi = cursor.fetchone()
    mevcut_emir = kontrol_kaydi['vana_durumu'] if kontrol_kaydi else 'CLOSE'
    aktif_mod = kontrol_kaydi['mod'] if (kontrol_kaydi and 'mod' in kontrol_kaydi.keys()) else 'AUTO'
    
    # Calculate dew point (matching frontend approximation logic)
    dew_point = hava_sicakligi - abs(1050 - hava_basinci) / 10.0

    # Auto-valve logic based on critical temperatures for frost prevention
    if aktif_mod == 'AUTO':
        if hava_sicakligi <= dew_point + 1.5: # Critical threshold
            if mevcut_emir != 'OPEN':
                mevcut_emir = 'OPEN'
                cursor.execute('UPDATE sistem_kontrol SET vana_durumu = ?, son_guncelleme = CURRENT_TIMESTAMP WHERE id = 1', ('OPEN',))
                print(f"❄️  FROST WARNING: Temp ({hava_sicakligi}°C) is close to Dew Point ({dew_point:.1f}°C). Auto-opening valve!")
        elif hava_sicakligi > dew_point + 3.0: # Safe threshold to close the valve
            if mevcut_emir == 'OPEN':
                mevcut_emir = 'CLOSE'
                cursor.execute('UPDATE sistem_kontrol SET vana_durumu = ?, son_guncelleme = CURRENT_TIMESTAMP WHERE id = 1', ('CLOSE',))
                print(f"☀️  TEMPERATURE SAFE: Temp ({hava_sicakligi}°C) is safely above dew point. Auto-closing valve.")

    conn.commit()
    conn.close()
    
    print(f"-> ESP Verisi İşlendi | Sıcaklık: {hava_sicakligi}°C | Hava Nemi: %{hava_nemi} | Vana Emri: {mevcut_emir}")
    
    return {
        "status": "success",
        "valve_command": mevcut_emir
    }

# -------------------------------------------------------------
#  2. ENDPOINT: RAG TABANLI YAPAY ZEKA ZİRAAT DANIŞMANI CHATBOTU
# -------------------------------------------------------------
@app.post("/api/web/chat")
def ai_agronomist_chat(data: ChatRequest):
    try:
        # Adım A: Çiftçinin mesajına göre ChromaDB'den ilgili PDF sayfalarını sorgula
        results = collection.query(query_texts=[data.user_message], n_results=3)
        retrieved_chunks = results['documents'][0] if results['documents'] else []
        context_text = "\n---\n".join(retrieved_chunks)
        
        # Adım B: SQLite'tan tarlanın en son anlık sensör verilerini çek
        conn = get_db_connection()
        cursor = conn.cursor()
        cursor.execute('SELECT hava_sicakligi, toprak_sicakligi, toprak_nemi, hava_basinci, hava_nemi FROM sensor_verileri ORDER BY id DESC LIMIT 1')
        last_sensor = cursor.fetchone()
        conn.close()
        
        air_t = last_sensor['hava_sicakligi'] if last_sensor else 20.0
        soil_t = last_sensor['toprak_sicakligi'] if last_sensor else 15.0
        soil_m = last_sensor['toprak_nemi'] if last_sensor else 50.0
        air_p = last_sensor['hava_basinci'] if last_sensor else 1013.25
        air_h = last_sensor['hava_nemi'] if last_sensor else 50.0

        # Adım C: Prompt oluşturma
        prompt = f"""
        Sen, otomatik don önleme sistemine entegre uzman bir Ziraat Mühendisliği Yapay Zeka Asistanısın (AI Agronomist).
        Gorevin, ciftcinin sorusuna, saglanan kilavuz dokumanlara ve tarlanin canli sensor verilerine dayanarak kesin tavsiyeler vermektir.
        
        [TARLANIN CANLI SENSOR VERILERI]
        - Hava Sicakligi: {air_t}°C
        - Hava Nemi: %{air_h}
        - Hava Basinci: {air_p} hPa
        - Toprak Sicakligi: {soil_t}°C
        - Toprak Nemi: %{soil_m}
        
        [REHBER DOKUMANLARDAN REFERANS BILGILER]
        {context_text}
        
        [CIFTCININ SORUSU]
        {data.user_message}
        
        Kurallar: 
        1. Cevabini tamamen saglanan referans bilgilere dayandir, kafandan bilgi uydurma.
        2. Canli sensor verilerinde don riski varsa (sicaklik sifira yakinsa) veya toprak nemi cok yuksekse sulama yapmamasi konusunda ciftciyi uyar.
        3. Net, teknik ama samimi bir ziraat muhendisi dili kullan.
        """
        
        # Güncel SDK ve gemini-2.5-flash modeli ile içerik üretimi
        response = ai_client.models.generate_content(
            model='gemini-2.5-flash',
            contents=prompt,
        )
        
        return {"status": "success", "response": response.text}
        
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Chatbot hatasi: {str(e)}")

# -------------------------------------------------------------
#  3. ENDPOINT: WEB SITESINE VERI GÖNDERME (GET)
# -------------------------------------------------------------
@app.get("/api/web/data")
def get_web_data():
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute('SELECT * FROM sensor_verileri ORDER BY id DESC LIMIT 30')
    kayitlar = cursor.fetchall()
    
    cursor.execute('SELECT vana_durumu, mod FROM sistem_kontrol ORDER BY id DESC LIMIT 1')
    kontrol_kaydi = cursor.fetchone()
    mevcut_emir = kontrol_kaydi['vana_durumu'] if kontrol_kaydi else 'CLOSE'
    aktif_mod = kontrol_kaydi['mod'] if (kontrol_kaydi and 'mod' in kontrol_kaydi.keys()) else 'AUTO'
    conn.close()
    
    return {
        "valve_status": mevcut_emir,
        "mode": aktif_mod,
        "telemetry_history": [dict(row) for row in kayitlar]
    }

# -------------------------------------------------------------
#  4. ENDPOINT: WEB SITESINDEN VANA EMRİ ALMA (POST)
# -------------------------------------------------------------
@app.post("/api/web/control")
def control_valve(data: ValveControl):
    yeni_komut = data.command
    if yeni_komut not in ['OPEN', 'CLOSE', 'AUTO']:
        raise HTTPException(status_code=400, detail="Gecersiz vana komutu")
        
    conn = get_db_connection()
    cursor = conn.cursor()
    
    if yeni_komut == 'AUTO':
        cursor.execute('UPDATE sistem_kontrol SET mod = ?, son_guncelleme = CURRENT_TIMESTAMP WHERE id = 1', ('AUTO',))
        print("-> Siteden AUTO modu aktifleştirildi.")
    else:
        # If manually toggling the valve, force the mode to MANUAL
        cursor.execute('UPDATE sistem_kontrol SET vana_durumu = ?, mod = ?, son_guncelleme = CURRENT_TIMESTAMP WHERE id = 1', (yeni_komut, 'MANUAL'))
        print(f"-> Siteden yeni vana komutu alindi: {yeni_komut} (MANUAL Override)")
        
    conn.commit()
    conn.close()
    return {"status": "success", "updated_valve_status": yeni_komut}

if __name__ == '__main__':
    import uvicorn
    uvicorn.run("app:app", host="0.0.0.0", port=5000, reload=True)