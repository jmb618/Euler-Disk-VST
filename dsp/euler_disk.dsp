declare name "3 Disques d'Euler - modele Darmendrail & Muller";
declare description "Trois disques d'Euler de 3 cm a 1 m, precession selon Omega^2 = 4g/(R alpha) et loi de puissance Omega ~ (tf-t)^-n. Compensation de resonance, saturation tube optionnelle, limiteur anti-clip. 31 gammes mystiques / occultes / dark / epic.";
declare version "7.1-stable";

import("stdfaust.lib");

//==============================================================
//  3 DISQUES D'EULER (de 30 mm a 1 m)
//
//  Base : L. Darmendrail & A. Muller, "The Euler disk and its
//  dynamic finite time singularity", Acoust. Sci. & Tech. 42(4),
//  2021.
//
//  Elements du modele (numeros d'equations du papier) :
//   (1) Omega^2 = 4 g / (R alpha)  : la precession (= hauteur du
//       "chirp" audible) depend du rayon R et de l'inclinaison alpha.
//       -> un grand disque precesse plus lentement : Omega ~ R^-1/2
//   (4) Omega ~ (tf - t)^-n : singularite en temps fini, avec un
//       exposant n qui depend de la dissipation (menu "Surface") :
//          jouet sur miroir concave n = 0,20   (Bildsten : 2/9)
//          verre / miroir plat      n = 0,26
//          table vinyle             n = 0,41
//          couvercle                n = 0,56
//          grille de ventilateur    n = 0,59
//          visqueux (Moffatt)       n = 1/6
//   Calibrage : disque de 75 mm (jouet) -> Omega = 70 Hz a 4,5 s
//   de l'arret, ~ 220 Hz a la fin (balayage mesure dans le papier).
//
//  Le son : un choc par tour de precession (frequence Omega) excite
//  les modes de flexion du disque ("ringing", Kessler & O'Reilly),
//  plus un "toc" de contact, un roulement et un grondement.
//
//  NOVEAUTES v6 : GESTION DU NIVEAU
//   1. Compensation de resonance : plus "Resonance du metal" est
//      grande, plus le volume baisse (les chocs se superposent et
//      s'accumulent). Le gain suit ~ (0,25 / tau0)^0,6.
//   2. Bouton "Saturation tube" :
//        OFF -> aucune distorsion, le son reste propre.
//        ON  -> saturation douce asymetrique type triode
//               (harmoniques paires + impaires), reglable par
//               "Drive" et "Chaleur".
//   3. Limiteur "brickwall" a anticipation (1,3 ms) toujours actif
//      en sortie : le signal ne peut jamais depasser 0 dBFS, meme
//      avec le reverb, la saturation ou une resonance extreme.
//
//  NOVEAUTES v7
//   1. 31 gammes et accords classes Mystique / Occulte / Dark / Epic
//      (menu "Gamme / accord"), 8 notes max par gamme : chaque disque
//      se cale sur la note la plus proche de la gamme.
//   2. Niveaux baisses : marge interne de -6 dB par voix, Niveau des
//      voix a 0,5 et Volume general a 0,25 a l'ouverture.
//   3. Ouverture : Douceur 0,01 (ouvert), Epaisseur basse (0,05),
//      diametres eleves (800, 500, 300 mm).
//   4. "Douceur" est maintenant un curseur 0 a 1 (0 = filtre ouvert
//      a 16 kHz, 1 = sombre a 1 kHz).
//==============================================================

nV = 3;

//------------------------- Interface globale ------------------
lancer = button("v:Euler/h:Global/[0]Lancer la piece");

pos    = hslider("v:Euler/h:Global/[1]Position (reculer / avancer)", 0, 0, 1, 0.0001) : si.smoo;

surf   = hslider("v:Euler/h:Global/[2]Surface (dissipation)[style:menu{'Miroir concave - jouet (n=0.20)':0;'Verre plat (n=0.26)':1;'Table vinyle (n=0.41)':2;'Couvercle (n=0.56)':3;'Grille de ventilateur (n=0.59)':4;'Visqueux Bildsten (n=2/9)':5;'Visqueux Moffatt (n=1/6)':6}]", 0, 0, 6, 1);

