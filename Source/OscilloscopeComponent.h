

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "../modules/juce_opengl/juce_opengl.h"

//==============================================================================
/*
*/
class OscilloscopeComponent    : public Component
{
public:
    OscilloscopeComponent();
    ~OscilloscopeComponent();

    void paint (Graphics&) override;
    void resized() override;

	void setWaveformColour(Colour colour);

	std::vector<float> buffer;
	int writeIndex;
	void pushSample(float sample);
	float readSample(int index);

	bool lock = false;
	int scopeStartSample;
private:

	Colour waveformColour;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilloscopeComponent)
};
