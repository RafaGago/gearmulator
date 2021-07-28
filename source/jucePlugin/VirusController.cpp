#include "VirusController.h"
#include "PluginProcessor.h"

// TODO: all sysex structs can be refactored to common instead of including this!
#include "../virusLib/microcontroller.h"

using MessageType = virusLib::Microcontroller::SysexMessageType;

namespace Virus
{
    static constexpr uint8_t kSysExStart[] = {0xf0, 0x00, 0x20, 0x33, 0x01};
    static constexpr auto kHeaderWithMsgCodeLen = 7;

    Controller::Controller(AudioPluginAudioProcessor &p, unsigned char deviceId) : m_processor(p), m_deviceId(deviceId)
    {
    }

    void Controller::parseMessage(const SysEx &msg)
    {
        if (msg.size() < 8)
            return; // shorter than expected!

        if (msg[msg.size() - 1] != 0xf7)
            return; // invalid end?!?

        for (auto i = 0; i < msg.size(); ++i)
        {
            if (i < 5)
            {
                if (msg[i] != kSysExStart[i])
                    return; // invalid header
            }
            else if (i == 5)
            {
                if (msg[i] != m_deviceId)
                    return; // not intended to this device!
            }
            else if (i == 6)
            {
                switch (msg[i])
                {
                case MessageType::DUMP_SINGLE:
                    parseSingle(msg);
                    break;
                case MessageType::DUMP_MULTI:
                    parseMulti(msg);
                    break;
                default:
                    std::cout << "Controller: Begin Unhandled SysEx! --" << std::endl;
                    printMessage(msg);
                    std::cout << "Controller: End Unhandled SysEx! --" << std::endl;
                }
            }
        }
    }

    void Controller::parseSingle(const SysEx &msg)
    {
        auto pos = kHeaderWithMsgCodeLen;
        const auto bankNum = msg[pos];
        pos++;
        const auto progNum = msg[pos];
        pos++;

        constexpr auto pageSize = 128;
        constexpr auto expectedDataSize = pageSize * 2; // we have 2 pages
        constexpr auto checkSumSize = 1;

        const auto dataSize = msg.size() - pos - 1;
        const auto hasChecksum = dataSize == expectedDataSize + checkSumSize;
        assert(hasChecksum || dataSize == expectedDataSize);

        const auto namePos = 128 + 112;
        assert(pos + namePos < msg.size());
        auto progName = parseAsciiText(msg, pos + namePos);
        DBG(progName);
    }

    void Controller::parseMulti(const SysEx &msg)
    {
        constexpr auto expectedDataSize = 2 + 256;
        constexpr auto checkSumSize = 1;
        const auto dataSize = msg.size() - kHeaderWithMsgCodeLen - 1;
        const auto hasChecksum = dataSize == expectedDataSize + checkSumSize;
        assert(hasChecksum || dataSize == expectedDataSize);

        constexpr auto startPos = kHeaderWithMsgCodeLen;

        MultiPatch patch;
        patch.bankNumber = msg[startPos];
        patch.progNumber = msg[startPos + 1];
        auto progName = parseAsciiText(msg, startPos + 2 + 3);
        [[maybe_unused]] auto dataSum = copyData(msg, startPos + 2, patch.data);
        if (hasChecksum)
        {
            const int expectedChecksum = msg[msg.size() - 2];
            const auto msgDeviceId = msg[5];
            const int checksum = (msgDeviceId + 0x11 + patch.bankNumber + patch.progNumber + dataSum) & 0x7f;
//            assert(checksum == expectedChecksum);
        }
        m_multis[patch.progNumber] = patch;
    }

    uint8_t Controller::copyData(const SysEx &src, int startPos, uint8_t *dst)
    {
        uint8_t sum = 0;
        for (auto i = 0; i < kDataSizeInBytes; i++)
        {
            dst[i] = src[startPos + i];
            sum += dst[i];
        }
        return sum;
    }

    juce::String Controller::parseAsciiText(const SysEx &msg, const int start)
    {
        char text[kNameLength + 1];
        text[kNameLength] = 0; // termination
        for (auto pos = 0; pos < kNameLength; ++pos)
            text[pos] = msg[start + pos];
        return juce::String(text);
    }

    void Controller::printMessage(const SysEx &msg) const
    {
        for (auto &m : msg)
        {
            std::cout << std::hex << (int)m << ",";
        }
        std::cout << std::endl;
    }

    void Controller::sendSysEx(const SysEx &msg)
    {
        synthLib::SMidiEvent ev;
        ev.sysex = msg;
        m_processor.addMidiEvent(ev);
    }

    std::vector<uint8_t> Controller::constructMessage(SysEx msg)
    {
        const uint8_t start[] = {0xf0, 0x00, 0x20, 0x33, 0x01, static_cast<uint8_t>(m_deviceId)};
        msg.insert(msg.begin(), std::begin(start), std::end(start));
        msg.push_back(0xf7);
        return msg;
    }

    void Controller::dispatchVirusOut(const std::vector<synthLib::SMidiEvent> &newData)
    {
        m_virusOut = newData;
        for (auto msg : m_virusOut)
        {
            if (msg.sysex.size() == 0)
            {
                // no sysex
                DBG(juce::String::formatted("Plain a:%04x b:%04x c:%04x", msg.a, msg.b, msg.c));
                return;
            }
            else
                parseMessage(msg.sysex);
        }
    }
}; // namespace Virus