chord  = hslider("v:Euler/h:Global/[3]Gamme / accord[style:menu{'Libre (tailles reelles)':0;'Mystique - Scriabin Promethee':1;'Mystique - Gamme par tons':2;'Mystique - Spectral 4-5-6-7-9-11':3;'Mystique - Enigmatique (Verdi)':4;'Mystique - Hirajoshi':5;'Mystique - Pelog (gamelan)':6;'Mystique - Lydien augmente':7;'Mystique - Lydien accord maj7 sharp11':8;'Mystique - Quintes cosmiques':9;'Mystique - Quartes superposees':10;'Occulte - Octatonique ton-demi':11;'Occulte - Double harmonique (Byzantin)':12;'Occulte - Phrygien dominant (Hijaz)':13;'Occulte - Dorien quarte augmentee':14;'Occulte - In Sen (Japon)':15;'Occulte - Raga Todi':16;'Occulte - Diabolus in musica':17;'Dark - Locrien':18;'Dark - Phrygien':19;'Dark - Mineur harmonique':20;'Dark - Hongrois mineur (Tzigane)':21;'Dark - Napolitain mineur':22;'Dark - Accord Tristan':23;'Dark - Diminue 7e':24;'Epic - Mineur naturel (Eolien)':25;'Epic - Dorien':26;'Epic - Mixolydien':27;'Epic - Lydien cinema':28;'Epic - Puissance (quintes et octaves)':29;'Epic - Sus2 heroique':30;'Epic - Mineur 11 ouvert':31}]", 1, 0, 31, 1);

tonic  = hslider("v:Euler/h:Global/[4]Tonique[style:menu{'Do':0;'Do#':1;'Re':2;'Re#':3;'Mi':4;'Fa':5;'Fa#':6;'Sol':7;'Sol#':8;'La':9;'La#':10;'Si':11}]", 0, 0, 11, 1);

duree  = hslider("v:Euler/h:Global/[5]Duree (disque de 75 mm) [unit:s]", 20, 6, 90, 0.1);
a0deg  = hslider("v:Euler/h:Global/[6]Inclinaison initiale [unit:deg]", 15, 5, 40, 0.5);
hrel   = hslider("v:Euler/h:Global/[7]Epaisseur relative (h/D)", 0.05, 0.03, 0.25, 0.001);
inharm = hslider("v:Euler/h:Global/[8]Inharmonicite (0 = pur, 1 = disque reel)", 0.7, 0, 1, 0.01);
tau0   = hslider("v:Euler/h:Global/[9]Resonance du metal [unit:s]", 0.25, 0.05, 1.5, 0.01);
grave  = hslider("v:Euler/h:Global/[10]Grondement de precession", 0.5, 0, 1, 0.01) : si.smoo;
douceurAmt = hslider("v:Euler/h:Global/[11]Douceur (0 = ouvert, 1 = sombre)", 0.01, 0, 1, 0.001) : si.smoo;
// 0 -> coupure 16 kHz (ouvert) ; 1 -> coupure 1 kHz (sombre), echelle logarithmique
douceur = min(ma.SR * 0.45, 16000.0 * pow(0.0625, douceurAmt));
wet    = hslider("v:Euler/h:Global/[12]Reverb", 0.15, 0, 0.8, 0.01) : si.smoo;
vol    = hslider("v:Euler/h:Global/[13]Volume", 0.25, 0, 1, 0.01) : si.smoo;

// --- saturation (nouveau)
satOn  = checkbox("v:Euler/h:Saturation/[0]Saturation tube (on/off)");
drive  = hslider("v:Euler/h:Saturation/[1]Drive", 3, 1, 12, 0.01) : si.smoo;
warm   = hslider("v:Euler/h:Saturation/[2]Chaleur (harmoniques paires)", 0.25, 0, 0.8, 0.01) : si.smoo;

//---------------------- Compensation de resonance -------------
// Plus la resonance est longue, plus les chocs successifs se
// superposent et l'energie croit ~ tau0 : on baisse le gain en
// consequence (1.0 a tau0 = 0,25 s, donc le son par defaut ne change pas).
rcomp = min(1.5, pow(0.25 / max(0.05, tau0), 0.85)) : si.smoo;

