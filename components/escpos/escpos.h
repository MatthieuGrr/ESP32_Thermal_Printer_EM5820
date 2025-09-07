#pragma once
/**
 * MIT License
 *
 * Enhanced ESC/POS library for ESP-IDF (ESP32-C6 tested) targeting EM5820-like printers.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Handle type (opaque to user) */
typedef struct escpos_ctx *escpos_handle_t;

/** User configuration for the ESC/POS driver */
typedef struct {
    uart_port_t uart_num;   // e.g. UART_NUM_1
    int tx_pin;             // e.g. GPIO17
    int rx_pin;             // if unused, set to -1
    int baud_rate;          // e.g. 19200 or 38400
    size_t tx_buffer_size;  // UART TX buffer size, e.g. 2048
} escpos_config_t;

/** Text justification */
typedef enum {
    ESC_POS_JUSTIFY_LEFT = 0,
    ESC_POS_JUSTIFY_CENTER = 1,
    ESC_POS_JUSTIFY_RIGHT = 2,
} escpos_justify_t;

/** Underline style */
typedef enum {
    ESC_POS_UNDERLINE_NONE = 0,
    ESC_POS_UNDERLINE_1   = 1,
    ESC_POS_UNDERLINE_2   = 2,
} escpos_underline_t;

/** Initialize the printer (installs and configures UART). */
esp_err_t escpos_init(const escpos_config_t *cfg, escpos_handle_t *out);

/** Deinitialize and free resources. */
esp_err_t escpos_deinit(escpos_handle_t handle);

/** Reset printer to defaults (ESC @). */
esp_err_t escpos_reset(escpos_handle_t handle);

/** Feed N lines (max 255 typical). */
esp_err_t escpos_feed_lines(escpos_handle_t handle, uint8_t lines);

/** Set bold (ESC E n). */
esp_err_t escpos_set_bold(escpos_handle_t handle, bool enable);

/** Set underline (ESC - n). */
esp_err_t escpos_set_underline(escpos_handle_t handle, escpos_underline_t style);

/** Quick justification helpers */
esp_err_t escpos_justify_left(escpos_handle_t handle);
esp_err_t escpos_justify_center(escpos_handle_t handle);
esp_err_t escpos_justify_right(escpos_handle_t handle);

/**
 * Set text size via GS ! n.
 * width_mul: 1..8, height_mul: 1..8
 */
esp_err_t escpos_set_text_size(escpos_handle_t handle, uint8_t width_mul, uint8_t height_mul);

/** Set reverse (white on black) text (GS B n). */
esp_err_t escpos_set_reverse(escpos_handle_t handle, bool enable);

/** Set italics (ESC 4/ESC 5). */
esp_err_t escpos_set_italic(escpos_handle_t handle, bool enable);

/** Print a zero-terminated string (no extra newline). */
esp_err_t escpos_print_text(escpos_handle_t handle, const char *text);

/** Print a line then LF. */
esp_err_t escpos_print_line(escpos_handle_t handle, const char *line);

/** Print a horizontal separator line (32 chars). */
esp_err_t escpos_print_separator(escpos_handle_t handle, char c);

/** Print a timestamp (YYYY-MM-DD HH:MM:SS). */
esp_err_t escpos_print_timestamp(escpos_handle_t handle);

/**
 * Try to cut paper (GS V m).
 * partial=true → partial cut (if supported), else full cut.
 */
esp_err_t escpos_cut(escpos_handle_t handle, bool partial);

/** Flush */
esp_err_t escpos_flush(escpos_handle_t handle);

//** Imprime un article à gauche et un prix à droite */
esp_err_t escpos_print_item_price(escpos_handle_t handle, const char *item, const char *price, int line_width);

esp_err_t escpos_print_bitmap(escpos_handle_t handle, const uint8_t *bitmap, uint16_t width, uint16_t height);

/**
 * Print a QR code using ESC/POS "GS ( k".
 * Many low-cost printers support this; some do not. This sends a common sequence:
 *  - Select model (49, 65)
 *  - Set module size (1..16)
 *  - Set error correction level (48..51)
 *  - Store data
 *  - Print
 */
esp_err_t escpos_print_qr(escpos_handle_t handle,
                          const char *data,
                          uint8_t module_size,    // typically 3..6
                          uint8_t ecc_level);     // 0=L,1=M,2=Q,3=H

#ifdef __cplusplus
}
#endif
