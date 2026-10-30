#pragma once
// Collecte les widgets Faust (sliders, boutons, bargraphs) + leurs metadonnees.
#include <map>
#include <string>
#include <vector>

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif
#include "faust/gui/meta.h"
#include "faust/gui/UI.h"

struct ParamUI : public UI
{
    struct Item
    {
        enum Kind { Button, Slider, Bargraph };
        Kind kind = Slider;
        std::string path;    // ex. "Euler/Voix/Voix 1/Diametre" (sans la boite racine)
        std::string label;
        FAUSTFLOAT* zone = nullptr;
        float init = 0, min = 0, max = 1, step = 0;
        std::map<std::string, std::string> meta;

        std::string get (const char* key) const
        {
            auto it = meta.find (key);
            return it == meta.end() ? std::string() : it->second;
        }
    };

    std::vector<Item> items;

    // --- groupes
    void openTabBox (const char* l)        { groups.push_back (l); }
    void openHorizontalBox (const char* l) { groups.push_back (l); }
    void openVerticalBox (const char* l)   { groups.push_back (l); }
    void closeBox()                        { if (! groups.empty()) groups.pop_back(); }

    // --- widgets
    void addButton (const char* l, FAUSTFLOAT* z)      { add (Item::Button, l, z, 0, 0, 1, 1); }
    void addCheckButton (const char* l, FAUSTFLOAT* z) { add (Item::Button, l, z, 0, 0, 1, 1); }
    void addVerticalSlider (const char* l, FAUSTFLOAT* z, FAUSTFLOAT i, FAUSTFLOAT a, FAUSTFLOAT b, FAUSTFLOAT s)
        { add (Item::Slider, l, z, i, a, b, s); }
    void addHorizontalSlider (const char* l, FAUSTFLOAT* z, FAUSTFLOAT i, FAUSTFLOAT a, FAUSTFLOAT b, FAUSTFLOAT s)
        { add (Item::Slider, l, z, i, a, b, s); }
    void addNumEntry (const char* l, FAUSTFLOAT* z, FAUSTFLOAT i, FAUSTFLOAT a, FAUSTFLOAT b, FAUSTFLOAT s)
        { add (Item::Slider, l, z, i, a, b, s); }
    void addHorizontalBargraph (const char* l, FAUSTFLOAT* z, FAUSTFLOAT a, FAUSTFLOAT b)
        { add (Item::Bargraph, l, z, a, a, b, 0); }
    void addVerticalBargraph (const char* l, FAUSTFLOAT* z, FAUSTFLOAT a, FAUSTFLOAT b)
        { add (Item::Bargraph, l, z, a, a, b, 0); }
    void addSoundfile (const char*, const char*, Soundfile**) {}

    // --- metadonnees ([style:menu{...}], [scale:log], [unit:s] ...)
    void declare (FAUSTFLOAT* z, const char* key, const char* val)
    {
        if (z != nullptr) pending[z][key] = val;
    }

private:
    std::vector<std::string> groups;
    std::map<FAUSTFLOAT*, std::map<std::string, std::string>> pending;

    static std::string trim (std::string s)
    {
        while (! s.empty() && s.back() == ' ') s.pop_back();
        size_t i = 0; while (i < s.size() && s[i] == ' ') ++i;
        return s.substr (i);
    }

    void add (Item::Kind k, const char* label, FAUSTFLOAT* z, float init, float mn, float mx, float step)
    {
        Item it;
        it.kind = k;
        it.label = trim (label);
        it.zone = z;
        it.init = init; it.min = mn; it.max = mx; it.step = step;
        // chemin sans la boite racine (le nom du DSP)
        for (size_t i = 1; i < groups.size(); ++i) it.path += trim (groups[i]) + "/";
        it.path += it.label;
        auto m = pending.find (z);
        if (m != pending.end()) { it.meta = m->second; pending.erase (m); }
        items.push_back (std::move (it));
    }
};