//---------------------- Exposant de la loi de puissance -------
nexp = (0.20, 0.26, 0.41, 0.56, 0.59, 0.2222, 0.1667) : ba.selectn(7, surf);

//---------------------- Gammes / accords (demi-tons) ----------
// 31 gammes de 8 notes maximum (rembourrees par repetition de la
// derniere note). Chaque disque est cale sur la note la plus proche
// de la gamme choisie (octaves libres), transposee par la Tonique.
// chord = 0 : Libre (hauteurs reelles) ; chord = 1..31 : gammes ci-dessous.
nScales = 31;
nNotes  = 8;

note(c, d) = ba.take(nNotes * c + d + 1,
    (
      0, 6, 10, 16, 21, 26, 26, 26,   //  1 Mystique - Scriabin Promethee
      0, 2, 4, 6, 8, 10, 10, 10,   //  2 Mystique - Gamme par tons
      0, 3.863, 7.02, 9.689, 14.039, 17.513, 17.513, 17.513,   //  3 Mystique - Spectral 4-5-6-7-9-11
      0, 1, 4, 6, 8, 10, 11, 11,   //  4 Mystique - Enigmatique (Verdi)
      0, 2, 3, 7, 8, 8, 8, 8,   //  5 Mystique - Hirajoshi
      0, 1, 3, 7, 8, 8, 8, 8,   //  6 Mystique - Pelog (gamelan)
      0, 2, 4, 6, 8, 9, 11, 11,   //  7 Mystique - Lydien augmente
      0, 4, 7, 11, 14, 18, 18, 18,   //  8 Mystique - Lydien accord maj7 sharp11
      0, 7, 14, 21, 28, 35, 35, 35,   //  9 Mystique - Quintes cosmiques
      0, 5, 10, 15, 20, 25, 25, 25,   // 10 Mystique - Quartes superposees
      0, 2, 3, 5, 6, 8, 9, 11,   // 11 Occulte - Octatonique ton-demi
      0, 1, 4, 5, 7, 8, 11, 11,   // 12 Occulte - Double harmonique (Byzantin)
      0, 1, 4, 5, 7, 8, 10, 10,   // 13 Occulte - Phrygien dominant (Hijaz)
      0, 2, 3, 6, 7, 9, 10, 10,   // 14 Occulte - Dorien quarte augmentee
      0, 1, 5, 7, 10, 10, 10, 10,   // 15 Occulte - In Sen (Japon)
      0, 1, 3, 6, 7, 8, 11, 11,   // 16 Occulte - Raga Todi
      0, 1, 6, 7, 7, 7, 7, 7,   // 17 Occulte - Diabolus in musica
      0, 1, 3, 5, 6, 8, 10, 10,   // 18 Dark - Locrien
      0, 1, 3, 5, 7, 8, 10, 10,   // 19 Dark - Phrygien
      0, 2, 3, 5, 7, 8, 11, 11,   // 20 Dark - Mineur harmonique
      0, 2, 3, 6, 7, 8, 11, 11,   // 21 Dark - Hongrois mineur (Tzigane)
      0, 1, 3, 5, 7, 8, 11, 11,   // 22 Dark - Napolitain mineur
      0, 6, 10, 15, 15, 15, 15, 15,   // 23 Dark - Accord Tristan
      0, 3, 6, 9, 9, 9, 9, 9,   // 24 Dark - Diminue 7e
      0, 2, 3, 5, 7, 8, 10, 10,   // 25 Epic - Mineur naturel (Eolien)
      0, 2, 3, 5, 7, 9, 10, 10,   // 26 Epic - Dorien
      0, 2, 4, 5, 7, 9, 10, 10,   // 27 Epic - Mixolydien
      0, 2, 4, 6, 7, 9, 11, 11,   // 28 Epic - Lydien cinema
      0, 7, 12, 19, 24, 31, 31, 31,   // 29 Epic - Puissance (quintes et octaves)
      0, 2, 7, 9, 14, 19, 19, 19,   // 30 Epic - Sus2 heroique
      0, 3, 7, 10, 14, 17, 17, 17    // 31 Epic - Mineur 11 ouvert
    ));

chordIdx = max(0, chord - 1);

