# Guía de Configuración: Proyecto SafeCross (Entorno Windows)

Esta guía detalla el proceso completo para configurar, compilar y ejecutar el ecosistema **SafeCross** desde cero en sistemas operativos Windows. El proyecto consta de tres capas: Base de Datos (MariaDB/MySQL), Backend (Node.js) y Hardware/Edge (NodeMCU ESP8266).

## Fase 0: Prerrequisitos de Software (Windows)

Antes de comenzar, asegúrate de descargar e instalar las siguientes herramientas en tu equipo:

1. [**Git para Windows**](https://git-scm.com/download/win)**:** Para clonar el repositorio.

2. [**Node.js (LTS)**](https://nodejs.org/es/)**:** Entorno de ejecución para la API REST (incluye `npm`).

3. **Base de Datos Relacional:** Puedes instalar [**MariaDB Server**](https://mariadb.org/download/) nativo o usar [**XAMPP**](https://www.apachefriends.org/) (que incluye MariaDB/MySQL).

4. [**DBeaver Community**](https://dbeaver.io/download/)**:** Cliente visual para administrar la base de datos.

5. [**Arduino IDE**](https://www.arduino.cc/en/software)**:** Entorno de desarrollo para el ESP8266.

6. **Drivers USB a UART:** Vital para que Windows reconozca el ESP8266. Dependiendo del chip de tu placa, descarga e instala el driver **CH340** o **CP2102**.

## Fase 1: Configuración de la Base de Datos

1. **Inicia el servicio de base de datos:**

   * Si usas *MariaDB nativo*, el servicio suele iniciar automáticamente con Windows.

   * Si usas *XAMPP*, abre el panel de control y haz clic en **Start** en el módulo **MySQL**.

2. **Conecta DBeaver:**

   * Abre DBeaver y crea una nueva conexión seleccionando **MariaDB** o **MySQL**.

   * **Host:** `localhost` | **Port:** `3306`

   * **Username:** `root` | **Password:** *(deja en blanco si usas XAMPP por defecto, o ingresa la contraseña que configuraste al instalar MariaDB).*

3. **Ejecuta el Script DDL:**

   * En DBeaver, haz clic derecho sobre tu conexión $\rightarrow$ **SQL Editor** $\rightarrow$ **New SQL script**.

   * Pega y ejecuta (seleccionando todo y presionando `Alt + X`) el código en squema.sql para crear la base de datos y sus tablas.

## Fase 2: Configuración del Servidor Backend (Node.js)

1. **Obtén tu Dirección IP Local (Crucial para el hardware):**

   * Abre la terminal de Windows (`cmd` o PowerShell).

   * Escribe el comando `ipconfig` y presiona Enter.

   * Busca tu adaptador de red activo (Wi-Fi o Ethernet) y anota la **Dirección IPv4** (Ej. `192.168.1.75`).

2. **Descarga el Proyecto e Instala Dependencias:**

   * En la misma terminal, navega a la carpeta donde guardarás el proyecto y clónalo:

     ```
     git clone https://github.com/andrewgzz/safecross.git
     
     ```

3. **Configura las credenciales de BD:**

   * Abre el archivo `server.js` (o `.env` si aplica) y verifica que el `pool` de conexiones de MySQL apunte a tus credenciales locales (`root`, sin contraseña si es XAMPP, y base de datos `safe_cross_db`).

4. **Inicia la API:**

   * En la terminal, ejecuta:

     ```
     node server.js
     ```

   * Verás un mensaje indicando que el servidor escucha en el puerto `3000`. **Mantén esta terminal abierta mientras uses el sistema.**

## Fase 3: Configuración de Hardware y Arduino IDE

### 3.1 Conexiones Físicas del Circuito

Verifica el cableado hacia el ESP8266 (usa resistencias para los LEDs y un divisor de voltaje para el pin Echo del HC-SR04 si opera a 5V):

* **LDR:** Pin `A0`

* **DHT11:** Pin `D4`

* **HC-SR04:** Trig en `D5` | Echo en `D6`

* **PIR:** Pin `D7`

* **LEDs:** Rojo en `D0` | Amarillo en `D1` | Verde en `D2`

### 3.2 Preparar Arduino IDE

1. Ve a **Archivo** $\rightarrow$ **Preferencias** y en "Gestor de URLs Adicionales" pega:
   `http://arduino.esp8266.com/stable/package_esp8266com_index.json`

2. Ve a **Herramientas** $\rightarrow$ **Placa** $\rightarrow$ **Gestor de tarjetas**, busca `esp8266` e instálalo.

3. Ve a **Programa** $\rightarrow$ **Incluir Librería** $\rightarrow$ **Administrar Bibliotecas** e instala:

   * `ArduinoJson` (por Benoit Blanchon)

   * `DHT sensor library` (por Adafruit)

### 3.3 Configurar Credenciales y Subir Código

1. Abre el archivo principal del firmware (`.ino`) en Arduino IDE.

2. Abre la pestaña del archivo `secrets.h` (o créalo en la misma carpeta del `.ino`) y modifícalo con los datos de tu red y la **Dirección IPv4** que obtuviste en la Fase 2:

   ```
   #define WIFI_SSID "TU_NOMBRE_DE_WIFI"
   #define WIFI_PASS "TU_CONTRASEÑA_DE_WIFI"
   // IMPORTANTE: Usa tu IPv4 local, NO uses localhost ni 127.0.0.1
   #define SERVER_URL "http://192.168.X.X:3000/api/mediciones" 
   ```

3. En Arduino IDE, ve a **Herramientas** $\rightarrow$ **Placa** y selecciona **NodeMCU 1.0 (ESP-12E Module)**.

4. En **Herramientas** $\rightarrow$ **Puerto**, selecciona el puerto `COM` que asignó Windows (ej. `COM3` o `COM4`). *Si la opción aparece deshabilitada, instala los drivers CH340/CP2102 mencionados en la Fase 0.*

5. Haz clic en el botón **Subir** (flecha hacia la derecha).

## Fase 4: Pruebas y Monitoreo

1. Una vez finalizada la carga del código, abre el **Monitor Serie** en Arduino IDE (icono de lupa en la esquina superior derecha).

2. Ajusta la velocidad a **9600 baudios**.

3. Observarás cómo el ESP8266 se conecta al Wi-Fi e inicia la máquina de estados del semáforo.

4. Cada 5 segundos, verás el registro indicando el envío de datos a la API REST. Si la conexión es exitosa, el servidor responderá con un código `HTTP 201`.

5. Ve a **DBeaver**, abre la tabla `MEDICION_TELEMETRIA` y presiona `F5` para refrescar; las lecturas de los sensores aparecerán almacenadas en tiempo real.
