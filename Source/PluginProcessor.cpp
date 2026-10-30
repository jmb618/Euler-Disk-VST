#include "PluginProcessor.h"
#include "PluginEditor.h"

// Faust : a n'inclure QUE dans ce fichier (la classe generee contient des tables statiques)
#include "ParamUI.h"
#include "EulerDSP.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <set>

//==============================================================================
struct EulerProcessor::Impl
{
    enum Kind { kBool, kChoice, kFloat };

    struct Binding
    {
        juce::RangedAudioParameter* param = nullptr;
        FAUSTFLOAT* zone = nullptr;
        Kind kind = kFloat;
        bool isLaunch = false;       // seul ce bouton est declenche par une note MIDI
        std::vector<float> menu;     // index de choix -> valeur Faust
    };

    struct VizZones
    {
        FAUSTFLOAT *omega = nullptr, *tilt = nullptr, *live = nullptr, *prog = nullptr;
    };

    // Impulsion de lancement (note MIDI OU clic sur LANCER). Elle est tenue pendant
    // 2 blocs audio : un clic plus court qu'un bloc (grand buffer hote) n'est plus perdu.
    struct LaunchListener : juce::AudioProcessorParameter::Listener
    {
        std::atomic<int>* pulse = nullptr;
        void parameterValueChanged (int, float v) override { if (v > 0.5f && pulse != nullptr) pulse->store (2); }
        void parameterGestureChanged (int, bool) override {}
    };

    EulerDSP dsp;
    ParamUI ui;
    std::vector<Binding> bindings;
    VizZones viz[3];
    FAUSTFLOAT* globalProg = nullptr;
    FAUSTFLOAT* health = nullptr;                 // drapeau verrouille du DSP (NaN / Inf)
    std::atomic<int> launchPulse { 0 };
    LaunchListener launchListener;
    juce::RangedAudioParameter* launchParam = nullptr;
    int currentSR = 48000;
};

