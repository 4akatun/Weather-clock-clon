#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <time.h>

#define TFT_CS        15
#define TFT_DC         0 
#define TFT_RST        2 
#define TFT_BACKLIGHT  5 
#define COLOR_GRIS     0x8410
const char* TZ_INFO = "CET-1CEST,M3.5.0,M10.5.0";

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
ESP8266WebServer server(80);
DNSServer dnsServer;

String _ssid, _pass, _api, _city, climaDesc = "Clear";
String _ch_hex = "#FFFF00", _cm_hex = "#00FFFF", _cs_hex = "#FF00FF";
uint16_t _colH, _colM, _colS;
bool _modoAnalogico = false, _modoNocheAuto = true;
int _brilloPorcentaje = 75; 
float temperatura = 0, viento = 0;
int humedad = 0;
unsigned long ultimaVezClima = 0, ultimoCambioIcono = 0;
int ultimoSegundo = -1, faseClima = 0;
const char* diasSemana[] = {"domingo", "lunes", "martes", "miercoles", "jueves", "viernes", "sabado"};

// --- FUNCIONES ---
uint16_t hexTo565(String hex) {
    if (!hex.startsWith("#") || hex.length() < 7) return ST77XX_WHITE;
    long rgb = strtol(hex.substring(1).c_str(), NULL, 16);
    return tft.color565((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

void actualizarBrillo(int hora) {
    int pwmValue = _modoNocheAuto && (hora >= 22 || hora < 8) ? 900 : map(_brilloPorcentaje, 0, 100, 1023, 0);
    analogWrite(TFT_BACKLIGHT, pwmValue);
}

void dibujarIconoClima(int x, int y, String tipo) {
    tft.fillRect(x-25, y-25, 50, 50, ST77XX_BLACK); 
    if (tipo == "Clear") {
        tft.fillCircle(x, y, 10, ST77XX_YELLOW);
        for(int i=0; i<360; i+=45) {
            float rad = i * PI / 180;
            tft.drawLine(x+12*cos(rad), y+12*sin(rad), x+18*cos(rad), y+18*sin(rad), ST77XX_YELLOW);
        }
    } else if (tipo == "Clouds") {
        tft.fillCircle(x-5, y+5, 8, COLOR_GRIS);
        tft.fillCircle(x+5, y, 10, ST77XX_WHITE);
        tft.fillCircle(x-8, y, 7, ST77XX_WHITE);
    } else if (tipo == "Rain") {
        tft.fillCircle(x, y-5, 8, COLOR_GRIS);
        for(int i=-6; i<=6; i+=6) tft.drawLine(x+i, y+5, x+i-3, y+13, ST77XX_BLUE);
    } else {
        tft.fillCircle(x, y, 8, ST77XX_WHITE);
    }
}

void obtenerClima() {
    if (_api.length() < 10 || WiFi.status() != WL_CONNECTED) return;
    WiFiClient client; HTTPClient http;
    if (http.begin(client, "http://api.openweathermap.org/data/2.5/weather?q=" + _city + "&units=metric&appid=" + _api)) {
        if (http.GET() == 200) {
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, http.getString());
            temperatura = doc["main"]["temp"];
            humedad = doc["main"]["humidity"];
            viento = doc["wind"]["speed"];
            climaDesc = doc["weather"][0]["main"].as<String>();
        }
        http.end();
    }
}

void dibujarAnalogico(int h, int m, int s, bool borrar = false) {
    int cx = 120, cy = 100, r = 80;
    uint16_t cH = borrar ? ST77XX_BLACK : _colH;
    uint16_t cM = borrar ? ST77XX_BLACK : _colM;
    uint16_t cS = borrar ? ST77XX_BLACK : _colS;
    if (!borrar) {
        tft.drawCircle(cx, cy, r + 2, ST77XX_WHITE);
        for(int i=0; i<12; i++) {
            float a = i * 30 * PI / 180;
            tft.drawLine(cx+(r-5)*sin(a), cy-(r-5)*cos(a), cx+r*sin(a), cy-r*cos(a), COLOR_GRIS);
        }
    }
    tft.drawLine(cx, cy, cx + 40 * sin((h%12 + m/60.0)*30*PI/180), cy - 40 * cos((h%12 + m/60.0)*30*PI/180), cH);
    tft.drawLine(cx, cy, cx + 60 * sin(m*6*PI/180), cy - 60 * cos(m*6*PI/180), cM);
    tft.drawLine(cx, cy, cx + 70 * sin(s*6*PI/180), cy - 70 * cos(s*6*PI/180), cS);
    tft.fillCircle(cx, cy, 3, borrar ? ST77XX_BLACK : ST77XX_WHITE);
}

void guardarConfig() {
    File f = LittleFS.open("/config.json", "w");
    if (f) {
        DynamicJsonDocument doc(1024);
        doc["ssid"] = _ssid; doc["pass"] = _pass; doc["api"] = _api; doc["city"] = _city;
        doc["ch"] = _ch_hex; doc["cm"] = _cm_hex; doc["cs"] = _cs_hex;
        doc["mode"] = _modoAnalogico ? "A" : "D";
        doc["bri"] = _brilloPorcentaje; doc["night"] = _modoNocheAuto;
        serializeJson(doc, f); f.close();
    }
}

bool cargarConfig() {
    if (!LittleFS.exists("/config.json")) return false;
    File f = LittleFS.open("/config.json", "r");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, f);
    _ssid = doc["ssid"].as<String>(); _pass = doc["pass"].as<String>();
    _api  = doc["api"].as<String>(); _city = doc["city"].as<String>();
    _ch_hex = doc["ch"] | "#FFFF00"; _cm_hex = doc["cm"] | "#00FFFF"; _cs_hex = doc["cs"] | "#FF00FF";
    _modoAnalogico = (doc["mode"].as<String>() == "A");
    _brilloPorcentaje = doc["bri"] | 75; _modoNocheAuto = doc["night"] | true;
    _colH = hexTo565(_ch_hex); _colM = hexTo565(_cm_hex); _colS = hexTo565(_cs_hex);
    f.close(); return true;
}

void handleRoot() {
    String h = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><style>body{font-family:sans-serif;background:#121212;color:#eee;padding:10px;} .card{background:#1e1e1e;padding:15px;border-radius:12px;margin-bottom:15px;border-left:5px solid #00e676;} input,select{width:100%;padding:10px;margin:5px 0;background:#2c2c2c;border:1px solid #444;color:#fff;border-radius:6px;} .btn{padding:12px;border:none;border-radius:6px;width:48%;font-weight:bold;cursor:pointer;} .apply{background:#2196F3;color:#fff;} .save{background:#00e676;color:#000;}</style></head><body>";
    h += "<h2>⚙️ SmartTV Pro</h2><form method='POST' action='/s'><div class='card'>WiFi:<input name='s' value='"+_ssid+"'><input name='p' type='password' value='"+_pass+"'></div>";
    h += "<div class='card'>API Clima:<input name='a' value='"+_api+"'><input name='c' value='"+_city+"'></div>";
    h += "<div class='card'>Modo:<select name='m'><option value='D' "+String(!_modoAnalogico?"selected":"")+">Digital</option><option value='A' "+String(_modoAnalogico?"selected":"")+">Analógico</option></select>Colores:<div style='display:flex;gap:5px'><input name='ch' type='color' value='"+_ch_hex+"'><input name='cm' type='color' value='"+_cm_hex+"'><input name='cs' type='color' value='"+_cs_hex+"'></div></div>";
    h += "<div class='card'>Brillo: "+String(_brilloPorcentaje)+"%<input type='range' name='br' min='5' max='100' value='"+String(_brilloPorcentaje)+"'><br><label><input type='checkbox' name='na' "+String(_modoNocheAuto?"checked":"")+"> Modo Noche</label></div>";
    h += "<div style='display:flex;justify-content:space-between;'><button name='act' value='p' class='btn apply'>APLICAR</button><button name='act' value='s' class='btn save'>GUARDAR</button></div></form></body></html>";
    server.send(200, "text/html", h);
}

void setup() {
    LittleFS.begin(); tft.init(240, 240, SPI_MODE3); tft.setRotation(2); tft.fillScreen(ST77XX_BLACK);
    pinMode(TFT_BACKLIGHT, OUTPUT); 
    cargarConfig();
    actualizarBrillo(12);

    configTime(0, 0, "pool.ntp.org", "time.google.com");
    setenv("TZ", TZ_INFO, 1); tzset();

    if (_ssid.length() > 1) {
        WiFi.begin(_ssid.c_str(), _pass.c_str());
        // Espera activa de conexión para no dejar la pantalla en espera infinita
        int intentos = 0;
        while (WiFi.status() != WL_CONNECTED && intentos < 20) { delay(500); intentos++; }
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.softAP("SmartTV_Pro");
        dnsServer.start(53, "*", WiFi.softAPIP());
    } else {
        obtenerClima(); // Primer intento de clima nada más conectar
    }

    server.on("/", handleRoot);
    server.on("/s", []() {
        _ssid = server.arg("s"); _pass = server.arg("p"); _api = server.arg("a"); _city = server.arg("c");
        _ch_hex = server.arg("ch"); _cm_hex = server.arg("cm"); _cs_hex = server.arg("cs");
        _modoAnalogico = (server.arg("m") == "A");
        _brilloPorcentaje = server.arg("br").toInt(); _modoNocheAuto = server.hasArg("na");
        _colH = hexTo565(_ch_hex); _colM = hexTo565(_cm_hex); _colS = hexTo565(_cs_hex);
        tft.fillScreen(ST77XX_BLACK);
        ultimoSegundo = -1; obtenerClima();
        if (server.arg("act") == "s") guardarConfig(); 
        server.sendHeader("Location", "/"); server.send(303);
    });
    server.begin();
}

void loop() {
    dnsServer.processNextRequest(); server.handleClient();
    time_t ahora = time(nullptr);
    struct tm * ti = localtime(&ahora);

    // Si ya tenemos hora válida (año > 2020)
    if (ti->tm_year > 110) {
        // Si acabamos de sincronizar y no tenemos clima, pedirlo una vez
        if (temperatura == 0 && _api.length() > 10) obtenerClima();

        if (ti->tm_sec != ultimoSegundo) {
            actualizarBrillo(ti->tm_hour);

            if (_modoAnalogico) {
                if (ultimoSegundo != -1) dibujarAnalogico(ti->tm_hour, ti->tm_min, ultimoSegundo, true);
                dibujarAnalogico(ti->tm_hour, ti->tm_min, ti->tm_sec);
                tft.setTextSize(2); tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
                char f[20]; sprintf(f, "%02d/%02d/%02d", ti->tm_mday, ti->tm_mon+1, (ti->tm_year+1900)%100);
                tft.setCursor((240-(strlen(f)*12))/2, 200); tft.print(f);
            } 
            else {
                tft.setCursor(5, 5); 
                tft.setTextSize(1); 
                tft.setTextColor(COLOR_GRIS, ST77XX_BLACK);
                tft.print("IP: "); 
                tft.print(WiFi.localIP().toString());

                tft.setTextSize(5); tft.setCursor(25, 30);
                tft.setTextColor(_colH, ST77XX_BLACK); tft.printf("%02d", ti->tm_hour);
                tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); tft.print(":");
                tft.setTextColor(_colM, ST77XX_BLACK); tft.printf("%02d", ti->tm_min);
                tft.setTextSize(2); tft.setCursor(185, 50); tft.setTextColor(_colS, ST77XX_BLACK); tft.printf("%02d", ti->tm_sec);

                if (millis() - ultimoCambioIcono > 7000) {
                    faseClima = (faseClima + 1) % 3;
                    dibujarIconoClima(120, 105, climaDesc);
                    tft.fillRect(0, 135, 240, 30, ST77XX_BLACK);
                    ultimoCambioIcono = millis();
                }

                tft.setTextSize(3); tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
                String v = (faseClima == 0) ? String(temperatura, 1) + " C" : (faseClima == 1) ? String(humedad) + " % HR" : String(viento * 3.6, 1) + " k/h";
                tft.setCursor((240-(v.length()*18))/2, 135); tft.print(v);

                tft.setTextSize(1); tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
                tft.setCursor((240-(_city.length()*6))/2, 175); tft.print(_city);

                tft.setTextSize(2); tft.setTextColor(COLOR_GRIS, ST77XX_BLACK);
                char f[32]; sprintf(f, "%02d/%02d/%02d | %s", ti->tm_mday, ti->tm_mon+1, (ti->tm_year+1900)%100, diasSemana[ti->tm_wday]);
                tft.setCursor((240-(strlen(f)*12))/2, 205); tft.print(f);
            }
            ultimoSegundo = ti->tm_sec;
        }
        if (millis() - ultimaVezClima > 900000) { obtenerClima(); ultimaVezClima = millis(); }
    }
}