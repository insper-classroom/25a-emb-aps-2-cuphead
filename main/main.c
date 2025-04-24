/*
 * LED blink with FreeRTOS e filtro média móvel + interrupção BTN_LIGA
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>

const int BTN_LIGA = 15; //branco
const int BTN_TROCA = 13; //azul
const int BTN_ATIRA = 11; //verde
const int BTN_PIKA = 14; //vermelho
const int BTN_PAUSA = 12; //amarelo
QueueHandle_t xQueueAdc;

typedef struct adc {
    int axis;
    int val;  
} adc_t;    

void adc_x_task(void *p) {
    int dataList[5] = {0};
    int index = 0;
    int sum = 0;
    int count = 0;
    
    while (1) {
        adc_init();
        adc_gpio_init(27);
        adc_select_input(1);
        
        uint16_t result = adc_read();
        int16_t raw_value = (result - 2047) / 7.96;
        if (raw_value > -30 && raw_value < 30) {
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
        
        adc_t adc;
        adc.axis = 0;  
        adc.val = mediamovel;
        if (adc.val != 0){
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
    
    while (1) {
        adc_init();
        adc_gpio_init(26);
        adc_select_input(0);
        
        uint16_t result = adc_read();
        int16_t raw_value = (result - 2047) / 7.96;
        if (raw_value > -30 && raw_value < 30) {
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
        
        adc_t adc;
        adc.axis = 1;  
        adc.val = mediamovel;
        if (adc.val != 0){
            xQueueSend(xQueueAdc, &adc, 0);

        }        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void desh_y_task(void *p){
    int dataList[5] = {0};
    int index = 0;
    int sum = 0;
    int count = 0;
    while(1){
        adc_init();
        adc_gpio_init(28); //dps colocar o numero q agt escolher aqui
        adc_select_input(2); //input vai depender daquela tabela

        uint16_t result = adc_read();
        int16_t raw_value = (result-2047)/7.96;
        printf("analog2: %d", raw_value);
        if (raw_value > -200 && raw_value < 200){
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
        
        adc_t adc;
        adc.axis = 3;  
        adc.val = mediamovel;
        if (adc.val > 200){
            xQueueSend(xQueueAdc, &adc, 0);

        }        
        vTaskDelay(pdMS_TO_TICKS(50));


    }
}

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


void btn_liga_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // borda de descida
    } else if (events == 0x8) { // borda de subida
    }
}

void btn_troca_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // borda de descida
    } else if (events == 0x8) { // borda de subida
    }
}

void btn_atira_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // borda de descida
    } else if (events == 0x8) { // borda de subida
    }
}

void btn_pika_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // borda de descida
    } else if (events == 0x8) { // borda de subida
    }
}

void btn_pausa_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // borda de descida
    } else if (events == 0x8) { // borda de subida
    }
}

int main() {
    stdio_init_all();

    // Configuração do botão BTN_LIGA com interrupção
    gpio_init(BTN_LIGA);
    gpio_set_dir(BTN_LIGA, GPIO_IN);
    gpio_pull_up(BTN_LIGA);

    gpio_init(BTN_TROCA);
    gpio_set_dir(BTN_TROCA, GPIO_IN);
    gpio_pull_up(BTN_TROCA);

    gpio_init(BTN_ATIRA);
    gpio_set_dir(BTN_ATIRA, GPIO_IN);
    gpio_pull_up(BTN_ATIRA);

    gpio_init(BTN_PIKA);
    gpio_set_dir(BTN_PIKA, GPIO_IN);
    gpio_pull_up(BTN_PIKA);

    gpio_init(BTN_PAUSA);
    gpio_set_dir(BTN_PAUSA, GPIO_IN);
    gpio_pull_up(BTN_PAUSA);

    gpio_set_irq_enabled_with_callback(
        BTN_LIGA,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true,
        &btn_liga_callback
    );

    gpio_set_irq_enabled_with_callback(
        BTN_TROCA,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true,
        &btn_troca_callback
    );
    gpio_set_irq_enabled_with_callback(
        BTN_ATIRA,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true,
        &btn_atira_callback
    );
    gpio_set_irq_enabled_with_callback(
        BTN_PIKA,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true,
        &btn_pika_callback
    );
    gpio_set_irq_enabled_with_callback(
        BTN_PAUSA,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true,
        &btn_pausa_callback
    );


    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(adc_x_task, "adc x task", 4095, NULL, 1, NULL);
    xTaskCreate(adc_y_task, "adc y task", 4095, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart task", 4095, NULL, 1, NULL);
    xTaskCreate(desh_y_task, "adc2 y task", 4095, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true) 
    ;
}
