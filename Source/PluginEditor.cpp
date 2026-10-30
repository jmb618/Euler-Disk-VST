#include "PluginEditor.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

//==============================================================================
// Palette et police pixel 3x5
//==============================================================================
namespace Ui
{
    const juce::Colour bg        (0xff0d0d0d);
    const juce::Colour panel     (0xff121212);
    const juce::Colour line      (0xff2a2a2a);
    const juce::Colour dim       (0xff7a7a7a);
    const juce::Colour text      (0xffdcdcdc);
    const juce::Colour accent    (0xffff6a13);
    const juce::Colour accentDim (0xff6b2d0a);   // meme teinte, plus sombre (arriere des disques)
    const juce::Colour accentOrb (0xff3a1a08);   // orbite du point de contact

    struct Glyph { char c; const char* r[5]; };

    const Glyph font[] = {
        {'A',{"010","101","111","101","101"}}, {'B',{"110","101","110","101","110"}},
        {'C',{"011","100","100","100","011"}}, {'D',{"110","101","101","101","110"}},
        {'E',{"111","100","110","100","111"}}, {'F',{"111","100","110","100","100"}},
        {'G',{"011","100","101","101","011"}}, {'H',{"101","101","111","101","101"}},
        {'I',{"111","010","010","010","111"}}, {'J',{"001","001","001","101","010"}},
        {'K',{"101","101","110","101","101"}}, {'L',{"100","100","100","100","111"}},
        {'M',{"101","111","111","101","101"}}, {'N',{"110","101","101","101","101"}},
        {'O',{"010","101","101","101","010"}}, {'P',{"110","101","110","100","100"}},
        {'Q',{"010","101","101","110","011"}}, {'R',{"110","101","110","101","101"}},
        {'S',{"011","100","010","001","110"}}, {'T',{"111","010","010","010","010"}},
        {'U',{"101","101","101","101","111"}}, {'V',{"101","101","101","101","010"}},
        {'W',{"101","101","111","111","101"}}, {'X',{"101","101","010","101","101"}},
        {'Y',{"101","101","010","010","010"}}, {'Z',{"111","001","010","100","111"}},
        {'0',{"111","101","101","101","111"}}, {'1',{"010","110","010","010","111"}},
        {'2',{"110","001","010","100","111"}}, {'3',{"110","001","010","001","110"}},
        {'4',{"101","101","111","001","001"}}, {'5',{"111","100","110","001","110"}},
        {'6',{"011","100","111","101","111"}}, {'7',{"111","001","010","010","010"}},
        {'8',{"111","101","111","101","111"}}, {'9',{"111","101","111","001","110"}},
        {'.',{"000","000","000","000","010"}}, {'-',{"000","000","111","000","000"}},
        {':',{"000","010","000","010","000"}}, {'+',{"000","010","111","010","000"}},
        {'=',{"000","111","000","111","000"}}, {'(',{"010","100","100","100","010"}},
        {')',{"010","001","001","001","010"}}, {'/',{"001","001","010","100","100"}},
        {'#',{"101","111","101","111","101"}}, {'\'',{"010","010","000","000","000"}},
        {',',{"000","000","000","010","100"}}, {'_',{"000","000","000","000","111"}},
        {'%',{"101","001","010","100","101"}}, {'>',{"100","010","001","010","100"}},
        {'<',{"001","010","100","010","001"}},
    };

    const Glyph* findGlyph (juce::juce_wchar wc)
    {
        if (wc > 127) return nullptr;
        const char c = (char) std::toupper ((int) wc);
        for (auto& g : font)
            if (g.c == c) return &g;
        return nullptr;
    }

    int textW (const juce::String& s, int scale) { return juce::jmax (0, s.length() * 4 * scale - scale); }

    // texte pixel dans un Graphics (chaque pixel du glyphe = scale x scale)
    void drawText (juce::Graphics& g, const juce::String& s, int x, int y, int scale, juce::Colour c)
    {
        g.setColour (c);
        for (int n = 0; n < s.length(); ++n)
            if (auto* gl = findGlyph (s[n]))
                for (int row = 0; row < 5; ++row)
                    for (int col = 0; col < 3; ++col)
                        if (gl->r[row][col] == '1')
                            g.fillRect (x + (n * 4 + col) * scale, y + row * scale, scale, scale);
    }

