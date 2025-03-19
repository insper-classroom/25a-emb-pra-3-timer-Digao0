/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #include <stdio.h>
 #include "pico/stdlib.h"
 #include "hardware/gpio.h"
 #include "hardware/rtc.h"
 #include "pico/util/datetime.h"
 
 const int TRIG = 16;
 const int ECHO = 18;
 volatile absolute_time_t start_time, end_time;
 volatile bool measuring = false;
 volatile bool valid_pulse = false;
 volatile bool estourou = false;
 
 int64_t press_timer_callback(alarm_id_t id, void *user_data) {
     estourou = true;
     return 0; 
 }
 
 void echo_irq_handler(uint gpio, uint32_t events) {
     if (events & GPIO_IRQ_EDGE_RISE) {
         start_time = get_absolute_time();  
     } else if (events & GPIO_IRQ_EDGE_FALL) {
         end_time = get_absolute_time();  
         measuring = false;
         valid_pulse = true;
     }
 }
 
 double calcula_distancia(){
     valid_pulse = false;  // reset
     measuring = true;
     estourou = false;
 
     gpio_put(TRIG, 1); //pulso no trig
     sleep_us(10);
     gpio_put(TRIG, 0);
     add_alarm_in_ms(38, press_timer_callback, NULL, false);
 
     //absolute_time_t timeout = make_timeout_time_ms(38);
     while (measuring && !estourou); 
 
     if (!valid_pulse) {
         printf("Falha\n");
         
         return -1.0;
     }
 
     
     int64_t pulse_time = absolute_time_diff_us(start_time, end_time);
 
     // distncia em cm
     double distance = (pulse_time * 0.0343) / 2;
     return distance;
 
 
 } 
 
 void print_datetime() {
     datetime_t t;
     rtc_get_datetime(&t);
     printf("[%04d-%02d-%02d %02d:%02d:%02d] ",
            t.year, t.month, t.day, t.hour, t.min, t.sec);
 }
 
 int main() {
     stdio_init_all();
 
     
     datetime_t t = {
         .year  = 2025,
         .month = 3,
         .day   = 19,
         .dotw  = 2, // Dia da semana (0=domingo, 1=segunda, etc.)
         .hour  = 12,
         .min   = 0,
         .sec   = 0
     };
 
     rtc_init();
     rtc_set_datetime(&t);
 
     gpio_init(TRIG);
     gpio_init(ECHO);
     gpio_set_dir(TRIG, GPIO_OUT);
     gpio_put(TRIG, 0);
     gpio_set_dir(ECHO, GPIO_IN);
     gpio_set_irq_enabled_with_callback(ECHO, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &echo_irq_handler);
 
     
     bool running = false;
 
     printf("Digite 's' para iniciar e 'p' para parar:\n");
 
     while (true){
         int command = getchar_timeout_us (10000);
 
         if (command == 's') {
             running = true;
             printf("Medição iniciada...\n");
         } else if (command == 'p') {
             running = false;
             printf("Medição parada.\n");
         }
 
         while (running) {
 
             command = getchar_timeout_us(0); // interrompe
             if (command == 'p') {
                 running = false;
                 printf("Medição parada.\n");
                 break; 
             }
 
 
             float distance = calcula_distancia();
             if (distance >= 0) {
                print_datetime(); 
                printf("Distância: %.2f cm\n", distance);
             }
             sleep_ms(500);
         }
 
     }
 
 }
 