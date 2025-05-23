#ifndef __RECEIVER_CPP
#define __RECEIVER_CPP

#include "boomareceiver.h"

bool BoomaReceiver::SetOption(ConfigOptions* opts, std::string name, int value){
    for( std::vector<Option>::iterator it = _options.begin(); it != _options.end(); it++ ) {
        if( (*it).Name == name ) {
            for( std::vector<OptionValue>::iterator valIt = (*it).Values.begin(); valIt != (*it).Values.end(); valIt++ ) {
                if( (*valIt).Value == value ) {

                    // Discard repeated setting of the same value
                    if( (*it).CurrentValue == value ) {
                        HLog("Value is the same as the current value, discarding option set");
                        return true;
                    }

                    // Set current value
                    (*it).CurrentValue = value;

                    // Update the stored copy of receiver options
                    std::map<std::string, std::string> optionsMap;
                    for( std::vector<Option>::iterator optIt = _options.begin(); optIt != _options.end(); optIt++ ) {
                        for( std::vector<OptionValue>::iterator optValIt = (*optIt).Values.begin(); optValIt != (*optIt).Values.end(); optValIt++ ) {
                            if( (*optValIt).Value == (*optIt).CurrentValue ) {
                                optionsMap[(*optIt).Name] = (*optValIt).Name;
                                continue;
                            }
                        }
                    }
                    opts->SetReceiverOptionsFor(GetName(), optionsMap);

                    // Report the change to the receiver implementation
                    if( _hasBuilded ) {
                        OptionChanged(opts, name, value);
                    }

                    // Receiver option set
                    return true;
                }
            }
            HError("Attempt to internal set receiver option '%s' to non-existing value %d", name.c_str(), value);
            return false;
        }
    }
    HError("Attempt to internal set non-existing receiver option '%s' to value '%d'", name.c_str(), value);
    return false;
};

int BoomaReceiver::GetRfAgcLevel(ConfigOptions* opts) {

    switch(opts->GetInputSourceDataType()) {
        case IQ_INPUT_SOURCE_DATA_TYPE:
        case I_INPUT_SOURCE_DATA_TYPE:
        case Q_INPUT_SOURCE_DATA_TYPE:
            return opts->GetRfAgcLevel() * 4;
        default:
            return opts->GetRfAgcLevel();
    }
}

int BoomaReceiver::GetOption(std::string name) {
    for( std::vector<Option>::iterator it = _options.begin(); it != _options.end(); it++ ) {
        if( (*it).Name == name ) {
            return (*it).CurrentValue;
        }
    }
    return -1;
};

bool BoomaReceiver::SetOption(ConfigOptions* opts, std::string name, std::string value){
    HLog("Setting option '%s' to value '%s'", name.c_str(), value.c_str());
    for( std::vector<Option>::iterator it = _options.begin(); it != _options.end(); it++ ) {
        if( (*it).Name == name ) {
            for( std::vector<OptionValue>::iterator valIt = (*it).Values.begin(); valIt != (*it).Values.end(); valIt++ ) {
                if( (*valIt).Name == value ) {
                    return SetOption(opts, name, (*valIt).Value);
                }
            }
            HError("Attempt to set receiver option '%s' to non-existing value '%s'", name.c_str(), value.c_str());
            return false;
        }
    }
    HError("Attempt to set non-existing receiver option '%s' to value '%s'", name.c_str(), value.c_str());
    return false;
};

int BoomaReceiver::SetRfGain(int gain) {
    _gainValue = gain;
    if( gain != 0 ) {
        float g = gain > 0 ? gain : ((float) 1 / ((float) gain * (float) -1));
        _rfAgc->SetGain(g);
        return _rfAgc->GetGain();
    } else {
        _rfAgc->SetEnabled(true);
        return 0;
    }
}

void BoomaReceiver::Build(ConfigOptions* opts, BoomaInput* input, BoomaDecoder* decoder) {

    // Can we build a receiver for the given input data type ?
    if( !IsDataTypeSupported(opts->GetInputSourceDataType()) ) {
        HError("Attempt to build receiver for unsupported input data type");
        _hasBuilded = false;
        throw new BoomaReceiverException("Attempt to build receiver for unsupported input data type");
    }

    // Set options from saved configuration
    std::map<std::string, std::string> storedOptions = opts->GetReceiverOptionsFor(GetName());
    for( std::map<std::string, std::string>::iterator it = storedOptions.begin(); it != storedOptions.end(); it++ ) {
        SetOption(opts, (*it).first, (*it).second);
    }

    // Set options from the command line
    for( std::map<std::string, std::string>::iterator it = opts->GetReceiverOptions()->begin(); it != opts->GetReceiverOptions()->end(); it++ ) {
        SetOption(opts, (*it).first, (*it).second);
    }

    // Check that the initial frequency is supported
    if( !IsFrequencySupported(opts, opts->GetFrequency()) ) {
        HLog("Configured frequency %ld is not valid for this receiver. Using default frequency %ld", opts->GetFrequency(), GetDefaultFrequency(opts));
        opts->SetFrequency(GetDefaultFrequency(opts));
    }

    // Add receiver gain/agc
    _gainValue = opts->GetRfGain();
    _rfAgc = new HAgc<int16_t>("receiver_agc", input->GetLastWriterConsumer(), GetRfAgcLevel(opts), 10, BLOCKSIZE, 6, false);
    if( opts->GetRfGain() != 0 ) {
        if( opts->GetRfGainEnabled() ) {
            float g =
                    opts->GetRfGain() > 0 ? opts->GetRfGain() : ((float) 1 / ((float) opts->GetRfGain() * (float) -1));
            _rfAgc->SetGain(g);
        } else {
            _rfAgc->SetGain(1);
        }
    }

    // Add preprocessing part of the receiver
    _preProcess = PreProcess(opts, _rfAgc);

    // Add the receiver chain
    _receive = Receive(opts, _preProcess);

    // Add postprocessing part of the receiver
    _postProcess = PostProcess(opts, _receive);

    // Add a splitter so that we can push fully processed samples through an optional decoder
    _decoder = new HSplitter<int16_t>("receiver_decoder_splitter", _postProcess->Consumer());
    if( decoder != NULL ) {
        _decoder->SetWriter(decoder->Writer());
    }

    // Receiver has been build and all components is initialized (or so they should be!)
    _hasBuilded = true;
};

bool BoomaReceiver::SetRfGainEnabled(bool enabled) {
    if( enabled ) {
        SetRfGain(_gainValue);
    } else {
        _rfAgc->SetGain(1);
    }
    return true;
}

#endif