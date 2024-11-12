#define OLD_POE_APPLICATION
#include "olcPixelGameEngine.h"

class olcMIDIViewer : public olc::PixelGamingEngine {
    public:
    olcMIDIViewer() {
        sAppName = "Messing with MIDI" 
    }
    public:
    bool OnUserCreate() override {
        return true;
    }
    bool OnUserUpdate(float fElapsedTime) override {
        return true;
    }
};

class MidiFile {
    public:
    MidiFile() {

    }
    MidiFile(const std::string& sFileName) {
        ParseFile(sFileName);
    }
    bool ParseFile(const std::string& sFileName) {
        std::ifstream ifs;
        ifs.open(sFileName, std::fstream::in | std::ios::binary);
    }
}