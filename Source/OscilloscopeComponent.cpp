

#include "../JuceLibraryCode/JuceHeader.h"
#include "OscilloscopeComponent.h"

//==============================================================================
OscilloscopeComponent::OscilloscopeComponent()
{
	//m_context.setComponentPaintingEnabled(true);
	//m_context.attachTo(*this);
	writeIndex = 0;
	buffer.resize(40000);
	scopeStartSample = 38000;
}

OscilloscopeComponent::~OscilloscopeComponent()
{
	//m_context.detach();
}

void OscilloscopeComponent::pushSample(float sample) {
	buffer.at(writeIndex) = sample;
	if (writeIndex == buffer.size()-1) {
		writeIndex = 0;
	}
	else {
		writeIndex++;
	}
}

float OscilloscopeComponent::readSample(int index) {
	if (writeIndex + index < buffer.size()) {
		return buffer.at(writeIndex + index);
	}
	else {
		return buffer.at(writeIndex + index - buffer.size());
	}
}


void OscilloscopeComponent::setWaveformColour(Colour colour) {
	waveformColour = colour;
}

void OscilloscopeComponent::paint (Graphics& g)
{	
	lock = true;
	g.setColour(Colours::black);
	g.fillRect(getBounds());

	g.setColour(waveformColour);
	
	const int x = getX();
	const int y = getY();
	const int width = getWidth();
	const int height = getHeight();
	float sampsPerPixel = float(buffer.size()-scopeStartSample) / float(width - 2);
	float yfactor = (height - 2) / 2;

	Path wave;
	wave.startNewSubPath(1, y + height / 2 - (float(readSample(scopeStartSample))*yfactor));

	int i;
	for (i = 0; i < width - 2; ++i) {
		int index = i*sampsPerPixel;

		//calculate highest and lowest sample value per pixel
		float max;
		max = readSample(scopeStartSample + index);
		float min = max;

		float current;
		bool maxFirst = false;
		for (int j = 1; j < sampsPerPixel; j++) {

			current = readSample(scopeStartSample + index + j);

			if (current > max) {
				max = current;
				maxFirst = false;
			}
			else {
				if (current < min) {
					min = current;
					maxFirst = true;
				}
			}
		}

		//draw
		if (maxFirst == true) {
			wave.lineTo(i + 0.5, y + height / 2 - (max)*yfactor);
			wave.lineTo(i + 1.5, y + height / 2 - (min)*yfactor);
		}
		else {
			wave.lineTo(i + 0.5, y + height / 2 - (min)*yfactor);
			wave.lineTo(i + 1.5, y + height / 2 - (max)*yfactor);
		}
	}

	g.strokePath(wave, PathStrokeType(1));
	lock = false;
}

void OscilloscopeComponent::resized()
{

}
