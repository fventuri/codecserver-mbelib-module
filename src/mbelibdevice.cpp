#include "mbelibdevice.hpp"
#include <codecserver/registry.hpp>
#include "mbelibsession.hpp"
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>

namespace MBELib {

using CodecServer::DeviceException;

Device::Device(unsigned int unvoicedQuality) {
    unvoiced_quality = unvoicedQuality;
}

Device::~Device() {
}

std::vector<std::string> Device::getCodecs() {
    return { "ambe" };
}

CodecServer::Session* Device::startSession(CodecServer::proto::Request* request) {
    std::cerr << "starting new session\n";
    MBELibSession* session = new MBELibSession(unvoiced_quality);
    try {
        session->renegotiate(request->settings());
        return session;
    } catch (std::invalid_argument) {
        std::cerr << "invalid or unsupported argument\n";
        return nullptr;
    }
    session->end();
    delete session;
    return nullptr;
}

}
