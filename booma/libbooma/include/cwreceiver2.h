#ifndef __CWRECEIVER2_H
#define __CWRECEIVER2_H

#include <hardtapi.h>

#include "booma.h"
#include "config.h"
#include "receiver.h"
#include "input.h"

class BoomaCwReceiver2 : public BoomaReceiver {

    private:

        bool _enableProbes;

        HProbe<int16_t>* _passthroughProbe;
        HProbe<int16_t>* _highpassProbe;
        HProbe<int16_t>* _gainProbe;
        HProbe<int16_t>* _humfilterProbe;
        HProbe<int16_t>* _preselectProbe;
        HProbe<int16_t>* _agcProbe;
        HProbe<int16_t>* _ifMixerProbe;
        HProbe<int16_t>* _ifFilterProbe;
        HProbe<int16_t>* _beatToneMixerProbe;
        HProbe<int16_t>* _postSelectProbe;

        // Preprocessing
        HPassThrough<int16_t>* _passthrough;
        HBiQuadFilter<HHighpassBiQuad<int16_t>, int16_t>* _highpass;
        HGain<int16_t>* _gain;
        HCombFilter<int16_t>* _humfilter;

        // Receiver
        HBiQuadFilter<HBandpassBiQuad<int16_t>, int16_t>* _preselect;
        HAgc<int16_t>* _agc;
        HMultiplier<int16_t>* _ifMixer;
        HCascadedBiQuadFilter<int16_t>* _ifFilter;
        HMultiplier<int16_t>* _beatToneMixer;
        HCascadedBiQuadFilter<int16_t>* _postSelect;
        
        static float _bandpassCoeffs[3][20];
        static float _cwCoeffs[];

        HWriterConsumer<int16_t>* PreProcess(ConfigOptions* opts, HWriterConsumer<int16_t>* previous);
        HWriterConsumer<int16_t>* Receive(ConfigOptions* opts, HWriterConsumer<int16_t>* previous);
        HWriterConsumer<int16_t>* PostProcess(ConfigOptions* opts, HWriterConsumer<int16_t>* previous);

    public:

        void OptionChanged(std::string name, int value) {
            HLog("Option %s has changed to value %d", name.c_str(), value);
        }

    public:

        BoomaCwReceiver2(ConfigOptions* opts);
        ~BoomaCwReceiver2();

        bool SetFrequency(long int frequency);
        bool SetRfGain(int gain);
};

#endif