    // reduit un nom de parametre pour le tenir dans la colonne de labels
    juce::String shortLabel (juce::String n)
    {
        if (n.length() > 3 && n[0] == 'V' && juce::CharacterFunctions::isDigit (n[1]) && n[2] == ' ')
            n = n.substring (3);
        n = n.upToFirstOccurrenceOf (" (", false, false);
        if (n.length() > 14) n = n.upToFirstOccurrenceOf (" ", false, false);
        return n.toUpperCase();
    }

    juce::String shortChoice (const juce::String& t, int maxChars)
    {
        juce::String s = t.replace ("Mystique - ", "MYS ").replace ("Occulte - ", "OCC ")
                          .replace ("Dark - ", "DRK ").replace ("Epic - ", "EPC ");
        if (s.length() > maxChars) s = s.substring (0, maxChars - 1) + ".";
        return s;
    }
}

//==============================================================================
// Rasteriseur pour les canevas de pixels (disques) + controles
//==============================================================================
namespace
{
    struct Raster
    {
        juce::Image::BitmapData& bd;

        void put (int x, int y, juce::Colour c)
        {
            if ((unsigned) x < (unsigned) bd.width && (unsigned) y < (unsigned) bd.height)
                bd.setPixelColour (x, y, c);
        }

        void clear (juce::Colour c)
        {
            for (int y = 0; y < bd.height; ++y)
                for (int x = 0; x < bd.width; ++x)
                    bd.setPixelColour (x, y, c);
        }

        void frame (juce::Colour c)
        {
            for (int x = 0; x < bd.width; ++x)  { put (x, 0, c); put (x, bd.height - 1, c); }
            for (int y = 0; y < bd.height; ++y) { put (0, y, c); put (bd.width - 1, y, c); }
        }

        void line (int x0, int y0, int x1, int y1, juce::Colour c)   // Bresenham
        {
            const int dx = std::abs (x1 - x0), sx = x0 < x1 ? 1 : -1;
            const int dy = -std::abs (y1 - y0), sy = y0 < y1 ? 1 : -1;
            int err = dx + dy;
            for (;;)
            {
                put (x0, y0, c);
                if (x0 == x1 && y0 == y1) break;
                const int e2 = 2 * err;
                if (e2 >= dy) { err += dy; x0 += sx; }
                if (e2 <= dx) { err += dx; y0 += sy; }
            }
        }

        void text (const juce::String& s, int x, int y, juce::Colour c)   // echelle 1
        {
            for (int n = 0; n < s.length(); ++n)
                if (auto* gl = Ui::findGlyph (s[n]))
                    for (int row = 0; row < 5; ++row)
                        for (int col = 0; col < 3; ++col)
                            if (gl->r[row][col] == '1') put (x + n * 4 + col, y + row, c);
        }
    };

    //==========================================================================
    // Controles lies aux parametres
    //==========================================================================
    constexpr int kLabelW = 120;
    constexpr int kValueW = 72;

    class ParamControl : public juce::Component
    {
    public:
        ParamControl (juce::RangedAudioParameter& prm, const juce::String& lbl)
            : p (prm), label (lbl), att (prm, [this] (float) { repaint(); }, nullptr)
        {
            setOpaque (true);
            att.sendInitialUpdate();
        }

    protected:
        void drawLabel (juce::Graphics& g) const
        {
            g.fillAll (Ui::bg);
            Ui::drawText (g, label, 0, (getHeight() - 10) / 2, 2, Ui::dim);
        }

        juce::RangedAudioParameter& p;
        juce::String label;
        juce::ParameterAttachment att;
    };

    //--------------------------------------------------------------------------
    class PixelSlider : public ParamControl
    {
    public:
        using ParamControl::ParamControl;

        void paint (juce::Graphics& g) override
        {
            drawLabel (g);
            const int h = getHeight(), tx = trackX(), tw = trackW();
            const float n = p.getValue();
            const int fillW = (int) (tw * n);

            g.setColour (Ui::line);
            g.fillRect (tx, h / 2 - 2, tw, 4);
            g.setColour (Ui::accent);
            g.fillRect (tx, h / 2 - 2, fillW, 4);
            g.setColour (Ui::text);
            g.fillRect (tx + fillW - 1, h / 2 - 7, 3, 14);

            const auto s = p.getCurrentValueAsText();
            Ui::drawText (g, s, getWidth() - Ui::textW (s, 2), (h - 10) / 2, 2, Ui::text);
        }

        void mouseDown (const juce::MouseEvent& e) override { att.beginGesture(); setFromX (e.x); }
        void mouseDrag (const juce::MouseEvent& e) override { setFromX (e.x); }
        void mouseUp (const juce::MouseEvent&) override     { att.endGesture(); }

