/**
 ******************************************************************************
 * @file    main.c
 * @author  Ac6
 * @version V1.0
 * @date    01-December-2013
 * @brief   Default main function.
 ******************************************************************************
 */
#include <Arduino.h>
#include "app_hal.h"
#include "stock_price_gui.h"
#include <appController.h>
#include <storage.h>


void setup()
{
    storage_init(); // Initialize storage for persistent data
    Serial.begin(115200);
   

    hal_setup();
    gui_code_init();
    app_controller_init();
  
    Serial.println("Setup done!");

}

void loop()
{
    hal_loop(); /* Do not use while loop in this function */
    app_controller_run();
}


