#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "escpos.h"  // ton fichier escpos.h/c avec le code précédent

#define UART_NUM       UART_NUM_1
#define TX_PIN         0
#define RX_PIN         1      // pas de RX nécessaire
#define BAUD_RATE      9600

void app_main(void)
{
    escpos_handle_t printer;
    escpos_config_t cfg = {
        .uart_num = UART_NUM,
        .tx_pin = TX_PIN,
        .rx_pin = RX_PIN,
        .baud_rate = BAUD_RATE,
        .tx_buffer_size = 2048
    };

    if (escpos_init(&cfg, &printer) != ESP_OK) {
        ESP_LOGE("MAIN", "Erreur initialisation ESC/POS !");
        return;
    }

    // Impression simple
    escpos_justify_center(printer);
    escpos_set_bold(printer, true);
    escpos_print_line(printer, "Bonjour EM5820 !");
    escpos_set_bold(printer, false);

    // Texte agrandi
    escpos_set_text_size(printer, 1, 2);
    escpos_print_line(printer, "GRAND TEXTE");
    escpos_set_text_size(printer, 0, 0); // revenir normal

    // Underline
    escpos_set_underline(printer, ESC_POS_UNDERLINE_1);
    escpos_print_line(printer, "Sous-ligne");
    escpos_set_underline(printer, ESC_POS_UNDERLINE_NONE);

    escpos_print_item_price(printer, "Texte gauche", "32", 32);
    escpos_flush(printer);

    escpos_justify_center(printer);
    escpos_print_separator(printer, '-');
    escpos_flush(printer);

    // Feed 3 lignes
    escpos_feed_lines(printer, 3);

    const uint8_t logo_test[32] = {
    0x00,0x00, 0x3C,0x3C, 0x42,0x42, 0xA9,0xA9,
    0x85,0x85, 0xA9,0xA9, 0x91,0x91, 0x42,0x42,
    0x3C,0x3C, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00
};
    escpos_print_bitmap(printer, logo_test, 16, 16); // 384 pixels largeur, 100 pixels hauteur

    escpos_feed_lines(printer, 3);


    // Cut le papier
    escpos_cut(printer, true); // true = coupe partielle

    // Déinit
    escpos_deinit(printer);

    ESP_LOGI("MAIN", "Impression terminée !");
}