        void mouseDoubleClick (const juce::MouseEvent&) override
        {
            att.setValueAsCompleteGesture (p.convertFrom0to1 (p.getDefaultValue()));
        }

        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
        {
            const float n = juce::jlimit (0.0f, 1.0f, p.getValue() + w.deltaY * 0.05f);
            att.setValueAsCompleteGesture (snap (p.convertFrom0to1 (n)));
        }

    private:
        int trackX() const { return kLabelW + 4; }
        int trackW() const { return juce::jmax (10, getWidth() - trackX() - kValueW - 6); }

        float snap (float v) const { return p.getNormalisableRange().snapToLegalValue (v); }

        void setFromX (int x)
        {
            const float n = juce::jlimit (0.0f, 1.0f, (float) (x - trackX()) / (float) trackW());
            att.setValueAsPartOfGesture (snap (p.convertFrom0to1 (n)));
        }
    };

    //--------------------------------------------------------------------------
    class PixelChoice : public ParamControl
    {
    public:
        using ParamControl::ParamControl;

        void paint (juce::Graphics& g) override
        {
            drawLabel (g);
            const int h = getHeight(), x0 = kLabelW + 4, w = getWidth() - x0;
            g.setColour (Ui::line);
            g.drawRect (x0, 1, w, h - 2, 1);

            const int maxChars = juce::jmax (4, (w - 16) / 8);
            Ui::drawText (g, Ui::shortChoice (p.getCurrentValueAsText(), maxChars), x0 + 6, (h - 10) / 2, 2, Ui::text);
            Ui::drawText (g, ">", getWidth() - 12, (h - 10) / 2, 2, Ui::accent);
        }

        void mouseDown (const juce::MouseEvent&) override
        {
            auto* ch = dynamic_cast<juce::AudioParameterChoice*> (&p);
            if (ch == nullptr) return;

            juce::PopupMenu m;
            for (int i = 0; i < ch->choices.size(); ++i)
                m.addItem (i + 1, ch->choices[i], true, i == ch->getIndex());

            juce::Component::SafePointer<PixelChoice> safe (this);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                             [safe] (int result)
                             {
                                 if (safe != nullptr && result > 0)
                                     safe->att.setValueAsCompleteGesture ((float) (result - 1));
                             });
        }

        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
        {
            auto* ch = dynamic_cast<juce::AudioParameterChoice*> (&p);
            if (ch == nullptr) return;
            const int idx = juce::jlimit (0, ch->choices.size() - 1, ch->getIndex() + (w.deltaY > 0 ? -1 : 1));
            att.setValueAsCompleteGesture ((float) idx);
        }
    };

    //--------------------------------------------------------------------------
    class PixelToggle : public ParamControl
    {
    public:
        using ParamControl::ParamControl;

        void paint (juce::Graphics& g) override
        {
            drawLabel (g);
            const bool on = p.getValue() > 0.5f;
            const int h = getHeight(), bx = getWidth() - 18, by = (h - 16) / 2;
            g.setColour (on ? Ui::accent : Ui::line);
            g.drawRect (bx, by, 16, 16, 2);
            if (on) g.fillRect (bx + 4, by + 4, 8, 8);
            const juce::String s = on ? "ON" : "OFF";
            Ui::drawText (g, s, bx - 8 - Ui::textW (s, 2), (h - 10) / 2, 2, on ? Ui::accent : Ui::dim);
        }

        void mouseDown (const juce::MouseEvent&) override
        {
            att.setValueAsCompleteGesture (p.getValue() > 0.5f ? 0.0f : 1.0f);
        }
    };

    //--------------------------------------------------------------------------
    // bouton maintenu : LANCER (la note MIDI fait la meme chose)
    class PixelHold : public ParamControl
    {
    public:
        using ParamControl::ParamControl;

        void paint (juce::Graphics& g) override
        {
            g.fillAll (down ? Ui::accent : Ui::bg);
            g.setColour (Ui::accent);
            g.drawRect (getLocalBounds(), 2);
            Ui::drawText (g, label, (getWidth() - Ui::textW (label, 3)) / 2, (getHeight() - 15) / 2, 3,
                          down ? Ui::bg : Ui::accent);
        }

        void mouseDown (const juce::MouseEvent&) override
        {
            down = true;
            att.beginGesture();
            att.setValueAsPartOfGesture (1.0f);
            repaint();
        }

        void mouseUp (const juce::MouseEvent&) override
        {
            att.setValueAsPartOfGesture (0.0f);
            att.endGesture();
            down = false;
            repaint();
        }

    private:
        bool down = false;
    };

    //--------------------------------------------------------------------------
    // selecteur local (non automatisable) : facteur de ralentissement de l'affichage
    class PixelCycle : public juce::Component
    {
    public:
        PixelCycle (const juce::String& lbl, const juce::StringArray& o, int start, std::function<void (int)> cb)
            : label (lbl), options (o), index (start), onChange (std::move (cb))
        {
            setOpaque (true);
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (Ui::bg);
            const int h = getHeight(), x0 = kLabelW + 4, w = getWidth() - x0;
            Ui::drawText (g, label, 0, (h - 10) / 2, 2, Ui::dim);
            g.setColour (Ui::line);
            g.drawRect (x0, 1, w, h - 2, 1);
            Ui::drawText (g, options[index], x0 + 6, (h - 10) / 2, 2, Ui::text);
            Ui::drawText (g, ">", getWidth() - 12, (h - 10) / 2, 2, Ui::accent);
        }

        void mouseDown (const juce::MouseEvent&) override { step (1); }
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
        {
            step (w.deltaY > 0 ? -1 : 1);
        }

    private:
        void step (int d)
        {
            const int n = options.size();
            index = ((index + d) % n + n) % n;
            if (onChange) onChange (index);
            repaint();
        }

        juce::String label;
        juce::StringArray options;
        int index;
        std::function<void (int)> onChange;
    };
}

