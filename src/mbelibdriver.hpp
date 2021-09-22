#pragma once

#include <codecserver/device.hpp>
#include <codecserver/driver.hpp>
#include "mbelibdevice.hpp"

namespace MBELib {

    class Driver: public CodecServer::Driver {
        public:
            std::string getIdentifier() override;
            Device* buildFromConfiguration(std::map<std::string, std::string> config) override;
    };

}
