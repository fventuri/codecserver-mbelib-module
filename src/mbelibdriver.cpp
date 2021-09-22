#include "mbelibdriver.hpp"
#include <codecserver/registry.hpp>
#include <iostream>

using namespace MBELib;

std::string Driver::getIdentifier(){
    return "mbelib";
}

Device* Driver::buildFromConfiguration(std::map<std::string, std::string> config){
    unsigned int unvoiced_quality = 1;
    if (config.find("unvoiced_quality") != config.end()) {
        try {
            unvoiced_quality = stoul(config["unvoiced_quality"]);
        } catch (std::invalid_argument) {
            std::cerr << "unable to create mbelib device: cannot parse unvoiced_quality\n";
            return nullptr;
        }
    }
    return new Device(unvoiced_quality);
}


static int registration = CodecServer::Registry::registerDriver(new Driver());
