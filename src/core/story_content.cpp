#include "core/story.h"

#include <string>

#include "core/collectible.h"

namespace eco {

namespace {

[[nodiscard]] bool IsCleanSnapshot(const StorySnapshot& s) noexcept {
    return s.solar != MachineState::BypassedDirty &&
           s.hydro != MachineState::BypassedDirty &&
           s.pendulum != MachineState::BypassedDirty &&
           s.harmonic != MachineState::BypassedDirty &&
           s.thermal != MachineState::BypassedDirty &&
           s.astrolabe != MachineState::BypassedDirty;
}

} // namespace

// Tutti i testi destinati al rendering usano esclusivamente caratteri ASCII (32..126)
// con apostrofi (e', piu', civilta', perche', cosi') per compatibilita' universale.

const char* ChapterTitle(Chapter c) {
    switch (c) {
        case Chapter::Prologo:
            return "PROLOGO - La Nebbia sull'Altopiano";
        case Chapter::Atto1_Luce:
            return "ATTO I - La Misura della Luce";
        case Chapter::Atto2_Acqua:
            return "ATTO II - Il Peso dell'Acqua";
        case Chapter::Atto3_Pendolo:
            return "ATTO III - Il Tempo del Pendolo";
        case Chapter::Atto4_Giardini:
            return "ATTO IV - L'Accordo dei Giardini";
        case Chapter::Atto5_Fumarole:
            return "ATTO V - Le Ceneri della Frattura";
        case Chapter::Epilogo:
            return "EPILOGO - Il Cuore di Meridia";
    }
    return "EQUILIBRIO - Le Cronache di Meridia";
}

std::vector<DialogueLine> DialogueForEvent(StoryEvent e, const StorySnapshot& s) {
    switch (e) {
        case StoryEvent::GameStarted:
            return {
                {"Elena Valli", "L'aria sulle Terrazze dell'Alba sa di cenere e pietra bagnata. Ecco finalmente la Conca di Meridia."},
                {"Elena Valli", "Gli specchi dell'Eliostato sono spenti da anni, ma la mia Lente Empirica [F] puo' ancora misurare la luce."},
                {"Matteo Verrini", "Dottoressa Valli, la vedo salire dalle terrazze. Se vuole corrente subito senza perdere la giornata, apra la fornace con [B]."},
                {"Elena Valli", "Sono salita fin qui per fermare il fumo che scende sui villaggi, Matteo. Prima misuro l'Eliostato con [F]."}
            };

        case StoryEvent::FirstScan:
            return {
                {"Elena Valli", "Lo gnomone di bronzo non mente: il raggio solare incide a 45 gradi esatti sulle lastre di travertino."},
                {"Elena Valli", "E' la legge del coseno di Lambert: il rendimento dipende dall'angolo tra lo specchio e il fascio di luce."},
                {"Elena Valli", "Mi avvicino al meccanismo [E], allineo l'Eliostato a 45 gradi e confermo la misura con [C]."}
            };

        case StoryEvent::SolarClean:
            return {
                {"Elena Valli", "Oltre l'ottanta per cento di rendimento! La pietra si scalda di luce pulita, senza un solo filo di fumo."},
                {"Elena Valli", "La Lente Empirica si e' tarata sul campo: ora posso attivare il Rilievo Metrico di Fase 2 con [V]."},
                {"Clara Sereni", "Ehi, laggiu' sulle terrazze! Ho visto accendersi gli specchi senza una sbuffata di pece. Raggiungimi al Quartiere dei Canali!"},
                {"Elena Valli", "Quella dev'essere Clara Sereni, l'ingegnera delle chiuse. Saliamo verso nord al Quartiere dei Canali per parlarle [E]."}
            };

        case StoryEvent::SolarBypassed:
            return {
                {"Matteo Verrini", "Vede, dottoressa? Con la fornace di emergenza i condotti si scaldano in un istante, senza stare a inseguire le ombre."},
                {"Elena Valli", "La corrente scorre, ma una coltre scura sta gia' calando sugli ulivi delle terrazze..."},
                {"Clara Sereni", "Chi ha acceso di nuovo la pece sulle terrazze?! Finche' bruciate scorie, il Ponte degli Archi resta chiuso!"},
                {"Elena Valli", "Clara non aprira' mai il passaggio con la fornace accesa. Posso tornare all'Eliostato [E] e calibrarlo con la luce."}
            };

        case StoryEvent::ClaraRecruited:
            return {
                {"Clara Sereni", "Ho controllato il flusso sui morsetti di bronzo. Niente fuliggine, solo angolo e pazienza. Avevi ragione tu, Elena."},
                {"Clara Sereni", "Tolgo il sigillo dal Ponte degli Archi: ora tutta la meta' orientale di Meridia, dai Giardini al Santuario, e' aperta."},
                {"Clara Sereni", "Tieni la mia lente strutturale. Premi [V] per la Trama Geodetica: vedrai le linee di forza che i Meridiani incisero nella pietra."},
                {"Eco di Meridia", "Quando due sguardi misurano senza fretta, il ponte torna a unire cio' che l'inverno aveva diviso."},
                {"Elena Valli", "Hai sentito quel canto salire dai cristalli, Clara? Scendiamo nelle Cisterne Ipogee a nord-ovest, poi esploreremo oltre il ponte."}
            };

        case StoryEvent::EnteredCanalQuarter:
            return {
                {"Elena Valli", "Il Quartiere dei Canali... Le vasche a gradini portano ancora i segni dell'acqua limpida che un tempo nutriva l'altopiano."},
                {"Elena Valli", "A est vedo il grande Ponte degli Archi, e tra le volte a ovest l'imbocco delle Cisterne Ipogee."}
            };

        case StoryEvent::EnteredWindPlateau:
            return {
                {"Elena Valli", "Sull'Altopiano dei Venti il basalto taglia la brezza e i cipressi si piegano tutti lungo la stessa rosa scolpita nella roccia."},
                {"Elena Valli", "In fondo alla cresta si staglia la Torre del Pendolo: i suoi oculi di pietra sembrano ancora attendere il mezzogiorno."}
            };

        case StoryEvent::EnteredHangingGardens:
            return {
                {"Elena Valli", "Oltre il Ponte degli Archi si aprono i Giardini Sospesi. Anche sotto la polvere si avverte il profumo dei semi dormienti."},
                {"Elena Valli", "Tra le vasche di giada si intravedono gli anelli del Risonatore Armonico e qualcuno che si prende cura delle aiuole."}
            };

        case StoryEvent::EnteredFumaroleCaldera:
            return {
                {"Elena Valli", "La Caldera delle Fumarole... Qui nacque l'Inverno di Pece. Le vecchie fornaci hanno segnato il basalto come cicatrici."},
                {"Elena Valli", "Vicino all'ingresso della Camera Geotermica scorgo la figura di Matteo Verrini, immerso nel vapore."}
            };

        case StoryEvent::EnteredHarmonySanctuary:
            return {
                {"Elena Valli", "Il Santuario dell'Armonia. Sotto i miei passi un intero mosaico astronomico di travertino, bronzo e giada."},
                {"Elena Valli", "Al centro della piazza riposa l'Astrolabio di Meridia: i suoi quattro anelli attendono che l'intera Conca ritrovi l'equilibrio."}
            };

        case StoryEvent::EnteredCisterns:
            return {
                {"Elena Valli", "Le Cisterne Ipogee. L'eco delle cascate riempie la volta, e lungo queste passerelle dovro' muovermi con attenzione [SPAZIO]."},
                {"Elena Valli", "La legge di Stevino governa questo bacino: a dieci metri di colonna d'acqua la pressione tocca 98.100 Pascal."},
                {"Elena Valli", "Sul fondo del bacino mi attende la Turbina Idrostatica; piu' in alto, verso est, intravedo il bagliore di una lanterna."}
            };

        case StoryEvent::EnteredPendulumTower:
            return {
                {"Elena Valli", "Dentro la Torre del Pendolo la luce piove dagli oculi come polvere d'oro. Ogni ballatoio sale verso il grande braccio d'ottone."},
                {"Elena Valli", "Galileo insegno' che il periodo del pendolo dipende solo dalla sua lunghezza: due metri esatti per 2,84 secondi di cadenza."}
            };

        case StoryEvent::EnteredGeothermalChamber:
            return {
                {"Elena Valli", "Colonne esagonali di basalto e archi di rame ossidato: la Camera Geotermica custodisce il calore naturale della faglia."},
                {"Elena Valli", "Con la sorgente fredda delle cisterne a 300 Kelvin, portare lo scambiatore di Carnot a 600 Kelvin garantisce il 50% di rendimento pulito."}
            };

        case StoryEvent::HydroClean:
            return {
                {"Elena Valli", "98.100 Pascal esatti! La turbina gira senza cavitazione e l'acqua torna a cantare limpida lungo le volte."},
                {"Eco di Meridia", "L'acqua non obbedisce al fuoco, ma al proprio peso. Chi ascolta la caduta della pietra non resta mai senz'acqua."},
                {"Lorenzo Altieri", "Da quassu' sento di nuovo il passo giusto delle paratoie! Se hai fiato per salire sui ballatoi alti a destra, vieni a parlarmi."}
            };

        case StoryEvent::HydroBypassed:
            return {
                {"Matteo Verrini", "Una pompa a pece fa salire pressione in tre secondi, dottoressa. Non sara' elegante, ma tiene in moto le condotte."},
                {"Elena Valli", "Un velo nero si allarga sulla superficie della cisterna. Il rimbombo della fornace soffoca il suono della cascata."},
                {"Eco di Meridia", "Una sorgente ferita ricorda a lungo il sapore del fumo."}
            };

        case StoryEvent::LorenzoRecruited:
            if (s.relics_collected >= 6) {
                return {
                    {"Lorenzo Altieri", "Elena Valli... Vedo dal tuo taccuino che hai gia' raccolto molte pietre parlate delle quattro ere. I Meridiani ti sorriderebbero."},
                    {"Lorenzo Altieri", "Io sono l'ultimo archivista di queste volte. Quando lo Scisma accese le fornaci dell'Era III, i miei avi chiusero qui la memoria della Conca."},
                    {"Lorenzo Altieri", "Prendi questa chiave di bronzo: apre la via profonda verso la Camera Geotermica e conferma che non sei venuta a saccheggiare."},
                    {"Elena Valli", "Ricostruiremo l'intera cronaca nel Codex [J], Lorenzo. E riaccenderemo ogni monumento senza piu' avvelenare l'altopiano."}
                };
            }
            return {
                {"Lorenzo Altieri", "Finalmente passi leggeri sui ballatoi alti. Sono Lorenzo Altieri, custode di quel che resta degli archivi di Meridia."},
                {"Lorenzo Altieri", "Qui sotto abbiamo salvato la storia delle quattro ere: dall'Alba delle Ombre fino alla notte in cui la paura del freddo porto' la pece."},
                {"Lorenzo Altieri", "Tieni la chiave delle camere profonde e cerca le reliquie sparse nei distretti: solo con la memoria intera l'Astrolabio si aprira' davvero."},
                {"Elena Valli", "Custodiro' ogni frammento nel Diario [J], Lorenzo. Ora proseguiamo il cammino lungo la Conca."}
            };

        case StoryEvent::PendulumClean:
            return {
                {"Elena Valli", "Due metri precisi di braccio: 2,84 secondi a ogni oscillazione! Il pendolo batte come un cuore di bronzo."},
                {"Clara Sereni", "Sento il battito arrivare fin dentro le chiuse dei canali, Elena! Tutta la rete idraulica si sta sincronizzando."},
                {"Eco di Meridia", "Il tempo non si affretta ne' si forza: chi trova la lunghezza giusta accorda la pietra alle stagioni."}
            };

        case StoryEvent::PendulumBypassed:
            return {
                {"Matteo Verrini", "Ho fatto innestare un volano a combustione sul perno della torre. Fa un baccano infernale, ma il ritmo regge."},
                {"Elena Valli", "Ogni colpo del bruciatore fa tremare le vetrate della torre. Non e' armonia, e' soltanto rumore forzato."},
                {"Eco di Meridia", "Un orologio spinto dal fumo misura solo il tempo che resta prima del silenzio."}
            };

        case StoryEvent::MiriamMet:
            if (s.pollution_level <= 25.0f) {
                return {
                    {"Miriam Ferro", "Benvenuta nei Giardini Sospesi, Elena. Senti? Da quando l'aria si e' schiarita, perfino i vecchi ulivi hanno cambiato profumo."},
                    {"Miriam Ferro", "Io sono Miriam. Ho custodito i semi dormienti per anni, aspettando che qualcuno sapesse riaccordare il Risonatore Armonico a 120 Hertz."},
                    {"Elena Valli", "I cristalli di giada vibrano gia' al solo avvicinarsi. Calibro il Risonatore [E] e restituiamo il cielo ai tuoi giardini."}
                };
            }
            return {
                {"Miriam Ferro", "Sei riuscita ad attraversare il Ponte degli Archi! Sono Miriam Ferro, botanica dei Giardini Sospesi."},
                {"Miriam Ferro", "La cenere soffoca le foglie e impedisce ai semi antichi di schiudersi. Il Risonatore qui accanto nasceva per pulire l'aria a 120 Hertz."},
                {"Elena Valli", "Se lo accordiamo sulla frequenza di risonanza senza usare la fornace, la polvere cadra' al suolo e le serre torneranno a vivere."}
            };

        case StoryEvent::HarmonicClean:
            return {
                {"Elena Valli", "Centoventi Hertz esatti! Gli anelli di vetro-giada cantano all'unisono e la foschia sopra i giardini si dissolve!"},
                {"Miriam Ferro", "Guarda le terrazze, Elena! Le foglie si aprono alla luce e i pollini tornano a danzare tra le vasche!"},
                {"Eco di Meridia", "Quando il suono e' puro, la polvere ritorna terra e la luce torna a nutrire le radici."}
            };

        case StoryEvent::HarmonicBypassed:
            return {
                {"Matteo Verrini", "Soffianti a pece avviati a pieno regime: la torre orientale eroga potenza, anche se l'odore non piacera' ai giardinieri."},
                {"Miriam Ferro", "Guarda i boccioli che stavano per aprirsi, Elena... Con questo fumo dovro' richiudere di nuovo i semi sotto vetro."},
                {"Elena Valli", "Mi dispiace, Miriam. Posso ancora spegnere la fornace e accordare il Risonatore a 120 Hertz."}
            };

        case StoryEvent::MatteoConfronted:
            if (IsCleanSnapshot(s)) {
                return {
                    {"Matteo Verrini", "Cosi' sei arrivata fin nella Caldera senza accendere nemmeno una delle mie fornaci. Devo ammettere che non ci credevo."},
                    {"Matteo Verrini", "Non guardarmi come un nemico, Elena. Vent'anni fa i villaggi a valle gelavano e nessuno sapeva leggere Stevino o Carnot. La pece li ha salvati."},
                    {"Elena Valli", "Lo so, Matteo. Ho letto i verbali dello Scisma: nessuno accese quelle fornaci per malizia. Ma oggi conosciamo di nuovo la via pulita."},
                    {"Matteo Verrini", "La Camera Geotermica e' qui sotto. Se davvero sai trarne calore a seicento Kelvin senza fumo, mostramelo con i fatti."}
                };
            }
            return {
                {"Matteo Verrini", "Finalmente di persona, dottoressa Valli. Vedo che almeno una volta hai capito perche' la Compagnia della Pece esiste."},
                {"Matteo Verrini", "E' facile giudicare il fumo dai libri, ma quando il freddo scende sui tetti a valle la gente chiede calore stasera, non tra un anno."},
                {"Elena Valli", "Ma il calore della pece ci sta togliendo il domani, Matteo. Nella Camera Geotermica c'e' energia pulita a 600 Kelvin pronta per tutti."},
                {"Matteo Verrini", "Scendi pure nella faglia, allora. Vedremo se la tua fisica sa tenere il passo della realta'."}
            };

        case StoryEvent::ThermalClean:
            return {
                {"Elena Valli", "Sorgente calda a 600 Kelvin contro 300 Kelvin: ciclo di Carnot al cinquanta per cento! Solo vapore bianco e calore stabile!"},
                {"Matteo Verrini", "Non sento odore di zolfo ne' di pece... Eppure i condotti verso valle sono caldi come non li vedevo da un inverno intero."},
                {"Clara Sereni", "Elena! Dal Santuario dell'Armonia a sud-est sale una colonna di luce turchese: l'Astrolabio Centrale si sta risvegliando!"}
            };

        case StoryEvent::ThermalBypassed:
            return {
                {"Matteo Verrini", "Carbone e pece iniettati nel pozzo: calore immediato, esattamente come facevano nell'Era III."},
                {"Lorenzo Altieri", "Che ferita profonda, Elena... I maestri di basalto avevano inciso la loro stele proprio per evitare che accadesse di nuovo."},
                {"Elena Valli", "Lo scambiatore risponde, ma la Caldera e' avvolta da una nube densa. Dal Santuario a sud-est giunge comunque un richiamo."}
            };

        case StoryEvent::AstrolabeUnlocked:
            return {
                {"Eco di Meridia", "Cinque leggi hanno risposto: luce, acqua, tempo, suono e calore. Nel Santuario dell'Armonia l'Astrolabio Centrale scioglie il proprio sigillo."},
                {"Elena Valli", "Cinque monumenti, un quinto di cerchio esatto: 72 gradi di fase! Andiamo al Santuario dell'Armonia a sud-est e allineiamo l'Astrolabio [E]."}
            };

        case StoryEvent::AstrolabeActivated:
            if (IsCleanSnapshot(s) && s.relics_collected >= kRelicsForTrueEnding) {
                return {
                    {"Eco di Meridia", "Ogni anello dell'Astrolabio brilla senza macchia e le quattro ere sono tornate intere nella vostra memoria."},
                    {"Eco di Meridia", "Il Sigillo dell'Armonia si schiude: oltre il Santuario, la Valle dei Semi vi attende sotto un cielo finalmente limpido."},
                    {"Clara Sereni", "Ce l'abbiamo fatta, Elena! I canali splendono, i giardini fioriscono e i villaggi hanno luce senza un solo filo di fumo."},
                    {"Matteo Verrini", "Tieni le chiavi delle fornaci, dottoressa. Non pensavo di poterlo dire, ma la pazienza del calcolo ha vinto sul serio."}
                };
            }
            if (IsCleanSnapshot(s)) {
                return {
                    {"Eco di Meridia", "Le cinque leggi scorrono pure attraverso la Conca e l'aria torna chiara. Eppure alcuni anelli della nostra storia riposano ancora nella pietra."},
                    {"Lorenzo Altieri", "Hai guarito l'altopiano senza accendere una sola fornace, Elena. Meridia respira di nuovo e i villaggi a valle sono salvi."},
                    {"Elena Valli", "E se continueremo a raccogliere le reliquie nel Codex [J], sapremo aprire fino in fondo anche l'ultimo segreto dei Meridiani."}
                };
            }
            return {
                {"Eco di Meridia", "L'Astrolabio ruota a fatica sotto un cielo di cenere. Le cinque forze rispondono, ma il fumo della fretta oscura ancora il mosaico."},
                {"Matteo Verrini", "La corrente c'e', Elena... ma a guardare i giardini grigi sotto questo cielo, non riesco piu' a chiamarla una vittoria."},
                {"Elena Valli", "La storia dell'Era III si e' ripetuta. Ma la pietra aspetta: possiamo ancora tornare ai monumenti forzati e calibrarli con cura."}
            };

        case StoryEvent::SmogCritical:
            return {
                {"Miriam Ferro", "Elena, la cenere ha superato il settanta per cento! Le foglie si stanno ripiegando e l'aria nei giardini brucia la gola."},
                {"Clara Sereni", "Le fornaci stanno soffocando l'intera Conca! Dobbiamo spegnerle e ricalibrare gli impianti con la fisica pulita."},
                {"Elena Valli", "Cosi' non sopravvivera' nulla. Torno ai monumenti alimentati a pece [E] per correggere le misure prima che sia troppo tardi."}
            };

        case StoryEvent::FirstRelic:
            return {
                {"Elena Valli", "Ho ritrovato la prima reliquia dei Meridiani! L'ho trascritta nel Codex dell'Archivio [J], accanto al Diario di Campo [H]."}
            };

        case StoryEvent::FourRelics:
            return {
                {"Elena Valli", "Quattro reliquie gia' catalogate nel Codex [J]. Pezzo dopo pezzo, le voci dei primi costruttori stanno uscendo dal silenzio."}
            };

        case StoryEvent::HalfRelics:
            return {
                {"Lorenzo Altieri", "Meta' dell'archivio perduto e' tornato alla luce, Elena. Cominci a vedere come lo Scisma dell'Era III non sia nato dall'odio, ma dalla paura."}
            };

        case StoryEvent::TwelveRelics:
            return {
                {"Lorenzo Altieri", "Dodici reliquie decifrate! Ora possediamo testimonianze sufficienti di tutte e quattro le ere per risvegliare l'intero Astrolabio Centrale."},
                {"Elena Valli", "Il filo che unisce l'Alba delle Ombre al Sigillo dell'Armonia e' finalmente chiaro. Meridia non era una leggenda: era una promessa."}
            };

        case StoryEvent::AllRelics:
            return {
                {"Elena Valli", "Tutte e sedici le reliquie della Conca di Meridia sono state ritrovate e trascritte nel Codex [J]!"},
                {"Eco di Meridia", "Nessuna voce e' andata perduta. Chi custodisce la memoria intera sa perche' misurare e' un atto d'amore verso la terra."}
            };

        case StoryEvent::Count:
            break;
    }
    return {};
}

Objective ObjectiveFor(Chapter c, const StorySnapshot& s) {
    if (s.simulation_completed) {
        if (IsCleanSnapshot(s) && s.relics_collected >= kRelicsForTrueEnding) {
            return {
                "Alba di Meridia - Conca Risanata",
                "Esplora la Conca, leggi Diario [H] / Codex [J] o Trama [V]."
            };
        }
        if (IsCleanSnapshot(s)) {
            return {
                "Conca Rinata - Cerca le Reliquie",
                "Trova almeno 12 reliquie [J] per il segreto dell'Astrolabio."
            };
        }
        return {
            "Conca Sotto Cenere - Ricalibra",
            "Spegni le fornaci ricalibrando i monumenti [E] in modo pulito."
        };
    }

    if (c == Chapter::Prologo) {
        return {
            "Misura l'Eliostato Solare",
            "Avvicinati allo specchio a sud e premi [F] con la Lente."
        };
    }

    if (c == Chapter::Atto1_Luce) {
        if (s.solar == MachineState::Broken) {
            return {
                "Calibra l'Eliostato (Lambert)",
                "Premi [E] sull'Eliostato: regola a 45 deg e conferma [C]."
            };
        }
        if (s.solar == MachineState::BypassedDirty && !s.clara_recruited) {
            return {
                "Produci Luce Pulita per Clara",
                "Clara rifiuta la pece! Premi [E] sull'Eliostato: 45 deg."
            };
        }
        return {
            "Parla con Clara Sereni ai Canali",
            "Sali nel Quartiere dei Canali e premi [E] vicino a Clara."
        };
    }

    if (c == Chapter::Atto2_Acqua) {
        if (s.location != LocationId::HypogeanCisterns) {
            if (s.mode == GameMode::Structure) {
                return {
                    "Torna all'Altopiano",
                    "Usa il portale [USCITA] o premi [TAB] per uscire."
                };
            }
            return {
                "Scendi nelle Cisterne Ipogee",
                "Entra dal portale nel Quartiere dei Canali (Ovest)."
            };
        }
        return {
            "Calibra la Turbina (Stevino)",
            "Salta [SPAZIO], misura [F] e regola il carico a 10 m [E]."
        };
    }

    if (c == Chapter::Atto3_Pendolo) {
        if (s.pendulum != MachineState::Broken && !s.clara_recruited) {
            if (s.mode == GameMode::Structure) {
                return {
                    "Torna da Clara Sereni ai Canali",
                    "Esci dalla Torre [USCITA / TAB] e vai al Quartiere dei Canali."
                };
            }
            return {
                "Parla con Clara Sereni ai Canali",
                "Premi [E] vicino a Clara per aprire il Ponte degli Archi."
            };
        }
        if (s.location == LocationId::PendulumTower) {
            return {
                "Calibra il Pendolo Isochronos",
                "Sali i ballatoi [SPAZIO], misura [F] e regola L=2.0 m [E]."
            };
        }
        if (s.mode == GameMode::Structure) {
            return {
                "Risali verso l'Altopiano",
                "Usa il portale di uscita a sinistra (o premi [TAB])."
            };
        }
        return {
            "Raggiungi la Torre del Pendolo",
            "Sali sull'Altopiano dei Venti (NO) ed entra nella Torre."
        };
    }

    if (c == Chapter::Atto4_Giardini) {
        if (s.mode == GameMode::Structure) {
            return {
                "Torna nell'Altopiano di Meridia",
                "Esci dalla struttura [USCITA / TAB] verso il Ponte."
            };
        }
        if (s.harmonic == MachineState::Broken) {
            return {
                "Accorda il Risonatore Armonico",
                "Attraversa il Ponte a Est: Giardini Sospesi, 120 Hz [E]."
            };
        }
        if (!s.lorenzo_recruited) {
            return {
                "Incontra Lorenzo nelle Cisterne",
                "Torna nelle Cisterne Ipogee e sali sul ballatoio alto [E]."
            };
        }
        return {
            "Esplora verso la Caldera a Nord-Est",
            "Segui i sentieri dai Giardini verso la Caldera delle Fumarole."
        };
    }

    if (c == Chapter::Atto5_Fumarole) {
        if (!s.lorenzo_recruited && s.thermal == MachineState::Broken) {
            if (s.location == LocationId::HypogeanCisterns) {
                return {
                    "Parla con Lorenzo Altieri",
                    "Raggiungi il ballatoio alto a destra e premi [E]."
                };
            }
        }
        if (s.location == LocationId::GeothermalChamber) {
            if (s.thermal == MachineState::Broken) {
                return {
                    "Calibra lo Scambiatore (Carnot)",
                    "Misura [F] e regola la sorgente calda a 600 K [E]."
                };
            }
            return {
                "Risali al Santuario dell'Armonia",
                "Esci dalla Camera [USCITA / TAB] e scendi a Sud-Est."
            };
        }
        if (s.mode == GameMode::Structure) {
            return {
                "Torna all'Aperto",
                "Esci dalla struttura [USCITA / TAB] verso la Caldera."
            };
        }
        if (s.thermal == MachineState::Broken) {
            return {
                "Scendi nella Camera Geotermica",
                "Raggiungi la Caldera delle Fumarole (NE) e entra nel portale."
            };
        }
        return {
            "Raggiungi il Santuario a Sud-Est",
            "Scendi verso il mosaico astronomico e l'Astrolabio Centrale."
        };
    }

    if (c == Chapter::Epilogo) {
        if (s.mode == GameMode::Structure) {
            return {
                "Torna al Santuario dell'Armonia",
                "Esci dalla struttura [USCITA / TAB] e vai a Sud-Est."
            };
        }
        if (!s.astrolabe_unlocked) {
            return {
                "Attiva i Cinque Monumenti",
                "Completa tutti e cinque gli impianti prima dell'Astrolabio."
            };
        }
        return {
            "Risveglia l'Astrolabio Centrale",
            "Santuario (SE): misura [F] e allinea la fase a 72 deg [E]."
        };
    }

    return {
        "Esplora la Conca di Meridia",
        "Misura con [F], calibra [E], consulta Diario [H] e Codex [J]."
    };
}

std::vector<std::string> IntroCrawl() {
    return {
        "LA CONCA DI MERIDIA - CRONACHE DELL'ALTOPIANO",
        "Secoli fa, una civilta' di astronomi, idraulici e botanici scolpi' questo altopiano nella pietra calcarea e nel basalto.",
        "I Meridiani non bruciavano carbone ne' pece: alimentavano la loro citta' con la luce del sole, il peso dell'acqua, il pendolo, il suono e il calore profondo.",
        "Poi venne la Grande Siccita' e con essa l'Inverno di Pece: le fornaci d'emergenza oscurarono gli specchi e dispersero gli archivi.",
        "Oggi i villaggi a valle respirano cenere e dipendono dai fumi della Compagnia della Pece.",
        "La naturalista Elena Valli sale sull'altopiano con una Lente Empirica e un Diario di Campo ancora bianco.",
        "I cinque grandi monumenti della Conca sono muti, ma le leggi della fisica attendono mani pazienti che sappiano misurarle.",
        "Ogni impianto ti porra' una scelta reale: la pazienza del calcolo pulito o la scorciatoia immediata della fornace.",
        "Esplora i sei distretti e le tre camere ipogee, raccogli le sedici reliquie delle quattro ere nel Codex [J] e rileggi il viaggio nel Diario [H].",
        "Misura con [F], calibra con [E], e scopri cosa sigillarono gli ultimi custodi al cuore di Meridia..."
    };
}

std::vector<std::string> EndingText(bool clean_path, int relics_found, int total_relics) {
    const std::string relic_summary =
        "Reliquie decifrate nel Codex: " + std::to_string(relics_found) + " / " +
        std::to_string(total_relics) + ".";

    if (clean_path && relics_found >= kRelicsForTrueEnding) {
        return {
            "L'ALBA DI MERIDIA - FINALE RIVELATO",
            "La cenere si dissolve del tutto sopra i sei distretti. I canali tornano turchesi e i Giardini Sospesi fioriscono sotto il sole.",
            "Nel Santuario dell'Armonia i quattro anelli dell'Astrolabio Centrale si allineano in un accordo limpido di luce e bronzo.",
            "Con la memoria delle quattro ere ricomposta, l'Eco di Meridia scioglie l'ultimo sigillo e apre il passo verso la Valle dei Semi.",
            "Clara Sereni riapre gli acquedotti verso valle, Lorenzo Altieri trascrive la nuova era e Miriam Ferro raccoglie i semi del risveglio.",
            "Persino Matteo Verrini depone le chiavi delle fornaci di pece: la scienza paziente ha dato luce ai villaggi senza ferire la terra.",
            relic_summary,
            "Puoi continuare a esplorare la Conca risanata, rileggere il Diario [H] o completare il Codex [J]."
        };
    }

    if (clean_path) {
        return {
            "LA RINASCITA DELLA CONCA - FINALE ARMONICO",
            "Tutte e cinque le leggi fisiche scorrono in equilibrio pulito. La cappa di cenere si dirada e le piogge acide sui villaggi cessano.",
            "L'Eliostato, le Cisterne, la Torre del Pendolo, i Giardini e la Camera Geotermica tornano a sostenere la vita senza bruciare pece.",
            "Eppure alcuni anelli dell'Astrolabio Centrale restano in penombra: molte pagine della storia dei Meridiani dormono ancora tra le rovine.",
            "Lorenzo Altieri osserva il mosaico: quando almeno dodici reliquie torneranno nel Codex, il passo della Valle dei Semi si aprira'.",
            relic_summary,
            "Continua a esplorare i sei distretti e le tre strutture per ritrovare le reliquie mancanti nel Codex [J]."
        };
    }

    return {
        "IL PREZZO DELLA FRETTA - FINALE GRIGIO",
        "I monumenti della Conca erogano energia verso i villaggi a valle, ma una coltre plumbea resta sospesa sopra il Ponte degli Archi.",
        "I Giardini Sospesi faticano a schiudere i propri semi e le acque delle cisterne portano ancora l'ombra scura delle fornaci di pece.",
        "Matteo Verrini ottiene la corrente rapida che aveva promesso, ma il suo stesso silenzio davanti all'Astrolabio tradisce il peso della scelta.",
        "L'Eco di Meridia tace nel mosaico: la frattura dell'Inverno di Pece si e' ripetuta identica.",
        relic_summary,
        "La pietra attende: torna ai monumenti forzati a fornace e ricalibrali con la fisica pulita [E] per guarire la Conca."
    };
}

} // namespace eco
