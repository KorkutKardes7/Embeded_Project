# 🌐 Comprehensive LoRa IoT & Data Ecosystem

An advanced, end-to-end Internet of Things (IoT) monorepo. This project features a distributed sensor mesh utilizing LoRa and Wi-Fi, built across various microcontrollers (ESP32, ESP8266, Arduino Nano). It is backed by a robust Python backend with vector data capabilities (ChromaDB), a modern Web Dashboard (TypeScript, Vite, Tailwind), a native Android application, and custom PCB hardware designed in KiCad.

---

## 📑 Table of Contents
1. [System Architecture](#-system-architecture)
2. [Hardware & Embedded Nodes](#-hardware--embedded-nodes)
3. [Backend Ecosystem](#-backend-ecosystem)
4. [Web Dashboard (Frontend)](#-web-dashboard-frontend)
5. [Mobile Application](#-mobile-application)
6. [Circuit & PCB Design](#-circuit--pcb-design)
7. [Getting Started & Installation](#-getting-started--installation)
8. [License](#-license)

---

## 🏗 System Architecture

The project consists of interconnected layers communicating from the physical world to user interfaces:

*   **Edge/Sensor Nodes:** Collect physical data and transmit via LoRa.
*   **Gateway Nodes:** Bridge LoRa communications to Wi-Fi/Internet (NodeMCU / ESP32).
*   **Backend Server:** Ingests data (`ingest.py`), processes and stores it in ChromaDB/SQLite.
*   **Client Interfaces:** Users interact with the data via a responsive Web App or a native Android App.

---

## 📟 Hardware & Embedded Nodes

All embedded projects are managed via **PlatformIO**.

*   📁 **`NodeMCU_LoLin_Esp-12E/` (Gateway / LoRa Hub)**
    *   Acts as a primary handler for receiving LoRa packets and pushing them to the backend.
    *   Features detailed `loraHelper.h/cpp` modules for robust RF communication.
    *   Includes a local web server interface managed by `page_handlers.h/cpp`.
*   📁 **`YD_Esp-32_S3/` & `Wemos_D1_Mini_ESP-12E/`**
    *   High-performance/Compact sensor network nodes capable of heavy duties or edge processing.
*   📁 **`Arduino_Nano/`**
    *   Low-power data collection node, bridging analog sensors over serial or RF modules.

---

## 🧠 Backend Ecosystem

Located in the 📁 `Backend/` directory, the Python-powered server handles data ingestion, storage, and API provisioning. 

**Key Components:**
*   `app.py` / `ui.py`: Core REST API application and backend user interface endpoints.
*   `ingest.py` / `update_db.py` / `init_db.py`: Scripts handling database initialization, data ingestion queues, and updates.
*   `add_humidity_db.py`: Specialized script for processing humidity/environmental sensor data.
*   `simulate_esp.py`: A testing utility that simulates traffic from ESP devices to test backend load and API integrity without needing physical hardware running.
*   **Database:** Utilizes `ChromaDB` (`chroma.sqlite3`)—a vector database tailored for AI/ML workloads, similarity searches, and advanced analytics on structured sensor data.

---

## 💻 Web Dashboard (Frontend)

Located in the 📁 `Web/` directory. A modern, lightning-fast dashboard to visualize telemetry data.

*   **Stack:** TypeScript, Vite, React/Vue (implied by Vite setup), Tailwind CSS.
*   **Configuration:** Highly strict TypeScript rules (`tsconfig.node.json`, `tsconfig.app.json`), styled with Tailwind (`tailwind.config.js`).

---

## 📱 Mobile Application

Located in the 📁 `Mobile/` directory, providing on-the-go monitoring.

*   **Stack:** Native Android developed with Android Studio.
*   **Build System:** Modern Gradle with Kotlin DSL (`build.gradle.kts`, `settings.gradle.kts`).
*   **Architecture:** Uses version catalogs (`libs.versions.toml`) for clean dependency management.

---

## 🔌 Circuit & PCB Design

Located in the 📁 `Embeded_Project-kicad/` directory.
*   Contains all KiCad schematic (`.kicad_sch`) and PCB layout (`.kicad_pcb`) files.
*   Custom-made footprints and tracing designed specifically to house the ESP chips, LoRa modules (e.g., SX1276/78), and power delivery systems.

---

## 🚀 Getting Started & Installation

### Prerequisites
Ensure your development environment has the following installed:
*   [PlatformIO IDE](https://platformio.org/) (VS Code extension)
*   [Python 3.9+](https://www.python.org/)
*   [Node.js v18+](https://nodejs.org/) & `npm` or `yarn`
*   [Android Studio](https://developer.android.com/studio)
*   [KiCad](https://www.kicad.org/) (for hardware edits)

### 1️⃣ Flashing the Firmware (PlatformIO)
1. Open the project in VS Code with the PlatformIO extension installed.
2. Navigate to your desired board folder (e.g., `cd NodeMCU_LoLin_Esp-12E`).
3. Connect your board via USB.
4. Run the upload command:
   ```bash
   pio run -t upload --environment <your_pio_env>
   ```

### 2️⃣ Running the Backend
1. Open a terminal and navigate to the backend:
   ```bash
   cd Backend
   ```
2. Create and activate a virtual environment:
   ```bash
   python -m venv venv
   source venv/bin/activate  # On Windows use: venv\Scripts\activate
   ```
3. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```
4. Initialize the database and run the server:
   ```bash
   python init_db.py
   python app.py
   ```
   *(Optional: Run `python simulate_esp.py` in another terminal to test the API with dummy data).*

### 3️⃣ Launching the Web App
1. Navigate to the web folder:
   ```bash
   cd Web
   ```
2. Install dependencies:
   ```bash
   npm install
   ```
3. Start the Vite development server:
   ```bash
   npm run dev
   ```

### 4️⃣ Compiling the Mobile App
1. Open Android Studio.
2. Select **File > Open** and choose the `Mobile/` directory.
3. Allow Gradle to sync completely.
4. Connect an Android device or use the Android Emulator, and click **Run**.

---

## 📜 License
This project is licensed under the MIT License - see the LICENSE file for details.