chordRow(d) = par(c, nScales, note(c, d)) : ba.selectn(nScales, chordIdx);

// note MIDI de la gamme la plus proche de m
snapNote(m) = n7
with {
    p(d)  = fmod(tonic + chordRow(d), 12);
    c(d)  = p(d) + 12 * floor((m - p(d)) / 12 + 0.5);
    dd(d) = abs(m - c(d));
    n0 = c(0);                                  d0 = dd(0);
    n1 = select2(dd(1) < d0, n0, c(1));         d1 = min(d0, dd(1));
    n2 = select2(dd(2) < d1, n1, c(2));         d2 = min(d1, dd(2));
    n3 = select2(dd(3) < d2, n2, c(3));         d3 = min(d2, dd(3));
    n4 = select2(dd(4) < d3, n3, c(4));         d4 = min(d3, dd(4));
    n5 = select2(dd(5) < d4, n4, c(5));         d5 = min(d4, dd(5));
    n6 = select2(dd(6) < d5, n5, c(6));         d6 = min(d5, dd(6));
    n7 = select2(dd(7) < d6, n6, c(7));
};

//---------------------- Diametres / temps commun --------------
diam(i) = hslider("v:Euler/h:Voix/v:Voix %j/[0]Diametre [unit:mm][scale:log]",
                  ba.take(i + 1, (800, 500, 300)), 30, 1000, 1)
with { j = i + 1; };

tdur(i) = duree * pow(diam(i) / 75.0, 0.5);          // duree du disque i (s)
Tmax    = max(tdur(0), max(tdur(1), tdur(2)));       // duree de l'ensemble

//---------------------- Progression ---------------------------
trigL   = lancer > lancer';
started = max(trigL) ~ _;

dt = 1.0 / (Tmax * ma.SR);

acc = loop ~ _
with {
    loop(prev) = select2(trigL, min(1, prev + started * dt), 0);
};

u = min(1, pos + acc) : hbargraph("v:Euler/h:Global/[14]Progression", 0, 1);

active = max(started, pos > 0.0005);

alpha0 = a0deg * ma.PI / 180.0;

