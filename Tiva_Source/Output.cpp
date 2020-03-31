/*
 * Output.cpp
 *
 *  Created on: 28.03.2020
 *      Author: Max
 */

#include <Output.h>

Output::Output()
{
    // TODO Auto-generated constructor stub

}

Output::~Output()
{
    // TODO Auto-generated destructor stub
}

void Output::init(System* sys)
{
    outputSys = sys;

    SysCtlPeripheralEnable(OUTPUT_TIMER_SYSCTL_PERIPH);
    SysCtlPeripheralEnable(OUTPUT_TIMER_PORT_SYSCTL_PERIPH);
    SysCtlDelay(2);

    TimerConfigure(OUTPUT_TIMER_BASE, OUTPUT_TIMER_CONFIG);
    TimerClockSourceSet(OUTPUT_TIMER_BASE, TIMER_CLOCK_PIOSC);

    GPIOPinConfigure(OUTPUT_TIMER_PIN_CONFIG);
    GPIOPinTypeTimer(OUTPUT_TIMER_PORT_BASE, OUTPUT_TIMER_PIN);

    TimerControlLevel(OUTPUT_TIMER_BASE, TIMER_A, false);
    TimerUpdateMode(OUTPUT_TIMER_BASE, TIMER_A, TIMER_UP_LOAD_TIMEOUT | TIMER_UP_MATCH_TIMEOUT);
}

void Output::setMaxDuty(float maxDuty)
{
    outputDutyMaxUS = maxDuty * 1000000.0f;
}

void Output::setMaxDutyPerc(uint32_t maxDutyPerc)
{
    outputDutyMaxUS = float(maxDutyPerc) * 1000000.0f / 100.0f;
}

void Output::setMaxOntimeUS(float maxOntimeUS)
{
    outputMatchValueMax = float(outputSys->getPIOSCFreq() / 1000000) * maxOntimeUS;;
}

void Output::tone(float freq, float ontimeUS)
{
    if (freq < 0.5f || ontimeUS < 1.0f)
    {
        if (outputActive)
        {
            TimerPrescaleMatchSet(OUTPUT_TIMER_BASE, TIMER_A, (((outputLoadValueNew - 1) & 0xff0000) >> 16));
            TimerMatchSet(OUTPUT_TIMER_BASE, TIMER_A, ((outputLoadValueNew - 1) & 0xffff));
            while (GPIOPinRead(OUTPUT_TIMER_PORT_BASE, OUTPUT_TIMER_PIN));
            TimerDisable(OUTPUT_TIMER_BASE, TIMER_A);
            outputLoadValue = 0;
            outputMatchValue = 0;
            outputActive = false;
        }
    }
    else
    {
        if (freq * ontimeUS > outputDutyMaxUS)
        {
            ontimeUS = outputDutyMaxUS / freq;
        }
        outputLoadValueNew = float(outputSys->getPIOSCFreq()) / freq - 1;
        outputMatchValueNew  = float(outputSys->getPIOSCFreq() / 1000000) * ontimeUS;
        if (outputMatchValueNew > outputMatchValueMax)
        {
            outputMatchValueNew = outputMatchValueMax;
        }

        // Down-counters are confusing, I know... Try figuring out whether you should add/remove 1 or not in this step!
        outputMatchValueNew = outputLoadValueNew - outputMatchValueNew;
        if (outputLoadValue != outputLoadValueNew)
        {
            outputLoadValue = outputLoadValueNew;
            TimerPrescaleSet(OUTPUT_TIMER_BASE, TIMER_A, ((outputLoadValueNew & 0xff0000) >> 16));
            TimerLoadSet(OUTPUT_TIMER_BASE, TIMER_A, (outputLoadValueNew & 0xffff));
        }
        if (outputMatchValue != outputMatchValueNew)
        {
            outputMatchValue = outputMatchValueNew ;
            TimerPrescaleMatchSet(OUTPUT_TIMER_BASE, TIMER_A, ((outputMatchValueNew & 0xff0000) >> 16));
            TimerMatchSet(OUTPUT_TIMER_BASE, TIMER_A, (outputMatchValueNew & 0xffff));
        }
        if (!outputActive)
        {
            outputActive = true;
            TimerEnable(OUTPUT_TIMER_BASE, TIMER_A);
        }
    }
}
