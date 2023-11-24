//
//  Main.cpp
//  OddChunkProblem_ConsoleApp
//
//  created by yu2924 on 2023-11-22
//

// This code reproduces a certain problem when reading metadata in wav files using juce::AudioFileReader.

#include <JuceHeader.h>
#include <filesystem>
#include <fstream>

// an simple RIFF writer implementation
namespace riffrw
{

	struct ChunkHeader
	{
		uint32_t ckid;
		uint32_t cksize;
		bool isContainer() const
		{
			return (ckid == *(uint32_t*)"RIFF") || (ckid == *(uint32_t*)"LIST");
		}
	};

	struct ChunkInfo
	{
		uint32_t hdroffset;
		ChunkHeader header;
		uint32_t type;
	};

	class RiffWriter
	{
	protected:
		std::fstream stream;
		std::list<ChunkInfo> ckstack;
	public:
		RiffWriter(const std::filesystem::path& path) : stream(path, std::ios::out | std::ios::binary | std::ios::trunc)
		{
		}
		~RiffWriter()
		{
			while(!ckstack.empty()) ascend();
		}
		operator bool() const
		{
			return stream.good();
		}
		bool descend(const char* ckid, const char* type_or_z = nullptr)
		{
			return descend(*(uint32_t*)ckid, type_or_z ? *(uint32_t*)type_or_z : 0);
		}
		bool descend(uint32_t ckid, uint32_t type_or_z = 0)
		{
			ChunkInfo ck = {};
			ck.hdroffset = (uint32_t)stream.tellp();
			ck.header.ckid = ckid;
			ckstack.push_back(ck);
			stream.write((const char*)&ck.header, 8);
			if(ck.header.isContainer()) stream.write((const char*)&type_or_z, 4);
			return stream.good();
		}
		bool ascend()
		{
			if(ckstack.empty()) return false;
			ChunkInfo& ck = ckstack.back();
			uint32_t endpos = (uint32_t)stream.tellp();
			ck.header.cksize = endpos - ck.hdroffset - 8;
			stream.seekp(ck.hdroffset + 4);
			stream.write((const char*)&ck.header.cksize, 4);
			stream.seekp(endpos);
			if(endpos & 0x01) stream.put(0);
			ckstack.pop_back();
			return stream.good();
		}
		bool write(const void* p, size_t c)
		{
			stream.write((const char*)p, c);
			return stream.good();
		}
		struct ScopedDescend
		{
			RiffWriter& writer;
			ScopedDescend(RiffWriter& w, uint32_t ckid, uint32_t type_or_z = 0) : writer(w) { writer.descend(ckid, type_or_z); }
			ScopedDescend(RiffWriter& w, const char* ckid, const char* type_or_z = nullptr) : writer(w) { writer.descend(ckid, type_or_z); }
			~ScopedDescend() { writer.ascend(); }
		};
	};

} // namespace riffrw

using namespace riffrw;

int main (int, char**)
{
	// --------------------------------------------------------------------------------
	// write testpattern
	// the integrity of the written testpattern.wav file can be checked by opening it in another application,
	// such as Adobe Audition, Sound Forge, or ocenaudio
	juce::File path = juce::File::getCurrentWorkingDirectory().getChildFile("testpattern.wav");
	{
		RiffWriter writer(std::wstring(path.getFullPathName().toUTF16()));
		RiffWriter::ScopedDescend ckRIFFWAVE(writer, "RIFF", "WAVE");
		{
			RiffWriter::ScopedDescend ckfmt(writer, "fmt ");
			struct PcmWaveFormat {
				uint16_t wFormatTag;
				uint16_t nChannels;
				uint32_t nSamplesPerSec;
				uint32_t nAvgBytesPerSec;
				uint16_t nBlockAlign;
				uint16_t wBitsPerSample;
			} wavfmt = { 3, 1, 44100, 44100 * 4, 4, 32 };
			writer.write(&wavfmt, sizeof(wavfmt));
		}
		{
			RiffWriter::ScopedDescend ckdata(writer, "data");
			std::vector<float> pcmdata(44100);
			for(size_t c = pcmdata.size(), i = 0; i < c; ++i) pcmdata[i] = std::sin(2.0f * juce::float_Pi * (float)i * 440.0f / 44100.0f) * 0.5f;
			writer.write(pcmdata.data(), pcmdata.size() * 4);
		}
		{
			RiffWriter::ScopedDescend ckLISTINFO(writer, "LIST", "INFO");
			// NOTICE: the point at issue, if the size of the chunk is odd, the next chunk will not be visible
			{ RiffWriter::ScopedDescend ckICMT(writer, "ICMT"); const char* pmeta = "ICMT: odd"; writer.write(pmeta, strlen(pmeta)); }
			{ RiffWriter::ScopedDescend ckCMNT(writer, "CMNT"); const char* pmeta = "CMNT: odd"; writer.write(pmeta, strlen(pmeta)); }
			{ RiffWriter::ScopedDescend ckCOMM(writer, "COMM"); const char* pmeta = "COMM: odd"; writer.write(pmeta, strlen(pmeta)); }
			{ RiffWriter::ScopedDescend ckIKEY(writer, "IKEY"); const char* pmeta = "IKEY: odd"; writer.write(pmeta, strlen(pmeta)); }
		}
	}
	// --------------------------------------------------------------------------------
	// read and validate
	{
		std::unique_ptr<juce::FileInputStream> str = std::make_unique<juce::FileInputStream>(path);
		if(str->failedToOpen()) return 0;
		juce::WavAudioFormat waf;
		std::unique_ptr<juce::AudioFormatReader> reader(waf.createReaderFor(str.release(), true));
		if(!reader) return 0;
		std::cout << "-- metadata begin --" << std::endl;
		const juce::StringPairArray& meta = reader->metadataValues;
		for(const auto& key : meta.getAllKeys())
		{
			std::cout << key.quoted().toRawUTF8() << "=" << meta[key].quoted().toRawUTF8() << std::endl;
		}
		std::cout << "-- metadata end --" << std::endl;
	}
    return 0;
}
