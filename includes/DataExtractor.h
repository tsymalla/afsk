#ifndef DATAEXTRACTOR_H
#define DATAEXTRACTOR_H

#include <iostream>
#include "AudioFile.h"
#include <vector>
#include <deque>
#include <bitset>
#include <iomanip>
#include <array>

namespace Definitions
{
    constexpr static unsigned int ONE_LENGTH = 320;
    constexpr static unsigned int START_STREAM_BYTE = 0x42 ^ 0x03;
    constexpr static unsigned int END_STREAM_BYTE = 0x00;
    constexpr static unsigned int MESSAGE_SIZE = 30;
    constexpr static unsigned int BYTE_SIZE_PACKED = 11;

    using Byte = uint8_t;
    using AudioData = AudioFile<float>;
    using BitStream = std::deque<bool>;
    using ByteStream = std::vector<Byte>;
    using ByteBitstream = std::bitset<Definitions::BYTE_SIZE_PACKED>;

    struct Message
    {
        std::array<Byte, MESSAGE_SIZE> data;
        Byte checksum;
    };

    using MessageList = std::vector<Message>;
}

class Logger
{
    /**
     * Logging helpers.
     */
    static void _log()
    {
        std::cout << std::endl;
    }

    template<typename T, typename ...Rest>
    [[maybe_unused]] constexpr static void _log(T str1, Rest ...rest)
    {
        std::cout << str1;
        _log(rest...);
    }

public:
    template <typename ... Args>
    constexpr static void log(Args ...args)
    {
        _log(std::forward<Args>(args)...);
    }
};

class DataExtractor final
{
public:
	/**
	 * Output format for the overloaded << operator.
	 */
	enum class E_OUT_FORMAT : unsigned int
	{
		ASCII = 0,
		MESSAGE = 1
	};
	
	explicit DataExtractor(std::string filename);
	void run();
	void setOutFormat(E_OUT_FORMAT outFormat);

	friend std::ostream& operator<<(std::ostream& os, const DataExtractor& extractor);
private:
    bool _isValidMessage(Definitions::Message& message);
    void _generateBitStream();
    void _extractByteStream();
    void _constructMessageList();
    bool _isEndOfByte(const Definitions::ByteBitstream&) const;
    
    std::string _filename;
    Definitions::BitStream _bitstream;
    Definitions::ByteStream _bytestream;
    Definitions::MessageList _messageList;
    bool _hasRun;
    E_OUT_FORMAT _outFormat;
};

inline std::ostream& operator<<(std::ostream& os, const DataExtractor& extractor)
{
	size_t messageNum = 1;
	const auto isMessageMode = (extractor._outFormat == DataExtractor::E_OUT_FORMAT::MESSAGE);
	
	for (const auto& message : extractor._messageList)
	{
		if (isMessageMode)
		{
			os << "Message #" << std::to_string(messageNum) << std::endl;
		}
		
		for (const auto& byte: message.data)
		{
			if (extractor._outFormat == DataExtractor::E_OUT_FORMAT::ASCII)
			{
				os << static_cast<unsigned char>(byte);
			}
			else
			{
				os << "0x" << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(byte) << " ";
			}
		}

		if (isMessageMode)
		{
			os << std::endl << "CHECKSUM: " << "0x" << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(message.checksum) << std::endl;
			++messageNum;
			os << std::endl;
		}
	}

	return os;
}

#endif
