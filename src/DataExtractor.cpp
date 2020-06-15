#include "../includes/DataExtractor.h"

DataExtractor::DataExtractor(std::string filename): _filename(std::move(filename)), _hasRun(false), _outFormat(E_OUT_FORMAT::ASCII)
{
}

void DataExtractor::run()
{
    Logger::log("Processing file ", _filename, "...");

    if (_hasRun)
    {
        Logger::log("Clearing previous data...");
        _bitstream.clear();
        _bytestream.clear();
        _messageList.clear();
    }

    Logger::log("Reading audio file and generating bitstream...");
    _generateBitStream();

    Logger::log("Generating bytestream...");
    _extractByteStream();

    Logger::log("Generating message list...");
    _constructMessageList();

    Logger::log("Done!");

    _hasRun = true;
}

void DataExtractor::setOutFormat(E_OUT_FORMAT outFormat)
{
    _outFormat = outFormat;
}

/**
 * Checksum calculation seems to be done by simple addition without overflow.
 */
bool DataExtractor::_isValidMessage(Definitions::Message& message)
{
    Definitions::Byte currentChecksum{ 0x00 };
    for (const auto& byte: message.data)
    {
        currentChecksum += byte;
    }

    return currentChecksum == message.checksum;
}

void DataExtractor::_generateBitStream()
{
    Definitions::AudioData audio;

    if (!audio.load(_filename))
    {
        throw std::runtime_error("Could not load file " + _filename);
    }

    std::vector<unsigned int> zeroCrossings;
    const auto numChannels = audio.getNumChannels();

    if (numChannels < 1)
    {
        throw std::runtime_error("No channels found.");
    }

    // use only first channel as the contents of both channels in our example files
    // seem to be identical.
    const auto& rawData = audio.samples[0];

    for (size_t i = 0; i < rawData.size() - 1; ++i)
    {
        const auto currentSample = rawData[i];
        const auto nextSample = rawData[i + 1];

        // if current sample is high / low and next sample is low / high, we have found a new zero crossing
        if (currentSample > 0.f && nextSample <= 0.f || currentSample < 0.f && nextSample >= 0.f || currentSample == 0.f && nextSample != 0.f)
        {
            zeroCrossings.push_back(i);
        }
    }

    // calculate how many samples we are able to read for a wavelength of t = 320 microseconds
    // at 44.1khz we have 44100 samples per second
    const auto SAMPLE_LENGTH_MICROSECONDS = static_cast<float>(audio.getSampleRate()) / 1000.f / 1000.f;

    // one wave at t=320 takes 14 samples, downpromote it to unsigned int
    const auto SINGLE_TIMEFRAME = static_cast<unsigned int>(static_cast<float>(Definitions::ONE_LENGTH) * SAMPLE_LENGTH_MICROSECONDS);
    const auto DOUBLE_TIMEFRAME = 2 * SINGLE_TIMEFRAME;

    for (size_t i = 0; i < zeroCrossings.size() - 1; ++i)
    {
        const auto currentCrossing = zeroCrossings[i];
        const auto nextCrossing = zeroCrossings[i + 1];
        const auto diff = nextCrossing - currentCrossing;

        // evaluates to true if time between two points != 640
        _bitstream.push_back(diff < DOUBLE_TIMEFRAME);
    }
}

void DataExtractor::_extractByteStream()
{
    Definitions::ByteBitstream currentByteEncoding;
    Definitions::Byte currentByte{ 0x00 };

    auto i = 0;
    for (const auto& bit : _bitstream)
    {
        currentByteEncoding.set(i, bit);
        ++i;

        // if we reached the end of an byte (one start bit, two stop bits, extract the data and convert it to a byte
        if (i % Definitions::BYTE_SIZE_PACKED == 0)
        {
            if (_isEndOfByte(currentByteEncoding))
            {
                for (size_t byteIndex = 1; byteIndex < Definitions::BYTE_SIZE_PACKED - 2; ++byteIndex)
                {
                    currentByte |= currentByteEncoding[byteIndex] << (byteIndex - 1);
                }

                _bytestream.push_back(currentByte);
            }

            i = 0;
            currentByteEncoding.reset();
            currentByte = 0x00;
        }
    }
}

void DataExtractor::_constructMessageList()
{
    if (_bytestream.empty() || _bytestream.size() + 2 <= Definitions::START_STREAM_BYTE)
    {
        throw std::runtime_error("Could not construct byte stream.");
    }

    // find start of data stream as it begins with 0x42 and 0x03
    size_t startIndex{ 0 };
    for (; startIndex < _bytestream.size() - 1; ++startIndex)
    {
        if ((_bytestream[startIndex] ^ _bytestream[startIndex + 1]) == Definitions::START_STREAM_BYTE)
        {
            // increment until we have our correct start index
            startIndex += 2;
            break;
        }
    }

    // iterate through byte stream backwards until we have found our concluding 0x00 byte
    size_t endIndex = _bytestream.size() - 1;
    for (; endIndex >= startIndex; --endIndex)
    {
        if (_bytestream[endIndex] == Definitions::END_STREAM_BYTE)
        {
            endIndex -= 1;
            break;
        }
    }

    // use the previously defined range to construct messages of MESSAGE_SIZE bytes
    // byteIndex is used for counting the current message size
    size_t byteIndex{ 0 };

    Definitions::Message currentMessage{};
    auto messageIndex{ 0 };

    for (size_t i = startIndex; i <= endIndex; ++i)
    {
        const auto& currentByte = _bytestream[i];
        currentMessage.data[byteIndex] = currentByte;
        // copy over the current byte to our message
        // generate checksum of all bytes in the current message and check against the next byte (the parity byte)

        ++byteIndex;
        if (byteIndex % Definitions::MESSAGE_SIZE == 0)
        {
            byteIndex = 0;
            // check if the following byte is the correct checksum for our current message
            currentMessage.checksum = _bytestream[i + 1];
            ++messageIndex;

            _messageList.push_back(currentMessage);

            if (!_isValidMessage(currentMessage))
            {
                std::cerr << "Invalid checksum for message #" << messageIndex << std::endl;
                continue;
            }

            // skip the checksum byte
            ++i;
        }
    }
}

bool DataExtractor::_isEndOfByte(const Definitions::ByteBitstream& bitstream) const
{
    return !bitstream.test(0) && bitstream.test(Definitions::BYTE_SIZE_PACKED - 2) && bitstream.test(Definitions::BYTE_SIZE_PACKED - 1);
}