//---------------------- Un disque (sortie stereo) -------------
voice(i) = out <: *(gl), *(gr)
with {
    j   = i + 1;
    dm  = diam(i);
    Rm  = dm / 2000.0;                                   // rayon en metres

    spd = hslider("v:Euler/h:Voix/v:Voix %j/[1]Vitesse (x)", 1, 0.5, 2, 0.01);
    pan = hslider("v:Euler/h:Voix/v:Voix %j/[2]Pan",
                  ba.take(i + 1, (-0.6, 0.0, 0.6)), -1, 1, 0.01) : si.smoo;
    lvl = hslider("v:Euler/h:Voix/v:Voix %j/[3]Niveau", 0.5, 0, 1, 0.01) : si.smoo;

    gl = sqrt((1 - pan) / 2);
    gr = sqrt((1 + pan) / 2);

    // --- cinematique : Eq. (1) et (4) du papier
    Ti   = tdur(i);
    r    = u * Tmax / Ti;
    uu   = min(1, r);
    tauR = max(0.002, (1 - uu) * Ti);                     // temps restant (s)
    tref = 4.5 * pow(dm / 75.0, 0.5);                     // repere de la phase finale (s)

    fRef = 70.0 * sqrt(0.0375 / Rm);                      // Omega a tref (Hz), calibre sur le jouet de 75 mm
    fSt  = sqrt(4.0 * 9.81 / (Rm * alpha0)) / (2 * ma.PI);// Omega initial (Eq. 1), alpha = alpha0

    // avant tref : croissance plus rapide, de fSt (debut) a fRef (tref)
    n2   = max(nexp, log(max(1.0, fRef / fSt))
                     / max(0.01, log(max(1.01, Ti / tref))));
    rt   = tauR / tref;
    omega = min(600, spd * fRef * pow(rt, 0 - select2(rt > 1, nexp, n2)));

    phase = (+(omega / ma.SR) : ma.frac) ~ _;
    wrap  = phase < phase';

    // inclinaison relative (alpha ~ 1/Omega^2, Eq. 1) -> intensite des chocs
    tilt4 = sqrt(min(1.0, fSt * spd / omega));
    lev   = 0.3 + 0.7 * pow(min(1.0, omega / 300.0), 0.6);

    ended   = r >= 1;
    endTrig = ended > ended';
    live    = active * (1 - ended);
    gcont   = live : si.smooth(ba.tau2pole(0.025));

    // --- hauteur : f ~ (h/D) / D, eventuellement calee sur l'accord
    f1free = max(40.0, 2200.0 * (hrel / 0.133) * (75.0 / dm));
    mfree  = 12 * (log(f1free / 440.0) / log(2.0)) + 69;
    fnote  = ba.midikey2hz(select2(chord > 0, mfree, snapNote(mfree)))
           : si.smooth(ba.tau2pole(0.03));

    // --- modes de flexion du disque
    ratio(k) = (1 - inharm) * (k + 1)
             + inharm * ba.take(k + 1, (1.0, 1.73, 2.33, 3.91, 4.11, 5.96));
    frk(k)   = fnote * ratio(k);
    fk(k)    = min(ma.SR * 0.45, frk(k));
    alive(k) = frk(k) < ma.SR * 0.42;
    tauS     = tau0 * pow(dm / 75.0, 0.3);                // un gros disque resonne plus longtemps
    tauk(k)  = tauS / (1 + 0.4 * (ratio(k) - 1));
    qk(k)    = max(1, ma.PI * fk(k) * tauk(k));
    sc(k)    = 0.06 * tauk(k) * ma.SR;
    wk(k)    = ba.take(k + 1, (1.0, 0.8, 0.6, 0.45, 0.35, 0.25)) * alive(k);

    bank(x) = x <: par(k, 6, *(sc(k)) : fi.resonbp(fk(k), qk(k), wk(k))) :> _;

    // --- excitation : un choc par tour de precession
    pulseAmp = 0.35 + 0.65 * tilt4;
    noiseV   = no.noise : @(i * 1013);
    excRaw   = wrap * gcont * pulseAmp * (0.75 + 0.25 * noiseV);
    exc      = excRaw : fi.lowpass(1, 3000);
    clack    = endTrig * active * 3.0;

    // compensation : chocs qui se superposent (omega * tauS) ET resonance globale (rcomp)
    comp = rcomp / sqrt(1.0 + omega * tauS);

    hits = (exc + clack) : bank : *(comp);

    // --- "toc" grave de contact avec la surface (plus grave pour un grand disque)
    fth   = 180.0 * pow(75.0 / dm, 0.5);
    thump = exc : fi.resonbp(fth, 2.5, 1) : *(40.0 * tilt4 * comp);

    // --- roulement (bruit) + grondement module par la precession
    modm    = pow(0.5 + 0.5 * cos(4 * ma.PI * phase), 2);
    rlvl    = 0.25 * sqrt(omega / 600.0);
    rolling = no.noise : @(i * 1013 + 500)
            : fi.resonbp(min(ma.SR * 0.45, fnote * 1.4), 1.2, 1.5)
            : *(modm * rlvl * gcont);
    rumble  = no.noise : @(i * 1013 + 900) : fi.lowpass(2, 150)
            : *(5.0 * (0.4 + 0.6 * modm) * rlvl * gcont);

    // --- la precession elle-meme (le "chirp" du papier) : Omega et ses harmoniques
    whirl = (os.osc(omega) * 0.5 + os.osc(2 * omega) * 0.35 + os.osc(3 * omega) * 0.2)
          * (0.2 * grave * lev * gcont * min(1.0, omega / 30.0));

    // marge interne (-6 dB) : les 3 voix + resonances ne doivent pas saturer
    vscale = 0.5;
    sig = ((hits + thump) * lev + rolling + rumble + whirl) * lvl * vscale;

    // --- sorties de visualisation (lues par l'interface pixel du plugin ;
    //     sans aucun effet sur le son)
    vOmega = omega : hbargraph("v:Euler/h:Voix/v:Voix %j/viz_omega", 0, 600);
    vTilt  = tilt4 : hbargraph("v:Euler/h:Voix/v:Voix %j/viz_tilt", 0, 1);
    vLive  = gcont : hbargraph("v:Euler/h:Voix/v:Voix %j/viz_live", 0, 1);
    vProg  = uu    : hbargraph("v:Euler/h:Voix/v:Voix %j/viz_prog", 0, 1);

    out = attach(sig, vOmega + vTilt + vLive + vProg);
};

