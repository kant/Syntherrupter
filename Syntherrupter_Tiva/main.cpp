/*
 * main.cpp
 *
 *  Created on: 26.03.2020
 *      Author: Max Zuidberg
 */


#include <stdint.h>
#include <stdbool.h>
#include "InterrupterConfig.h"
#include "System.h"
#include "GUI.h"


Coil coils[COIL_COUNT];


void sysTickISR()
{
    System::systemTimeIncrement();
}

void uartUsbISR()
{
    MIDI::usbUart.ISR();
}

void uartMidiISR()
{
    MIDI::midiUart.ISR();
}


int main(void)
{
    System::init(sysTickISR);
    System::setSystemTimeResUS(16);
    GUI::init();

    // Initialize Coil objects
    for (uint32_t coil = 0; coil < COIL_COUNT; coil++)
    {
        coils[coil].init(coil);
    }
    MIDI::init(115200, uartUsbISR, GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_5, uartMidiISR);

    while (42)
    {
        uint32_t state = GUI::update();

        if (state)
        {
            MIDI::process();

            switch (COIL_COUNT)
            {
                case 6:
                    coils[5].updateData();
                    coils[5].updateOutput();
                case 5:
                    coils[4].updateData();
                    coils[4].updateOutput();
                case 4:
                    coils[3].updateData();
                    coils[3].updateOutput();
                case 3:
                    coils[2].updateData();
                    coils[2].updateOutput();
                case 2:
                    coils[1].updateData();
                    coils[1].updateOutput();
                case 1:
                    coils[0].updateData();
                    coils[0].updateOutput();
                    break;
            }
            /*for (uint32_t coil = 0; coil < COIL_COUNT; coil++)
            {
                // Run non-static coil object methods
                coils[coil].update();
            }*/
        }
        else
        {
            // Emergency stop or something similar. Don't generate outputs
            // and delete all tones
            for (uint32_t coil = 0; coil < COIL_COUNT; coil++)
            {
                ToneList* tl = &(coils[coil].toneList);
                for (uint32_t tone = 0; tone < MAX_VOICES; tone++)
                {
                    tl->deleteTone(tl->firstTone);
                }
            }
        }
    }
}

