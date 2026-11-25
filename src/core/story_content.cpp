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

[[nodiscard]] CalibrationApproach EffectiveSolarApproach(const StorySnapshot& s) noexcept {
    if (s.solar_approach != CalibrationApproach::None) {
        return s.solar_approach;
    }
    if (s.solar == MachineState::BypassedDirty) {
        return CalibrationApproach::DirtyImmediate;
    }
    if (s.solar == MachineState::CleanActive) {
        return CalibrationApproach::CleanSlow;
    }
    return CalibrationApproach::None;
}

[[nodiscard]] CalibrationApproach EffectiveHydroApproach(const StorySnapshot& s) noexcept {
    if (s.hydro_approach != CalibrationApproach::None) {
        return s.hydro_approach;
    }
    if (s.hydro == MachineState::BypassedDirty) {
        return CalibrationApproach::DirtyImmediate;
    }
    if (s.hydro == MachineState::CleanActive) {
        return CalibrationApproach::CleanSlow;
    }
    return CalibrationApproach::None;
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
                {"Elena Valli", "L'aria sulle Terrazze dell'Alba sa di cenere fredda e calcare bagnato. Nessuna bussola pronta: qui dovro' imparare a leggere la pietra."},
                {"Elena Valli", "Sul pavimento di travertino corrono condotti d'ottone spenti e il quadrante di una grande meridiana: l'ombra dello stilo indica gia' dove cade il sole."},
                {"Matteo Verrini", "Dottoressa Valli, la sento salire dalle terrazze. A valle fa freddo stasera: se vuole corrente subito senza inseguire ombre, innesti la fornace a pece [B]."},
                {"Elena Valli", "Sono salita proprio per spegnere quella cenere, Matteo. Prima osservo le incisioni [E], sondo i fasci con la Lente [Q] e metto alla prova la mia ipotesi."}
            };

        case StoryEvent::FirstScan:
            return {
                {"Elena Valli", "La Lente Empirica [Q] raccoglie il riflesso sul travertino: l'ombra dello gnomone eguaglia esattamente l'altezza dello stilo verticale."},
                {"Elena Valli", "Per la legge del coseno di Lambert, uno specchio rende al massimo quando la sua normale divide a meta' il quadrante tra orizzonte e zenit."},
                {"Elena Valli", "Posso orientare gli specchi di bronzo [E] lungo i condotti e lanciare un Impulso di Prova [T] sull'Eliostato prima di scegliere il regime."}
            };

        case StoryEvent::ObservationClueRead:
            return {
                {"Elena Valli", "Annoto il rilievo sul taccuino: i Meridiani non nascondevano codici segreti, incidevano nella pietra i limiti fisici di ogni impianto."},
                {"Elena Valli", "Osservare sul campo, scegliere un'ipotesi sul banco [E] e verificare la conseguenza con un Impulso di Prova [T]: cosi' si governa Meridia senza andare alla cieca."}
            };

        case StoryEvent::SolarTrialRun:
            return {
                {"Elena Valli", "L'Impulso di Prova [T] ha percorso i condotti d'ottone! Un lampo breve sulle lastre ha scosso la polvere dalle incisioni e misurato lo scarto ottico."},
                {"Matteo Verrini", "Sento i morsetti fischiare fin dal colle, dottoressa. Vede quanto trema l'ottone appena si cerca potenza vera?"},
                {"Elena Valli", "Ora leggo la risposta reale della macchina: posso cercare l'Armonia Pulita [INVIO], forzare il Regime Critico [R] o cedere al Bypass a Pece [B]."}
            };

        case StoryEvent::SolarClean:
            if (EffectiveSolarApproach(s) == CalibrationApproach::RiskyEfficient) {
                return {
                    {"Elena Valli", "Fascio concentrato in regime supercritico! Il riverbero d'ambra ha vetrificato i rovi aprendo la Cresta Arsa, ma sento le lastre vibrare sotto i piedi."},
                    {"Clara Sereni", "Ehi laggiu'! Quanta potenza avete mandato nei cavi?! Niente fumo di pece, lo ammetto, ma i giunti fischiano: sali subito ai Canali a parlarmi [E]!"},
                    {"Elena Valli", "Zero combustione, ma l'Eliostato lavora al limite termico. Seguo i condotti incandescenti verso nord fino al Quartiere dei Canali."}
                };
            }
            return {
                {"Elena Valli", "Equilibrio armonico raggiunto! I condotti d'ottone pulsano d'oro caldo, i rovi di pece si sbriciolano e le prime rondini tornano a sfiorare gli ulivi!"},
                {"Elena Valli", "La Lente Empirica si e' accordata alla luce pulita: ora posso consultare il Rilievo Metrico di Fase 2 con [V]."},
                {"Clara Sereni", "Dalle chiuse ho visto schiarirsi il cielo sopra le terrazze senza un solo sbuffo nero! Segui l'acquedotto a nord e vieni al Ponte degli Archi!"},
                {"Elena Valli", "Quella e' Clara Sereni, l'ingegnera dei canali. Il respiro di Meridia sta tornando regolare: saliamo a nord per incontrarla [E]."}
            };

        case StoryEvent::SolarRisky:
            return {
                {"Elena Valli", "Regime Critico innestato! Il raggio sovraccarico ha fuso in vetro vivo la spina di rovi sulla Cresta Arsa, aprendo un varco diretto tra le rocce fumanti!"},
                {"Matteo Verrini", "Perbacco, dottoressa... trentadue unita' di spinta pura senza toccare la mia pece! Ma senta come cantano tesi i tiranti sotto il portico."},
                {"Clara Sereni", "Chi sta mandando corrente in sovraccarico dalle Terrazze?! Ingegno audace, non c'e' fuliggine, ma i miei manometri tremano: vieni subito ai Canali [E]!"},
                {"Elena Valli", "La Lente registra il Rilievo Metrico [V], ma il riverbero termico scalda il travertino. Saliamo ai Canali per verificare la tenuta con Clara."}
            };

        case StoryEvent::SolarBypassed:
            return {
                {"Matteo Verrini", "Vede, dottoressa? Con la fornace di pece i condotti vanno in pressione in tre secondi, senza perdere mezza mattina dietro alle meridiane."},
                {"Elena Valli", "La corrente scorre, ma un fumo acre di catrame avvolge gli ulivi... Le rondini sono sparite e i rovi neri stanno serrando i passaggi bassi."},
                {"Clara Sereni", "*coff* Ancora pece sulle Terrazze?! Parlo dietro un panno bagnato, ascoltami bene: finche' bruciate scorie, il Ponte degli Archi resta chiuso!"},
                {"Elena Valli", "L'aria brucia in gola. Posso sempre riaprire il banco dell'Eliostato [E] per ricalibrarlo, o usare un Seme d'Ulivo Bianco contro i rovi."}
            };

        case StoryEvent::ClaraRecruited:
            if (EffectiveSolarApproach(s) == CalibrationApproach::RiskyEfficient) {
                return {
                    {"Clara Sereni", "Eccoti, Elena. Ho la chiave inglese sui bulloni del collettore da quando hai mandato quel fascio supercritico: una resa impressionante, zero catrame."},
                    {"Clara Sereni", "Hai fegato da vendere, ma ascolta il ronzio negli archi: la pietra antica sopporta la tensione solo se chi la guida sa dove fermarsi."},
                    {"Clara Sereni", "Tolgo comunque il sigillo dal Ponte degli Archi e unisco la mia lente alla tua [V]: usa la Trama Geodetica per tenere d'occhio gli sforzi nelle Cisterne!"},
                    {"Eco di Meridia", "La corda tesa porta lontano la freccia, ma chiede mano ferma per non spezzare l'arco."},
                    {"Elena Valli", "Staro' attenta ai carichi idraulici, Clara. Scendo nelle Cisterne Ipogee a ovest per studiare i pozzi di Stevino."}
                };
            }
            return {
                {"Clara Sereni", "Guarda i morsetti di bronzo, Elena: tiepidi, puliti, neanche un grano di fuliggine. Non sentivo il ponte respirare cosi' da quando ero bambina."},
                {"Clara Sereni", "Valerio degli Archi incastro' questi conci a secco perche' vibrassero con l'acqua limpida: apro per te il Ponte degli Archi e anche il Passaggio degli Artigiani!"},
                {"Clara Sereni", "Prendi la mia lente strutturale. Premendo [V] vedrai la Trama Geodetica: le linee di forza con cui i Meridiani legarono luce e canali."},
                {"Eco di Meridia", "Quando due sguardi misurano senza fretta, il ponte torna a unire cio' che l'inverno aveva diviso."},
                {"Elena Valli", "Anche i cristalli dell'arcata hanno risposto! Ora seguiamo la caduta dell'acqua nelle Cisterne Ipogee a ovest, poi faremo insieme il bilancio."}
            };

        case StoryEvent::EnteredCanalQuarter:
            if (s.pollution_level >= 52.0f) {
                return {
                    {"Elena Valli", "Il Quartiere dei Canali... Una schiuma densa color pece rallenta le vasche a gradini e non si vede una sola carpa sotto il pelo dell'acqua."},
                    {"Elena Valli", "Lungo le sponde restano pero' gli idrometri scolpiti e le leve delle chiuse [E]: leggendo la pendenza potro' deviare la piena verso le Cisterne a ovest."}
                };
            }
            return {
                {"Elena Valli", "Il Quartiere dei Canali. Lungo gli argini vedo colonnine idrometriche incise e chiuse a leva [E] che governano la spinta delle correnti."},
                {"Elena Valli", "Osservando la schiuma nei salti d'acqua e il livello sul Guado sommerso, capiro' come raggiungere le Cisterne Ipogee a ovest e il Ponte degli Archi a est."}
            };

        case StoryEvent::EnteredWindPlateau:
            return {
                {"Elena Valli", "Sull'Altopiano dei Venti le raffiche spazzano la cresta a intervalli regolari: quando il basalto fischia, devo ripararmi dietro i monoliti frangivento."},
                {"Elena Valli", "In fondo al sentiero si staglia la Torre del Pendolo: i suoi oculi di pietra sembrano ancora attendere il mezzogiorno."}
            };

        case StoryEvent::EnteredHangingGardens:
            return {
                {"Elena Valli", "Oltre il Ponte degli Archi si aprono i Giardini Sospesi. Tra le aiuole emergono quattro piastre di risonanza in giada."},
                {"Elena Valli", "Se le calpesto nell'ordine armonico giusto, il ponte di giada si tendera' verso il Risonatore e verso Miriam Ferro."}
            };

        case StoryEvent::EnteredFumaroleCaldera:
            return {
                {"Elena Valli", "La Caldera delle Fumarole... Il terreno vibra prima che ogni geyser di vapore erutti. Lungo i bordi vedo valvole di sfogo in bronzo [E]."},
                {"Elena Valli", "Chiudendo le valvole giuste calmero' i getti per raggiungere Matteo Verrini e l'ingresso della Camera Geotermica."}
            };

        case StoryEvent::EnteredHarmonySanctuary:
            return {
                {"Elena Valli", "Il Santuario dell'Armonia. Attorno al mosaico astronomico brillano cinque sigilli radiali dedicati a luce, acqua, vento, suono e calore."},
                {"Elena Valli", "Accordando i cinque sigilli [E] attorno alla piazza, l'Astrolabio Centrale ritrovera' la sintesi dell'intera Conca."}
            };

        case StoryEvent::EnteredCisterns:
            return {
                {"Elena Valli", "Le Cisterne Ipogee. Il battente delle cascate rimbomba sotto le volte, e sulle colonne sommerse scorgo le tacche metriche di profondita' in calce e bronzo."},
                {"Elena Valli", "Qui governa la legge di Stevino: il peso della colonna d'acqua decide da solo tra quiete armonica, spinta supercritica e cavitazione distruttiva."},
                {"Elena Valli", "Osservero' le colonne piezometriche [E] prima di provare la Turbina in basso; lassu' nell'ala est brilla la lanterna di un archivista."}
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

        case StoryEvent::HydroTrialRun:
            return {
                {"Elena Valli", "L'Impulso di Prova [T] ha mandato un'onda misurata lungo la condotta forzata! I canali di scolo hanno sussultato, rivelando la direzione dell'errore piezometrico."},
                {"Lorenzo Altieri", "Sento il colpo d'assaggio sulle paratoie! Proprio cosi' facevano gli antichi fontanieri: interrogavano il peso dell'acqua prima di serrare le chiuse."},
                {"Elena Valli", "Il manometro ha parlato chiaro: ora decido se stabilizzare la Turbina in Armonia [INVIO], spingerla allo Sfioratore Critico [R] o aprire il Bypass [B]."}
            };

        case StoryEvent::HydroClean:
            if (EffectiveHydroApproach(s) == CalibrationApproach::RiskyEfficient) {
                return {
                    {"Elena Valli", "Carico spinto in banda supercritica! La girante ruggisce ad altissima resa e l'onda di ritorno ha spalancato a pressione lo Sfioratore Alto!"},
                    {"Lorenzo Altieri", "Per le volte di Valerio! L'acqua corre velocissima senza una goccia di pece, ma le colonne tremano fino quassu': sali con cautela a parlarmi [E]!"},
                    {"Clara Sereni", "Elena, nei Canali la corrente e' balzata del quarantacinque per cento! Spinge forte sotto gli archi, vieni a vedere appena risali!"}
                };
            }
            return {
                {"Elena Valli", "Colonna di Stevino in perfetto equilibrio laminare! Nessuna cavitazione: l'acqua schiarisce come vetro azzurro e i tritoni luminosi tornano sulle banchine."},
                {"Eco di Meridia", "L'acqua non obbedisce al fuoco, ma al proprio peso. Chi ascolta la caduta della pietra non resta mai senz'acqua."},
                {"Lorenzo Altieri", "Meraviglioso... il canto limpido delle paratoie! Scendo verso la vasca chiara e apro la nicchia sigillata dei cilindri: vieni a parlarmi [E]!"}
            };

        case StoryEvent::HydroRisky:
            return {
                {"Elena Valli", "Turbina innestata in Regime Supercritico! Le tubazioni piezometriche gocciolano vapore freddo e la spinta ha spalancato la paratoia dello Sfioratore Alto!"},
                {"Clara Sereni", "Elena, che portata mostruosa! Su nei Canali l'acqua fila in getti bianchi rapidissimi: ottima corrente per le dinamo, ma occhio a non farti trascinare!"},
                {"Lorenzo Altieri", "Ingegno temerario, ragazza mia! Hai aperto il passaggio alto al Prisma senza sporcare la falda, ma sento vibrare ogni chiave di volta: vieni a parlarmi [E]."}
            };

        case StoryEvent::HydroBypassed:
            return {
                {"Matteo Verrini", "Una pompa ausiliaria a pece manda in pressione le condotte in un attimo, dottoressa. Non sara' silenziosa, ma l'acqua sale ai serbatoi."},
                {"Elena Valli", "Una patina nera ed oleosa copre il bacino... Le lucciole di grotta si sono spente tutte e il vapore acido copre il suono della cascata."},
                {"Lorenzo Altieri", "*coff* Che esalazione amara! Devo ritirarmi sulle volte piu' alte per coprire i cilindri d'archivio prima che l'umidita' acida li corroda..."},
                {"Eco di Meridia", "Una sorgente ferita ricorda a lungo il sapore del fumo."}
            };

        case StoryEvent::LorenzoRecruited: {
            const CalibrationApproach h_app = EffectiveHydroApproach(s);
            if (h_app == CalibrationApproach::DirtyImmediate) {
                return {
                    {"Lorenzo Altieri", "Parla piano, Elena... quassu' sotto le volte alte cerco di tenere all'asciutto le ultime pergamene di pietra lontano dai fumi della pompa a pece."},
                    {"Lorenzo Altieri", "Sono Lorenzo Altieri. Capisco la fretta per i villaggi, proprio quella paura spacco' il Consiglio nell'Era III, ma l'acido sta gia' mordendo i fregi."},
                    {"Lorenzo Altieri", "Ti affido comunque la chiave della Camera Geotermica, nella speranza che tu torni alla Turbina [E] per restituire respiro limpido a queste acque."},
                    {"Elena Valli", "Conservero' la memoria nel Codex [J], Lorenzo, e non lascero' che le Cisterne restino avvelenate."}
                };
            }
            if (h_app == CalibrationApproach::RiskyEfficient) {
                return {
                    {"Lorenzo Altieri", "Tieniti alla ringhiera, Elena! Sono Lorenzo Altieri: non vedevo lo Sfioratore Alto spalancato dalla pressione fin dai racconti di mio nonno."},
                    {"Lorenzo Altieri", "Hai cavato dalla cisterna una forza prodigiosa senza bruciare pece, ma ascolta il tremito nei pilastri: l'audacia va sempre bilanciata col limite della roccia."},
                    {"Lorenzo Altieri", "Prendi la chiave di bronzo delle camere profonde e raccogli le reliquie nel Codex [J]: i nostri avi le forgiarono come strumenti vivi per domare questi flussi."},
                    {"Elena Valli", "Grazie, Lorenzo. Salgo ora al Ponte degli Archi da Clara per tirare le somme di questo primo grande anello idraulico e solare."}
                };
            }
            if (s.relics_collected >= 4) {
                return {
                    {"Lorenzo Altieri", "Elena Valli... guardo quest'acqua limpida e vedo dal tuo taccuino che stai gia' usando le antiche reliquie come strumenti vivi. I Meridiani ti sorriderebbero."},
                    {"Lorenzo Altieri", "Sono Lorenzo Altieri, ultimo archivista delle volte. Per ringraziarti d'aver guarito il bacino senza fumo ne' sovraccarico, ho aperto la Nicchia d'Archivio qui accanto."},
                    {"Lorenzo Altieri", "Prendi questa chiave di bronzo verso la Camera Geotermica: quando risalirai al Ponte degli Archi, Clara vedra' scorrere di nuovo lo zaffiro dell'Era II."},
                    {"Elena Valli", "Custodiro' ogni voce nel Codex [J], Lorenzo. Risalgo al Quartiere dei Canali da Clara per il bilancio delle acque e della luce."}
                };
            }
            return {
                {"Lorenzo Altieri", "Finalmente posso scendere anch'io vicino allo specchio d'acqua limpida. Sono Lorenzo Altieri, custode degli archivi sommersi di Meridia."},
                {"Lorenzo Altieri", "Sentendo cantare la Turbina in armonia ho dischiuso la nicchia sigillata: qui conservammo la memoria dello Scisma quando la paura del gelo accese la pece."},
                {"Lorenzo Altieri", "Tieni la chiave delle camere profonde e cerca le reliquie nei distretti: non sono semplici cimeli, ma strumenti capaci di guidare chi osserva."},
                {"Elena Valli", "Le annotero' tutte nel Diario [H] e nel Codex [J], Lorenzo. Ora risalgo da Clara al Ponte degli Archi."}
            };
        }

        case StoryEvent::BioPlanterSeeded:
            return {
                {"Elena Valli", "Il Seme d'Ulivo Bianco ha preso radice nella conca di pietra! Guarda come le fibre argentate spaccano la crosta nera e sciolgono i Rovi di Pece!"},
                {"Clara Sereni", "Incredibile, Elena! L'aria attorno alla vasca si e' rinfrescata di colpo e il vecchio passaggio ostruito e' tornato percorribile!"},
                {"Eco di Meridia", "Dove una radice paziente spezza la cenere, il passo dell'uomo ritrova il sentiero."}
            };

        case StoryEvent::LaminarFordOpened:
            return {
                {"Elena Valli", "Seguendo le proporzioni del Diario di Valerio degli Archi, le paratoie tagliano l'acqua senza piu' un vortice: e' regime laminare puro!"},
                {"Clara Sereni", "Hai riaperto il Guado degli Idraulici! Guarda le pietre piatte affiorare asciutte: ora hai una scorciatoia diretta tra il Ponte, la Terrazza Nord e le Cisterne!"}
            };

        case StoryEvent::VerticalSliceSynthesis: {
            const CalibrationApproach s_app = EffectiveSolarApproach(s);
            const CalibrationApproach h_app = EffectiveHydroApproach(s);
            const std::string tools_note =
                (s.white_olive_planted || s.laminar_ford_open || s.relics_collected >= 4)
                    ? "E hai rimesso al lavoro perfino gli strumenti antichi -- tra innesti d'ulivo, chiuse laminari e lenti -- con " +
                          std::to_string(s.relics_collected) + " reliquie gia' vive nel Codex [J]."
                    : "Continua a cercare gli strumenti dei Meridiani nel Codex [J]: con " +
                          std::to_string(s.relics_collected) + " reliquie trovate hai appena scalfito il loro arsenale da campo.";

            if (s_app == CalibrationApproach::CleanSlow && h_app == CalibrationApproach::CleanSlow) {
                return {
                    {"Clara Sereni", "Fermati un istante qui sul Ponte degli Archi, Elena, e guarda sotto di noi: rondini sulle Terrazze dorate e carpe d'argento nell'acqua trasparente dei Canali."},
                    {"Lorenzo Altieri", "Dalle Cisterne sale un respiro che non udivo da quarant'anni: hai unito Lambert e Stevino nell'Armonia Lenta, senza bruciare pece ne' forzare una sola valvola."},
                    {"Elena Valli", tools_note},
                    {"Clara Sereni", "Hai dimostrato che la misura paziente guarisce la pietra e apre vie che la fretta chiudeva. Ora la tua bussola geodetica e' stabile: l'intera Conca ti aspetta!"}
                };
            }
            if (s_app == CalibrationApproach::RiskyEfficient && h_app == CalibrationApproach::RiskyEfficient) {
                return {
                    {"Clara Sereni", "Senti come vibra il parapetto del Ponte degli Archi, Elena?! Cresta Arsa vetrificata a sud e rapide supercritiche a ovest: una potenza da capogiro senza un grammo di pece!"},
                    {"Matteo Verrini", "Devo ammetterlo, dottoressa: a valle le lampade brillano come a mezzogiorno. Certo, le vostre condotte fischiano come caldaie sotto sforzo."},
                    {"Lorenzo Altieri", "Hai aperto scorciatoie ardite nella roccia e negli sfioratori alti, ma la pietra di Meridia e' tesa come una corda di violino."},
                    {"Elena Valli", tools_note + " Se vorremo allentare la tensione strutturale, potremo sempre ricalibrare i banchi [E] prima di salire verso la Torre."}
                };
            }
            if (s_app != CalibrationApproach::DirtyImmediate && h_app != CalibrationApproach::DirtyImmediate) {
                const bool solar_was_risky = (s_app == CalibrationApproach::RiskyEfficient);
                return {
                    {"Clara Sereni", solar_was_risky
                        ? "Dal Ponte degli Archi si legge tutta la tua mano, Elena: a sud la Cresta Arsa fuma ancora di luce supercritica, mentre qui l'acqua delle Cisterne scorre dolce come seta."
                        : "Guarda che contrasto dal Ponte, Elena: sulle Terrazze volano le rondini nella quiete dorata, mentre dai Canali balza la spinta supercritica delle Cisterne!"},
                    {"Lorenzo Altieri", "Hai saputo alternare prudenza armonica e audacia calcolata senza mai piegarti alle fornaci di pece."},
                    {"Elena Valli", tools_note},
                    {"Clara Sereni", "La rete regge bene e la bussola di rilievo e' ormai tarata sul ponte: scegli tu se lasciare quella spinta critica o addolcirla [E] proseguendo il viaggio."}
                };
            }
            if (s_app == CalibrationApproach::DirtyImmediate && h_app == CalibrationApproach::DirtyImmediate) {
                return {
                    {"Matteo Verrini", "I villaggi a valle hanno corrente piena in tempo record, dottoressa Valli. Dal punto di vista dei numeri immediati, missione compiuta."},
                    {"Clara Sereni", "*coff* Guardati intorno dal Ponte, pero', Elena: neanche una rondine, canali neri senza carpe, rovi di pece sui passaggi bassi e Lorenzo chiuso nelle volte alte."},
                    {"Elena Valli", "Sento il peso di questa cenere ad ogni respiro. " + tools_note},
                    {"Clara Sereni", "La pietra dei Meridiani perdona chi corregge i propri calcoli: puoi riaprire entrambi i banchi [E] e spegnere le fornaci quando sarai pronta."}
                };
            }
            // Caso misto: un impianto pulito/rischioso e uno sporco a pece
            return {
                {"Clara Sereni", "Da qui sul Ponte degli Archi la Conca sembra divisa a meta', Elena: dove hai misurato davvero la natura respira, dove hai acceso la pece i rovi chiudono ancora il passo."},
                {"Lorenzo Altieri", "E' lo specchio esatto dell'Era III: sappiamo che la fisica pulita funziona, eppure la tentazione della scorciatoia lascia una scia amara nell'acqua e nell'aria."},
                {"Elena Valli", tools_note},
                {"Clara Sereni", "Il varco avanti e' aperto, ma se torni a ricalibrare l'impianto a pece [E] o innesti l'Ulivo Bianco vedrai rifiorire anche la meta' ferita."}
            };
        }

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
                    {"Miriam Ferro", "Io sono Miriam. I maestri accordavano le quattro piastre di giada in sequenza crescente per stendere il ponte verso il Risonatore a 120 Hertz."},
                    {"Elena Valli", "I cristalli di giada vibrano gia' al solo avvicinarsi. Seguo l'ordine armonico delle piastre e calibro il Risonatore [E]."}
                };
            }
            return {
                {"Miriam Ferro", "Sei riuscita ad attraversare il Ponte degli Archi! Sono Miriam Ferro, botanica dei Giardini Sospesi."},
                {"Miriam Ferro", "La cenere soffoca le foglie e i ponti di giada dormono. Attiva le quattro piastre acustiche in sequenza per raggiungere il Risonatore a 120 Hertz."},
                {"Elena Valli", "Se accordiamo le piastre e il Risonatore senza usare la fornace, la polvere cadra' al suolo e le serre torneranno a vivere."}
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
                {"Eco di Meridia", "Cinque leggi hanno risposto: luce, acqua, tempo, suono e calore. Nel Santuario dell'Armonia i cinque sigilli radiali chiamano l'Astrolabio."},
                {"Elena Valli", "Cinque monumenti, un quinto di cerchio esatto! Andiamo al Santuario, accordiamo i sigilli [E] e risvegliamo l'Astrolabio Centrale."}
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
                {"Elena Valli", "La storia dell'Era III si e' ripetuta. Ma la pietra aspetta: possiamo ancora tornare ai monumenti forzati e calibrarli con cura [E]."}
            };

        case StoryEvent::SmogCritical:
            return {
                {"Miriam Ferro", "Elena, la cenere ha superato il settanta per cento! Le foglie si stanno ripiegando e l'aria nei giardini brucia la gola."},
                {"Clara Sereni", "Le fornaci stanno soffocando l'intera Conca! Dobbiamo spegnerle e ricalibrare gli impianti [E] con la fisica pulita."},
                {"Elena Valli", "Cosi' non sopravvivera' nulla. Torno ai monumenti alimentati a pece [E] per correggere le misure prima che sia troppo tardi."}
            };

        case StoryEvent::FirstRelic:
            return {
                {"Elena Valli", "Ho recuperato il primo manufatto dei Meridiani! Non e' solo memoria storica: nel Codex [J] posso leggere come usarlo sul campo accanto al Diario [H]."}
            };

        case StoryEvent::FourRelics:
            return {
                {"Elena Valli", "Quattro reliquie catalogate nel Codex [J]: strumenti ottici, idraulici e botanici stanno tornando operativi nelle mie mani."}
            };

        case StoryEvent::HalfRelics:
            return {
                {"Lorenzo Altieri", "Meta' dell'archivio perduto e' tornato alla luce, Elena. Cominci a vedere come lo Scisma dell'Era III non sia nato dall'odio, ma dalla paura."}
            };

        case StoryEvent::TwelveRelics:
            return {
                {"Lorenzo Altieri", "Dodici reliquie decifrate! Ora possediamo strumenti e testimonianze sufficienti delle quattro ere per risvegliare l'intero Astrolabio Centrale."},
                {"Elena Valli", "Il filo che unisce l'Alba delle Ombre al Sigillo dell'Armonia e' finalmente chiaro. Meridia non era una leggenda: era un metodo."}
            };

        case StoryEvent::AllRelics:
            return {
                {"Elena Valli", "Tutte e sedici le reliquie della Conca di Meridia sono state ritrovate e rimesse in opera nel Codex [J]!"},
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
        if (s.observations_inspected == 0) {
            return {
                "Taccuino: Leggi le Ombre dell'Alba",
                "Osserva meridiane e condotti spenti nel travertino [E], poi usa [Q]."
            };
        }
        return {
            "Taccuino: Segui i Condotti d'Ottone",
            "Segui le tracce metalliche fino al riflettore e sonda la luce [Q]."
        };
    }

    if (c == Chapter::Atto1_Luce) {
        if (s.solar == MachineState::Broken) {
            if (s.trials_completed == 0) {
                return {
                    "Ipotesi sulle Terrazze dell'Alba",
                    "Orienta gli specchi [E], formula un'ipotesi sul banco e prova [T]."
                };
            }
            return {
                "Conseguenza e Scelta Solare",
                "Valuta l'esito sul banco [E]: Armonia, Regime Critico o Pece."
            };
        }
        if (s.solar == MachineState::BypassedDirty && !s.clara_recruited) {
            return {
                "Fumo sulle Terrazze: Il Veto ai Canali",
                "La pece serra il Ponte: ricalibra l'Eliostato [E] o sali ai Canali."
            };
        }
        if (EffectiveSolarApproach(s) == CalibrationApproach::RiskyEfficient && !s.clara_recruited) {
            return {
                "Cresta Arsa: Segui i Cavi Tesi",
                "I condotti vibrano verso nord: confrontati con Clara ai Canali [E]."
            };
        }
        return {
            "Segui l'Acquedotto verso Nord",
            "I condotti dorati salgono ai Canali: parla con Clara al Ponte [E]."
        };
    }

    if (c == Chapter::Atto2_Acqua) {
        if (s.location != LocationId::HypogeanCisterns) {
            if (s.mode == GameMode::Structure) {
                return {
                    "Risali verso il Corso d'Acqua",
                    "Attraversa l'uscita [TAB] e torna a leggere il flusso dei canali."
                };
            }
            if (s.zone == ZoneId::DawnTerraces) {
                return {
                    "Oltre l'Arco dell'Alba",
                    "Risali a nord: osserva schiuma, sponde e idrometri nei Canali."
                };
            }
            if (s.has_hydraulic_diary && !s.laminar_ford_open) {
                return {
                    "Taccuino: Schiuma e Chiuse Laminari",
                    "Prova il Diario sulle chiuse [E] per calmare i vortici verso ovest."
                };
            }
            return {
                "Taccuino: Ascolta la Caduta d'Acqua",
                "Leggi idrometri e correnti nei Canali [E] fino ai pozzi a ovest."
            };
        }
        if (s.trials_completed == 0 && s.observations_inspected < 2) {
            return {
                "Indagine nei Pozzi di Calma",
                "Osserva le tacche sommerse [E], formula un'ipotesi e prova con [T]."
            };
        }
        return {
            "Il Peso dell'Acqua nella Cisterna",
            "Sul banco della Turbina [E] scegli tra equilibrio, spinta o pece."
        };
    }

    if (c == Chapter::Atto3_Pendolo) {
        if (s.location == LocationId::HypogeanCisterns && !s.lorenzo_recruited) {
            return {
                "La Lanterna sotto le Volte Alte",
                "Raggiungi Lorenzo Altieri nell'ala est delle Cisterne [E]."
            };
        }
        if (!s.clara_recruited || (!s.slice_completed &&
            (s.location == LocationId::HypogeanCisterns || s.zone == ZoneId::CanalQuarter))) {
            if (s.mode == GameMode::Structure) {
                return {
                    "Risali al Quartiere dei Canali",
                    "Esci dalle Cisterne [TAB] per fare il bilancio con Clara al Ponte."
                };
            }
            return {
                "Bilancio al Ponte degli Archi",
                "Parla con Clara Sereni [E] per valutare luci, acque e varchi aperti."
            };
        }
        if (s.location == LocationId::PendulumTower) {
            return {
                "Calibra il Pendolo Isochronos",
                "Ascolta l'ordine delle campane e regola il braccio d'ottone [E]."
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
            "Segui le raffiche sull'Altopiano dei Venti (NO) fino alla Torre."
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
                "Attraversa il Ponte a Est: ascolta le piastre di giada [E]."
            };
        }
        if (!s.lorenzo_recruited) {
            return {
                "Incontra Lorenzo nelle Cisterne",
                "Torna nelle Cisterne Ipogee e sali nell'ala orientale [E]."
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
                    "Raggiungi il ballatoio orientale e premi [E]."
                };
            }
        }
        if (s.location == LocationId::GeothermalChamber) {
            if (s.thermal == MachineState::Broken) {
                return {
                    "Calibra lo Scambiatore (Carnot)",
                    "Osserva il salto termico, prova con [T] e calibra il vapore [E]."
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
                "Raggiungi la Caldera delle Fumarole (NE) ed entra nel portale."
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
            "Santuario (SE): accorda i sigilli e allinea la fase armillare [E]."
        };
    }

    return {
        "Esplora la Conca di Meridia",
        "Osserva con [Q], sperimenta e calibra [E], leggi Diario [H] e Codex [J]."
    };
}

