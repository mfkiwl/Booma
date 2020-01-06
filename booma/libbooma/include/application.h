#ifndef __APPLICATION_H
#define __APPLICATION_H

#include <thread>

#include <hardtapi.h>

#include "config.h"
#include "input.h"
#include "receiver.h"
#include "output.h"
#include "booma.h"

class BoomaApplication {

    public:

        // constructor and destructor
        BoomaApplication(std::string appName, std::string appVersion, int argc, char** argv);
        ~BoomaApplication();

        // Run receiver chain
        void Run() {
	    HLog("Run receiver chain");
            _isTerminated = false;
            _current = new std::thread( [this]()  {
                if( _opts->GetEnableProbes() ) {
                    this->_input->GetProcessor()->Run(100);
                } else {
                    this->_input->GetProcessor()->Run();
                }
                _isTerminated = true;
                _isRunning = false;
                _current = NULL;
            } );
            _isRunning = true;
            HLog("Receiver chain is running");
        }

        // Halt receiver chain
        void Halt(bool wait = true) {
	    HLog("Halt receiver chain");
            _isTerminated = true;
            if( wait )
            {
	            HLog("Wait for receiver chain to halt");
                if( _current != NULL ) {
 		            HLog("Joining active thread");
                    _current->join();
                    _current = NULL;
                    HLog("Thread joined");
                }

		        HLog("Resetting running state");
                _isTerminated = false;
                _isRunning = false;
            }
            HLog("Receiver chain is halted");
        }

        // Wait for the receiver chain to exit
        void Wait() {
            HLog("Waiting for active receiver chain to halt");
            if( _current != NULL ) {
	            HLog("Has active thread, joining");
                _current->join();
                _current = NULL;
                HLog("Active thread halted");
            }
            _isRunning = false;
            HLog("Active receiver chain has halted");
        }

        // Public control functions
        bool ChangeReceiver();
        bool ChangeReceiver(ReceiverModeType receiverModeType);
        bool SetFrequency(long int frequency);
        long int GetFrequency();
        bool ChangeFrequency(int stepSize);
        bool SetVolume(int volume);
        bool ChangeVolume(int stepSize);
        int GetVolume();
        bool ToggleDumpRf();
        bool GetDumpRf();
        bool ToggleDumpAudio();
        bool GetDumpAudio();
        bool SetRfGain(int gain);
        bool ChangeRfGain(int stepSize);
        int GetRfGain();
        int GetSignalLevel();
        int GetRfFftSize();
        int GetRfSpectrum(double* spectrum);
        int GetAudioFftSize();
        int GetAudioSpectrum(double* spectrum);
        int GetSampleRate() {
            return SAMPLERATE;
        }

    private:

        // Configuration and state
        ConfigOptions* _opts;
        bool _isTerminated;
        bool _isRunning;
        std::thread* _current;

        // Structural blocks
        BoomaInput* _input;
        BoomaReceiver* _receiver;
        BoomaOutput* _output;

        HGain<int16_t>* _outputWriter;
        HSoundcardWriter<int16_t>* _soundcardWriter;
        HNullWriter<int16_t>* _nullWriter;

        // Splitting audio and RF
        HWriter<int16_t>* _audioWriter;
        HSplitter<int16_t>* _audioSplitter;
        HMute<int16_t>* _audioMute;

        // Signal level reporting
        HSignalLevel<int16_t>* _signalLevel;
        HCustomWriter<HSignalLevelResult>* _signalLevelWriter;
        int SignalLevelCallback(HSignalLevelResult* result, size_t length);
        int _signalStrength;

        // RF spectrum reporting
        HFft<int16_t>* _rfFft;
        HCustomWriter<HFftResults>* _rfFftWriter;
        int RfFftCallback(HFftResults* result, size_t length);
        HRectangularWindow<int16_t>* _rfFftWindow;
        double* _rfSpectrum;
        int _rfFftSize;

        // Audio spectrum reporting
        HFft<int16_t>* _audioFft;
        HCustomWriter<HFftResults>* _audioFftWriter;
        int AudioFftCallback(HFftResults* result, size_t length);
        HRectangularWindow<int16_t>* _audioFftWindow;
        double* _audioSpectrum;
        int _audioFftSize;

        // Convenience functions for creating a receiver chain
        bool SetInput();
        bool SetInputReader();
        bool SetReceiver();
        bool SetOutput();
        bool InitializeReceiver();
};

#endif
