import os
import PyPDF2
import chromadb
from langchain_text_splitters import RecursiveCharacterTextSplitter
from chromadb.utils.embedding_functions import SentenceTransformerEmbeddingFunction

# Ayarlar
DATA_FOLDER = "./data"  # PDF'lerin olduğu klasör
CHROMADB_PATH = "./chroma_db"
MODEL_NAME = "intfloat/multilingual-e5-base"
COLLECTION_NAME = "tarim_ve_don_rehberi"

def extract_text_from_pdfs(folder_path):
    """Klasördeki tüm PDF'lerin metinlerini birleştirir."""
    combined_text = ""
    pdf_files = [f for f in os.listdir(folder_path) if f.endswith('.pdf')]
    
    if not pdf_files:
        print(f"-> Hata: '{folder_path}' klasöründe hiç PDF bulunamadı!")
        return None

    for file_name in pdf_files:
        file_path = os.path.join(folder_path, file_name)
        print(f"-> {file_name} okunuyor...")
        try:
            with open(file_path, "rb") as f:
                reader = PyPDF2.PdfReader(f)
                for page in reader.pages:
                    text = page.extract_text()
                    if text:
                        combined_text += text + "\n"
        except Exception as e:
            print(f"-> {file_name} okunurken hata oluştu: {e}")
            
    return combined_text

def main():
    # 1. PDF metinlerini topla
    print("1. PDF'lerden metin çıkarma işlemi başlatıldı...")
    raw_text = extract_text_from_pdfs(DATA_FOLDER)
    if not raw_text:
        return

    # 2. Metinleri mantıklı parçalara böl (Chunking)
    print("2. Metinler küçük parçalara (chunks) ayrılıyor...")
    text_splitter = RecursiveCharacterTextSplitter(
        separators=["\n\n", "\n", ". ", "? ", "! ", " ", ""], 
        chunk_size=500, 
        chunk_overlap=100
    )
    chunks = text_splitter.split_text(raw_text)
    print(f"-> Toplam {len(chunks)} adet metin parçası oluşturuldu.")

    # 3. Yerel ChromaDB'ye Kaydet (Embedding & Storage)
    print("3. Vektör veritabanına kayıt işlemi başlatılıyor (E5 Modeli yükleniyor)...")
    chromadb_client = chromadb.PersistentClient(path=CHROMADB_PATH)
    embedding_fn = SentenceTransformerEmbeddingFunction(model_name=MODEL_NAME)
    
    collection = chromadb_client.get_or_create_collection(
        name=COLLECTION_NAME, 
        embedding_function=embedding_fn
    )

    # Benzersiz ID'ler üret ve veritabanına ekle
    chunk_ids = [f"id_{i}" for i in range(len(chunks))]
    collection.add(documents=chunks, ids=chunk_ids)
    
    print("🎉 Başarılı! Tüm PDF verileri gömüldü (embedded) ve ChromaDB'ye kaydedildi.")

if __name__ == '__main__':
    main()