std::vector<std::string> IntroCrawl() {
    return {
        "LA CONCA DI MERIDIA - CRONACHE DELL'ALTOPIANO",
        "Secoli fa, una civilta' di astronomi, idraulici e botanici scolpi' questo altopiano nella pietra calcarea e nel basalto.",
        "I Meridiani non bruciavano carbone ne' pece: alimentavano la citta' con la luce del sole, il peso dell'acqua, il pendolo, il suono e il calore profondo.",
        "Poi venne la Grande Siccita' e con essa l'Inverno di Pece: le fornaci d'emergenza oscurarono gli specchi e dispersero gli archivi.",
        "Oggi i villaggi a valle respirano cenere e dipendono dai fumi della Compagnia della Pece.",
        "La naturalista Elena Valli sale sulle Terrazze dell'Alba senza mappe precompilate, armata solo di una Lente Empirica [Q] e di un Diario di Campo bianco.",
        "Qui la natura parla a chi sa osservarla: l'inclinazione di un'ombra, la schiuma in un canale e le tacche su una colonna svelano le leggi celate nella pietra.",
        "Formula la tua ipotesi, lancia un Impulso di Prova [T] e scegli: l'Armonia Lenta che guarisce, il Regime Critico ad alto rischio o la Fornace a Pece immediata.",
        "Raccogli le sedici reliquie delle quattro ere nel Codex [J] per trasformarle in strumenti vivi sul campo e rileggi ogni scelta nel Diario [H].",
        "Osserva con [Q], sperimenta sul banco [E] e scopri quale futuro attende il cuore sigillato di Meridia..."
    };
}

