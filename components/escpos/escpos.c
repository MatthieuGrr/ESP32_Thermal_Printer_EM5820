#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#define TAG "ESC_POS"

typedef enum {
    ESC_POS_UNDERLINE_NONE = 0,
    ESC_POS_UNDERLINE_1 = 1,
    ESC_POS_UNDERLINE_2 = 2
} escpos_underline_t;

typedef enum {
    ESC_POS_JUSTIFY_LEFT = 0,
    ESC_POS_JUSTIFY_CENTER = 1,
    ESC_POS_JUSTIFY_RIGHT = 2
} escpos_justify_t;

typedef struct {
    int uart_num;
    int tx_pin;
    int rx_pin;   // si tu n'utilises pas RX, mettre -1
    int baud_rate;
    int tx_buffer_size;
} escpos_config_t;

typedef struct {
    escpos_config_t cfg;
    bool inited;
} escpos_ctx_t;

typedef escpos_ctx_t* escpos_handle_t;

// -------------------- Low-level write --------------------
static inline esp_err_t escpos_uart_write(escpos_handle_t h, const void *buf, size_t len) {
    if (!h || !h->inited) return ESP_ERR_INVALID_STATE;
    int w = uart_write_bytes(h->cfg.uart_num, buf, len);
    return (w < 0) ? ESP_FAIL : ESP_OK;
}

// -------------------- Init / Deinit --------------------
esp_err_t escpos_init(const escpos_config_t *cfg, escpos_handle_t *out) {
    if (!cfg || !out) return ESP_ERR_INVALID_ARG;

    escpos_handle_t h = calloc(1, sizeof(*h));
    if (!h) return ESP_ERR_NO_MEM;
    h->cfg = *cfg;

    uart_config_t uc = {
        .baud_rate = (cfg->baud_rate > 0) ? cfg->baud_rate : 19200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    int rx_buf = 256; // safe pour ESP32
    int tx_buf = (cfg->tx_buffer_size > 0) ? cfg->tx_buffer_size : 2048;

    esp_err_t err = uart_driver_install(cfg->uart_num, rx_buf, tx_buf, 0, NULL, 0);
    if (err != ESP_OK) { free(h); return err; }

    err = uart_param_config(cfg->uart_num, &uc);
    if (err != ESP_OK) { uart_driver_delete(cfg->uart_num); free(h); return err; }

    err = uart_set_pin(cfg->uart_num,
                       cfg->tx_pin,
                       (cfg->rx_pin >= 0) ? cfg->rx_pin : UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE);
    if (err != ESP_OK) { uart_driver_delete(cfg->uart_num); free(h); return err; }

    h->inited = true;
    *out = h;

    // Reset imprimante
    const uint8_t reset[] = {0x1B, 0x40};
    escpos_uart_write(h, reset, sizeof(reset));
    vTaskDelay(pdMS_TO_TICKS(100));

    return ESP_OK;
}

esp_err_t escpos_deinit(escpos_handle_t h) {
    if (!h) return ESP_ERR_INVALID_ARG;
    if (h->inited) { uart_driver_delete(h->cfg.uart_num); h->inited = false; }
    free(h);
    return ESP_OK;
}

// -------------------- Text --------------------
esp_err_t escpos_print_line(escpos_handle_t h, const char *line) {
    if (!line) return ESP_OK;
    escpos_uart_write(h, line, strlen(line));
    const uint8_t crlf[] = {0x0D, 0x0A};
    return escpos_uart_write(h, crlf, sizeof(crlf));
}

esp_err_t escpos_feed_lines(escpos_handle_t h, uint8_t lines) {
    uint8_t cmd[] = {0x1B, 0x64, lines};
    return escpos_uart_write(h, cmd, sizeof(cmd));
}

esp_err_t escpos_print_text(escpos_handle_t handle, const char *text)
{
    if (!handle || !text) return ESP_ERR_INVALID_ARG;

    size_t len = strlen(text);
    if (len == 0) return ESP_OK; // rien à faire

    // Utilise la fonction UART existante
    return escpos_uart_write(handle, (const uint8_t *)text, len);
}

// -------------------- Formatting --------------------
esp_err_t escpos_set_bold(escpos_handle_t h, bool enable) {
    uint8_t cmd[] = {0x1B, 0x45, (uint8_t)(enable ? 1 : 0)};
    return escpos_uart_write(h, cmd, sizeof(cmd));
}

esp_err_t escpos_set_underline(escpos_handle_t h, escpos_underline_t style) {
    if (style > ESC_POS_UNDERLINE_2) style = ESC_POS_UNDERLINE_2;
    uint8_t cmd[] = {0x1B, 0x2D, (uint8_t)style};
    return escpos_uart_write(h, cmd, sizeof(cmd));
}

esp_err_t escpos_set_justify(escpos_handle_t h, escpos_justify_t j) {
    if (!h) return ESP_ERR_INVALID_ARG;

    uint8_t val;
    switch (j) {
        case ESC_POS_JUSTIFY_LEFT:   val = 0x00; break;
        case ESC_POS_JUSTIFY_CENTER: val = 0x01; break;
        case ESC_POS_JUSTIFY_RIGHT:  val = 0x02; break;
        default:                     val = 0x00; break; // fallback à gauche
    }

    uint8_t cmd[] = {0x1B, 0x61, val};
    return escpos_uart_write(h, cmd, sizeof(cmd));
}

// -------------------- Cut --------------------
esp_err_t escpos_cut(escpos_handle_t h, bool partial) {
    vTaskDelay(pdMS_TO_TICKS(100));
    uint8_t cmd[] = {0x1D, 0x56, (uint8_t)(partial ? 1 : 0)};
    return escpos_uart_write(h, cmd, sizeof(cmd));
}

// -------------------- Text size --------------------
esp_err_t escpos_set_text_size(escpos_handle_t h, uint8_t width, uint8_t height) {
    if (width > 7) width = 7;
    if (height > 7) height = 7;
    uint8_t n = (width << 4) | height;
    uint8_t cmd[] = {0x1D, 0x21, n};
    return escpos_uart_write(h, cmd, sizeof(cmd));
}

// -------------------- Timestamp --------------------
esp_err_t escpos_print_timestamp(escpos_handle_t h) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
    return escpos_print_line(h, buf);
}