//==============================================================================
// Disque fil de fer 3D (un par voix)
//==============================================================================
class DiscView : public juce::Component
{
public:
    static constexpr int LW = 106, LH = 72, SCALE = 3;   // 318 x 216 pixels ecran

    DiscView (int voiceIndex, juce::RangedAudioParameter* diam)
        : voice (voiceIndex), diamParam (diam), img (juce::Image::ARGB, LW, LH, true)
    {
        setOpaque (true);
        setSize (LW * SCALE, LH * SCALE);
        if (diamParam != nullptr)
        {
            att = std::make_unique<juce::ParameterAttachment> (*diamParam, [] (float) {}, nullptr);
            setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        }
    }

    // dt en secondes ; slowK = facteur de ralentissement de l'affichage
    void update (double dt, const EulerProcessor::Viz& z, float dmm, float hrel, float slowK)
    {
        // la precession tourne EXACTEMENT a Omega / slowK tours par seconde
        if (z.live > 0.02f)
            angle = std::fmod (angle + (double) z.omega / (double) slowK * dt, 1.0);

        render (z, dmm, hrel, slowK);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);   // pixels francs
        g.drawImage (img, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
    }

    // glisser verticalement = diametre
    void mouseDown (const juce::MouseEvent&) override
    {
        if (diamParam != nullptr) startNorm = diamParam->getValue();
        dragging = false;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (diamParam == nullptr || att == nullptr) return;
        if (! dragging) { att->beginGesture(); dragging = true; }
        const float n = juce::jlimit (0.0f, 1.0f, startNorm - (float) e.getDistanceFromDragStartY() / 220.0f);
        att->setValueAsPartOfGesture (diamParam->getNormalisableRange().snapToLegalValue (diamParam->convertFrom0to1 (n)));
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragging && att != nullptr) att->endGesture();
        dragging = false;
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        if (att != nullptr && diamParam != nullptr)
            att->setValueAsCompleteGesture (diamParam->convertFrom0to1 (diamParam->getDefaultValue()));
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        if (att == nullptr || diamParam == nullptr) return;
        const float n = juce::jlimit (0.0f, 1.0f, diamParam->getValue() + w.deltaY * 0.04f);
        att->setValueAsCompleteGesture (diamParam->getNormalisableRange().snapToLegalValue (diamParam->convertFrom0to1 (n)));
    }