std::vector<std::string> EndingText(bool clean_path, int relics_found, int total_relics) {
    const std::string relic_summary =
        "Reliquie decifrate nel Codex: " + std::to_string(relics_found) + " / " +
        std::to_string(total_relics) + ".";

    if (clean_path && relics_found >= kRelicsForTrueEnding) {
        return {
            "L'ALBA DI MERIDIA - FINALE RIVELATO",
            "La cenere si dissolve del tutto sopra i sei distretti. I canali tornano turchesi, le rondini tagliano il cielo e i Giardini Sospesi fioriscono.",
            "Nel Santuario dell'Armonia i quattro anelli dell'Astrolabio Centrale si allineano in un accordo limpido di luce e bronzo.",
            "Con gli strumenti e la memoria delle quattro ere ricomposti, l'Eco di Meridia scioglie l'ultimo sigillo verso la Valle dei Semi.",
            "Clara Sereni riapre gli acquedotti verso valle, Lorenzo Altieri trascrive la nuova era e Miriam Ferro raccoglie i semi del risveglio.",
            "Persino Matteo Verrini depone le chiavi delle fornaci di pece: il metodo paziente dell'osservazione ha dato luce ai villaggi senza ferire la terra.",
            relic_summary,
            "Puoi continuare a esplorare la Conca risanata, rileggere il Diario [H] o consultare gli strumenti nel Codex [J]."
        };
    }

    if (clean_path) {
        return {
            "LA RINASCITA DELLA CONCA - FINALE ARMONICO",
            "Tutte e cinque le leggi fisiche scorrono senza fumo di combustione. La cappa di cenere si dirada e le piogge acide sui villaggi cessano.",
            "L'Eliostato, le Cisterne, la Torre del Pendolo, i Giardini e la Camera Geotermica tornano a sostenere la vita senza bruciare pece.",
            "Eppure alcuni anelli dell'Astrolabio Centrale restano in penombra: diversi strumenti e cronache dei Meridiani dormono ancora tra le rovine.",
            "Lorenzo Altieri osserva il mosaico: quando almeno dodici reliquie torneranno nel Codex, il passo della Valle dei Semi si aprira'.",
            relic_summary,
            "Continua a esplorare i sei distretti e le tre strutture per ritrovare le reliquie mancanti nel Codex [J]."
        };
    }

    return {
        "IL PREZZO DELLA FRETTA - FINALE GRIGIO",
        "I monumenti della Conca erogano energia verso i villaggi a valle, ma una coltre plumbea resta sospesa sopra il Ponte degli Archi.",
        "I rovi di pece stringono ancora i sentieri bassi, la fauna tace e le acque delle cisterne portano l'ombra scura delle fornaci d'emergenza.",
        "Matteo Verrini ottiene la corrente rapida che aveva promesso, ma il suo stesso silenzio davanti all'Astrolabio tradisce il peso della scelta.",
        "L'Eco di Meridia tace nel mosaico: la frattura dell'Inverno di Pece si e' ripetuta identica.",
        relic_summary,
        "La pietra attende: torna ai monumenti forzati a fornace e ricalibrali senza pece [E] per guarire la Conca."
    };
}

