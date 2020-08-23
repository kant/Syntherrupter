/*
 * ToneList.cpp
 *
 *  Created on: 17.08.2020
 *      Author: Max Zuidberg
 */

#include <ToneList.h>

ToneList::ToneList()
{
    // To prevent excessive copy operations when ordering the tones,
    // tones only contains the pointers to the actual Tone objects.
    for (uint32_t tone = 0; tone < MAX_VOICES; tone++)
    {
        tones[tone] = &(unorderedTones[tone]);
    }
}

ToneList::~ToneList()
{
    // TODO Auto-generated destructor stub
}

void ToneList::removeDeadTones()
{
    uint32_t deadTones = 0;
    for (uint32_t tone = 0; tone < MAX_VOICES; tone++)
    {
        if (tone >= activeTones)
        {
            break;
        }
        if (!tones[tone]->ontimeUS || !tones[tone]->nextFireUS)
        {
            deadTones++;
            tones[tone]->owner      = 0;
            tones[tone]->origin     = 0;
            tones[tone]->nextFireUS = 0;
        }
        else if (deadTones)
        {
            Tone *temp              = tones[tone - deadTones];
            tones[tone - deadTones] = tones[tone];
            tones[tone]             = temp;
        }
    }
    activeTones -= deadTones;
}

uint32_t ToneList::available()
{
    return (MAX_VOICES - activeTones);
}

Tone* ToneList::updateTone(uint32_t ontimeUS, uint32_t periodUS, void* owner, void* origin, Tone* tone)
{
    Tone* targetTone = 0;
    if (tone != ((Tone*) 0))
    {
        if (tone->owner == owner)
        {
            targetTone = tone;
        }
    }
    else if (activeTones < maxVoices)
    {
        targetTone       = tones[activeTones++];
        targetTone->type = Tone::Type::newdflt;
    }
    if (targetTone)
    {
        targetTone->ontimeUS = ontimeUS;
        targetTone->owner    = owner;
        targetTone->origin   = origin;
        if (periodUS != targetTone->periodUS)
        {
            targetTone->periodUS = periodUS;
            targetTone->update(sys.getSystemTimeUS());
        }
        return targetTone;
    }
    return 0;
}

void ToneList::deleteTone(Tone* tone)
{
    if (tone != ((Tone*) 0))
    {
        tone->ontimeUS = 0;
        removeDeadTones();
    }
}

void ToneList::setMaxOntimeUS(float maxOntimeUS)
{
    this->maxOntimeUS = maxOntimeUS;
}

void ToneList::setMaxDuty(float maxDuty)
{
    this->maxDuty = maxDuty;
}

void ToneList::setMaxVoices(uint32_t maxVoices)
{
    if (maxVoices > MAX_VOICES)
    {
        maxVoices = MAX_VOICES;
    }
    this->maxVoices = maxVoices;
}

void ToneList::limit()
{
    float totalDuty = 0.0f;
    for (uint32_t tone = 0; tone < MAX_VOICES; tone++)
    {
        if (tone >= activeTones)
        {
            break;
        }
        float ontimeUS = tones[tone]->ontimeUS;
        if (ontimeUS > maxOntimeUS)
        {
            tones[tone]->ontimeUS = maxOntimeUS;
            ontimeUS = maxOntimeUS;
        }
        totalDuty += ontimeUS / float(tones[tone]->periodUS);
    }
    if (totalDuty > maxDuty)
    {
        // Duty of all notes together exceeds coil limit; reduce ontimes.

        // Factor by which ontimes must be reduced.
        totalDuty = maxDuty / totalDuty;
        for (uint32_t tone = 0; tone < MAX_VOICES; tone++)
        {
            if (tone >= activeTones)
            {
                break;
            }
            tones[tone]->ontimeUS *= totalDuty;
        }
    }
}

uint32_t ToneList::getOntimeUS(uint32_t timeUS)
{
    if (!timeUS)
    {
        timeUS = sys.getSystemTimeUS();
    }
    uint32_t highestOntimeUS = 0;
    for (uint32_t tone = 0; tone < MAX_VOICES; tone++)
    {
        if (tone >= activeTones)
        {
            break;
        }
        Tone *currentTone = tones[tone];
        if (timeUS >= currentTone->nextFireUS)
        {
            if (timeUS < currentTone->nextFireEndUS)
            {
                if (currentTone->ontimeUS > highestOntimeUS)
                {
                    highestOntimeUS = currentTone->ontimeUS;
                }
            }
            currentTone->update(timeUS);
        }
    }
    return highestOntimeUS;
}
