/*
 * GUI.cpp
 *
 *  Created on: 26.03.2020
 *      Author: Max
 */

#include <GUI.h>

GUI::GUI()
{
    // TODO Auto-generated constructor stub

}

GUI::~GUI()
{
    // TODO Auto-generated destructor stub
}

void GUI::midiUartISR()
{
    // Read and clear the asserted interrupts
    volatile uint32_t uartIntStatus;
    uartIntStatus = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, uartIntStatus);

    /*while (UARTCharsAvail(UART0_BASE))
    {
        guiMidi.newData(UARTCharGet(UART0_BASE));
    }*/
}

void GUI::init(System* sys, void (*midiISR)(void), void(*oneshotISRs[])(void))
{
    guiSys = sys;
    bool cfgInEEPROM = guiCfg.init(guiSys);
    guiNxt.init(guiSys, 3, 115200, guiNxtTimeoutUS);
    guiMidi.init(guiSys, 0, 115200, midiISR);
    for (uint32_t i = 0; i < COIL_COUNT; i++)
    {
        // As of now all coils share the same init settings. However this is
        // not mandatory.
        //coils[i].out.init(guiSys, i);
        coils[i].one.init(guiSys, i, oneshotISRs[i]);
        coils[i].filteredFrequency.init(guiSys, 1.8f, 5.0f);
        coils[i].filteredOntimeUS.init(guiSys, 2.0f, 30.0f);
    }

    // Try to modify and read a Nextion value. If this works we know the
    // Nextion is ready.
    while (guiNxt.getVal("comOk") != 1)
    {
        guiNxt.sendCmd("rest");
        guiSys->delayUS(700000);
        guiNxt.setVal("comOk", 1);
        guiSys->delayUS(10000);
    }

    //guiMode = nxtFWUpdate;

    /*
     * If there is a valid configuration in the EEPROM, load it and send it to
     * the Nextion GUI. Note: While the Nextion class has methods for setting
     * component values without knowing the Nextion command structure, it is
     * easier here to create the commands directly.
     * Data format is equal to the microcontroller command format and
     * documented in a separate file.
     */
    if (cfgInEEPROM)
    {
        // User 2 Settings defaults to the max coil settings.
        uint32_t allCoilsMaxOntimeUS = 0;
        uint32_t allCoilsMaxBPS      = 0;
        uint32_t allCoilsMaxDutyPerm = 0;

        // Settings of all coils
        const char *AllCoilSettings = "TC_Settings";
        for (uint32_t i = 0; i < COIL_COUNT; i++)
        {
            // Load settings
            uint32_t maxOntimeUS = guiCfg.getCoilsMaxOntimeUS(i);
            uint32_t maxBPS      = guiCfg.getCoilsMaxBPS(i);
            uint32_t maxDutyPerm = guiCfg.getCoilsMaxDutyPerm(i);

            if (maxOntimeUS > allCoilsMaxOntimeUS)
            {
                allCoilsMaxOntimeUS = maxOntimeUS;
            }
            if (maxBPS > allCoilsMaxBPS)
            {
                allCoilsMaxBPS = maxBPS;
            }
            if (maxDutyPerm > allCoilsMaxDutyPerm)
            {
                allCoilsMaxDutyPerm = maxDutyPerm;
            }

            // Apply to coil objects
            //coils[i].out.setMaxDutyPerm(maxDutyPerm);
            //coils[i].out.setMaxOntimeUS(maxOntimeUS);
            coils[i].one.setMaxOntimeUS(maxOntimeUS);

            // Send to Nextion
            guiNxt.printf("%s.coil%iOn.val=%i\xff\xff\xff",
                          AllCoilSettings, i + 1, maxOntimeUS);
            guiNxt.printf("%s.coil%iBPS.val=%i\xff\xff\xff",
                          AllCoilSettings, i + 1, maxBPS);
            guiNxt.printf("%s.coil%iDuty.val=%i\xff\xff\xff",
                          AllCoilSettings, i + 1, maxDutyPerm);
            // Give time to the UART to send the data
            guiSys->delayUS(10000);
        }

        // Settings of the 3 users
        const char *AllUsersPage = "User_Settings";
        for (uint32_t i = 0; i < 3; i++)
        {
            uint32_t maxOntimeUS = guiCfg.getUsersMaxOntimeUS(i);
            uint32_t maxBPS      = guiCfg.getUsersMaxBPS(i);
            uint32_t maxDutyPerm = guiCfg.getUsersMaxDutyPerm(i);

            if (i == 2)
            {
                maxOntimeUS = allCoilsMaxOntimeUS;
                maxBPS      = allCoilsMaxBPS;
                maxDutyPerm = allCoilsMaxDutyPerm;
            }

            guiNxt.printf("%s.u%iName.txt=\"%s\"\xff\xff\xff",
                          AllUsersPage, i, guiCfg.userNames[i]);
            guiNxt.printf("%s.u%iCode.txt=\"%s\"\xff\xff\xff",
                          AllUsersPage, i, guiCfg.userPwds[i]);
            guiNxt.printf("%s.u%iOntime.val=%i\xff\xff\xff",
                          AllUsersPage, i, maxOntimeUS);
            guiNxt.printf("%s.u%iBPS.val=%i\xff\xff\xff",
                          AllUsersPage, i, maxBPS);
            guiNxt.printf("%s.u%iDuty.val=%i\xff\xff\xff",
                          AllUsersPage, i, maxDutyPerm);
            // Give time to the UART to send the data
            guiSys->delayUS(10000);
        }

        // Other Settings
        uint32_t buttonHoldTime = guiCfg.otherSettings[0] & 0xffff;
        uint32_t sleepDelay     = (guiCfg.otherSettings[0] & 0xffff0000) >> 16;
        uint32_t dispBrightness = guiCfg.otherSettings[1] & 0xff;
        guiNxt.printf("Other_Settings.nHoldTime.val=%i\xff\xff\xff",
                      buttonHoldTime);
        guiNxt.printf("thsp=%i\xff\xff\xff", sleepDelay);
        guiNxt.printf("dim=%i\xff\xff\xff", dispBrightness);
    }
    guiNxt.setVal("TC_Settings.maxCoilCount", COIL_COUNT);

    // Initialization completed.
    guiNxt.sendCmd("vis pStartup,1");
}

