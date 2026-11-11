-- ============================================================
-- SCRIPT DE CREACIÓN DE BASE DE DATOS: SAFE CROSS (IoT)
-- Equipo 2 - Implementación de Internet de las Cosas
-- ============================================================

CREATE DATABASE IF NOT EXISTS safe_cross_db;
USE safe_cross_db;

-- 1. Tabla: CRUCE_PEATONAL
CREATE TABLE IF NOT EXISTS CRUCE_PEATONAL (
    id_cruce INT AUTO_INCREMENT PRIMARY KEY,
    ubicacion VARCHAR(150) NOT NULL,
    zona_urbana VARCHAR(100) NOT NULL,
    nivel_riesgo_vial VARCHAR(50) DEFAULT 'MEDIO',
    estado_operativo VARCHAR(20) DEFAULT 'ACTIVO',
    fecha_instalacion DATE NOT NULL
);

-- 2. Tabla: NODO_CONTROLADOR
CREATE TABLE IF NOT EXISTS NODO_CONTROLADOR (
    id_dispositivo INT AUTO_INCREMENT PRIMARY KEY,
    id_cruce INT NOT NULL,
    modelo_microcontrolador VARCHAR(50) NOT NULL DEFAULT 'ESP32 NodeMCU',
    direccion_mac VARCHAR(17) NOT NULL UNIQUE,
    direccion_ip VARCHAR(15) NOT NULL,
    estado_conexion VARCHAR(20) DEFAULT 'ONLINE',
    ultima_conexion TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (id_cruce) REFERENCES CRUCE_PEATONAL(id_cruce) ON DELETE CASCADE
);

-- 3. Tabla: DISPOSITIVO_HW (Sensores y Actuadores)
CREATE TABLE IF NOT EXISTS DISPOSITIVO_HW (
    id_hw INT AUTO_INCREMENT PRIMARY KEY,
    id_dispositivo INT NOT NULL,
    tipo_componente ENUM('SENSOR', 'ACTUADOR') NOT NULL,
    nombre_componente VARCHAR(50) NOT NULL,
    pin_gpio INT NOT NULL,
    unidad_medida VARCHAR(20),
    FOREIGN KEY (id_dispositivo) REFERENCES NODO_CONTROLADOR(id_dispositivo) ON DELETE CASCADE
);

-- 4. Tabla: MEDICION_TELEMETRIA
CREATE TABLE IF NOT EXISTS MEDICION_TELEMETRIA (
    id_medicion BIGINT AUTO_INCREMENT PRIMARY KEY,
    id_hw INT NOT NULL,
    valor_medido DECIMAL(8,2) NOT NULL,
    fecha_registro TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (id_hw) REFERENCES DISPOSITIVO_HW(id_hw) ON DELETE CASCADE
);

-- 5. Tabla: EVENTO_CRUCE
CREATE TABLE IF NOT EXISTS EVENTO_CRUCE (
    id_evento BIGINT AUTO_INCREMENT PRIMARY KEY,
    id_cruce INT NOT NULL,
    tipo_evento VARCHAR(50) NOT NULL,
    duracion_fase_seg INT NOT NULL,
    peatones_estimados INT DEFAULT 0,
    extension_tiempo_concedida TINYINT(1) DEFAULT 0,
    fecha_evento TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (id_cruce) REFERENCES CRUCE_PEATONAL(id_cruce) ON DELETE CASCADE
);

-- 6. Tabla: ALERTA_OPERATIVA
CREATE TABLE IF NOT EXISTS ALERTA_OPERATIVA (
    id_alerta INT AUTO_INCREMENT PRIMARY KEY,
    id_dispositivo INT NOT NULL,
    codigo_alerta VARCHAR(20) NOT NULL,
    descripcion TEXT NOT NULL,
    nivel_severidad ENUM('BAJA', 'MEDIA', 'ALTA', 'CRITICA') NOT NULL,
    fecha_alerta TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (id_dispositivo) REFERENCES NODO_CONTROLADOR(id_dispositivo) ON DELETE CASCADE
);


-- DATOS DE PRUEBAAAAAAA INICIALES (Para probar en la API REST )


INSERT INTO CRUCE_PEATONAL (ubicacion, zona_urbana, nivel_riesgo_vial, fecha_instalacion) 
VALUES ('Av. Eugenio Garza Sada / Alfonso Reyes', 'Zona Tec', 'ALTO', '2026-02-10');

INSERT INTO NODO_CONTROLADOR (id_cruce, direccion_mac, direccion_ip) 
VALUES (1, 'AA:BB:CC:DD:EE:01', '192.168.1.50');

INSERT INTO DISPOSITIVO_HW (id_dispositivo, tipo_componente, nombre_componente, pin_gpio, unidad_medida) VALUES 
(1, 'SENSOR', 'Sensor PIR Presencia', 4, 'BOOLEAN'),
(1, 'SENSOR', 'Sensor Ultrasónico HC-SR04', 5, 'CM'),
(1, 'SENSOR', 'Fotoresistencia LDR', 34, 'LUX'),
(1, 'SENSOR', 'Sensor Clima DHT22', 13, 'CELSIUS'),
(1, 'ACTUADOR', 'Semáforo LED Verde', 14, 'STATE');

INSERT INTO MEDICION_TELEMETRIA (id_hw, valor_medido) VALUES 
(1, 1.00), -- Peatón detectado
(2, 8.50), -- Objeto a 8.5 cm
(3, 420.00), -- Luz ambiental
(4, 26.50); -- Temperatura

USE safe_cross_db;
SELECT * FROM DISPOSITIVO_HW;
