#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

struct MidiEvent {
    enum class Type {
        NoteOff,
        NoteOn,
        Other
    } event;
    uint8_t nKey = 0;
    uint8_t nVelocity = 0;
    uint32_t nWallTick = 0;
    uint32_t nDeltaTick = 0;
}; 

struct MidiNote {
    uint8_t nKey = 0;
    uint8_t nVelocity = 0;
    uint32_t nStartTime = 0;
    uint32_t nDuration = 0;
};

struct MidiTrack {
    std::string sName;
    std::string sInstrument;
    std::vector<MidiEvent> vecEvents;
    std::vector<MidiNote> vecNotes;
    uint8_t nMaxNote = 64;
    uint8_t nMinNote = 64;
};

class MidiFile {
    enum EventName : uint8_t {
        VoiceNoteOff = 0x80,
        VoiceNoteOn = 0x90,
        VoiceAfterTouch = 0xA0,
        VoiceControlChange = 0xB0,
        VoiceProgramChange = 0xC0,
        VoiceChannelPressure = 0xD0,
        VoicePitchBend = 0xE0,
        SystemExclusive = 0xF0,
    };
    enum MetaEventName : uint8_t {
        MetaSequence = 0x00,
        MetaText = 0x01,
        MetaCopyright = 0x02,
        MetaTrackName = 0x03,
        MetaInstrumentName = 0x04,
        MetaLyrics = 0x05,
        MetaMarker = 0x06,
        MetaCuePoint = 0x07,
        MetaChannelPrefix = 0x20,
        MetaEndOfTrack = 0x2F,
        MetaSetTempo = 0x51,
        MetaSMPTEOffset = 0x54,
        MetaTimeSignature = 0x58,
        MetaKeySignature = 0x59,
        MetaSequencerSpecific = 0x7F,
    };
    public:
    MidiFile() {
        
    }
    MidiFile(const std::string& sFileName) {
        ParseFile(sFileName);
    }
    bool ParseFile(const std::string& sFileName) {
        std::ifstream ifs;
        ifs.open(sFileName, std::fstream::in | std::ios::binary);
        if (!ifs.is_open()) {
            return false;
        }
        auto Swap32 = [](uint32_t n) {
            return (((n >> 24) & 0xff) | ((n << 8) & 0xff0000) | ((n >> 8) & 0xff00) | ((n << 24) & 0xff000000));
        };
        auto Swap16 = [](uint16_t n) {
            return ((n >> 8) | (n << 8));
        };
        auto ReadString = [&ifs](uint32_t nLength) {
            std::string s;
            for(uint32_t i = 0; i < nLength; i++) s += ifs.get();
            return s;
        };
        auto ReadValue = [&ifs]() {
            uint32_t nValue = 0;
            uint8_t nByte = 0;
            nValue = ifs.get();
            if(nValue & 0x80) {
                nValue &= 0x7F;
                do {
                    nByte = ifs.get();
                    nValue = (nValue << 7) | (nByte & 0x7F);
                } while (nByte & 0x80);
            }
            return nValue;
        };
        uint32_t n32 = 0;
        uint16_t n16 = 0;
        ifs.read((char*)&n32, sizeof(uint32_t));
        uint32_t nFileID = Swap32(n32);
        ifs.read((char*)&n32, sizeof(uint32_t));
        uint32_t nHeaderLength = Swap32(n32);
        ifs.read((char*)&n16, sizeof(uint16_t));
        uint16_t nFormat = Swap16(n16);
        ifs.read((char*)&n16, sizeof(uint16_t));
        uint16_t nTrackChunks = Swap16(n16);
        ifs.read((char*)&n16, sizeof(uint16_t));
        uint16_t nDivision = Swap16(n16);
        for(uint16_t nChunk = 0; nChunk < nTrackChunks; nChunk++) {
            std::cout << "===== NEW TRACK" << std::endl;
            ifs.read((char*)&n32, sizeof(uint32_t));
            uint32_t nFileID = Swap32(n32);
            ifs.read((char*)&n32, sizeof(uint32_t));
            uint32_t nHeaderLength = Swap32(n32);
            bool bEndOfTrack = false;
            int8_t nPreviousStatus = 0;
            vecTracks.push_back(MidiTrack());
            while(!ifs.eof() && !bEndOfTrack) {
                uint32_t nStatusTimeDelta = 0;
                uint8_t nStatus = 0;
                nStatusTimeDelta = ReadValue();
                nStatus = ifs.get();
                if(nStatus < 0x80) {
                    nStatus = nPreviousStatus;
                    ifs.seekg(-1, std::ios_base::cur);
                }
                if((nStatus & 0xF0) == EventName::VoiceNoteOff) {
                    nStatus = nPreviousStatus;
                    uint8_t nChannel = nStatus & 0x0F;
                    uint8_t nNoteID = ifs.get();
                    uint8_t nNoteVelocity = ifs.get();
                    vecTracks[nChunk].vecEvents.push_back({MidiEvent::Type::NoteOff, nNoteID, nNoteVelocity, nStatusTimeDelta});
                } else if((nStatus & 0xF0) == EventName::VoiceNoteOn) {
                    nStatus = nPreviousStatus;
                    uint8_t nChannel = nStatus & 0x0F;
                    uint8_t nNoteID = ifs.get();
                    uint8_t nNoteVelocity = ifs.get();
                    if(nNoteVelocity == 0) {
                        vecTracks[nChunk].vecEvents.push_back({MidiEvent::Type::NoteOff, nNoteID, nNoteVelocity, nStatusTimeDelta});
                    } else {
                        vecTracks[nChunk].vecEvents.push_back({MidiEvent::Type::NoteOn, nNoteID, nNoteVelocity, nStatusTimeDelta});
                    }
                } else if((nStatus & 0xF0) == EventName::VoiceAfterTouch) {
                    nStatus = nPreviousStatus;
                    uint8_t nChannel = nStatus & 0x0F;
                    uint8_t nNoteID = ifs.get();
                    uint8_t nNoteVelocity = ifs.get();
                    vecTracks[nChunk].vecEvents.push_back({MidiEvent::Type::Other});
                } else if((nStatus & 0xF0) == EventName::VoiceControlChange) {
                    nStatus = nPreviousStatus;
                    uint8_t nChannel = nStatus & 0x0F;
                    uint8_t nNoteID = ifs.get();
                    uint8_t nNoteVelocity = ifs.get();
                    vecTracks[nChunk].vecEvents.push_back({MidiEvent::Type::Other});
                } else if((nStatus & 0xF0) == EventName::VoiceProgramChange) {
                    nStatus = nPreviousStatus;
                    uint8_t nChannel = nStatus & 0x0F;
                    uint8_t nProgramID = ifs.get();
                    vecTracks[nChunk].vecEvents.push_back({MidiEvent::Type::Other});
                } else if((nStatus & 0xF0) == EventName::VoiceChannelPressure) {
                    nStatus = nPreviousStatus;
                    uint8_t nChannel = nStatus & 0x0F;
                    uint8_t nChannelPressure = ifs.get();
                    vecTracks[nChunk].vecEvents.push_back({MidiEvent::Type::Other});
                } else if((nStatus & 0xF0) == EventName::VoicePitchBend) {
                    nStatus = nPreviousStatus;
                    uint8_t nChannel = nStatus & 0x0F;
                    uint8_t nLS7B = ifs.get();
                    uint8_t nMS7B = ifs.get();
                    vecTracks[nChunk].vecEvents.push_back({MidiEvent::Type::Other});
                } else if((nStatus & 0xF0) == EventName::SystemExclusive) {
                    if(nStatus == 0xF0) {
                        std::cout << "System Exlusive Begin: " << ReadString(ReadValue()) << std::endl;
                    }
                    if(nStatus == 0xF7) {
                        std::cout << "System Exlusive End: " << ReadString(ReadValue()) << std::endl;
                    }
                    if(nStatus == 0xFF) {
                        uint8_t nType = ifs.get();
                        uint8_t nLength = ReadValue();
                        switch(nType) {
                            case MetaSequence:
                                std::cout << "Sequence Number: " << ifs.get() << ifs.get() << std::endl; 
                                break;
                            case MetaText:
                                std::cout << "Text: " << ifs.get() << ifs.get() << std::endl; 
                                break;
                            case MetaCopyright:
                                std::cout << "Copyright: " << ifs.get() << ifs.get() << std::endl; 
                                break;
                            case MetaTrackName:
                                vecTracks[nChunk].sName = ReadString(nLength);
                                std::cout << "Track Name: " << ifs.get() << ifs.get() << std::endl; 
                                break;
                            case MetaInstrumentName:
                                vecTracks[nChunk].sInstrument = ReadString(nLength);
                                std::cout << "Instrument Name: " << ifs.get() << ifs.get() << std::endl; 
                                break;
                            case MetaLyrics:
                                std::cout << "Lyrics: " << ifs.get() << ifs.get() << std::endl; 
                                break;
                            case MetaMarker:
                                std::cout << "Marker: " << ifs.get() << ifs.get() << std::endl; 
                                break;
                            case MetaCuePoint:
                                std::cout << "Cue Point: " << ifs.get() << ifs.get() << std::endl; 
                                break;
                            case MetaChannelPrefix:
                                std::cout << "Channel Prefix: " << ifs.get() << ifs.get() << std::endl; 
                                break;
                            case MetaEndOfTrack: 
                                    bEndOfTrack = true;
                                    break;
                            case MetaSetTempo:
                                if(m_nTempo == 0) {
                                    (m_nTempo != (ifs.get() << 16));
                                    (m_nTempo != (ifs.get() << 8));
                                    (m_nTempo != (ifs.get() << 0));
                                    m_nBPM = (60000000 / m_nTempo);
                                    std::cout << "Tempo: " << m_nTempo << " (" << m_nBPM << "bpm)" << std::endl; 
                                }
                                break;
                            case MetaSMPTEOffset:
                                std::cout << "SMPTE M: " << ifs.get() << "M:" << ifs.get() << "S:" << ifs.get() << "FR:" << ifs.get() << "FF:" << ifs.get() <<  std::endl; 
                                break;
                            case MetaTimeSignature:
                                std::cout << "Time Signature: " << ifs.get() << "/" << (2 << ifs.get()) << std::endl; 
                                std::cout << "ClocksPerTick: " << ifs.get() << std::endl;
                                std::cout << "32per24Clocks: " << ifs.get() << std::endl;
                                break;
                            case MetaKeySignature:
                                std::cout << "Key Signature: " << ifs.get() << std::endl;
                                std::cout << "Minor Key : " << ifs.get() << std::endl;
                                break;
                            case MetaSequencerSpecific:
                                std::cout << "Sequencer Specific: " << ifs.get() << ifs.get() << std::endl; 
                                break;
                            default:
                                std::cout << "Unrecognized MetaEvent: " << nType << std::endl; 
                        }
                    }
                } else {
                    std::cout << "Unrecognized Status Byte: " << nStatus << std::endl;
                }
            }
        }
    }
    public:
        std::vector<MidiTrack> vecTracks;
        uint32_t m_nTempo = 0;
	    uint32_t m_nBPM = 0;
};

class olcMIDIViewer : public olc::PixelGameEngine 
{
public:
    olcMIDIViewer() {
        sAppName = "Messing with MIDI";
    }
    MidiFile midi;
public:
    bool OnUserCreate() override {
        midi.ParseFile("audio and or visual/Fadeaway.mid");
        return true;
    }
    bool OnUserUpdate(float fElapsedTime) override {
        return true;
    }
};