ZoneInterludeText ZoneInterludeCopy(ZoneId from, ZoneId to) noexcept {
    if (from == ZoneId::DawnTerraces && to == ZoneId::CanalQuarter) {
        return {
            "ARCO DELL'ALBA",
            "Terrazze dell'Alba  ->  Quartiere dei Canali",
            "I condotti d'ottone lasciano le meridiane di travertino per seguire il suono delle chiuse.",
            "Sotto gli archi d'acquedotto, la schiuma e gli idrometri raccontano il peso dell'acqua."
        };
    }
    if (from == ZoneId::CanalQuarter && to == ZoneId::DawnTerraces) {
        return {
            "SCALINATA DELLE TERRAZZE",
            "Quartiere dei Canali  ->  Terrazze dell'Alba",
            "Il fragore delle paratoie sfuma dietro gli ulivi bianchi e i quadranti solari.",
            "A sud la conca si riapre sulla luce obliqua dei riflettori dell'Eliostato."
        };
    }
    if (from == ZoneId::CanalQuarter && to == ZoneId::WindPlateau) {
        return {
            "SCALINATA DI BASALTO",
            "Quartiere dei Canali  ->  Altopiano dei Venti",
            "I gradini salgono lungo la roccia scura mentre la brezza si fa raffica tesa.",
            "Sulla cresta, solo chi ascolta i monoliti frangivento raggiunge la Torre."
        };
    }
    if (from == ZoneId::WindPlateau && to == ZoneId::CanalQuarter) {
        return {
            "DISCESA DEGLI ACQUEDOTTI",
            "Altopiano dei Venti  ->  Quartiere dei Canali",
            "Il fischio del basalto si placa scendendo verso le vasche a gradini.",
            "Dalle creste del tempo, la strada riporta al respiro costante dei canali."
        };
    }
    if (from == ZoneId::CanalQuarter && to == ZoneId::HangingGardens) {
        return {
            "PONTE DEGLI ARCHI",
            "Quartiere dei Canali  ->  Giardini Sospesi",
            "Ogni concio del ponte regge senza malta sul vuoto luminoso della conca.",
            "Oltre la campata, quattro piastre di giada custodiscono il silenzio dei semi."
        };
    }
    if (from == ZoneId::HangingGardens && to == ZoneId::CanalQuarter) {
        return {
            "PONTE DEGLI ARCHI",
            "Giardini Sospesi  ->  Quartiere dei Canali",
            "Il profumo dei pergolati svanisce tra le arcate sospese sull'acqua.",
            "Verso occidente si distinguono le chiuse e la discesa alle Cisterne Ipogee."
        };
    }
    if (from == ZoneId::HangingGardens && to == ZoneId::FumaroleCaldera) {
        return {
            "SOGLIA D'OSSIDIANA",
            "Giardini Sospesi  ->  Caldera delle Fumarole",
            "Le foglie lasciano il posto al basalto caldo e al respiro ritmico del vapore.",
            "Qui nacque l'Inverno di Pece: solo le valvole antiche calmano la faglia."
        };
    }
    if (from == ZoneId::FumaroleCaldera && to == ZoneId::HangingGardens) {
        return {
            "PASSO DELLE SERRE",
            "Caldera delle Fumarole  ->  Giardini Sospesi",
            "Il sibilo delle fumarole si spegne tra le arcate di giada e i terrazzamenti.",
            "L'aria torna piu' mite dove le vasche acustiche proteggono le radici."
        };
    }
    if (from == ZoneId::HangingGardens && to == ZoneId::HarmonySanctuary) {
        return {
            "COLONNATO DEL SANTUARIO",
            "Giardini Sospesi  ->  Santuario dell'Armonia",
            "Un viale di colonne intrecciate conduce al mosaico delle cinque leggi.",
            "Al centro della piazza, l'Astrolabio attende che ogni sigillo torni a vibrare."
        };
    }
    if (from == ZoneId::HarmonySanctuary && to == ZoneId::HangingGardens) {
        return {
            "PERGOLATO DI GIADA",
            "Santuario dell'Armonia  ->  Giardini Sospesi",
            "Lasciato il cerchio dell'Astrolabio, il sentiero risale fra le campane verdi.",
            "I giardini orientali custodiscono il battito discreto della Conca."
        };
    }
    return {
        "SENTIERO DI MERIDIA",
        "Passaggio dell'Altopiano",
        "La pietra calcarea collega ogni distretto lungo la stessa trama antica.",
        "Ogni varco apre una misura diversa della luce, dell'acqua e del vento."
    };
}