void GUI::setError(const char* err)
{
    bool endOfErr = false;
    for (uint32_t i = 0; i < guiErrorLen; i++)
    {
        if (!endOfErr)
        {
            guiErrorTxt[i] = err[i];
            endOfErr = (err[i] == '\0');
        }
        else
        {
            guiErrorTxt[i] = '\0';
        }
    }
}

void GUI::showError()
{
    guiNxt.setPage("Error");
    guiNxt.setTxt("tInfo", guiErrorTxt);
}

bool GUI::checkValue(uint32_t val)
{
    if (val == guiNxt.receiveErrorVal)
    {
        setError("Wert unplausibel");
        return false;
    }
    if (val == guiNxt.receiveTimeoutVal)
    {
        setError("Zeitüberschreitung");
        return false;
    }
    return true;
}

bool GUI::update()
{
    // Used to detect timeouts
    uint32_t time = 0;

    // receive and store command
    if (guiNxt.charsAvail())
    {
        guiCommand = guiNxt.getChar();
        switch (guiCommand)
        {
            case 'm':
            {
                time = guiSys->getSystemTimeUS();
                while (guiNxt.charsAvail() < 2)
                {
                    if (guiSys->getSystemTimeUS() - time > guiNxtTimeoutUS)
                    {
                        setError("Data timeout");
                        showError();
                        guiSys->error();
                    }
                }
                char modeByte0 = guiNxt.getChar();
                char modeByte1 = guiNxt.getChar();
                if (modeByte0 == 'e' && modeByte1 == 'x')
                {
                    guiMode = exit;
                }
                else if (modeByte0 == 's' && modeByte1 == 'i')
                {
                    guiMode = simple;
                }
                else if (modeByte0 == 'm' && modeByte1 == 'l')
                {
                    guiMode = midiLive;
                }
                else if (modeByte0 == 'u' && modeByte1 == 's')
                {
                    guiMode = userSelect;
                }
                else if (modeByte0 == 'n' && modeByte1 == 'u')
                {
                    guiMode = nxtFWUpdate;
                }
                else if (modeByte0 == 's' && modeByte1 == 'e')
                {
                    guiMode = settings;
                }
                break;
            }
            case 'd':
            {
                time = guiSys->getSystemTimeUS();
                uint32_t i = 0;
                while (i < 6)
                {
                    if (guiNxt.charsAvail())
                    {
                        guiCommandData[i++] = guiNxt.getChar();
                        time = guiSys->getSystemTimeUS();
                    }
                    if (guiSys->getSystemTimeUS() - time > guiNxtTimeoutUS)
                    {
                        setError("Data timeout");
                        showError();
                        guiSys->error();
                    }
                }
                break;
            }
            case 'c':
            {
                time = guiSys->getSystemTimeUS();
                uint32_t i = 0;
                while (i < 3)
                {
                    if (guiNxt.charsAvail())
                    {
                        guiCommandData[i++] = guiNxt.getChar();
                        time = guiSys->getSystemTimeUS();
                    }
                    if (guiSys->getSystemTimeUS() - time > guiNxtTimeoutUS)
                    {
                        setError("Data timeout");
                        showError();
                        guiSys->error();
                    }
                }
                break;
            }
            case 'u':
            {
                // We need at least 1 additional byte.
                time = guiSys->getSystemTimeUS();
                while (!guiNxt.charsAvail())
                {
                    if (guiSys->getSystemTimeUS() - time > guiNxtTimeoutUS)
                    {
                        setError("Data timeout");
                        showError();
                        guiSys->error();
                    }
                }
                guiCommandData[0] = guiNxt.getChar();

                // Now we can determine the amount of data that follows and wait for it.
                uint32_t cmdLength = (guiCommandData[0] & 0b00011111) + 1;
                uint32_t i = 1;
                while (i <= cmdLength)
                {
                    if (guiNxt.charsAvail())
                    {
                        guiCommandData[i++] = guiNxt.getChar();
                        time = guiSys->getSystemTimeUS();
                    }
                    if (guiSys->getSystemTimeUS() - time > guiNxtTimeoutUS)
                    {
                        setError("Data timeout");
                        showError();
                        guiSys->error();
                    }
                }
                break;
            }
            default:
            {
                guiCommand = 0;
            }
        }
    }

    // operation
    switch (guiMode)
    {
        case userSelect:
        {
            if (guiCommand == 'd')
            {
                uint32_t user = guiCommandData[0];
                if (user < 2)
                {
                    guiUserMaxOntimeUS = guiCfg.getUsersMaxOntimeUS(user);
                    guiUserMaxBPS      = guiCfg.getUsersMaxBPS(user);
                    guiUserMaxDutyPerm = guiCfg.getUsersMaxDutyPerm(user);
                }
                // Data applied; clear command byte.
                guiCommand = 0;
            }
            guiMode = idle;
        }

        case simple:
        {
            // Apply new data
            if (guiCommand == 'd')
            {
                for (uint32_t i = 0; i < COIL_COUNT; i++)
                {
                    // check if current coil is affected by command. Skip otherwise
                    // Data format documented in separate file
                    if (guiCommandData[0] & (1 << i))
                    {
                        uint32_t ontimeUS  = (guiCommandData[2] << 8) + guiCommandData[1];
                        uint32_t frequency = (guiCommandData[4] << 8) + guiCommandData[3];
                        coils[i].filteredOntimeUS.setTarget(ontimeUS);
                        coils[i].filteredFrequency.setTarget(frequency);
                    }
                }
                // Data applied; clear command byte.
                guiCommand = 0;
            }
            // Apply outputs
            for (uint32_t i = 0; i < COIL_COUNT; i++)
            {
                float f = coils[i].filteredFrequency.getFiltered();
                float o = coils[i].filteredOntimeUS.getFiltered();
                //coils[i].out.tone(f, o);
            }
            break;
        }
        case midiLive:
        {
            // Apply new data
            // Data format documented in separate file
            if (guiCommand == 'c')
            {
                uint32_t coil = guiCommandData[0] - 1;
                if (coil < COIL_COUNT)
                {
                    uint32_t channels = (guiCommandData[2] << 8) + guiCommandData[1];
                    guiMidi.setChannels(coil, channels);
                }
                // Data applied; clear command byte.
                guiCommand = 0;
            }
            else if (guiCommand == 'd')
            {
                for (uint32_t i = 0; i < COIL_COUNT; i++)
                {
                    // check if current coil is affected by command. Skip otherwise
                    // Data format documented in separate file
                    if (guiCommandData[0] & (1 << i))
                    {
                        uint32_t ontimeUS  = (guiCommandData[2] << 8) + guiCommandData[1];
                        uint32_t dutyPerm = (guiCommandData[4] << 8) + guiCommandData[3];
                        uint32_t volMode = (guiCommandData[0] & (0b11 << 6)) >> 6;
                        guiMidi.setVolSettings(i, ontimeUS, dutyPerm, volMode);
                    }
                }
                // Data applied; clear command byte.
                guiCommand = 0;
            }
            if (!guiMidi.isEnabled())
            {
                guiMidi.enable();
                guiMidi.play();
            }
            guiMidi.process();
            for (uint32_t coil = 0; coil < COIL_COUNT; coil++)
            {
                // TODO Feed outputs
                uint32_t time = guiSys->getExactSystemTimeUS();
                if (time - coils[coil].lastFiredUS > coils[coil].minOffUS)
                {
                    for (uint32_t note = 0; note < guiMidi.activeNotes[coil]; note++)
                    {
                        if ((time % guiMidi.orderedNotes[coil][note]->periodUS) < guiMidi.orderedNotes[coil][note]->halfPeriodUS)
                        {
                            if (!guiMidi.orderedNotes[coil][note]->fired)
                            {
                                guiMidi.orderedNotes[coil][note]->fired = true;
                                coils[coil].one.shot(guiMidi.orderedNotes[coil][note]->finishedOntimeUS);
                                coils[coil].lastFiredUS = time + guiMidi.orderedNotes[coil][note]->finishedOntimeUS;
                            }
                        }
                        else
                        {
                            guiMidi.orderedNotes[coil][note]->fired = false;
                        }
                    }
                }
            }
            break;
        }
        case settings:
        {
            if (guiCommand == 'd')
            {
                // Coil, user or other settings changed.
                // Data format documented in separate file.
                uint32_t settings = (guiCommandData[0] & (0b11 << 6)) >> 6;
                uint32_t number =    guiCommandData[0] & 0b11111;
                uint32_t data   =   (guiCommandData[4] << 24)
                                  + (guiCommandData[3] << 16)
                                  + (guiCommandData[2] << 8)
                                  +  guiCommandData[1];
                if (settings == 1 && (number - 1) < COIL_COUNT)
                {
                    // Coil limits. Number ranges from 1-6 instead of 0-5.
                    number--;
                    guiCfg.coilSettings[number] = data;
                    //coils[number].out.setMaxDutyPerm(guiCfg.getCoilsMaxDutyPerm(number));
                    //coils[number].out.setMaxOntimeUS(guiCfg.getCoilsMaxOntimeUS(number));
                    coils[number].one.setMaxOntimeUS(guiCfg.getCoilsMaxOntimeUS(number));
                }
                else if (settings == 0 && number < 3)
                {
                    // User limits.
                    guiCfg.userSettings[number] = data;
                    guiUserMaxOntimeUS = guiCfg.getUsersMaxOntimeUS(number);
                    guiUserMaxBPS      = guiCfg.getUsersMaxBPS(number);
                    guiUserMaxDutyPerm = guiCfg.getUsersMaxDutyPerm(number);
                }
                else if (settings == 2 && number < 10)
                {
                    guiCfg.otherSettings[number] = data;
                }
                // Data applied; clear command byte.
                guiCommand = 0;
            }
            else if (guiCommand == 'u')
            {
                uint32_t length = (guiCommandData[0] & 0b00011111) + 1;
                uint32_t user   = (guiCommandData[0] & 0b11000000) >> 6;
                bool     pwd    =  guiCommandData[0] & 0b00100000;
                if (pwd)
                {
                    // Data contains user password
                    for (uint32_t i = 0; i < length; i++)
                    {
                        guiCfg.userPwds[user][i] = guiCommandData[i + 1];
                    }
                    guiCfg.userPwds[user][length] = '\0';
                }
                else
                {
                    // Data contains user name
                    for (uint32_t i = 0; i < length; i++)
                    {
                        guiCfg.userNames[user][i] = guiCommandData[i + 1];
                    }
                    guiCfg.userNames[user][length] = '\0';
                }
                // Data applied; clear command byte.
                guiCommand = 0;
            }
            break;
        }
        case nxtFWUpdate:
        {
            // Stop normal operation and pass data between Nextion UART and USB UART
            // After upload is done (1 sec timeout) the uC performs a reset.

            // Stop Nextion UARTstdio operation
            guiNxt.disableStdio();

            // Stop all MIDI UARTs
            guiMidi.disable();
            // UARTs are supposed to be initialized.
            uint32_t nxtUARTBase = guiNxt.getUARTBase();
            uint32_t usbUARTBase = UART0_BASE;
            uint32_t baudRate = guiNxt.getBaudRate();
            // Re-init to make sure settings are correct
            // (We do assume they are already enabled. TODO)
            UARTConfigSetExpClk(nxtUARTBase, guiSys->getClockFreq(), baudRate,
                                (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
            UARTConfigSetExpClk(usbUARTBase, guiSys->getClockFreq(), baudRate,
                                (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
            UARTFIFOEnable(nxtUARTBase);
            UARTFIFOEnable(usbUARTBase);

            bool uploadStarted = false;
            uint32_t timeUS = guiSys->getExactSystemTimeUS();
            while (42)
            {
                if (UARTCharsAvail(nxtUARTBase))
                {
                    unsigned char c = UARTCharGet(nxtUARTBase);
                    UARTCharPutNonBlocking(usbUARTBase, c);
                }
                if (UARTCharsAvail(usbUARTBase))
                {
                    uploadStarted = true;
                    timeUS = guiSys->getSystemTimeUS();
                    unsigned char c = UARTCharGet(usbUARTBase);
                    UARTCharPutNonBlocking(nxtUARTBase, c);
                }
                if (uploadStarted && (guiSys->getSystemTimeUS() - timeUS) > 1000000)
                {
                    SysCtlReset();
                }
            }
        }
        case exit:
        {
            // Disable all outputs
            for (uint32_t i = 0; i < COIL_COUNT; i++)
            {
                guiMidi.disable();
                coils[i].filteredOntimeUS.setTarget(0.0f);
                //coils[i].out.tone(0.0f, 0.0f);
            }

            // Update EEPROM
            guiCfg.update();

            // Now we can enter idle
            guiMode = idle;
        }
        default: // includes idle
        {
            break;
        }
    }
    return true;
}