private:
    static juce::String fmtFreq (float v) { return v < 100.0f ? juce::String (v, 1) : juce::String (juce::roundToInt (v)); }

    void render (const EulerProcessor::Viz& z, float dmm, float hrel, float slowK)
    {
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::readWrite);
        Raster r { bd };
        r.clear (Ui::panel);
        r.frame (Ui::line);

        // ---- geometrie du disque
        const float frac = std::log (juce::jlimit (30.0f, 1000.0f, dmm) / 30.0f) / std::log (1000.0f / 30.0f);
        const float R  = 8.0f + 22.0f * frac;                              // rayon en pixels
        const float hh = juce::jlimit (1.0f, R * 0.18f, hrel * R);          // demi-epaisseur

        // inclinaison : decroit quand Omega augmente (alpha ~ 1/Omega^2), exageree pour rester lisible
        const float alphaDeg = z.prog >= 0.999f ? 3.0f : 5.0f + 30.0f * z.tilt * z.tilt;
        const float al = juce::degreesToRadians (alphaDeg);
        const float sa = std::sin (al), ca = std::cos (al);

        // precession : azimut de l'inclinaison
        const float psi = (float) angle * juce::MathConstants<float>::twoPi;
        const float sp = std::sin (psi), cp = std::cos (psi);

        const float se = 0.4226f, ce = 0.9063f;            // camera : elevation 25 deg
        const float cx = (float) (LW / 2), cy = 50.0f;
        const float Cz = R * sa + hh * ca;                 // hauteur du centre (le bord bas touche la table)

        struct P3 { float sx, sy, depth; };
        auto pt = [&] (float lx, float ly, float side) -> P3
        {
            const float wx = lx * (-sp) + ly * (-ca * cp) + side * hh * (sa * cp);
            const float wy = lx * cp    + ly * (-ca * sp) + side * hh * (sa * sp);
            const float wz = Cz + ly * sa + side * hh * ca;
            return { cx + wx, cy - wz * ce + wy * se, wy };
        };
        auto seg = [&] (const P3& a, const P3& b)
        {
            const bool back = (a.depth + b.depth) < 0.0f;      // partie arriere : teinte sombre
            r.line (juce::roundToInt (a.sx), juce::roundToInt (a.sy),
                    juce::roundToInt (b.sx), juce::roundToInt (b.sy),
                    back ? Ui::accentDim : Ui::accent);
        };

        // ---- orbite du point de contact sur la table (pointillee)
        const float g0 = R * ca - hh * sa;
        for (int i = 0; i < 96; i += 2)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 96.0f;
            r.put (juce::roundToInt (cx + g0 * std::cos (a)), juce::roundToInt (cy + g0 * std::sin (a) * se), Ui::accentOrb);
        }

        // ---- anneaux haut et bas + aretes
        constexpr int N = 48;
        std::array<P3, N> top, bot;
        for (int i = 0; i < N; ++i)
        {
            const float th = juce::MathConstants<float>::twoPi * (float) i / (float) N;
            top[(size_t) i] = pt (R * std::cos (th), R * std::sin (th), +1.0f);
            bot[(size_t) i] = pt (R * std::cos (th), R * std::sin (th), -1.0f);
        }
        for (int i = 0; i < N; ++i)
        {
            seg (top[(size_t) i], top[(size_t) ((i + 1) % N)]);
            seg (bot[(size_t) i], bot[(size_t) ((i + 1) % N)]);
            if (i % 4 == 0) seg (top[(size_t) i], bot[(size_t) i]);
        }

        // ---- rayons sur la face du dessus : le disque roule sans tourner sur lui-meme,
        //      son repere tourne de -psi*cos(alpha) par rapport a l'axe d'inclinaison
        const float thm = -psi * ca;
        const auto hub = pt (0.0f, 0.0f, +1.0f);
        seg (hub, pt (R * std::cos (thm), R * std::sin (thm), +1.0f));
        seg (hub, pt (0.55f * R * std::cos (thm + juce::MathConstants<float>::halfPi),
                      0.55f * R * std::sin (thm + juce::MathConstants<float>::halfPi), +1.0f));

        // ---- point de contact (bloc 3x3, meme couleur)
        const int gx = juce::roundToInt (cx + g0 * cp), gy = juce::roundToInt (cy + g0 * sp * se);
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                r.put (gx + dx, gy + dy, Ui::accent);

        // ---- textes
        const auto id = juce::String ("VOIX ") + juce::String (voice + 1);
        const auto dm = juce::String (juce::roundToInt (dmm)) + " MM";
        const auto om = "OMEGA " + fmtFreq (z.omega) + " HZ";
        const float revS = z.omega / slowK;
        const auto rv = juce::String (revS, revS < 10.0f ? 2 : 1) + " T/S";

        r.text (id, 4, 4, Ui::dim);
        r.text (dm, LW - 4 - juce::jmax (0, dm.length() * 4 - 1), 4, Ui::text);
        r.text (om, 4, LH - 9, Ui::dim);
        r.text (rv, LW - 4 - juce::jmax (0, rv.length() * 4 - 1), LH - 9, Ui::accent);
    }

    int voice;
    juce::RangedAudioParameter* diamParam;
    std::unique_ptr<juce::ParameterAttachment> att;
    juce::Image img;
    double angle = 0.0;           // tours de precession affiches
    float startNorm = 0.0f;
    bool dragging = false;
};

