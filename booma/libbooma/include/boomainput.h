#ifndef __INPUT_H
#define __INPUT_H

#include <hardtapi.h>
#include "configoptions.h"
#include "boomaexception.h"
#include "boomaprocessor.h"
#include "boomainputreader.h"
#include "booma.h"

class BoomaInput {

public:

    private:

        BoomaProcessor* _processor;

        BoomaInputReader* _inputReader;
        HWriter<int16_t>* _rfWriter;
        HSplitter<int16_t>* _rfSplitter;
        HBreaker<int16_t>* _rfBreaker;
        HBufferedWriter<int16_t>* _rfBuffer;
        HPassThrough<int16_t>* _passthrough;
        HGain<int16_t>* _rfGain;
        HIqMultiplier<int16_t>* _ifMultiplier;

        HProbe<int16_t>* _passthroughProbe;
        HProbe<int16_t>* _rfGainProbe;
        HProbe<int16_t>* _ifMultiplierProbe;

        int _virtualFrequency;
        int _hardwareFrequency;
        int _ifFrequency;

        bool SetInputReader(ConfigOptions* opts);
        bool SetReaderFrequencies(ConfigOptions *opts, int frequency);

    public:

        class BoomaInputException : public BoomaException {

            public:

                BoomaInputException(std::string reason) : BoomaException(reason) {}
                std::string Type() { return "BoomaInputException"; }
        };

        BoomaInput(ConfigOptions* opts, bool* isTerminated);
        ~BoomaInput();

        HWriterConsumer<int16_t>* GetLastWriterConsumer() {
            return _ifMultiplier != nullptr ? _ifMultiplier->Consumer() : _rfGain->Consumer();
        }

        void Run(int blocks = 0);

        void Halt();

        bool SetDumpRf(bool enabled);

        bool SetFrequency(ConfigOptions* opts, int frequency);

        int SetRfGain(int gain);

        int GetIfFrequency() {
            return _ifFrequency;
        }
};

#endif
