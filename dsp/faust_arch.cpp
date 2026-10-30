/* Architecture Faust minimale : on ne genere que la classe DSP (EulerDSP).
   Le plugin JUCE (Source/) se charge de l'audio, des parametres et de l'UI. */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

#include "faust/dsp/dsp.h"
#include "faust/gui/meta.h"
#include "faust/gui/UI.h"

#ifndef FAUSTCLASS
#define FAUSTCLASS EulerDSP
#endif

<<includeIntrinsic>>

<<includeclass>>
