#pragma once

#include <codecserver/device.hpp>
#include <codecserver/proto/request.pb.h>

namespace MBELib {

    class Device: public CodecServer::Device {
        public:
            Device(unsigned int unvoicedQuality);
            ~Device();
            std::vector<std::string> getCodecs() override;
            CodecServer::Session* startSession(CodecServer::proto::Request* request) override;
        private:
            unsigned int unvoiced_quality;
    };

}
