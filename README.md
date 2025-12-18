📺 SmallTV Pro - WiFi Smart Clock & Weather Station

Este proyecto transforma el dispositivo GeekMagic SmallTV Pro en un reloj inteligente totalmente personalizado utilizando un ESP8266. Cuenta con sincronización horaria NTP, carrusel de clima dinámico (OpenWeatherMap), portal cautivo para configuración y persistencia de datos mediante LittleFS.
🚀 Características

    Gestión WiFi: Portal cautivo automático si no hay red guardada.

    Memoria Flash: Almacenamiento de credenciales, API Keys y colores en config.json usando LittleFS.

    Reloj Dual-Color: Horas, minutos y segundos personalizables con colores independientes.

    Clima Dinámico: Carrusel automático (cada 5s) de Temperatura, Humedad y Viento.

    Interfaz Web: Servidor HTTP siempre activo en la IP local para cambios en tiempo real.

🛠 Configuración de Hardware (Pinout)

Para este dispositivo, la configuración de los pines para la pantalla ST7789 de 1.54" (240x240) es crítica. Según la investigación en las comunidades de Home Assistant y Tasmota, el mapeo es el siguiente:
Función TFT	Pin ESP8266 (GPIO)	Nota
TFT_CS	GPIO 15 (D8)	Chip Select
TFT_DC	GPIO 0 (D3)	Data/Command
TFT_RST	GPIO 2 (D4)	Reset
TFT_SCL	GPIO 14 (D5)	Reloj SPI
TFT_SDA	GPIO 13 (D7)	Datos SPI
BACKLIGHT	GPIO 5 (D1)	Activo en LOW para encender

    Nota: El backlight en este dispositivo funciona de forma inversa a lo habitual; poner el pin en LOW enciende la pantalla.

📚 Librerías Necesarias

Para compilar este proyecto en PlatformIO, asegúrate de incluir estas dependencias en tu platformio.ini:
Ini, TOML

lib_deps =
    adafruit/Adafruit GFX Library @ ^1.11.9
    adafruit/Adafruit ST7735 and ST7789 Library @ ^1.9.4
    bblanchon/ArduinoJson @ ^6.21.4
    arduino-libraries/NTPClient @ ^3.2.1

💻 Instalación

    Clona este repositorio.

    Asegúrate de configurar el sistema de archivos a littlefs en tu entorno de desarrollo.

    Sube el código a tu SmallTV Pro.

    Al encender, conéctate a la red WiFi SmartTV_Pro desde tu móvil.

    Accede a 192.168.4.1, introduce tus credenciales y tu OpenWeather API Key.

    ¡Disfruta de tu reloj!

🤝 Créditos y Referencias

Este proyecto no habría sido posible sin la ingeniería inversa y la información compartida por la comunidad:

    Concepto Original: Dispositivo diseñado por GeekMagic.

    Investigación de Pines: Gracias a los usuarios del hilo de Home Assistant Community que identificaron los GPIO correctos para ESPHome.

    Soporte Tasmota: Información técnica valiosa extraída de las discusiones en el GitHub de Tasmota sobre el hardware del SmallTV Pro.

🔌 Esquema de Conexión Interno (Referencia)

Si has abierto el dispositivo o estás usando un NodeMCU externo para pruebas, este es el esquema de flujo de datos:

    Alimentación: El ESP8266 y la pantalla comparten la línea de 3.3V. No alimentes la pantalla con 5V, ya que podrías dañar el controlador ST7789.

    Bus SPI:

        MOSI (GPIO 13) -> SDA de la pantalla.

        SCK (GPIO 14) -> SCL de la pantalla.

    Control: Los pines GPIO 0 (DC) y GPIO 15 (CS) gestionan el flujo de comandos. Es vital que GPIO 15 tenga una resistencia de pull-down (que el SmallTV ya trae internamente).

Descripción: Vista del circuito impreso donde se integran los componentes. Se aprecia el bus de datos hacia la pantalla, los reguladores de voltaje y la disposición de los componentes SMD que permiten un diseño compacto dentro de la carcasa cúbica.
![PCB del proyecto](/screenshot/pcb.jpg)
    
Descripción: Detalle del corazón del proyecto: el chip ESP8266EX. Es un SoC de bajo consumo con stack TCP/IP integrado que permite la conexión WiFi y la gestión de la pantalla TFT. A su derecha se observa el oscilador de 26.000 MHz.
![esp8266](/screenshot/esp8266.jpg)
    
Descripción: Parte posterior del panel LCD. Se trata de una pantalla con resolución de 240x240 píxeles que utiliza el controlador ST7789, ofreciendo ángulos de visión amplios y colores vivos para la interfaz del reloj.
![pantalla](/screenshot/pantalla.jpg)
   
Descripción: Esta es la pantalla principal del dispositivo. Muestra la dirección IP local para el acceso a la configuración, la hora sincronizada por NTP con segundos en color púrpura, el estado del clima mediante iconos dinámicos, la temperatura en grados Celsius y la fecha completa con el día de la semana.
![digital](/screenshot/reloj-digital.jpg)
    
Descripción: Un diseño clásico de esfera analógica. Las manecillas de hora, minutos y segundos son personalizables en color a través de la web. Este modo prioriza la estética limpia, mostrando únicamente la esfera y la fecha en formato numérico en la parte inferior. 
![analogico](/screenshot/reloj-analogico.jpg)
    
Descripción: Interfaz administrativa accesible vía navegador. Permite configurar el WiFi (SSID/Pass), la API Key de OpenWeather, la ciudad, los colores del reloj, el brillo general del panel y activar el "Modo Noche" automático.
![web](/screenshot/weather-web.png)

📝 Instrucciones para Contribuir

    Haz un Fork del proyecto.

    Crea una nueva rama: git checkout -b feature/MejoraIncreible.

    Haz tus cambios y un Commit: git commit -m 'Añadida nueva animación'.

    Envía un Push a la rama: git push origin feature/MejoraIncreible.

    Abre un Pull Request.
