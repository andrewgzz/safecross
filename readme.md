# SafeCross

Proyecto de semáforo peatonal inteligente (IoT). El sistema usa un ESP8266 (NodeMCU) para leer sensores y controlar LEDs, y manda la información a una API en Node.js que la guarda en una base de datos MariaDB.

## Requisitos
- **Hardware:** ESP8266, sensores (PIR, HC-SR04, DHT11, LDR), LEDs.
- **Software:** Node.js, MariaDB (o MySQL), Arduino IDE.

---

## Cómo correr el proyecto

### 1. Base de datos
Primero, necesitas crear la base de datos y las tablas. Abre DBeaver (o tu gestor favorito) y ejecuta esto:

```sql
CREATE DATABASE safe_cross_db;
USE safe_cross_db;

CREATE TABLE DISPOSITIVO_HW (
    id_hw INT AUTO_INCREMENT PRIMARY KEY,
    nombre_componente VARCHAR(50),
    tipo_sensor VARCHAR(30),
    unidad_medida VARCHAR(20),
    ubicacion_pin VARCHAR(10)
);

CREATE TABLE MEDICION_TELEMETRIA (
    id_medicion BIGINT AUTO_INCREMENT PRIMARY KEY,
    id_hw INT,
    valor_medido FLOAT,
    fecha_registro TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (id_hw) REFERENCES DISPOSITIVO_HW(id_hw)
);

-- Inserta los sensores básicos para que no falle la placa
INSERT INTO DISPOSITIVO_HW (nombre_componente, tipo_sensor, unidad_medida, ubicacion_pin) VALUES
('PIR Presencia', 'Movimiento', 'Boolean', 'D7'),
('Ultrasónico HC-SR04', 'Proximidad', 'Centímetros', 'D5/D6'),
('Fotoresistencia LDR', 'Iluminación', 'ADC', 'A0'),
('Sensor DHT11', 'Clima', 'Celsius', 'D4');
```

### 2. Levantar la API (Node.js)
Abre una terminal en la carpeta de la API (backend) y ejecuta:

```bash
npm install
node server.js
```
*(Ojo: Si tu usuario o contraseña de MariaDB es distinto, actualízalo en el archivo `server.js` antes de correrlo. El servidor levantará en el puerto 3000).*

### 3. Subir el código al ESP8266
Abre el archivo `.ino` en Arduino IDE. 

Antes de subirlo a la placa, abre el archivo `secrets.h` (o créalo si no está) y pon los datos de tu red WiFi y la IP de la computadora donde estás corriendo la API:

```cpp
#define WIFI_SSID "TU_RED_WIFI"
#define WIFI_PASS "TU_CONTRASEÑA"
#define SERVER_URL "http://192.168.X.X:3000/api/mediciones" // Cambia las X por tu IP local
```

Sube el código y abre el Monitor Serial a 9600 baudios para ver cómo se conecta y empieza a mandar los datos.
