

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             oscillosc
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Performs processing on an input signal.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2017, linux_make

 type:             Component
 mainClass:        MainContentComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/

#include "OscilloscopeComponent.h"
#include <list>

#pragma once

//========================================================
class CustomLookAndFeel : public LookAndFeel_V4
{
public:
	void setUIColour(Colour colour) {
		UIColour = colour;
		UIColourDark = colour.withBrightness(0.2);
	}

	void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
		float minSliderPos, float maxSliderPos, const Slider::SliderStyle, Slider& slider) override
	{
		g.setColour(UIColourDark);
		g.drawRect(x, 0, width, height);
		g.setColour(UIColour);
		g.drawVerticalLine(sliderPos, 0, height);
	}

	void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton,
		bool isButtonDown) override {
		auto buttonArea = button.getLocalBounds();
		if (isMouseOverButton) {
			g.setColour(UIColourDark);
		}
		else {
			g.setColour(UIColour);
		}
		g.drawRect(buttonArea);
	}

	void drawTickBox(Graphics &g, Component &component, float x, float y, float w, float h, bool ticked, bool isEnabled, bool isMouseOverButton,
		bool isButtonDown) {
		if (ticked) {
			g.setColour(UIColour);
			g.fillRect(int(x), int(y), int(w), int(h));
		}
		else {
			if (isMouseOverButton) {
				g.setColour(UIColour.withBrightness(0.6));
			}
			else {
				g.setColour(UIColourDark);
			}
			g.drawRect(int(x), int(y), int(w), int(h));
		}
	}

	void drawComboBox(Graphics& g, int width, int height, bool,
		int, int, int, int, ComboBox& box)
	{
		Rectangle<int> boxBounds(0, 0, width, height);

		g.setColour(UIColour);
		g.drawRect(boxBounds);

		Rectangle<int> arrowZone(width - 30, 0, 20, height);
		Path path;
		path.startNewSubPath(arrowZone.getX() + 3.0f, arrowZone.getCentreY() - 2.0f);
		path.lineTo(static_cast<float> (arrowZone.getCentreX()), arrowZone.getCentreY() + 3.0f);
		path.lineTo(arrowZone.getRight() - 3.0f, arrowZone.getCentreY() - 2.0f);

		g.strokePath(path, PathStrokeType(1.0f));
	}

	void drawMenuBarItem(Graphics& g, int width, int height,
		int itemIndex, const String& itemText,
		bool isMouseOverItem, bool isMenuOpen,
		bool /*isMouseOverBar*/, MenuBarComponent& menuBar)
	{
		if (!menuBar.isEnabled())
		{
			g.setColour(UIColour);
		}
		else if (isMenuOpen || isMouseOverItem)
		{
			g.fillAll(Colours::black);
			g.setColour(UIColour);
		}
		else
		{
			g.setColour(Colours::black);
		}

		g.setFont(getMenuBarFont(menuBar, itemIndex, itemText));
		g.drawFittedText(itemText, 0, 0, width, height, Justification::centred, 1);
	}

	void drawLevelMeter(Graphics& g, int width, int height, float level)
	{
		auto meter = roundToInt(width*level);
		if (level < 0.95) {
			g.setColour(UIColour);
			g.drawRect(0, 0, width, height);
		}
		else {
			g.setColour(Colours::red);
		}
		g.fillRect(0, 0, meter,height);
	}

private:
	Colour UIColour;
	Colour UIColourDark;
};

class MainContentComponent   : public AudioAppComponent, Slider::Listener, Button::Listener, private Timer
{
public:
    //==============================================================================
	
	//std::list<float> buffer;

    MainContentComponent()
		: deviceSettings(deviceManager,
			0,     // minimum input channels
			2,   // maximum input channels
			0,     // minimum output channels
			0,   // maximum output channels
			false, // ability to select midi inputs
			false, // ability to select midi output device
			false, // treat channels as stereo pairs
			false) // hide advanced options

    {
		setSize(600, 400);
		setWantsKeyboardFocus(true);
		setAudioChannels(2, 2);
		frameRate = 28;
		startTimerHz(frameRate);

		//oscilloscope
		addAndMakeVisible(&oscilloscope);

		//settings
		settingsButton.addListener(this);
		settingsButton.setButtonText("settings");
		settingsButton.setToggleState(false, sendNotification);
		addAndMakeVisible(&settingsButton);

		//audio device
		addAndMakeVisible(&deviceSettings);
		deviceSettings.setVisible(false);

		deviceConfigurationButton.setButtonText("device");
		deviceConfigurationButton.setToggleState(false, sendNotification);
		deviceConfigurationButton.addListener(this);
		addAndMakeVisible(&deviceConfigurationButton);
		deviceConfigurationButton.setVisible(false);

		//oscilloscope zoom
		zoomLabel.setJustificationType(Justification::right);
		addAndMakeVisible(zoomLabel);
		zoomLabel.setVisible(false);

        zoomSlider.setRange (2000, oscilloscope.buffer.size()-1,50);
		zoomSlider.setValue(oscilloscope.scopeStartSample);
		zoomSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
		zoomSlider.setMouseCursor(MouseCursor::LeftRightResizeCursor);
		zoomSlider.addListener(this);
        addAndMakeVisible (&zoomSlider);
		zoomSlider.setVisible(false);

		//oscilloscope frame rate
		frameRateLabel.setJustificationType(Justification::right);
		addAndMakeVisible(frameRateLabel);
		frameRateLabel.setVisible(false);

		frameRateSlider.setRange(1, 120, 1);
		frameRateSlider.setValue(frameRate);
		frameRateSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
		frameRateSlider.setMouseCursor(MouseCursor::LeftRightResizeCursor);
		frameRateSlider.addListener(this);
		addAndMakeVisible(&frameRateSlider);
		frameRateSlider.setVisible(false);

		//look and feel
		setLookAndFeel(&customLookAndFeel);
		customLookAndFeel.setUIColour(Colours::white);
		oscilloscope.setWaveformColour(Colour(uint8(190), uint8(190), uint8(190)));
    }

