#include "PluginEditor.h"

// ===========================================================================
// Petit helper : configuration d'un slider "knob"
// ===========================================================================
static void styleKnob(juce::Slider& s)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);

    // Pas de TextBox intégrée, on dessine le texte nous–mêmes
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    s.setMouseDragSensitivity(150);
}

// ===========================================================================
// Constructeur de l’éditeur
// ===========================================================================
SimpleReverbAudioProcessorEditor::SimpleReverbAudioProcessorEditor(SimpleReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    // Look & Feel global
    setLookAndFeel(&lnf);

    // Taille de la fenêtre
    setSize(560, 300);

    // -----------------------------------------------------------------------
    // Bandeau supérieur : Mode
    // -----------------------------------------------------------------------
    addAndMakeVisible(panelTop);

    addAndMakeVisible(lblMode);
    lblMode.setJustificationType(juce::Justification::centred);
    lblMode.setInterceptsMouseClicks(false, false);

    addAndMakeVisible(modeBox);
    modeBox.addItem("Delay", 1);
    modeBox.addItem("Reverb", 2);

    // -----------------------------------------------------------------------
    // Zone centrale : knobs
    // -----------------------------------------------------------------------
    addAndMakeVisible(panelKnobs);

    styleKnob(delayMs);
    styleKnob(feedback);
    styleKnob(wet);
    styleKnob(roomSize);

    // Ajout visuel des sliders
    for (auto* c : { &delayMs, &feedback, &wet, &roomSize })
        addAndMakeVisible(*c);

    // Labels au-dessus des knobs
    for (auto* L : { &lblDelay, &lblFb, &lblWet, &lblRoom })
    {
        L->setJustificationType(juce::Justification::centred);
        L->setInterceptsMouseClicks(false, false);
        addAndMakeVisible(*L);
    }

    // -----------------------------------------------------------------------
    // Attachments APVTS (liaison UI <-> paramètres DSP)
    // -----------------------------------------------------------------------
    modeAtt = std::make_unique<APVTS::ComboBoxAttachment>(processor.apvts, "mode", modeBox);
    delayAtt = std::make_unique<APVTS::SliderAttachment>(processor.apvts, "delayTimeMs", delayMs);
    fbAtt = std::make_unique<APVTS::SliderAttachment>(processor.apvts, "feedback", feedback);
    wetAtt = std::make_unique<APVTS::SliderAttachment>(processor.apvts, "wet", wet);
    roomAtt = std::make_unique<APVTS::SliderAttachment>(processor.apvts, "roomSize", roomSize);

    // === Formattage du texte sous chaque knob ==============================
    // Affichage propre avec unités adaptées
    delayMs.textFromValueFunction = [](double v)
        {
            return juce::String(v, 2) + " ms";
        };

    feedback.textFromValueFunction = [](double v)
        {
            return juce::String(v, 3) + " s";
        };

    wet.textFromValueFunction = [](double v)
        {
            return juce::String(v * 100.0, 1) + " %";
        };

    roomSize.textFromValueFunction = [](double v)
        {
            return juce::String(v * 100.0, 1) + " %";
        };
    // ======================================================================
}

// ===========================================================================
// Destructeur
// ===========================================================================
SimpleReverbAudioProcessorEditor::~SimpleReverbAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

// ===========================================================================
// paint() : dessin du fond + titre
// ===========================================================================
void SimpleReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    // Cadre bois
    skin::drawWoodFrame(g, r, 12.0f);

    // Panneau "cork" intérieur
    auto inner = r.reduced(12.0f);
    skin::fillCork(g, inner);

    // Titre central
    g.setColour(juce::Colours::black.withAlpha(0.65f));
    juce::Font titleFont(20.0f);
    titleFont.setBold(true);
    g.setFont(titleFont);

    auto titleArea = inner.removeFromTop(36).toNearestInt();
    g.drawFittedText("SIMPLE DELAY / REVERB UNIT",
        titleArea,
        juce::Justification::centred,
        1);
}

// Optionnel : petit helper si tu veux un LED plus tard
static void drawLED(juce::Graphics& g, juce::Point<float> c, bool on)
{
    float r = 5.0f;
    juce::Colour core = on ? juce::Colour::fromRGB(243, 90, 74)
        : juce::Colours::black.withAlpha(0.6f);
    g.setColour(core);
    g.fillEllipse(c.x - r, c.y - r, 2.0f * r, 2.0f * r);

    if (on)
    {
        g.setGradientFill(juce::ColourGradient(core.withAlpha(0.6f), c,
            core.withAlpha(0.0f), c, true));
        g.fillEllipse(c.x - 2.2f * r, c.y - 2.2f * r,
            4.4f * r, 4.4f * r);
    }
}

// ===========================================================================
// resized() : layout des composants
// ===========================================================================
void SimpleReverbAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(12);

    // --- Bandeau supérieur ---
    auto top = bounds.removeFromTop(56);
    panelTop.setBounds(top);

    auto row = top.reduced(12);
    auto left = row.removeFromLeft(100);
    lblMode.setBounds(left);
    modeBox.setBounds(row.removeFromLeft(160).reduced(8, 6));

    // --- Zone des knobs ---
    auto knobs = bounds.removeFromTop(getHeight() - 90);
    panelKnobs.setBounds(knobs);

    auto area = knobs.reduced(16);
    auto colW = area.getWidth() / 4;   // 4 colonnes

    auto place = [](juce::Label& L, juce::Component& C, juce::Rectangle<int> slot)
        {
            auto lab = slot.removeFromTop(20); // label au-dessus
            L.setBounds(lab);
            C.setBounds(slot.reduced(10));     // knob au centre
        };

    place(lblDelay, delayMs, area.removeFromLeft(colW));
    place(lblFb, feedback, area.removeFromLeft(colW));
    place(lblWet, wet, area.removeFromLeft(colW));
    place(lblRoom, roomSize, area.removeFromLeft(colW));
}
