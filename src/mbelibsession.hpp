#pragma once

#include <codecserver/session.hpp>
#include "mbelibdevice.hpp"
#include <codecserver/proto/framing.pb.h>
#include <codecserver/proto/request.pb.h>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
extern "C" {
#include <mbelib.h>
}

using namespace CodecServer::proto;

namespace MBELib {

    // 50ms x 8kHz sampling rate = 160 samples
    // 160 sample x 16 bit (2 bytes) = 320 bytes
    static int constexpr AudioSamples = 160;
    static int constexpr AudioSize = 320;

    typedef enum {
        MBEModeUnknown,
        Ambe3600x2400,              // D-Star
        Ambe3600x2450,              // DMR, dPMR, YSF V/D type 1 (DN), NXDN
        Ambe2450,                   // YSF V/D type 2 (does not use FEC in AMBE codec)
        Imbe7200x4400               // YSF VW
    } MBEMode;

    static int constexpr MaxFrames = 16;

    class MBELibSession: public CodecServer::Session {
        public:
            MBELibSession(unsigned int unvoiced_quality);
            void encode(char* input, size_t size) override;
            void decode(char* input, size_t size) override;
            size_t read(char* output) override;
            void end() override;
            FramingHint* getFraming() override;
            void renegotiate(Settings settings) override;
        private:
            unsigned int unvoiced_quality;
            mbe_parms *curr_mp;
            mbe_parms *prev_mp;
            mbe_parms *prev_mp_enhanced;
            MBEMode mbeMode;
            size_t frameBits;
            size_t dataBits;
            size_t frameSize;
            size_t audioSize;
            char* ringBuffer;
            int ringBufferSize;
            int ringBufferHead;
            int ringBufferTail;
            std::mutex ringBufferMutex;
            std::condition_variable ringBufferCV;
            short* parseRatePString(std::string input);
            size_t decodeAmbe3600x2400(char* frame, char* output);
            size_t decodeAmbe3600x2450(char* frame, char* output);
            size_t decodeAmbe2450(char* frame, char* output);
            size_t decodeImbe7200x4400(char* frame, char* output);
    };

}
