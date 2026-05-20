import gradio as gr
import requests

# FastAPI backend adresimiz
API_URL = "http://127.0.0.1:5000/api/web/chat"

def predict(message, history):
    """Gradiodan gelen kullanıcı mesajını FastAPI Chat endpointine yönlendirir."""
    payload = {"user_message": message}
    
    try:
        response = requests.post(API_URL, json=payload, timeout=15)
        
        if response.status_code == 200:
            result = response.json()
            if result.get("status") == "success":
                return result.get("response")
            else:
                return "Sistemden geçersiz bir yanıt döndü."
        else:
            return f"Backend Hatası (Kod: {response.status_code}): {response.text}"
            
    except requests.exceptions.ConnectionError:
        return "❌ FastAPI Backend sunucusuna bağlanılamadı! Lütfen önce app.py'yi çalıştırın."
    except Exception as e:
        return f"Bir hata oluştu: {str(e)}"

# Gradio Chat arabirimi tasarımı
with gr.Blocks(theme=gr.themes.Soft(primary_hue="green", secondary_hue="emerald")) as demo:
    gr.Markdown(
        """
        # 🌾 Akıllı Tarım & Don Önleme Yapay Zeka Asistanı
        ### Canlı sensör verileri ve ziraat rehber dokümanları ile desteklenen yapay zeka chatbotu.
        """
    )
    
    gr.ChatInterface(
        fn=predict,
        examples=["Don riski var mı? Vana ne zaman açılmalı?", "BME280 sensörü ne işe yarar?", "Toprak nemi kritik seviyede mi?"],
        textbox=gr.Textbox(placeholder="Ziraat mühendisine tarlan hakkında bir soru sor...", container=False, scale=7)
    )

if __name__ == "__main__":
    # Arayüzü lokalde ayağa kaldır
    demo.launch(server_name="127.0.0.1", server_port=7860)