//==============================================================================
static bool endsWith (const std::string& s, const std::string& suffix)
{
    return s.size() >= suffix.size()
        && s.compare (s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// "menu{'Do':0;'Re#':1}"  ->  noms + valeurs Faust
static bool parseMenu (const std::string& s, juce::StringArray& names, std::vector<float>& values)
{
    if (s.compare (0, 4, "menu") != 0) return false;
    auto open = s.find ('{'), close = s.rfind ('}');
    if (open == std::string::npos || close == std::string::npos || close <= open) return false;

    const std::string body = s.substr (open + 1, close - open - 1);
    size_t pos = 0;
    while (pos < body.size())
    {
        size_t semi = body.find (';', pos);
        std::string item = body.substr (pos, semi == std::string::npos ? std::string::npos : semi - pos);
        pos = (semi == std::string::npos) ? body.size() : semi + 1;

        size_t a = item.find ('\'');
        size_t q = item.rfind ("':");     // le dernier "':" (les noms peuvent contenir ':')
        if (a == std::string::npos || q == std::string::npos || q <= a) continue;

        names.add (juce::String::fromUTF8 (item.substr (a + 1, q - a - 1).c_str()));
        values.push_back ((float) std::atof (item.substr (q + 2).c_str()));
    }
    return names.size() > 1;
}

static juce::String makeId (const std::string& path, std::set<std::string>& used)
{
    std::string id;
    for (unsigned char c : path) id += std::isalnum (c) ? (char) c : '_';
    std::string unique = id;
    for (int n = 2; used.count (unique) > 0; ++n) unique = id + "_" + std::to_string (n);
    used.insert (unique);
    return juce::String (unique);
}

// "Euler/Voix/Voix 2/Pan" -> "V2 Pan" ; "Euler/Global/Surface" -> "Surface"
static juce::String makeName (const std::string& path, const std::string& label)
{
    auto p = path.find ("Voix ");
    if (p != std::string::npos && p + 5 < path.size() && std::isdigit ((unsigned char) path[p + 5]))
        return juce::String ("V") + juce::String::charToString ((juce::juce_wchar) path[p + 5])
             + " " + juce::String::fromUTF8 (label.c_str());
    return juce::String::fromUTF8 (label.c_str());
}

//==============================================================================
EulerProcessor::EulerProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      impl (new Impl)
{
    impl->dsp.init (48000);
    impl->dsp.buildUserInterface (&impl->ui);

    std::set<std::string> usedIds;

    for (auto& it : impl->ui.items)
    {
        // ---- bargraphs : on retient les zones pour la visualisation
        if (it.kind == ParamUI::Item::Bargraph)
        {
            for (int v = 0; v < 3; ++v)
            {
                const std::string base = "Voix " + std::to_string (v + 1) + "/";
                auto& z = impl->viz[v];
                if      (endsWith (it.path, base + "viz_omega")) z.omega = it.zone;
                else if (endsWith (it.path, base + "viz_tilt"))  z.tilt  = it.zone;
                else if (endsWith (it.path, base + "viz_live"))  z.live  = it.zone;
                else if (endsWith (it.path, base + "viz_prog"))  z.prog  = it.zone;
            }
            if (endsWith (it.path, "/Progression")) impl->globalProg = it.zone;
            if (endsWith (it.path, "/health"))      impl->health     = it.zone;
            continue;
        }

        // ---- parametres
        const auto id   = juce::ParameterID { makeId (it.path, usedIds), 1 };
        const auto name = makeName (it.path, it.label);

        Impl::Binding b;
        b.zone = it.zone;

        if (it.kind == ParamUI::Item::Button)
        {
            b.kind  = Impl::kBool;
            b.isLaunch = endsWith (it.path, "Lancer la piece");
            b.param = new juce::AudioParameterBool (id, name, false);
            if (b.isLaunch) impl->launchParam = b.param;
        }
        else
        {
            juce::StringArray names;
            std::vector<float> values;

            if (parseMenu (it.get ("style"), names, values))
            {
                int def = 0; float best = 1e9f;
                for (int i = 0; i < (int) values.size(); ++i)
                    if (std::abs (values[(size_t) i] - it.init) < best) { best = std::abs (values[(size_t) i] - it.init); def = i; }

                b.kind  = Impl::kChoice;
                b.menu  = values;
                b.param = new juce::AudioParameterChoice (id, name, names, def);
            }
            else
            {
                float skew = 1.0f;
                if (it.get ("scale") == "log" && it.min > 0.0f)
                {
                    const float mid = std::sqrt (it.min * it.max);
                    skew = std::log (0.5f) / std::log ((mid - it.min) / (it.max - it.min));
                }
                juce::NormalisableRange<float> range (it.min, it.max, it.step > 0 ? it.step : 0.0f, skew);

                const int digits = it.step >= 1.0f ? 0 : (it.step >= 0.1f ? 1 : (it.step >= 0.01f ? 2 : 3));
                const juce::String unit (it.get ("unit"));
                auto attrs = juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction ([digits, unit] (float v, int)
                    {
                        const auto t = juce::String (v, digits);
                        return unit.isEmpty() ? t : t + " " + unit;
                    })
                    .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });

                b.kind  = Impl::kFloat;
                b.param = new juce::AudioParameterFloat (id, name, range, it.init, attrs);
            }
        }

        addParameter (b.param);
        paramPaths.emplace_back (it.path, b.param);
        impl->bindings.push_back (std::move (b));
    }

    if (impl->launchParam != nullptr)
    {
        impl->launchListener.pulse = &impl->launchPulse;
        impl->launchParam->addListener (&impl->launchListener);
    }
}

EulerProcessor::~EulerProcessor()
{
    if (impl->launchParam != nullptr)
        impl->launchParam->removeListener (&impl->launchListener);
}

//==============================================================================
void EulerProcessor::prepareToPlay (double sampleRate, int)
{
    // On ne reinitialise le DSP que si la frequence d'echantillonnage change vraiment.
    // Les hotes rappellent prepareToPlay a chaque lecture / changement de buffer : tout
    // remettre a zero interrompait la piece en cours (le son s'arretait).
    const int sr = sampleRate > 1000.0 ? (int) (sampleRate + 0.5) : 48000;
    if (sr != impl->currentSR)
    {
        impl->dsp.init (sr);
        impl->currentSR = sr;
    }
}

