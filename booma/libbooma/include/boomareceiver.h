#ifndef __RECEIVER_H
#define __RECEIVER_H

#include <hardtapi.h>
#include "configoptions.h"
#include "boomainput.h"
#include "boomadecoder.h"
#include "option.h"

#include "boomareceiverexception.h"

class BoomaReceiver {

    private:

        // Input gain/agc
        int _gainValue;
        HAgc<int16_t>* _rfAgc;
        HProbe<int16_t>* _rfAgcProbe;

        // Major receiver hooks
        HWriterConsumer<int16_t>* _preProcess;
        HWriterConsumer<int16_t>* _receive;
        HWriterConsumer<int16_t>* _postProcess;
        HSplitter<int16_t>* _spectrum;
        HSplitter<int16_t>* _decoder;

        // RF spectrum reporting
        HFftOutput<int16_t>* _rfFft;
        HCustomWriter<HFftResults>* _rfFftWriter;
        int RfFftCallback(HFftResults* result, size_t length);
        HRectangularWindow<int16_t>* _rfFftWindow;
        double* _rfSpectrum;
        int _rfFftSize;
        int _rfSpectrumSize;

        // Audio spectrum reporting
        HFftOutput<int16_t>* _audioFft;
        HCustomWriter<HFftResults>* _audioFftWriter;
        int AudioFftCallback(HFftResults* result, size_t length);
        HRectangularWindow<int16_t>* _audioFftWindow;
        double* _audioSpectrum;
        int _audioFftSize;
        int _audioSpectrumSize;

        std::vector<Option> _options;

        int _frequency;

        bool _hasBuilded;

        bool SetOption(ConfigOptions* opts, std::string name, int value);

    protected:

        virtual std::string GetName() = 0;

        virtual HWriterConsumer<int16_t>* PreProcess(ConfigOptions* opts, HWriterConsumer<int16_t>* previous) = 0;
        virtual HWriterConsumer<int16_t>* Receive(ConfigOptions* opts, HWriterConsumer<int16_t>* previous) = 0;
        virtual HWriterConsumer<int16_t>* PostProcess(ConfigOptions* opts, HWriterConsumer<int16_t>* previous) = 0;

        virtual bool IsDataTypeSupported(InputSourceDataType datatype) = 0;

        void RegisterOption(Option option) {
            HLog("Registering option %s", option.Name.c_str());
            _options.push_back(option);
        }

        virtual void OptionChanged(ConfigOptions* opts, std::string name, int value) = 0;

        virtual bool SetInternalFrequency(ConfigOptions* opts, int frequency) = 0;

        /**
         * Base class for all Booma receivers
         *
         * @param opts Receiver options
         * @param initialFrequency The initial frequency being received on
         */
        BoomaReceiver(ConfigOptions* opts, int initialFrequency):
            _hasBuilded(false),
            _frequency(initialFrequency),
            _rfAgc(nullptr),
            _rfAgcProbe(nullptr),
            _spectrum(nullptr),
            _rfFft(nullptr),
            _rfFftWindow(nullptr),
            _rfFftWriter(nullptr),
            _rfSpectrum(nullptr),
            _rfFftSize(1024),
            _audioFft(nullptr),
            _audioFftWindow(nullptr),
            _audioFftWriter(nullptr),
            _audioSpectrum(nullptr),
            _audioFftSize(256) {

            HLog("Creating BoomaReceiver with initial frequency %d", _frequency);

            _rfSpectrumSize = opts->GetInputSourceDataType() == REAL_INPUT_SOURCE_DATA_TYPE
                    ? _rfFftSize / 2
                    : _rfFftSize;
            _rfSpectrum = new double[_rfSpectrumSize];
            memset((void*) _rfSpectrum, 0, sizeof(double) * _rfFftSize / 2);

            _audioSpectrumSize = _audioFftSize / 2;
            _audioSpectrum = new double[_audioSpectrumSize];
            memset((void*) _audioSpectrum, 0, sizeof(double) * _audioFftSize / 2);

        }

        int GetRfAgcLevel(ConfigOptions* opts);

    public:

        virtual ~BoomaReceiver() {
            SAFE_DELETE(_rfAgc);
            SAFE_DELETE(_rfAgcProbe);
            SAFE_DELETE(_spectrum);
            SAFE_DELETE(_rfFft);
            SAFE_DELETE(_rfFftWriter);
            SAFE_DELETE(_rfFftWindow);
            SAFE_DELETE(_rfSpectrum);
            SAFE_DELETE(_audioFft);
            SAFE_DELETE(_audioFftWriter);
            SAFE_DELETE(_audioFftWindow);
            SAFE_DELETE(_audioSpectrum);
        }

        virtual int GetOutputFilterWidth() {
            return 4000;
        };

        std::vector<Option>* GetOptions() {
            return &_options;
        }

        int GetOption(std::string name);

        bool SetOption(ConfigOptions* opts, std::string name, std::string value);

        virtual std::string GetOptionInfoString() = 0;

        void Build(ConfigOptions* opts, BoomaInput* input, BoomaDecoder* decoder = NULL);

        bool SetFrequency(ConfigOptions* opts, int frequency) {
            _frequency = frequency;
            return SetInternalFrequency(opts, _frequency);
        }

        int GetFrequency() {
            return _frequency;
        }

        virtual long GetDefaultFrequency(ConfigOptions* opts) = 0;
        virtual bool IsFrequencySupported(ConfigOptions* opts, long frequency) = 0;

        HWriterConsumer<int16_t>* GetLastWriterConsumer() {
            return _decoder->Consumer();
        }

        HWriterConsumer<int16_t>* GetSpectrumConsumer() {
            return _spectrum->Consumer();
        }

        int SetRfGain(int gain);

        int GetRfGain() {
            return _gainValue;
        }

        float GetRfAgcCurrentGain() {
            return _rfAgc->GetGain();
        }

        int GetRfSpectrum(double* spectrum);
        int GetRfFftSize();
        int GetAudioFftSize();
        int GetAudioSpectrum(double* spectrum);
};

#endif