    ~MainContentComponent()
    {
        shutdownAudio();
		setLookAndFeel(nullptr);
    }

    void prepareToPlay (int, double) override {
	}

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        auto* device = deviceManager.getCurrentAudioDevice();
        auto activeInputChannels  = device->getActiveInputChannels();
        auto activeOutputChannels = device->getActiveOutputChannels();
        auto maxInputChannels  = activeInputChannels .getHighestBit() + 1;
        auto maxOutputChannels = activeOutputChannels.getHighestBit() + 1;

        auto level = 0.25;

        for (auto channel = 0; channel < 1; ++channel)
        {
            if ((! activeOutputChannels[channel]) || maxInputChannels == 0)
            {
                bufferToFill.buffer->clear (channel, bufferToFill.startSample, bufferToFill.numSamples);
            }
            else
            {
                auto actualInputChannel = channel % maxInputChannels;

                if (! activeInputChannels[channel]){
                    bufferToFill.buffer->clear (channel, bufferToFill.startSample, bufferToFill.numSamples);
                }
				else
				{
					auto* inBuffer = bufferToFill.buffer->getReadPointer(0,
						bufferToFill.startSample);
					for (auto sample = 0; sample < bufferToFill.numSamples; ++sample){

							oscilloscope.pushSample(inBuffer[sample]);
						
					}	
					bufferToFill.buffer->clear();
				}
            }
        }
    }

    void releaseResources() override {}

	void timerCallback() override {
		oscilloscope.repaint();
	}

	void sliderValueChanged(Slider* changedSlider) override{
		
		if (changedSlider == &zoomSlider) {
			oscilloscope.scopeStartSample = zoomSlider.getValue();
			zoomLabel.setText("samps:" + String(oscilloscope.buffer.size() - zoomSlider.getValue()), dontSendNotification);
		}

		if (changedSlider == &frameRateSlider) {
			frameRateLabel.setText("fps:" + String(frameRateSlider.getValue()), dontSendNotification);
			frameRate = frameRateSlider.getValue();
			stopTimer();
			startTimerHz(frameRate);
		}
	}

	void buttonClicked(Button* clickedButton) override {
		if (clickedButton == &settingsButton) {
			settingsButtonClicked();
		}
		else if (clickedButton == &deviceConfigurationButton) {
			deviceConfigurationButtonClicked();
		}
	}

	void settingsButtonClicked() {
		if (settingsButton.getToggleState()) {
			zoomLabel.setVisible(true);
			zoomSlider.setVisible(true);
			frameRateLabel.setVisible(true);
			frameRateSlider.setVisible(true);
			deviceConfigurationButton.setVisible(true);
		}
		else {
			zoomLabel.setVisible(false);
			zoomSlider.setVisible(false);
			frameRateLabel.setVisible(false);
			frameRateSlider.setVisible(false);
			deviceSettings.setVisible(false);
			deviceConfigurationButton.setVisible(false);
		}
	}

	void deviceConfigurationButtonClicked() {
		if (deviceConfigurationButton.getToggleState()) {
			deviceSettings.setVisible(true);
		}
		else {
			deviceSettings.setVisible(false);
		}
	}

	bool keyPressed(const KeyPress &pressedKey) override {
		if (pressedKey.getTextCharacter() == ' ') {
			if (isTimerRunning()) {
				stopTimer();
			}
			else {
				startTimerHz(frameRate);
			}
		}
		return pressedKey.isValid();
	}

	void resized() override
	{
		oscilloscope.setBounds(0, 0, getWidth(), getHeight());

		settingsButton.setBounds(2, getHeight() - 23, 20, 20);

		zoomLabel.setBounds(21, getHeight() - 22, 100, 17);
		zoomSlider.setBounds(110, getHeight() - 22, getWidth() / 5, 17);

		frameRateLabel.setBounds(getWidth() / 5 + 93, getHeight() - 22, 70, 17);
		frameRateSlider.setBounds(getWidth() / 5 + 152, getHeight() - 22, getWidth() / 5, 17);

		deviceConfigurationButton.setBounds(getWidth() - 26, getHeight() - 23, 20, 20);
		deviceSettings.setBounds(getWidth() - 291, getHeight() - 233, 300, getHeight() - 280);

	}

private:

	OscilloscopeComponent oscilloscope;

	AudioDeviceSelectorComponent deviceSettings;

	ToggleButton settingsButton;
	ToggleButton deviceConfigurationButton;

	Label zoomLabel;
	Label frameRateLabel;

    Slider zoomSlider;
	Slider frameRateSlider;

	CustomLookAndFeel customLookAndFeel;
	//LookAndFeel_V4 lookAndFeel_V4;

	int frameRate;
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