//==============================================================================
// Petit tube a vide en pixels : il s'allume avec la saturation
//==============================================================================
class TubeView : public juce::Component
{
public:
    TubeView (juce::RangedAudioParameter* on, juce::RangedAudioParameter* drive, juce::RangedAudioParameter* warm)
        : onP (on), driveP (drive), warmP (warm)
    {
        setOpaque (true);

        const char* rows[AH] = {
            "....XXXXXX....",
            "..XXooooooXX..",
            ".XooooooooooX.",
            ".XooooooooooX.",
            "XooooooooooooX", "XooooooooooooX", "XooooooooooooX", "XooooooooooooX", "XooooooooooooX",
            "XooooooooooooX", "XooooooooooooX", "XooooooooooooX", "XooooooooooooX", "XooooooooooooX",
            ".XooooooooooX.",
            ".XXXXXXXXXXXX.",
            "..XXXXXXXXXX..",
            "..X..X..X..X..",
            "..X..X..X..X..",
        };
        for (int y = 0; y < AH; ++y) art[(size_t) y] = rows[y];

        for (int y = 3; y <= 13; ++y)                        // plaques
            for (int x : { 4, 9 })
                if (art[(size_t) y][(size_t) x] == 'o') art[(size_t) y][(size_t) x] = 'P';
        for (int y = 5; y <= 11; ++y)                        // filament
            for (int x : { 6, 7 })
                if (art[(size_t) y][(size_t) x] == 'o') art[(size_t) y][(size_t) x] = 'F';

        setSize (AW * SC, AH * SC);
    }

