#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//=============================================================================
// Petit “skin” : bois, liège, métal brossé, vis
//=============================================================================
namespace skin
{
    static float hash11(float x)
    {
        x = std::sin(x * 127.1f) * 43758.5453f;
        return x - std::floor(x);
    }

    static void fillBrushedMetal(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour base)
    {
        auto top = base.brighter(0.15f);
        auto bot = base.darker(0.25f);
        g.setGradientFill(juce::ColourGradient::vertical(top, bot, r));
        g.fillRoundedRectangle(r, 6.0f);

        juce::Image img(juce::Image::ARGB, (int)r.getWidth(), (int)r.getHeight(), true);
        juce::Graphics gg(img);

        for (int y = 0; y < img.getHeight(); ++y)
        {
            float a = 0.06f + 0.06f * hash11((float)y * 0.33f);
            gg.setColour(juce::Colours::black.withAlpha(a));
            gg.drawLine(0.0f, (float)y, (float)img.getWidth(), (float)y);
        }

        g.setOpacity(1.0f);
        g.drawImageAt(img, (int)r.getX(), (int)r.getY());
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawRoundedRectangle(r, 6.0f, 1.0f);
    }

    static void fillCork(juce::Graphics& g, juce::Rectangle<float> r)
    {
        juce::Colour c1(0xffCBAE8A), c2(0xffB69473);
        g.setGradientFill(juce::ColourGradient::vertical(c1, c2, r));
        g.fillRect(r);

        juce::Random rng(12345);
        g.setColour(juce::Colours::black.withAlpha(0.06f));

        for (int i = 0; i < 350; ++i)
        {
            auto p = r.getRelativePoint(rng.nextFloat(), rng.nextFloat());
            float rad = 0.5f + rng.nextFloat() * 1.8f;
            g.fillEllipse(p.x - rad, p.y - rad, rad * 2, rad * 2);
        }
    }

    static void drawWoodFrame(juce::Graphics& g, juce::Rectangle<float> r, float thick)
    {
        auto frame = r;
        juce::Colour a(0xff8a582a), b(0xffb97634);

        g.setGradientFill(juce::ColourGradient::vertical(b, a, frame));
        g.fillRect(frame.removeFromTop(thick));

        frame = r;
        g.setGradientFill(juce::ColourGradient::vertical(b, a, frame));
        g.fillRect(frame.removeFromBottom(thick));

        frame = r;
        g.setGradientFill(juce::ColourGradient::horizontal(b, a, frame));
        g.fillRect(frame.removeFromLeft(thick));

        frame = r;
        g.setGradientFill(juce::ColourGradient::horizontal(b, a, frame));
        g.fillRect(frame.removeFromRight(thick));

        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawRect(r, 1.0f);
    }

    static void drawScrew(juce::Graphics& g, juce::Point<float> p)
    {
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillEllipse(p.x - 4, p.y - 4, 8, 8);
        g.setColour(juce::Colours::white.withAlpha(0.25f));
        g.drawLine(p.x - 2.5f, p.y, p.x + 2.5f, p.y, 1.2f);
    }
}

