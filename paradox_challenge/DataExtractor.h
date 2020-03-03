#ifndef DATAEXTRACTOR_H
#define DATAEXTRACTOR_H

#include <iostream>
#include "AudioFile.h"
#include <vector>
#include <deque>
#include <bitset>
#include <iomanip>
#include <array>

class DataExtractor final
{
	constexpr static unsigned int ONE_LENGTH = 320;
	constexpr static unsigned int START_STREAM_BYTE = 0x42 ^ 0x03;
	constexpr static unsigned int END_STREAM_BYTE = 0x00;
	constexpr static unsigned int BYTESTREAM_SIZE = 64 * 31;
	constexpr static unsigned int MESSAGE_SIZE = 30;
	constexpr static unsigned int BYTE_SIZE_PACKED = 11;

	using Byte = uint8_t;
	using AudioData = AudioFile<float>;
	using BitStream = std::deque<bool>;
	using ByteStream = std::vector<Byte>;

	struct Message
	{
		std::array<Byte, MESSAGE_SIZE> data;
		Byte checksum;
	};
	
	using MessageList = std::vector<Message>;
	
public:
	enum class E_OUT_FORMAT : unsigned int
	{
		ASCII = 0,
		BINARY = 1
	};
	
	explicit DataExtractor(std::string filename);
	void reset(std::string filename);
	void run();
	void setOutFormat(E_OUT_FORMAT outFormat);

	MessageList getMessageList() const;

	void _log()
	{
		std::cout << std::endl;
	}
	
	template<typename T, typename ...Rest>
	void _log(T str1, Rest ...rest)
	{
		std::cout << str1;
		_log(rest...);
	}

	template <typename ... Args>
	void log(Args ...args)
	{
		_log(std::forward<Args>(args)...);
	}
	
	friend std::ostream& operator<<(std::ostream& os, const DataExtractor& extractor);
private:
	static bool _isValidMessage(Message& message);
	void _generateBitstream();
	void _extractByteStream();
	void _constructMessageList();
	
	std::string _filename;
	BitStream _bitstream;
	ByteStream _bytestream;
	MessageList _messageList;
	bool _hasRun;
	E_OUT_FORMAT _outFormat;
};

inline std::ostream& operator<<(std::ostream& os, const DataExtractor& extractor)
{
	size_t messageNum = 1;
	const auto isBinaryMode = (extractor._outFormat == DataExtractor::E_OUT_FORMAT::BINARY);
	
	for (const auto& message : extractor._messageList)
	{
		if (isBinaryMode)
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

		if (isBinaryMode)
		{
			os << std::endl << "CHECKSUM: " << "0x" << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(message.checksum) << std::endl;
			++messageNum;
			os << std::endl;
		}
	}

	return os;
}

#endif