    void tick (double dt)
    {
        float target = 0.0f;
        if (onP != nullptr && onP->getValue() > 0.5f)
        {
            const float d = driveP != nullptr ? juce::jmax (1.0f, EulerProcessor::actualValue (driveP)) : 1.0f;
            const float w = warmP != nullptr ? EulerProcessor::actualValue (warmP) : 0.0f;
            const float k = juce::jlimit (0.0f, 1.0f, std::log (d) / std::log (12.0f));
            target = (0.12f + 0.88f * std::pow (k, 0.85f)) * (0.9f + 0.1f * juce::jlimit (0.0f, 1.0f, w / 0.8f));
        }

        glow += (target - glow) * juce::jmin (1.0f, (float) dt * 6.0f);
        if (std::abs (glow - drawn) > 0.003f) { drawn = glow; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        static const float bayer[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
        const juce::Colour grey (0xff5a5a5a), plate (0xff3a3a3a), fil (0xff333333), hot (0xffffd9b0);

        g.fillAll (Ui::bg);
        for (int y = 0; y < AH; ++y)
            for (int x = 0; x < AW; ++x)
            {
                const char c = art[(size_t) y][(size_t) x];
                juce::Colour col;
                switch (c)
                {
                    case 'X': col = grey.interpolatedWith (Ui::accent, drawn); break;
                    case 'P': col = plate.interpolatedWith (Ui::accent, drawn * 0.8f); break;
                    case 'F': col = fil.interpolatedWith (Ui::accent, drawn)
                                      .interpolatedWith (hot, juce::jmax (0.0f, (drawn - 0.7f) / 0.3f) * 0.6f); break;
                    case 'o':
                    {
                        const float d = std::abs ((float) x - 6.5f) / 6.5f;
                        const float gl = drawn * (1.05f - 0.9f * d);
                        if (gl <= (bayer[y & 3][x & 3] + 0.5f) / 16.0f) continue;
                        col = Ui::accent.withMultipliedBrightness (0.35f + 0.65f * drawn);
                        break;
                    }
                    default: continue;
                }
                g.setColour (col);
                g.fillRect (x * SC, y * SC, SC, SC);
            }
    }

private:
    static constexpr int AW = 14, AH = 19, SC = 5;
    std::array<std::string, AH> art;
    juce::RangedAudioParameter *onP, *driveP, *warmP;
    float glow = 0.0f, drawn = 0.0f;
};

//==============================================================================
class ProgressStrip : public juce::Component
{
public:
    explicit ProgressStrip (EulerProcessor& p) : proc (p) { setOpaque (true); }

    void tick()
    {
        const float v = proc.getGlobalProgress();
        if (v != last) { last = v; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (Ui::line);
        g.fillRect (0, 0, getWidth(), getHeight());
        g.setColour (Ui::text);
        g.fillRect (0, 0, (int) (getWidth() * juce::jlimit (0.0f, 1.0f, last)), getHeight());
    }

private:
    EulerProcessor& proc;
    float last = -1.0f;
};

//==============================================================================
// Editeur
//==============================================================================
namespace
{
    constexpr int kM = 20, kCW = 318, kGap = 16, kRow = 24, kCtl = 22;
    constexpr int kW = 2 * kM + 3 * kCW + 2 * kGap;     // 1026
    constexpr int kDiscY = 72, kDiscH = 216;
    constexpr int kVoiceRowsY = kDiscY + kDiscH + 8;          // 296
    constexpr int kDiv2 = kVoiceRowsY + 4 * kRow + 12;        // 404
    constexpr int kTitle2Y = kDiv2 + 8, kLine2Y = kTitle2Y + 14, kGlobalY = kLine2Y + 6;   // 412 / 426 / 432
    constexpr int kDiv3 = kGlobalY + 4 * kRow + 12;           // 540
    constexpr int kTitle3Y = kDiv3 + 8, kLine3Y = kTitle3Y + 14, kFootY = kLine3Y + 6;     // 548 / 562 / 568
    constexpr int kH = kFootY + 4 * kRow + 28;                // 660

    int colX (int k) { return kM + k * (kCW + kGap); }

    const float kSlowK[5] = { 1.0f, 5.0f, 10.0f, 20.0f, 50.0f };

    void sortBy (std::vector<EulerProcessor::ParamEntry>& v, const juce::StringArray& keys)
    {
        auto rank = [&] (const EulerProcessor::ParamEntry& e) -> int
        {
            const auto n = e.second->getName (64);
            for (int i = 0; i < keys.size(); ++i)
                if (n.containsIgnoreCase (keys[i])) return i;
            return 999;
        };
        std::stable_sort (v.begin(), v.end(),
                          [&] (const EulerProcessor::ParamEntry& a, const EulerProcessor::ParamEntry& b)
                          { return rank (a) < rank (b); });
    }

    juce::Component* makeControl (juce::RangedAudioParameter& par)
    {
        const auto name = Ui::shortLabel (par.getName (64));
        if (dynamic_cast<juce::AudioParameterBool*> (&par) != nullptr)   return new PixelToggle (par, name);
        if (dynamic_cast<juce::AudioParameterChoice*> (&par) != nullptr) return new PixelChoice (par, name);
        return new PixelSlider (par, name);
    }

    juce::RangedAudioParameter* findByName (const std::vector<EulerProcessor::ParamEntry>& v, const char* key)
    {
        for (auto& e : v)
            if (e.second->getName (64).containsIgnoreCase (key)) return e.second;
        return nullptr;
    }
}

PixelEditor::PixelEditor (EulerProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    using Entry = EulerProcessor::ParamEntry;

    // ---- classement des parametres (par chemin Faust)
    std::vector<Entry> voice[3], global, sat;
    juce::RangedAudioParameter* launch = nullptr;
    juce::RangedAudioParameter* pos = nullptr;

    for (auto& e : proc.getParamEntries())
    {
        const std::string& path = e.first;
        int v = -1;
        for (int k = 0; k < 3; ++k)
            if (path.find ("Voix " + std::to_string (k + 1) + "/") != std::string::npos) v = k;

        if (v >= 0)                                               voice[v].push_back (e);
        else if (path.find ("Lancer") != std::string::npos)       launch = e.second;
        else if (path.find ("Position") != std::string::npos)     pos = e.second;
        else if (path.find ("Saturation/") != std::string::npos)  sat.push_back (e);
        else                                                      global.push_back (e);
    }

    const juce::StringArray voiceKeys  { "Diametre", "Vitesse", "Pan", "Niveau" };
    const juce::StringArray globalKeys { "Surface", "Gamme", "Tonique", "Duree", "Inclinaison", "Epaisseur",
                                         "Inharm", "Resonance", "Grondement", "Douceur", "Reverb", "Volume" };
    const juce::StringArray satKeys    { "Saturation", "Drive", "Chaleur" };

    for (auto& v : voice) sortBy (v, voiceKeys);
    sortBy (global, globalKeys);
    sortBy (sat, satKeys);

    for (int v = 0; v < 3; ++v) diamParam[v] = findByName (voice[v], "Diametre");
    hrelParam = findByName (global, "Epaisseur");

    auto place = [this] (juce::Component* c, int x, int y, int w, int h)
    {
        comps.add (c);
        addAndMakeVisible (c);
        c->setBounds (x, y, w, h);
    };

    // ---- en-tete : LANCER, POSITION, progression
    if (launch != nullptr) place (new PixelHold (*launch, "LANCER"), 170, 14, 110, 30);
    if (pos != nullptr)    place (makeControl (*pos), 300, 16, kW - kM - 300, kCtl);

    progress = new ProgressStrip (proc);
    place (progress, 300 + kLabelW + 4, 44, kW - kM - 300 - kLabelW - 4 - kValueW - 6, 4);

    // ---- les trois disques + leurs reglages dessous
    for (int v = 0; v < 3; ++v)
    {
        discs[v] = new DiscView (v, diamParam[v]);
        place (discs[v], colX (v), kDiscY, kCW, kDiscH);

        for (size_t i = 0; i < voice[v].size(); ++i)
            place (makeControl (*voice[v][i].second), colX (v), kVoiceRowsY + (int) i * kRow, kCW, kCtl);
    }

    // ---- reglages globaux, groupes en 3 colonnes
    const int perCol = juce::jmax (1, (int) (global.size() + 2) / 3);
    for (size_t i = 0; i < global.size(); ++i)
    {
        const int col = juce::jmin (2, (int) i / perCol), row = (int) i % perCol;
        place (makeControl (*global[i].second), colX (col), kGlobalY + row * kRow, kCW, kCtl);
    }

    // ---- pied : saturation + tube + affichage
    for (size_t i = 0; i < sat.size(); ++i)
        place (makeControl (*sat[i].second), colX (0), kFootY + (int) i * kRow, kCW, kCtl);

    tube = new TubeView (findByName (sat, "Saturation"), findByName (sat, "Drive"), findByName (sat, "Chaleur"));
    place (tube, colX (1) + (kCW - tube->getWidth()) / 2, kTitle3Y, tube->getWidth(), tube->getHeight());

    place (new PixelCycle ("RALENTI", juce::StringArray { "1/1", "1/5", "1/10", "1/20", "1/50" },
                           proc.getSlowIndex(), [this] (int i) { proc.setSlowIndex (i); }),
           colX (2), kFootY, kCW, kCtl);

    setSize (kW, kH);
    lastMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

PixelEditor::~PixelEditor() { stopTimer(); }

void PixelEditor::paint (juce::Graphics& g)
{
    g.fillAll (Ui::bg);

    Ui::drawText (g, "EULER DISK", kM, 22, 3, Ui::text);

    g.setColour (Ui::line);
    g.fillRect (kM, 62, kW - 2 * kM, 1);          // sous l'en-tete
    g.fillRect (kM, kDiv2, kW - 2 * kM, 1);         // sous les voix
    g.fillRect (kM, kDiv3, kW - 2 * kM, 1);         // sous les reglages globaux

    const char* titles[3] = { "PIECE", "METAL", "SORTIE" };
    for (int k = 0; k < 3; ++k)
    {
        Ui::drawText (g, titles[k], colX (k), kTitle2Y, 2, Ui::accent);
        g.setColour (Ui::line);
        g.fillRect (colX (k), kLine2Y, kCW, 1);
    }

    Ui::drawText (g, "SATURATION", colX (0), kTitle3Y, 2, Ui::accent);
    Ui::drawText (g, "AFFICHAGE",  colX (2), kTitle3Y, 2, Ui::accent);
    g.setColour (Ui::line);
    g.fillRect (colX (0), kLine3Y, kCW, 1);
    g.fillRect (colX (2), kLine3Y, kCW, 1);

    Ui::drawText (g, "GLISSER UN DISQUE = DIAMETRE", colX (2), kFootY + 36, 2, Ui::dim);
    Ui::drawText (g, "DOUBLE CLIC = REINITIALISER",  colX (2), kFootY + 52, 2, Ui::dim);
    Ui::drawText (g, "NOTE MIDI = LANCER",           colX (2), kFootY + 68, 2, Ui::dim);
}

void PixelEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double dt = juce::jlimit (0.0, 0.1, (now - lastMs) / 1000.0);
    lastMs = now;

    const float K = kSlowK[juce::jlimit (0, 4, proc.getSlowIndex())];
    const float hrel = hrelParam != nullptr ? EulerProcessor::actualValue (hrelParam) : 0.05f;

    for (int v = 0; v < 3; ++v)
    {
        const float d = diamParam[v] != nullptr ? EulerProcessor::actualValue (diamParam[v]) : 300.0f;
        discs[v]->update (dt, proc.getViz (v), d, hrel, K);
    }
    tube->tick (dt);
    progress->tick();
}