std::vector<ZoneLandmarkCopy> ZoneLandmarks(ZoneId zone) {
    switch (zone) {
        case ZoneId::DawnTerraces:
            return {
                {
                    "dawn_sundial",
                    "Meridiana Madre del Mezzogiorno",
                    "Quando l'ombra dello stilo eguaglia la sua altezza verticale, il raggio taglia a meta' l'angolo retto tra terra e cielo.",
                    "Osserva l'ombra sul travertino: per Lambert il coseno e' massimo quando lo specchio guarda dritto la bisettrice solare."
                },
                {
                    "dawn_gnomon",
                    "Obelisco del Coseno e dei Condotti",
                    "Segui le vene d'ottone nel suolo: fredde quando dormono, oro quieto nell'armonia, ambra vibrante nel sovraccarico, nere sotto la pece.",
                    "Ruota gli specchi [E] per caricare le piastre solari del cancello, poi verifica l'ipotesi con un Impulso di Prova [T]."
                },
                {
                    "dawn_olives",
                    "Conca degli Ulivi Bianchi",
                    "Le radici d'argento dell'Era I bevevano la cenere e scioglievano i rovi spinosi lungo le scalinate del colle.",
                    "Se trovi un Seme d'Ulivo Bianco nei Canali, innestalo nelle vasche aride [E] per aprire i sentieri bloccati dai rovi."
                },
                {
                    "dawn_crest",
                    "Cresta dei Rovi Vetrificati",
                    "Un fascio concentrato oltre la soglia ordinaria non nutre le foglie, ma fonde la selce in un passo di vetro rovente.",
                    "Sul banco [E] il Regime Critico [R] con stabilizzatore aperto apre scorciatoie arse al prezzo di forte stress termico."
                }
            };
        case ZoneId::CanalQuarter:
            return {
                {
                    "canal_plaque",
                    "Idrometro di Valerio degli Archi",
                    "Pendenza dolce da' specchio laminare e svela il guado; portata forzata gonfia le rapide; scorie di pece addensano vortici ciechi.",
                    "Osserva il colore della schiuma e tira le leve delle chiuse [E]; col Diario dell'Idraulico attivi il regime laminare."
                },
                {
                    "canal_amphora",
                    "Pozzo di Calma di Stevino",
                    "Dieci braccia metriche d'acqua quieta cantano sulla girante; oltre dodici e mezzo il getto urla e spalanca gli sfioratori alti.",
                    "Nelle Cisterne a ovest leggi le tacche sulle colonne sommerse prima di provare la Turbina con [T]."
                },
                {
                    "canal_arch",
                    "Pilone del Ponte degli Archi",
                    "Conci calcarei serrati senza malta: tengono insieme le due rive finche' l'aria resta libera dal catrame acido.",
                    "Parla con Clara Sereni [E] dopo le Terrazze e torna qui dopo le Cisterne per il bilancio dell'anello idraulico."
                },
                {
                    "canal_ford",
                    "Soglia del Guado degli Idraulici",
                    "Sotto il pelo torbido riposano i lastroni di ispezione che univano direttamente le chiuse centrali all'imbocco delle Cisterne.",
                    "Riduci l'inquinamento o applica il Codice delle Chiuse Laminari [E] per far emergere il passaggio asciutto."
                }
            };
        case ZoneId::WindPlateau:
            return {
                {
                    "wind_rose",
                    "Rosa dei Venti in Basalto",
                    "Dodici inverni di raffiche scolpiti nella roccia per insegnare agli osservatori il ritmo della cresta.",
                    "Quando il vento annuncia la raffica, riparati subito dietro un monolite frangivento."
                },
                {
                    "wind_stele",
                    "Stele Anemometrica",
                    "Chi corre contro la burrasca spreca fiato; chi attende dietro il basalto sale senza fatica.",
                    "Avanza tra una raffica e l'altra seguendo la scia d'aria verso la Torre del Pendolo."
                },
                {
                    "wind_cypress",
                    "Cipressi Piegati",
                    "Le loro chiome inclinano tutte verso sud-est, indicando l'asse esatto della corrente d'alta quota.",
                    "Dalla Torre del Pendolo a nord si sincronizzano tutte le chiuse della Conca."
                }
            };
        case ZoneId::HangingGardens:
            return {
                {
                    "garden_acoustic",
                    "Vasca Acustica di Giada",
                    "I maestri della Seconda Era accordavano la pietra come uno strumento per posare la polvere al suolo.",
                    "Calpesta le quattro piastre di risonanza nell'ordine armonico corretto per tendere il ponte di giada."
                },
                {
                    "garden_pergola",
                    "Pergolato dei Semi Dormienti",
                    "Sotto questi archi rampicanti Miriam custodisce i germogli sopravvissuti all'Inverno di Pece.",
                    "Se sbagli sequenza le piastre si azzerano: ascolta il tono crescente di ogni nodo."
                },
                {
                    "garden_terrace",
                    "Colonna dei Pollini",
                    "Quando il Risonatore canta a centoventi Hertz, il cielo orientale torna limpido in un respiro.",
                    "Raggiungi il Risonatore Armonico oltre il ponte di giada e calibralo senza usare la fornace."
                }
            };
        case ZoneId::FumaroleCaldera:
            return {
                {
                    "caldera_seals",
                    "Sigilli Geotermici di Basalto",
                    "I maestri della faglia scavarono sfiatatoi di rame per raccogliere il calore senza bruciare pece.",
                    "Osserva il tremore del vapore prima dell'eruzione e chiudi le valvole di sfogo [E] per calmare i geyser."
                },
                {
                    "caldera_relief",
                    "Bassorilievo dello Scisma",
                    "Qui la fretta accese le prime fornaci d'emergenza: le colonne di basalto portano ancora la fuliggine.",
                    "Un getto attivo respinge chi passa: attendi la pausa o aziona la valvola collegata."
                },
                {
                    "caldera_brazier",
                    "Braciere della Faglia Profonda",
                    "Segna la discesa alla Camera Geotermica dove il ciclo di Carnot dona calore pulito ai villaggi.",
                    "Parla con Matteo Verrini e scendi nella Camera Geotermica per regolare lo scambiatore a 600 K."
                }
            };
        case ZoneId::HarmonySanctuary:
            return {
                {
                    "sanctuary_mosaic",
                    "Mosaico delle Cinque Leggi",
                    "Luce, acqua, vento, suono e calore si incontrano in un cerchio diviso in cinque archi armonici.",
                    "Accorda i cinque sigilli radiali [E] attorno alla piazza per preparare la sintesi dell'Astrolabio."
                },
                {
                    "sanctuary_medallions",
                    "Medaglioni delle Quattro Ere",
                    "Alba delle Ombre, Acque e Suono, Inverno di Pece e Sigillo dell'Armonia: l'intera cronaca in pietra.",
                    "Raccogli almeno dodici reliquie nel Codex [J] per schiudere il segreto della Valle dei Semi."
                },
                {
                    "sanctuary_braziers",
                    "Bracieri di Giada del Congedo",
                    "Quando nessuna fornace brucia piu' scorie, la fiamma dei bracieri lascia spazio a una luce turchese.",
                    "Attiva i cinque monumenti e allinea l'Astrolabio Centrale [E] per risvegliare Meridia."
                }
            };
        case ZoneId::Count:
            break;
    }
    return {};
}

} // namespace eco
