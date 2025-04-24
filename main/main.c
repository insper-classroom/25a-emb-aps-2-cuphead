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
 
 const int BTN_LIGA  = 15; // branco
 const int BTN_TROCA = 13; // azul
 const int BTN_ATIRA = 11; // verde
 const int BTN_PIKA  = 14; // vermelho
 const int BTN_PAUSA = 12; // amarelo
 
 QueueHandle_t xQueueAdc;
 
 typedef struct {
     int axis;
     int val;
 } adc_t;
 
 // Task X (ADC channel 1 on GPIO 27)
 void adc_x_task(void *p) {
     int dataList[5] = {0};
     int index = 0;
     int sum = 0;
     int count = 0;
     adc_gpio_init(27);

     while (1) {
         adc_select_input(1);
         uint16_t result = adc_read();
         int16_t raw_value = (result - 2047) / 7.96;
         if (raw_value > -90 && raw_value < 90) {
          raw_value = 0;
         }
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
 
         adc_t adc = { .axis = 0, .val = mediamovel };
         if (adc.val != 0) {
             xQueueSend(xQueueAdc, &adc, 0);
         }
 
         vTaskDelay(pdMS_TO_TICKS(50));
     }
 }
 
 void adc_y_task(void *p) {
     int dataList[5] = {0};
     int index = 0;
     int sum = 0;
     int count = 0;
     adc_gpio_init(26);

     while (1) {

         adc_select_input(0);
         uint16_t result = adc_read();
         int16_t raw_value = (result - 2047) / 7.96;
         if (raw_value > -90 && raw_value < 90) {
            raw_value = 0;
         }
 
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
 
         if (mediamovel < -90 && mediamovel > 90) {
             adc_t adc = { .axis = 1, .val = mediamovel };
             if (adc.val != 0) {
                 xQueueSend(xQueueAdc, &adc, 0);
             }
         }
         vTaskDelay(pdMS_TO_TICKS(50));
     }
 }
 
 // Task Deslizador Y (ADC channel 2 on GPIO 28)
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
 
         adc_t adc = { .axis = 3, .val = mediamovel };
         if (adc.val > 200) {
             xQueueSend(xQueueAdc, &adc, 0);
         }
 
         vTaskDelay(pdMS_TO_TICKS(50));
     }
 }
 
 // UART task: envia valores da fila pela porta serial
 void uart_task(void *p) {
     adc_t adc;
     while (1) {
         if (xQueueReceive(xQueueAdc, &adc, portMAX_DELAY)) {
             int16_t val = adc.val;
             uint8_t axis = adc.axis;
             uint8_t val_0 = val & 0xFF;
             uint8_t val_1 = (val >> 8) & 0xFF;
             uint8_t sync = 0xFF;
             putchar(sync);
             putchar(axis);
             putchar(val_0);
             putchar(val_1);
         }
     }
 }
 
 // Callback único para os 5 botões
 void btn_callback(uint gpio, uint32_t events) {
     uint8_t axis;
     if (gpio == BTN_LIGA)      axis = 4;
     else if (gpio == BTN_TROCA) axis = 5;
     else if (gpio == BTN_ATIRA) axis = 6;
     else if (gpio == BTN_PIKA)  axis = 7;
     else if (gpio == BTN_PAUSA) axis = 8;
     else return;
 
     if (events == GPIO_IRQ_EDGE_FALL) {       // borda de descida
         adc_t adc = { .axis = axis, .val = 1 };
         xQueueSendFromISR(xQueueAdc, &adc, NULL);
     }
     else if (events == GPIO_IRQ_EDGE_RISE) {  // borda de subida
         adc_t adc = { .axis = axis, .val = 0 };
         xQueueSendFromISR(xQueueAdc, &adc, NULL);
     }
 }
 
 int main() {
     stdio_init_all();
     xQueueAdc = xQueueCreate(32, sizeof(adc_t));
 
     // Configurações individuais dos botões
     gpio_init(BTN_LIGA);
     gpio_set_dir(BTN_LIGA, GPIO_IN);
     gpio_pull_up(BTN_LIGA);
     gpio_set_irq_enabled_with_callback(
         BTN_LIGA,
         GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
         true,
         &btn_callback
     );
 
     gpio_init(BTN_TROCA);
     gpio_set_dir(BTN_TROCA, GPIO_IN);
     gpio_pull_up(BTN_TROCA);
     gpio_set_irq_enabled_with_callback(
         BTN_TROCA,
         GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
         true,
         &btn_callback
     );
 
     gpio_init(BTN_ATIRA);
     gpio_set_dir(BTN_ATIRA, GPIO_IN);
     gpio_pull_up(BTN_ATIRA);
     gpio_set_irq_enabled_with_callback(
         BTN_ATIRA,
         GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
         true,
         &btn_callback
     );
 
     gpio_init(BTN_PIKA);
     gpio_set_dir(BTN_PIKA, GPIO_IN);
     gpio_pull_up(BTN_PIKA);
     gpio_set_irq_enabled_with_callback(
         BTN_PIKA,
         GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
         true,
         &btn_callback
     );
 
     gpio_init(BTN_PAUSA);
     gpio_set_dir(BTN_PAUSA, GPIO_IN);
     gpio_pull_up(BTN_PAUSA);
     gpio_set_irq_enabled_with_callback(
         BTN_PAUSA,
         GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
         true,
         &btn_callback
     );
     adc_init();

     // Criação das tarefas
     xTaskCreate(adc_x_task,   "adc x task",  4095, NULL, 1, NULL);
     xTaskCreate(adc_y_task,   "adc y task",  4095, NULL, 1, NULL);
     xTaskCreate(uart_task,    "uart task",   4095, NULL, 1, NULL);
     xTaskCreate(desh_y_task,  "adc2 y task", 4095, NULL, 1, NULL);
 
     vTaskStartScheduler();
 
     while (1) ;
 }
 