// -------------------- Reverse --------------------
esp_err_t escpos_set_reverse(escpos_handle_t h, bool enable) {
    uint8_t cmd[] = {0x1D, 0x42, (uint8_t)(enable ? 1 : 0)}; // GS B n
    return escpos_uart_write(h, cmd, sizeof(cmd));
}

// -------------------- Justify --------------------
esp_err_t escpos_justify_left(escpos_handle_t h) {
    return escpos_set_justify(h, ESC_POS_JUSTIFY_LEFT);
}

esp_err_t escpos_justify_center(escpos_handle_t h) {
    return escpos_set_justify(h, ESC_POS_JUSTIFY_CENTER);
}

esp_err_t escpos_justify_right(escpos_handle_t h) {
    return escpos_set_justify(h, ESC_POS_JUSTIFY_RIGHT);
}

// -------------------- Set Italic --------------------
esp_err_t escpos_set_italic(escpos_handle_t h, bool enable) {
    uint8_t cmd[] = {0x1B, 0x34 + (enable ? 1 : 0)}; // ESC 4 / ESC 5
    return escpos_uart_write(h, cmd, sizeof(cmd));
}

// -------------------- Print Separator --------------------
esp_err_t escpos_print_separator(escpos_handle_t h, char c) {
    char line[33];
    for (int i = 0; i < 32; i++) line[i] = c;
    line[32] = '\0';
    return escpos_print_line(h, line);
}

// -------------------- Flush --------------------
esp_err_t escpos_flush(escpos_handle_t h) {
    if (!h) return ESP_ERR_INVALID_ARG;
    return uart_wait_tx_done(h->cfg.uart_num, pdMS_TO_TICKS(100));
}

// ---- Imprime un article à gauche et un prix à droite ----
esp_err_t escpos_print_item_price(escpos_handle_t h, const char *item, const char *price, int line_width)
{
    if (!h || !item || !price) return ESP_ERR_INVALID_ARG;
    if (line_width <= 0) line_width = 32; // valeur par défaut

    char line[128]; // assez grand pour la ligne
    int len_item  = strlen(item);
    int len_price = strlen(price);

    // Calculer le nombre d'espaces entre les deux
    int spaces = line_width - len_item - len_price;
    if (spaces < 1) spaces = 1;

    // Construire la ligne
    snprintf(line, sizeof(line), "%s%*s%s", item, spaces, "", price);

    // Imprimer
    return escpos_print_line(h, line);
}

esp_err_t escpos_print_bitmap(escpos_handle_t h, const uint8_t *bitmap, uint16_t width, uint16_t height)
{
    if (!h || !bitmap) return ESP_ERR_INVALID_ARG;

    uint8_t xL = width / 8 % 256;
    uint8_t xH = width / 8 / 256;
    uint8_t yL = height % 256;
    uint8_t yH = height / 256;

    uint8_t header[] = {0x1D, 0x76, 0x30, 0x00, xL, xH, yL, yH};

    // Envoyer le header
    escpos_uart_write(h, header, sizeof(header));
    // Envoyer les données
    return escpos_uart_write(h, bitmap, (width / 8) * height);
}