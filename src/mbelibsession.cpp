#include "mbelibsession.hpp"
#include <cstring>
#include <iostream>
#include <sstream>

using namespace MBELib;

MBELibSession::MBELibSession(unsigned int unvoiced_quality) {
    this->unvoiced_quality = unvoiced_quality;

    curr_mp = (mbe_parms*) malloc(sizeof(mbe_parms));
    prev_mp = (mbe_parms*) malloc(sizeof(mbe_parms));
    prev_mp_enhanced = (mbe_parms*) malloc(sizeof(mbe_parms));
    mbe_initMbeParms(curr_mp, prev_mp, prev_mp_enhanced);

    mbeMode = MBEModeUnknown;

    frameBits = 0;
    dataBits = 0;
    frameSize = 0;
    audioSize = 0;

    // initialize ring buffer
    ringBufferSize = 0;
    ringBuffer = nullptr;
    ringBufferHead = 0;
    ringBufferTail = 0;
}

void MBELibSession::encode(char* input, size_t size) {
    //std::cerr << "mbelib software lib does not support encoding\n";
}

void MBELibSession::decode(char* input, size_t size) {
    std::unique_lock<std::mutex> lock(ringBufferMutex);
    int nextHead = (ringBufferHead + size) % ringBufferSize;
    while (nextHead == ringBufferTail)
        ringBufferCV.wait(lock);
    memcpy(ringBuffer + ringBufferHead, input, size);
    ringBufferHead = nextHead;
    lock.unlock();
    ringBufferCV.notify_one();
}

size_t MBELibSession::read(char* output) {
    std::unique_lock<std::mutex> lock(ringBufferMutex);
    while (ringBufferHead == ringBufferTail)
        ringBufferCV.wait(lock);
    char frameBuffer[64];
    memcpy(frameBuffer, ringBuffer + ringBufferTail, frameSize);
    ringBufferTail = (ringBufferTail + frameSize) % ringBufferSize;
    lock.unlock();
    ringBufferCV.notify_one();

    switch (mbeMode) {
        case Ambe3600x2400:
            return decodeAmbe3600x2400(frameBuffer, output);
        case Ambe3600x2450:
            return decodeAmbe3600x2450(frameBuffer, output);
        case Ambe2450:
            return decodeAmbe2450(frameBuffer, output);
        case Imbe7200x4400:
            return decodeImbe7200x4400(frameBuffer, output);
    }

    return 0;
}

void MBELibSession::end() {
    // free everything
    free(curr_mp);
    free(prev_mp);
    free(prev_mp_enhanced);

    free(ringBuffer);
    ringBufferHead = 0;
    ringBufferTail = 0;
}

CodecServer::proto::FramingHint* MBELibSession::getFraming() {
    CodecServer::proto::FramingHint* framing;
    framing = new CodecServer::proto::FramingHint();
    framing->set_channelbits(frameBits);
    framing->set_channelbytes((int)((frameBits + 7) / 8));
    framing->set_audiosamples(AudioSamples);
    framing->set_audiobytes(AudioSize);
    return framing;
}

void MBELibSession::renegotiate(CodecServer::proto::Settings settings) {
    std::cout << "renegotiating: direction:";
    std::map<std::string, std::string> args(settings.args().begin(), settings.args().end());

    unsigned char direction = 0;
    for (int dir: settings.directions()) {
        if (dir == Settings_Direction_ENCODE) {
            throw std::invalid_argument("mbelib software decoder does not support encoding");
        } else if (dir == Settings_Direction_DECODE) {
            std::cout << " decode";
        }
    }

    std::cout << "; ";

    if (args.find("index") != args.end()) {
        std::string indexStr = args["index"];
        std::cout << "index: " << indexStr << "\n";
        unsigned char index = std::stoi(indexStr);
        switch (index) {
            case 33:
                frameBits = 72;
                dataBits = 49;
                mbeMode = Ambe3600x2450;
                break;
            case 34:
                frameBits = 49;
                dataBits = 49;
                mbeMode = Ambe2450;
                break;
            case 59:
                frameBits = 144;
                dataBits = 88;
                mbeMode = Imbe7200x4400;
                break;
            default:
                std::cout << "invalid rate index\n";
        }
    } else if (args.find("ratep") != args.end()) {
        std::string ratepStr = args["ratep"];
        short* rateP = parseRatePString(ratepStr);
        if (rateP == nullptr) {
            std::cout << "invalid ratep string\n";
        } else {
            std::cout << "ratep: " << ratepStr << "\n";
            frameBits = rateP[5] & 0xff;
            dataBits = rateP[0] & 0xff;
            if (frameBits == 72 && dataBits == 48) {
                mbeMode = Ambe3600x2400;
            } else if (frameBits == 144 && dataBits == 88) {
                mbeMode = Imbe7200x4400;
            } else {
                std::cout << "invalid ratep string\n";
            }
        }
    } else {
        std::cout << "invalid parameters\n";
    }

    CodecServer::proto::FramingHint* framing = getFraming();
    frameSize = framing->channelbytes();
    audioSize = framing->audiobytes();

    // reinitialize the ring buffer now that we know the frame size
    free(ringBuffer);
    ringBufferSize = MaxFrames * frameSize;
    ringBuffer = (char*) malloc(ringBufferSize);
    ringBufferHead = 0;
    ringBufferTail = 0;
}

