#include "web_server.h"
#include "hal_camera.h"
#include "car_control.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include <string.h>

static const char *TAG = "WEB_SERVER";
static QueueHandle_t s_commandQueue;

// ============ VIDEO STREAM ============
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static esp_err_t stream_handler(httpd_req_t *req) {
    esp_err_t res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) return res;

    ESP_LOGI(TAG, "Клієнт підключився до відеопотоку");

    while (true) {
        uint8_t *jpg_buf = NULL;
        size_t jpg_len = 0;

        if (!cameraCaptureJpeg(&jpg_buf, &jpg_len)) {
            continue;
        }

        char part_buf[128];
        res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        if (res == ESP_OK) {
            size_t hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, (unsigned)jpg_len);
            res = httpd_resp_send_chunk(req, part_buf, hlen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)jpg_buf, jpg_len);
        }
        free(jpg_buf);

        if (res != ESP_OK) {
            ESP_LOGW(TAG, "Клієнт відключився від стріму");
            break;
        }
    }
    return res;
}

// ============ MAIN PAGE ============
static const char* HTML_PAGE = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
  body{font-family:sans-serif;text-align:center;background:#222;color:#fff;margin:0;padding:10px;}
  img{width:100%;max-width:480px;border-radius:10px;margin-bottom:15px;}
  button{font-size:22px;padding:18px;margin:5px;width:90px;border-radius:10px;border:none;background:#378ADD;color:#fff;}
  button:active{background:#1D9E75;}
</style>
</head><body>
<h2>Car Control</h2>
<img src='http://192.168.4.1:81/stream'>
<div><button onclick="send('forward')">FORWARD</button></div>
<div>
  <button onclick="send('left')">LEFT</button>
  <button onclick="send('stop')">STOP</button>
  <button onclick="send('right')">RIGHT</button>
</div>
<div><button onclick="send('backward')">BACKWARD</button></div>
<script>
function send(cmd) {
  fetch('/cmd?c=' + cmd)
    .then(r => r.text())
    .then(t => console.log('cmd response:', t))
    .catch(e => console.error('cmd error:', e));
}
</script>
</body></html>
)rawliteral";

static esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_send(req, HTML_PAGE, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ============ COMMAND HANDLER ============
static esp_err_t cmd_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, ">>> cmd_handler викликано");

    char query[32];
    char value[16];
    CarCommand cmd = CAR_STOP;

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        ESP_LOGI(TAG, "query = %s", query);
        if (httpd_query_key_value(query, "c", value, sizeof(value)) == ESP_OK) {
            ESP_LOGI(TAG, "value = %s", value);
            if      (strcmp(value, "forward")  == 0) cmd = CAR_FORWARD;
            else if (strcmp(value, "backward") == 0) cmd = CAR_BACKWARD;
            else if (strcmp(value, "left")     == 0) cmd = CAR_LEFT;
            else if (strcmp(value, "right")    == 0) cmd = CAR_RIGHT;
            else                                      cmd = CAR_STOP;
        }
    } else {
        ESP_LOGW(TAG, "Не вдалося прочитати query string");
    }

    ESP_LOGI(TAG, "команда визначена: %d, кладу в чергу", (int)cmd);
    xQueueSend(s_commandQueue, &cmd, portMAX_DELAY);
    ESP_LOGI(TAG, "команда покладена в чергу успішно");

    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ============ SERVER STARTUP — ON CORE 0 ============
// Note: esp_http_server processes all requests on a SINGLE worker thread.
// stream_handler never returns (while(true)), so the video stream
// runs on a separate server (different port) — otherwise it would
// permanently occupy the only thread and /cmd from buttons would never be handled.
static void start_httpd() {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.stack_size = 8192;
    config.max_uri_handlers = 8;
    config.max_open_sockets = 4;   // allow several concurrent connections (stream + commands)
    config.core_id = 0;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root_uri = { .uri = "/",    .method = HTTP_GET, .handler = root_handler, .user_ctx = NULL };
        httpd_uri_t cmd_uri  = { .uri = "/cmd", .method = HTTP_GET, .handler = cmd_handler,  .user_ctx = NULL };

        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &cmd_uri);

        ESP_LOGI(TAG, "Веб-сервер запущено на ядрі 0 (/, /cmd), max_open_sockets=4");
    } else {
        ESP_LOGE(TAG, "Не вдалося запустити веб-сервер");
    }
}

// Separate server for the video stream (port 81), so it does not block command handling.
static void start_stream_httpd() {
    httpd_handle_t stream_server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 81;
    config.ctrl_port = 32769; // must differ from the main server's control port
    config.stack_size = 8192;
    config.max_uri_handlers = 2;
    config.max_open_sockets = 2;
    config.core_id = 0;

    if (httpd_start(&stream_server, &config) == ESP_OK) {
        httpd_uri_t stream_uri = { .uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL };
        httpd_register_uri_handler(stream_server, &stream_uri);
        ESP_LOGI(TAG, "Сервер стріму запущено на порту 81 (/stream)");
    } else {
        ESP_LOGE(TAG, "Не вдалося запустити сервер стріму");
    }
}

// ============ WiFi Access Point ============
static void wifi_init_softap() {
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.ap.ssid, "ESP32-Car");
    wifi_config.ap.ssid_len = strlen("ESP32-Car");
    strcpy((char*)wifi_config.ap.password, "12345678");
    wifi_config.ap.channel = 1;
    wifi_config.ap.max_connection = 2;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
    esp_wifi_set_max_tx_power(40);

    ESP_LOGI(TAG, "AP запущено: ESP32-Car / 12345678, IP: 192.168.4.1");
}

void webServerStart(QueueHandle_t commandQueue) {
    s_commandQueue = commandQueue;
    wifi_init_softap();
    start_httpd();
    start_stream_httpd();
}