//=============================================================================
// Look&Feel : dessin des knobs + labels (avec texte de valeur !)
//=============================================================================
class FancyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FancyLookAndFeel()
    {
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(120, 200, 255));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white.withAlpha(0.15f));
        setColour(juce::Slider::thumbColourId, juce::Colours::white);
        setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.15f));
        setColour(juce::ComboBox::textColourId, juce::Colours::white);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
        float pos, float a0, float a1, juce::Slider& s) override
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(6.0f);
        auto centre = bounds.getCentre();
        float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

        // Corps du knob
        juce::Colour top = juce::Colour::fromRGB(60, 64, 70);
        juce::Colour bot = juce::Colour::fromRGB(32, 36, 41);
        g.setGradientFill(juce::ColourGradient::vertical(top, bot, bounds));
        g.fillEllipse(bounds);
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawEllipse(bounds, 1.2f);

        // Anneau
        const float ring = radius - 6.0f;
        juce::Rectangle<float> arc(centre.x - ring, centre.y - ring, ring * 2.0f, ring * 2.0f);

        juce::Path bg;
        bg.addArc(arc.getX(), arc.getY(), arc.getWidth(), arc.getHeight(), a0, a1, true);
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.strokePath(bg, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        float ang = a0 + pos * (a1 - a0);

        juce::Path fg;
        fg.addArc(arc.getX(), arc.getY(), arc.getWidth(), arc.getHeight(), a0, ang, true);
        g.setColour(findColour(juce::Slider::rotarySliderFillColourId));
        g.strokePath(fg, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Ticks
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        for (int i = 0; i <= 10; ++i)
        {
            float t = a0 + (a1 - a0) * (i / 10.0f);
            auto p1 = centre.getPointOnCircumference(ring + 2.0f, t);
            auto p2 = centre.getPointOnCircumference(ring - 6.0f, t);
            g.drawLine({ p1, p2 }, (i % 5 == 0) ? 1.5f : 1.0f);
        }

        // Thumb
        auto thumb = centre.getPointOnCircumference(ring - 1.0f, ang);
        g.setColour(findColour(juce::Slider::thumbColourId));
        g.fillEllipse(thumb.x - 4.0f, thumb.y - 4.0f, 8.0f, 8.0f);

        // === Texte sous le knob ===
        if (s.getTextBoxPosition() == juce::Slider::NoTextBox)
        {
            juce::String valueText = s.getTextFromValue(s.getValue());

            g.setFont(juce::Font(13.0f, juce::Font::plain));
            g.setColour(juce::Colours::white.withAlpha(0.95f));

            auto textArea = juce::Rectangle<int>(
                x,
                y + h - 22,  // position plus basse
                w,
                20
            );

            g.drawFittedText(valueText, textArea, juce::Justification::centred, 1);
        }
    }

    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        g.setColour(label.findColour(juce::Label::textColourId));
        g.setFont(juce::Font(12.5f, juce::Font::bold));
        g.drawFittedText(label.getText(), label.getLocalBounds(),
            juce::Justification::centred, 1);
    }
};

//=============================================================================
// Panneau métal vissé (header + zone knobs)
//=============================================================================
class GlassPanel : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        skin::fillBrushedMetal(g, r, juce::Colour::fromRGB(165, 170, 175));

        const float inset = 10.0f;
        skin::drawScrew(g, { r.getX() + inset,     r.getY() + inset });
        skin::drawScrew(g, { r.getRight() - inset, r.getY() + inset });
        skin::drawScrew(g, { r.getX() + inset,     r.getBottom() - inset });
        skin::drawScrew(g, { r.getRight() - inset, r.getBottom() - inset });
    }
};

//=============================================================================
// Editor principal
//=============================================================================
class SimpleReverbAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SimpleReverbAudioProcessorEditor(SimpleReverbAudioProcessor& p);
    ~SimpleReverbAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SimpleReverbAudioProcessor& processor;
    FancyLookAndFeel            lnf;

    using APVTS = juce::AudioProcessorValueTreeState;

    juce::ComboBox modeBox;
    juce::Label    lblMode{ {}, "Mode" };

    juce::Slider delayMs, feedback, wet, roomSize;

    juce::Label  lblDelay{ {}, "PRE-DELAY" },
        lblFb{ {}, "DECAY" },
        lblWet{ {}, "BLEND" },
        lblRoom{ {}, "WIDTH" };

    GlassPanel panelTop, panelKnobs;

    std::unique_ptr<APVTS::ComboBoxAttachment> modeAtt;
    std::unique_ptr<APVTS::SliderAttachment>   delayAtt, fbAtt, wetAtt, roomAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleReverbAudioProcessorEditor)
};