short* MBELibSession::parseRatePString(std::string input) {
    if (input.length() != 29) return nullptr;
    std::vector<std::string> parts;
    size_t pos_start = 0, pos_end;
    while ((pos_end = input.find(":", pos_start)) != std::string::npos) {
        parts.push_back(input.substr(pos_start, pos_end - pos_start));
        pos_start = pos_end + 1;
    }
    parts.push_back(input.substr(pos_start));

    if (parts.size() != 6) return nullptr;

    short* data = (short*) malloc(sizeof(short) * 6);
    for (int i = 0; i < parts.size(); i++) {
        std::string part = parts[i];
        if (part.length() != 4) {
            free(data);
            return nullptr;
        }

        std::stringstream ss;
        ss << std::hex << part;
        ss >> data[i];
    }

    return data;
}

// D-Star
size_t MBELibSession::decodeAmbe3600x2400(char *frame, char* output) {

    const int dW[9][8] = {
        { 0, 0, 3, 2, 1, 1, 0, 0 }, { 1, 1, 0, 0, 3, 2, 1, 1 },
        { 3, 2, 1, 1, 0, 0, 3, 2 }, { 0, 0, 3, 2, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 3, 2, 1, 1 }, { 3, 2, 1, 1, 0, 0, 3, 2 },
        { 0, 0, 3, 2, 1, 1, 0, 0 }, { 1, 1, 0, 0, 3, 2, 1, 1 },
        { 3, 3, 2, 1, 0, 0, 3, 3 }
    };
    const int dX[9][8] = {
        { 10, 22, 11, 9, 10, 22, 11, 23 }, { 8, 20, 9, 21, 10, 8, 9, 21 },
        { 8, 6, 7, 19, 8, 20, 9, 7}, { 6, 18, 7, 5, 6, 18, 7, 19 },
        { 4, 16, 5, 17, 6, 4, 5, 17 }, { 4, 2, 3, 15, 4, 16, 5, 3 },
        { 2, 14, 3, 1, 2, 14, 3, 15 }, { 0, 12, 1, 13, 2, 0, 1, 13 },
        { 0, 12, 10, 11, 0, 12, 1, 13 }
    };


    char ambe_fr[4][24] = { 0 };
    char ambe_d[49] = { 0 };

    // deinterleave
    for (int i = 0; i < 9; ++i) {
        const int *w = dW[i];
        const int *x = dX[i];
        unsigned char frameByte = frame[i];
        ambe_fr[w[0]][x[0]] = (frameByte >> 0) & 0x01;
        ambe_fr[w[1]][x[1]] = (frameByte >> 1) & 0x01;
        ambe_fr[w[2]][x[2]] = (frameByte >> 2) & 0x01;
        ambe_fr[w[3]][x[3]] = (frameByte >> 3) & 0x01;
        ambe_fr[w[4]][x[4]] = (frameByte >> 4) & 0x01;
        ambe_fr[w[5]][x[5]] = (frameByte >> 5) & 0x01;
        ambe_fr[w[6]][x[6]] = (frameByte >> 6) & 0x01;
        ambe_fr[w[7]][x[7]] = (frameByte >> 7) & 0x01;
    }

    int errs = 0;
    int errs2 = 0;
    char err_str[64];
    mbe_processAmbe3600x2400Frame((short*) output, &errs, &errs2, err_str, ambe_fr, ambe_d, curr_mp, prev_mp, prev_mp_enhanced, unvoiced_quality);
//    if (errs || errs2 || err_str[0])
//        std::cerr << "mbe_processAmbe3600x2400Frame - errs=" << errs << " - errs2=" << errs2 << " - err_str=\"" << err_str << "\"\n";
    return audioSize;
}

