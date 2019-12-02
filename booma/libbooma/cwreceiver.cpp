#include "cwreceiver.h"

// Narrow butterworth bandpass filters, bandwidth 100Hz or 60hz around 1000-1100 or 770-830. 4th. order, 4 biquads cascaded
// Removes (almost) anything but the mixed down signal at the center frequency
//
// Designed using http://www.micromodeler.com/dsp/
//
// Do not forget that the a1 and a2 coefficients should be multiplied by -1 in the input coefficients
// (thus saving some cycles in the filter)
// Narrow butterworth bandpass filter, bandwidth 100Hz around 1000-1100. 4th. order, 4 biquads cascaded
// Removes (almost) anything but the mixed down signal at the center frequency
//
// Designed using http://www.micromodeler.com/dsp/
float BoomaCwReceiver::_bandpassCoeffs[] =
{
    0.06318758096656742, -0.12637516193313483, 0.06318758096656742, 1.9822798942156608, -0.9928732078139422,// b0, b1, b2, a1, a2
    0.0625, -0.125, 0.0625, 1.986908612864604, -0.9971024098670853,// b0, b1, b2, a1, a2
    0.000244140625, 0.00048828125, 0.000244140625, 1.9814486970269125, -0.9926669567470747,// b0, b1, b2, a1, a2
    0.000244140625, 0.00048828125, 0.000244140625, 1.9851881609275535, -0.996895491657892// b0, b1, b2, a1, a2
};

#define CW_TONE_FREQUENCY 850

BoomaCwReceiver::BoomaCwReceiver(int samplerate, int frequency, int gain, HWriterConsumer<int16_t>* previous):
    BoomaReceiver(samplerate) {
    HLog("Creating CW receiver chain");

    // Add hum filter to remove 50Hz harmonics and the very lowest part of the spectrum (incl. 50Hz)
    // These components, which have very high levels, will completely botch the rest of the chain
    // if allowed through (50Hz input is here a.o., with an insanely high level)
    HLog("- HumFilter");
    _humFilter = new HHumFilter<int16_t>(previous, GetSampleRate(), 50, 600, BLOCKSIZE);

    // Increase signal strength after mixing to avoid losses before filtering and mixing
    HLog("- RF gain");
    _gain = new HGain<int16_t>(_humFilter->Consumer(), gain, BLOCKSIZE);

    // Highpass filter before mixing to remove some of the lowest frequencies that may
    // get mirrored back into the final frequency range and cause (more) distortion.
    // (In this receiver, the results are good when the cutoff frequency is located at the local oscillator frequency)
    HLog("- Preselect");
    _preselect = new HBiQuadFilter<HBandpassBiQuad<int16_t>, int16_t>(_gain->Consumer(), frequency, GetSampleRate(), 0.8071f, 1, BLOCKSIZE);

    // Mix down to the output frequency.
    // 17200Hz - 16160Hz = 1040Hz  (place it somewhere inside the bandpass filter pass region)
    HLog("- Mixer");
    _multiplier = new HMultiplier<int16_t>(_preselect->Consumer(), GetSampleRate(), frequency - CW_TONE_FREQUENCY, BLOCKSIZE);

    // Narrow butterworth bandpass filter, bandwidth 100Hz around 1000-1100. 4th. order, 4 biquads cascaded
    HLog("- Bandpass");
    _bandpass = new HCascadedBiQuadFilter<int16_t>(_multiplier->Consumer(), _bandpassCoeffs, 20, BLOCKSIZE);

    // General lowpass filtering after mixing down to IF
    HLog("- Lowpass");
    _lowpass = new HBiQuadFilter<HLowpassBiQuad<int16_t>, int16_t>(_bandpass->Consumer(), 1000, GetSampleRate(), 0.7071f, 1, BLOCKSIZE);
}

bool BoomaCwReceiver::SetFrequency(long int frequency) {

    // This receiver only operates from 820 - samplerate/2
    if( frequency >= GetSampleRate() / 2 || frequency <= CW_TONE_FREQUENCY ) {
        HError("Unsupported frequency %ld, must be greater than  %d and less than %d", frequency, CW_TONE_FREQUENCY, GetSampleRate() / 2);
        return false;
    }

    // Set new multiplier frequency and adjust the preselect bandpass filter
    _multiplier->SetFrequency(frequency - CW_TONE_FREQUENCY);
    ((HBiQuadFilter<HBandpassBiQuad<int16_t>, int16_t>*) _preselect)->SetCoefficients(frequency, GetSampleRate(), 0.8071f, 1, BLOCKSIZE);

    // Ready
    return true;
}

bool BoomaCwReceiver::SetRfGain(int gain) {

    // Sane gain values are between 1 and 100
    if( gain < 1 || gain > 100 ) {
        HError("Unsupported rf gain value %d, must be between 1 and 100", gain);
        return false;
    }

    // Set rf gain
    _gain->SetGain(gain);

    // Done
    return true;
}