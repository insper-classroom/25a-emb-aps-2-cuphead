/*
 * APS2
 */

 #include <FreeRTOS.h>
 #include <task.h>
 #include <semphr.h>
 #include <queue.h>
 #include "hardware/adc.h"
 #include "hardware/gpio.h"
 #include "pico/stdlib.h"
 #include <stdio.h>
 
 const int BTN_LIGA   = 15; // branco
 const int BTN_TROCA  = 13; // azul
 const int BTN_ATIRA  = 11; // verde
 const int BTN_PIKA   = 14; // vermelho
 const int BTN_PAUSA  = 12; // amarelo
 const int LED_LIGA   = 10;
 const int VIBRADOR   = 9;
 
 QueueHandle_t      xQueueAdc;
 SemaphoreHandle_t  xEnableSemaphore;
 SemaphoreHandle_t  xVibeSemaphore;
 
 typedef struct {
     int axis;
     int val;
 } adc_t;
 
 // Task X (ADC channel 1 on GPIO 27)
 void adc_x_task(void *p) {
     int dataList[5] = {0};
     int index = 0, sum = 0, count = 0;
     adc_gpio_init(27);
 
     while (1) {
         adc_select_input(1);
         uint16_t result = adc_read();
         int16_t raw_value = (result - 2047) / 7.96;
         if (raw_value > -90 && raw_value < 90) raw_value = 0;
         if (count < 5) {
             dataList[index] = raw_value;
             sum += raw_value;
             count++;
         } else {
             sum = sum - dataList[index] + raw_value;
             dataList[index] = raw_value;
         }
         index = (index + 1) % 5;
         int mediamovel = sum / 5;
 
         adc_t evt = { .axis = 0, .val = mediamovel };
         if (mediamovel != 0) xQueueSend(xQueueAdc, &evt, 0);
 
         vTaskDelay(pdMS_TO_TICKS(50));
     }
 }
 
 void adc_y_task(void *p) {
     int dataList[5] = {0};
     int index = 0, sum = 0, count = 0;
     adc_gpio_init(26);
 
     while (1) {
         adc_select_input(0);
         uint16_t result = adc_read();
         int16_t raw_value = (result - 2047) / 7.96;
         if (raw_value > -90 && raw_value < 90) raw_value = 0;
         if (count < 5) {
             dataList[index] = raw_value;
             sum += raw_value;
             count++;
         } else {
             sum = sum - dataList[index] + raw_value;
             dataList[index] = raw_value;
         }
         index = (index + 1) % 5;
         int mediamovel = sum / 5;
 
         if (mediamovel < -90 || mediamovel > 90) {
             adc_t evt = { .axis = 1, .val = mediamovel };
             xQueueSend(xQueueAdc, &evt, 0);
         }
 
         vTaskDelay(pdMS_TO_TICKS(50));
     }
 }
 
 void desh_y_task(void *p) {
     int dataList[5] = {0};
     int index = 0, sum = 0, count = 0;
     adc_gpio_init(28);
 
     while (1) {
         adc_select_input(2);
         uint16_t result = adc_read();
         int16_t raw_value = (result - 2047) / 7.96;
         if (raw_value > -200 && raw_value < 200) raw_value = 0;
         if (count < 5) {
             dataList[index] = raw_value;
             sum += raw_value;
             count++;
         } else {
             sum = sum - dataList[index] + raw_value;
             dataList[index] = raw_value;
         }
         index = (index + 1) % 5;
         int mediamovel = sum / 5;
 
         if (mediamovel > 200 || mediamovel < -200) {
             adc_t evt = { .axis = 3, .val = mediamovel };
             xQueueSend(xQueueAdc, &evt, 0);
         }
 
         vTaskDelay(pdMS_TO_TICKS(50));
     }
 }
 
 void led_task(void *p) {
     while (1) {
         if (xSemaphoreTake(xEnableSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
             gpio_put(LED_LIGA, 1);
             xSemaphoreGive(xEnableSemaphore);
         } else {
             gpio_put(LED_LIGA, 0);
         }
     }
 }
 
 // Vibrator Task: vibra ao receber sinal de semáforo
 void vibrate_task(void *p) {
     while (1) {
         if (xSemaphoreTake(xVibeSemaphore, portMAX_DELAY) == pdTRUE) {
             gpio_put(VIBRADOR, 1);
             vTaskDelay(pdMS_TO_TICKS(3000)); 
             gpio_put(VIBRADOR, 0);
         }
     }
 }
 
 void uart_task(void *p) {
     adc_t adc;
 
     while (1) {
         if (!xQueueReceive(xQueueAdc, &adc, portMAX_DELAY))
             continue;
 
         if (adc.axis == 4) {
             if (xSemaphoreTake(xEnableSemaphore, 0) == pdTRUE) {
                 // fecha semáforo
             } else {
                 xSemaphoreGive(xEnableSemaphore);
             }
             continue;
         }
 
         if (xSemaphoreTake(xEnableSemaphore, 0) != pdTRUE) {
             continue;
         }
         xSemaphoreGive(xEnableSemaphore);
 
         if (adc.axis == 5) { // BTN_TROCA (azul)
             putchar('q'); putchar(adc.val ? '1' : '2');
             if (adc.val == 1) xSemaphoreGive(xVibeSemaphore); // opcional: azul vibra também
             continue;
         }
         if (adc.axis == 6) { // BTN_ATIRA (verde)
             putchar('e'); putchar(adc.val ? '1' : '2');
             continue;
         }
         if (adc.axis == 7) { // BTN_PIKA (vermelho)
             putchar('r'); putchar(adc.val ? '1' : '2');
             if (adc.val == 1) {
                 xSemaphoreGive(xVibeSemaphore); // dispara vibração ao pressionar vermelho
             }
             continue;
         }
         if (adc.axis == 8) { // BTN_PAUSA (amarelo)
             putchar('u'); putchar(adc.val ? '1' : '2');
             continue;
         }
         if (adc.axis == 1) {
             if (adc.val > 200) putchar('s');
             if (adc.val < -200) putchar('w');
             continue;
         }
         if (adc.axis == 0) {
             if (adc.val > 200) putchar('d');
             if (adc.val < -200) putchar('a');
             continue;
         }
         if (adc.axis == 3) {
             if (adc.val > 200) putchar('j');
             if (adc.val < -200) putchar('b');
             continue;
         }
     }
 }
 
 void btn_callback(uint gpio, uint32_t events) {
     uint8_t axis;
     if (gpio == BTN_LIGA)      axis = 4;
     else if (gpio == BTN_TROCA) axis = 5;
     else if (gpio == BTN_ATIRA) axis = 6;
     else if (gpio == BTN_PIKA)  axis = 7;
     else if (gpio == BTN_PAUSA) axis = 8;
     else return;
 
     if (axis == 4) {
         if (events == GPIO_IRQ_EDGE_FALL) {
             adc_t ev = { .axis = 4, .val = 1 };
             xQueueSendFromISR(xQueueAdc, &ev, NULL);
         }
         return;
     }
 
     if (events == GPIO_IRQ_EDGE_FALL) {
         adc_t ev = { .axis = axis, .val = 1 };
         xQueueSendFromISR(xQueueAdc, &ev, NULL);
     } else if (events == GPIO_IRQ_EDGE_RISE) {
         adc_t ev = { .axis = axis, .val = 0 };
         xQueueSendFromISR(xQueueAdc, &ev, NULL);
     }
 }
 
 int main() {
     stdio_init_all();
 
     gpio_init(LED_LIGA);
     gpio_set_dir(LED_LIGA, GPIO_OUT);
     gpio_put(LED_LIGA, 0);
 
     gpio_init(VIBRADOR);
     gpio_set_dir(VIBRADOR, GPIO_OUT);
     gpio_put(VIBRADOR, 0);
 
     xQueueAdc        = xQueueCreate(32, sizeof(adc_t));
     xEnableSemaphore = xSemaphoreCreateBinary();
     xVibeSemaphore   = xSemaphoreCreateBinary();
 
     gpio_init(BTN_LIGA);
     gpio_set_dir(BTN_LIGA, GPIO_IN);
     gpio_pull_up(BTN_LIGA);
     gpio_set_irq_enabled_with_callback(
         BTN_LIGA,
         GPIO_IRQ_EDGE_FALL,
         true,
         &btn_callback
     );
 
     gpio_init(BTN_TROCA);
     gpio_set_dir(BTN_TROCA, GPIO_IN);
     gpio_pull_up(BTN_TROCA);
     gpio_set_irq_enabled_with_callback(
         BTN_TROCA,
         GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
         true,
         &btn_callback
     );
 
     gpio_init(BTN_ATIRA);
     gpio_set_dir(BTN_ATIRA, GPIO_IN);
     gpio_pull_up(BTN_ATIRA);
     gpio_set_irq_enabled_with_callback(
         BTN_ATIRA,
         GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
         true,
         &btn_callback
     );
 
     gpio_init(BTN_PIKA);
     gpio_set_dir(BTN_PIKA, GPIO_IN);
     gpio_pull_up(BTN_PIKA);
     gpio_set_irq_enabled_with_callback(
         BTN_PIKA,
         GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
         true,
         &btn_callback
     );
 
     gpio_init(BTN_PAUSA);
     gpio_set_dir(BTN_PAUSA, GPIO_IN);
     gpio_pull_up(BTN_PAUSA);
     gpio_set_irq_enabled_with_callback(
         BTN_PAUSA,
         GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
         true,
         &btn_callback
     );
 
     adc_init();
 
     xTaskCreate(adc_x_task,   "adc_x_task",   4095, NULL, 1, NULL);
     xTaskCreate(adc_y_task,   "adc_y_task",   4095, NULL, 1, NULL);
     xTaskCreate(desh_y_task,  "desh_y_task",  4095, NULL, 1, NULL);
     xTaskCreate(uart_task,    "uart_task",    4095, NULL, 1, NULL);
     xTaskCreate(led_task,     "led_task",     256,  NULL, 1, NULL);
     xTaskCreate(vibrate_task, "vibrate_task", 256,  NULL, 1, NULL);
 
     vTaskStartScheduler();
 
     while (1) ;
 }
 