// DMR, NXDN
size_t MBELibSession::decodeAmbe3600x2450(char *frame, char* output) {

    const int dW[9][8] = {
        { 2, 1, 0, 0, 2, 1, 0, 0 }, { 2, 1, 0, 0, 2, 1, 0, 0 },
        { 3, 1, 0, 0, 3, 1, 0, 0 }, { 3, 1, 1, 0, 3, 1, 1, 0 },
        { 3, 1, 1, 0, 3, 1, 1, 0 }, { 3, 2, 1, 0, 3, 1, 1, 0 },
        { 3, 2, 1, 0, 3, 2, 1, 0 }, { 3, 2, 1, 0, 3, 2, 1, 0 },
        { 3, 2, 1, 0, 3, 2, 1, 0 }
    };
    const int dX[9][8] = {
        { 2, 9, 4, 22, 3, 10, 5, 23 }, { 0, 7, 2, 20, 1, 8, 3, 21 },
        { 12, 5, 0, 18, 13, 6, 1, 19 }, { 10, 3, 21, 16, 11, 4, 22, 17 },
        { 8, 1, 19, 14, 9, 2, 20, 15 }, { 6, 10, 17, 12, 7, 0, 18, 13 },
        { 4, 8, 15, 10, 5, 9, 16, 11 }, { 2, 6, 13, 8, 3, 7, 14, 9 },
        { 0, 4, 11, 6, 1, 5, 12, 7 }
    };


    char ambe_fr[4][24] = { 0 };
    char ambe_d[49] = { 0 };

    // deinterleave
    for (int i = 0; i < 9; ++i) {
        const int *w = dW[i];
        const int *x = dX[i];
        unsigned char frameByte = frame[i];
        ambe_fr[w[0]][x[0]] = (frameByte >> 0) & 0x01;
        ambe_fr[w[1]][x[1]] = (frameByte >> 1) & 0x01;
        ambe_fr[w[2]][x[2]] = (frameByte >> 2) & 0x01;
        ambe_fr[w[3]][x[3]] = (frameByte >> 3) & 0x01;
        ambe_fr[w[4]][x[4]] = (frameByte >> 4) & 0x01;
        ambe_fr[w[5]][x[5]] = (frameByte >> 5) & 0x01;
        ambe_fr[w[6]][x[6]] = (frameByte >> 6) & 0x01;
        ambe_fr[w[7]][x[7]] = (frameByte >> 7) & 0x01;
    }

    int errs = 0;
    int errs2 = 0;
    char err_str[64];
    mbe_processAmbe3600x2450Frame((short*) output, &errs, &errs2, err_str, ambe_fr, ambe_d, curr_mp, prev_mp, prev_mp_enhanced, unvoiced_quality);
//    if (errs || errs2 || err_str[0])
//        std::cerr << "mbe_processAmbe3600x2450Frame - errs=" << errs << " - errs2=" << errs2 << " - err_str=\"" << err_str << "\"\n";
    return audioSize;
}

// YSF - DN (Digital Narrow or V/D - Voice+Digital)
size_t MBELibSession::decodeAmbe2450(char *frame, char* output) {

    const int dX[7][8] = {
        { 20, 2, 37, 19, 1, 36, 18, 0 }, { 5, 40, 22, 4, 39, 21, 3, 38 },
        { 43, 25, 7, 42, 24, 6, 41, 23 }, { 28, 10, 45, 27, 9, 44, 26, 8 },
        { 13, 48, 30, 12, 47, 29, 11, 46 }, { 17, 34, 16, 33, 15, 32, 14, 31 },
        { 0, 0, 0, 0, 0, 0, 0, 35, }
    };


    char ambe_d[49] = { 0 };

    // deinterleave
    for (int i = 0; i < 6; ++i) {
        const int *x = dX[i];
        unsigned char frameByte = frame[i];
        ambe_d[x[0]] = (frameByte >> 0) & 0x01;
        ambe_d[x[1]] = (frameByte >> 1) & 0x01;
        ambe_d[x[2]] = (frameByte >> 2) & 0x01;
        ambe_d[x[3]] = (frameByte >> 3) & 0x01;
        ambe_d[x[4]] = (frameByte >> 4) & 0x01;
        ambe_d[x[5]] = (frameByte >> 5) & 0x01;
        ambe_d[x[6]] = (frameByte >> 6) & 0x01;
        ambe_d[x[7]] = (frameByte >> 7) & 0x01;
    }
    ambe_d[dX[6][7]] = (frame[6] >> 7) & 0x01;

    int errs = 0;
    int errs2 = 0;
    char err_str[64];
    mbe_processAmbe2450Data((short*) output, &errs, &errs2, err_str, ambe_d, curr_mp, prev_mp, prev_mp_enhanced, unvoiced_quality);
//    if (errs || errs2 || err_str[0])
//        std::cerr << "mbe_processAmbe2450Data - errs=" << errs << " - errs2=" << errs2 << " - err_str=\"" << err_str << "\"\n";
    return audioSize;
}