//---------------------- Saturation tube (optionnelle) ----------
// Courbe tanh polarisee : y = tanh(drive*x + b) - tanh(b)
// -> ecretage asymetrique (demi-onde positive et negative n'ecretent
//    pas au meme niveau) = harmoniques paires + impaires, comme une
//    triode. Le DC introduit par la polarisation est retire, et le
//    niveau de sortie est normalise pour que le drive ne fasse pas
//    exploser le volume.
satMix = satOn : si.smooth(ba.tau2pole(0.02));

tube = *(drive) : +(warm) : ma.tanh : (_ - ma.tanh(warm))
     : fi.dcblocker
     : *(0.85 / max(0.2, ma.tanh(drive)));

satCh = _ <: *(1 - satMix), (tube : *(satMix)) :> _;

satStage = par(k, 2, satCh);

//---------------------- Limiteur brickwall a anticipation ------
// Garantit |s| <= 0,95 : le gain est calcule sur le signal non retarde,
// l'audio est retarde de N-1 echantillons, donc la reduction de gain
// est deja complete quand le pic arrive. Attaque instantanee lissee
// par une moyenne glissante (pas de clic), relachement lent (150 ms).
limN    = 64;                                   // ~1,3 ms a 48 kHz
ceiling = 0.95;
rr      = 1.0 / (0.15 * ma.SR);

relf = ((*(1 - rr) : +(rr)), _ : min) ~ _;      // attaque immediate, retour lent vers 1

// IMPORTANT : on n'utilise PAS ba.slidingMean (integrateur : l'erreur d'arrondi
// s'accumule, le gain finit par deriver vers 0 et le son se coupe definitivement
// jusqu'au redemarrage). Moyenne par somme directe des N derniers echantillons :
// aucune memoire infinie, donc aucune derive possible.
meanN(n) = _ <: par(i, n, @(i)) :> _ : /(n);

// minimum glissant (min-hold) sur N echantillons : le gain est deja au plus bas
// sur toute la fenetre d'anticipation, puis la moyenne le lisse sans jamais
// depasser le gain requis -> plafond garanti, sans clic.
minN(n) = _ <: par(i, n, @(i)) : ba.parallelMin(n);

// protection : un echantillon non fini (NaN / Inf) ou aberrant est remplace par 0
safe(x) = select2(abs(x) < 1000.0, 0.0, x);

gcalc = (abs, abs : max) : max(1e-6) : (ceiling / _) : min(1)
      : relf : minN(limN) : meanN(limN);

limiter = safe, safe <: (@(limN - 1), @(limN - 1), gcalc)
        : route(3, 4, (1, 1), (3, 2), (2, 3), (3, 4))
        : *, *
        : par(k, 2, max(-0.99) : min(0.99));    // filet de securite ultime

//---------------------- Surveillance (watchdog) ----------------
// Drapeau VERROUILLE : passe a 1 des qu'un echantillon non fini (NaN / Inf) ou
// aberrant arrive devant le limiteur. Le plugin (PluginProcessor.cpp) lit ce
// drapeau apres chaque bloc et remet le DSP a zero si besoin : un NaN dans un
// filtre recursif ne peut ainsi JAMAIS couper le son jusqu'au redemarrage.
isBad(x) = 1.0 - (abs(x) < 1000.0);
guard    = \(l, r).(attach(l, (max(isBad(l), isBad(r)) : (max ~ _)
                                : hbargraph("v:Euler/h:Global/health", 0, 1))), r);

//---------------------- Master ---------------------------------
reverb = _, _ <: (*(1 - wet), *(1 - wet),
                  (re.zita_rev1_stereo(20, 200, 6000, 3, 2, 48000) : *(wet), *(wet)))
         :> _, _;

process = par(i, nV, voice(i)) :> _, _
        : par(k, 2, *(vol) : fi.dcblocker : fi.lowpass(2, douceur))
        : satStage
        : reverb
        : guard
        : limiter;