bool EulerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void EulerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // une note MIDI = "Lancer la piece"
    for (const auto meta : midi)
        if (meta.getMessage().isNoteOn())
            impl->launchPulse.store (2);

    const int pulse = impl->launchPulse.load();

    // parametres JUCE -> zones Faust
    for (auto& b : impl->bindings)
    {
        float v = 0.0f;
        switch (b.kind)
        {
            case Impl::kBool:
                v = static_cast<juce::AudioParameterBool*> (b.param)->get() ? 1.0f : 0.0f;
                if (b.isLaunch && pulse > 0) v = 1.0f;
                break;
            case Impl::kChoice:
            {
                int idx = static_cast<juce::AudioParameterChoice*> (b.param)->getIndex();
                idx = juce::jlimit (0, (int) b.menu.size() - 1, idx);
                v = b.menu[(size_t) idx];
                break;
            }
            case Impl::kFloat:
                v = static_cast<juce::AudioParameterFloat*> (b.param)->get();
                break;
        }
        *b.zone = v;
    }
    if (pulse > 0)
    {
        int expected = pulse;                       // decremente sans ecraser une nouvelle impulsion
        impl->launchPulse.compare_exchange_strong (expected, pulse - 1);
    }

    if (buffer.getNumChannels() >= 2)
    {
        const int n = buffer.getNumSamples();
        float* outs[2] = { buffer.getWritePointer (0), buffer.getWritePointer (1) };
        impl->dsp.compute (n, nullptr, outs);

        // --- WATCHDOG : un NaN / Inf dans un filtre recursif ne disparait jamais tout seul
        //     (le son restait coupe jusqu'au redemarrage). On le detecte et on remet le DSP a zero.
        bool bad = impl->health != nullptr && *impl->health > 0.5f;
        for (int ch = 0; ch < 2 && ! bad; ++ch)
            for (int i = 0; i < n; ++i)
                if (! std::isfinite (outs[ch][i]) || std::abs (outs[ch][i]) > 4.0f) { bad = true; break; }

        if (bad)
        {
            buffer.clear();
            impl->dsp.instanceClear();
        }

        for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
            buffer.clear (ch, 0, n);
    }
    else
    {
        buffer.clear();
    }
}

//==============================================================================
EulerProcessor::Viz EulerProcessor::getViz (int v) const
{
    Viz z;
    if (v < 0 || v > 2) return z;
    auto rd = [] (const FAUSTFLOAT* p, float d) { return p != nullptr ? (float) *p : d; };
    const auto& q = impl->viz[v];
    z.omega = rd (q.omega, 0.0f);
    z.tilt  = rd (q.tilt,  1.0f);
    z.live  = rd (q.live,  0.0f);
    z.prog  = rd (q.prog,  0.0f);
    return z;
}

float EulerProcessor::getGlobalProgress() const
{
    return impl->globalProg != nullptr ? (float) *impl->globalProg : 0.0f;
}

float EulerProcessor::actualValue (const juce::RangedAudioParameter* p)
{
    return p != nullptr ? p->convertFrom0to1 (p->getValue()) : 0.0f;
}

//==============================================================================
juce::AudioProcessorEditor* EulerProcessor::createEditor()
{
    return new PixelEditor (*this);
}

void EulerProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::XmlElement xml ("EULERDISK");
    xml.setAttribute ("slow", slowIndex.load());
    for (auto* p : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (r != impl->launchParam)          // LANCER n'est pas un reglage : jamais sauvegarde
                xml.setAttribute (r->getParameterID(), (double) r->getValue());
    copyXmlToBinary (xml, dest);
}

void EulerProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        slowIndex.store (juce::jlimit (0, 4, xml->getIntAttribute ("slow", 3)));
        for (auto* p : getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
                if (r != impl->launchParam && xml->hasAttribute (r->getParameterID()))
                    r->setValueNotifyingHost ((float) xml->getDoubleAttribute (r->getParameterID()));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EulerProcessor();
}