// YSF VW (Voice Wide or Voice FR)
size_t MBELibSession::decodeImbe7200x4400(char *frame, char* output) {

    const int dW[18][8] = {
        { 0, 1, 5, 4, 3, 2, 1, 0 }, { 3, 2, 1, 0, 4, 5, 2, 3 },
        { 4, 6, 2, 3, 0, 1, 6, 4 }, { 0, 1, 6, 4, 3, 2, 1, 0 },
        { 3, 2, 1, 0, 4, 6, 2, 3 }, { 4, 6, 2, 3, 0, 1, 6, 4 },
        { 0, 1, 6, 4, 3, 2, 1, 0 }, { 3, 2, 1, 0, 4, 6, 2, 3 },
        { 5, 6, 2, 3, 0, 1, 6, 4 }, { 0, 1, 6, 5, 3, 2, 1, 0 },
        { 3, 2, 1, 0, 5, 6, 2, 3 }, { 5, 6, 2, 3, 0, 1, 6, 5 },
        { 0, 1, 6, 5, 3, 2, 1, 0 }, { 3, 2, 1, 0, 5, 7, 2, 3 },
        { 5, 7, 2, 3, 0, 1, 7, 5 }, { 0, 1, 7, 5, 4, 2, 1, 0 },
        { 4, 3, 2, 0, 5, 7, 3, 4 }, { 5, 7, 3, 4, 1, 2, 7, 5 }
    };
    const int dX[18][8] = {
        { 21, 20, 1, 10, 19, 20, 21, 22 }, { 17, 18, 19, 20, 9, 0, 19, 18 },
        { 7, 13, 17, 16, 19, 18, 14, 8 }, { 17, 16, 12, 6, 15, 16, 17, 18 },
        { 13, 14, 15, 16, 5, 11, 15, 14 }, { 3, 9, 13, 12, 15, 14, 10, 4 },
        { 13, 12, 8, 2, 11, 12, 13, 14 }, { 9, 10, 11, 12, 1, 7, 11, 10 },
        { 14, 5, 9, 8, 11, 10, 6, 0 }, { 9, 8, 4, 13, 7, 8, 9, 10 },
        { 5, 6, 7, 8, 12, 3, 7, 6 }, { 10, 1, 5, 4, 7, 6, 2, 11 },
        { 5, 4, 0, 9, 3, 4, 5, 6 }, { 1, 2, 3, 4, 8, 6, 3, 2 },
        { 6, 4, 1, 0, 3, 2, 5, 7 }, { 1, 0, 3, 5, 14, 0, 1, 2 },
        { 12, 21, 22, 0, 4, 2, 22, 13 }, { 2, 0, 20, 11, 22, 21, 1, 3 }
    };


    char imbe_fr[8][23] = { 0 };
    char imbe_d[88] = { 0 };

    // deinterleave
    for (int i = 0; i < 18; ++i) {
        const int *w = dW[i];
        const int *x = dX[i];
        unsigned char frameByte = frame[i];
        imbe_fr[w[0]][x[0]] = (frameByte >> 0) & 0x01;
        imbe_fr[w[1]][x[1]] = (frameByte >> 1) & 0x01;
        imbe_fr[w[2]][x[2]] = (frameByte >> 2) & 0x01;
        imbe_fr[w[3]][x[3]] = (frameByte >> 3) & 0x01;
        imbe_fr[w[4]][x[4]] = (frameByte >> 4) & 0x01;
        imbe_fr[w[5]][x[5]] = (frameByte >> 5) & 0x01;
        imbe_fr[w[6]][x[6]] = (frameByte >> 6) & 0x01;
        imbe_fr[w[7]][x[7]] = (frameByte >> 7) & 0x01;
    }

    int errs = 0;
    int errs2 = 0;
    char err_str[64];
    mbe_processImbe7200x4400Frame((short*) output, &errs, &errs2, err_str, imbe_fr, imbe_d, curr_mp, prev_mp, prev_mp_enhanced, unvoiced_quality);
//    if (errs || errs2 || err_str[0])
//        std::cerr << "mbe_processImbe7200x4400Frame - errs=" << errs << " - errs2=" << errs2 << " - err_str=\"" << err_str << "\"\n";
    return audioSize;
}
