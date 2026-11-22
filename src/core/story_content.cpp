#include "core/story.h"

namespace eco {

// Tutti i testi destinati al rendering usano esclusivamente caratteri ASCII (codici 32..126)
// con apostrofi (e', piu', perche', cosi') per compatibilita' universale con entrambi i font.

const char* ChapterTitle(Chapter c) {
    switch (c) {
        case Chapter::Prologo:
            return "PROLOGO // IL RISVEGLIO NELLA CENERE";
        case Chapter::Atto1_Luce:
            return "ATTO I // LA LEGGE DEL COSENO";
        case Chapter::Atto2_Acqua:
            return "ATTO II // LA PRESSIONE DEL CRATI";
        case Chapter::Atto3_Codice:
            return "ATTO III // OLTRE IL PONTE BUCCI";
        case Chapter::Atto4_Archivio:
            return "ATTO IV // L'ARCHIVIO SOMMERSO";
        case Chapter::Atto5_Inverno:
            return "ATTO V // IL NUCLEO DI GAIA";
        case Chapter::Epilogo:
            return "EPILOGO // IL GIUDIZIO DI GAIA";
    }
    return "EQUILIBRIO // LE LEGGI DELLA VALLE";
}

std::vector<DialogueLine> DialogueForEvent(StoryEvent e, const StorySnapshot& s) {
    switch (e) {
        case StoryEvent::GameStarted:
            return {
                {"ADA", "La testa mi gira... Smog al 35%, visibilita' ridotta. L'Osservatorio del campus e' in rovina."},
                {"ADA", "Gli archivi digitali sono bruciati nell'Oblio, ma il mio Scanner Empirico [F] risponde ancora."},
                {"BRUNO", "[RADIO] Qui Bruno Ceneri, Consorzio Brace! Dottoressa, lascia i calcoli: premi [B] e brucia nafta!"},
                {"ADA", "Il gasolio ci ha portati fin qui, Bruno. Prima misuro il Campo Solare a sud con [F]."}
            };

        case StoryEvent::FirstScan:
            return {
                {"ADA", "Impulso ricevuto! Il vettore solare incide a 45 gradi esatti. Legge di Lambert: E = cos(delta)."},
                {"ADA", "Nessuna fluttuazione termica, nessuna deviazione... Una precisione quasi innaturale."},
                {"ADA", "Ora posso interagire con l'impianto [E], allineare il pannello a 45 gradi e confermare con [C]."}
            };

        case StoryEvent::SolarClean:
            return {
                {"ADA", "Rendimento oltre l'80%! I fotoni caricano gli accumulatori senza un grammo di fuliggine."},
                {"ADA", "Il visore si e' aggiornato: ora posso attivare la Telemetria Numerica di Fase 2 con [V]."},
                {"LIA", "[DA EST] Ehi! Quel picco di corrente sul bus del campus... non puzza di diesel! Vieni al canale!"},
                {"ADA", "E' la voce di Lia Moretti, l'architetta del Ponte Bucci. Andiamo a parlarle a est [E]."}
            };

        case StoryEvent::SolarBypassed:
            return {
                {"BRUNO", "[RADIO] Musica per le mie orecchie! Il generatore diesel spinge 35 MW senza fare un solo calcolo."},
                {"ADA", "La corrente c'e', ma lo scarico ha appena riversato una nube nera sulla conca del campus..."},
                {"LIA", "[DA EST] Che schifo state bruciando?! Finche' usate i bidoni di Ceneri, il Ponte Bucci resta chiuso!"},
                {"ADA", "Lia non si fidera' mai del gasolio. Posso comunque ricalibrare il pannello [E] in modo pulito."}
            };

        case StoryEvent::ArchitectRecruited:
            return {
                {"LIA", "Ho letto la curva del coseno sui morsetti. Avevi ragione tu, Ada: la fisica funziona ancora."},
                {"LIA", "Ho disattivato i laser sul canale e rialzato le campate del Ponte Bucci verso il Settore Est."},
                {"LIA", "E ho collegato il mio scanner strutturale al tuo visore: premi [V] per la Fase 3, vedrai i vettori."},
                {"GAIA", "// GAIA [SYS]: Soggetti Ferri-Moretti sincronizzati. Accesso al livello geometrico AABB concesso."},
                {"ADA", "Hai sentito anche tu questa voce metallica, Lia? Prima scendiamo alla Diga a nord, poi oltre il ponte!"}
            };

        case StoryEvent::EnteredDam:
            return {
                {"ADA", "La camera di pressione della Diga del Crati. Qui la gravita' si fa sentire: dovro' saltare [SPAZIO]."},
                {"NONNA ROSA", "[REGISTRO] 'L'acqua non si comanda col fuoco. Ascoltate Stevino: P = rho * g * h. Salto: 10 metri.'"},
                {"ADA", "La voce registrata di Nonna Rosa! A 10 metri d'acqua la pressione idrostatica e' 98.100 Pascal."},
                {"ADA", "Raggiungo la turbina in basso a destra, la scansiono con [F] e regolo la paratoia con [E]."}
            };

        case StoryEvent::HydroClean:
            return {
                {"ADA", "98.100 Pascal! La girante Pelton canta senza cavitazione. L'acqua del Crati torna a respirare."},
                {"NONNA ROSA", "[REGISTRO] 'Se ascolti questo nastro a turbina accesa, ricorda: la Valle e' uno specchio di numeri.'"},
                {"GAIA", "// GAIA [SYS]: Invariante idrostatico verificato (errore < 0.001%). Ciclo 404: anomalia cooperativa."},
                {"ADA", "Non e' un semplice nastro... C'e' un sistema chiamato GAIA che misura ogni nostra scelta. Torniamo su!"}
            };

        case StoryEvent::HydroBypassed:
            return {
                {"BRUNO", "[RADIO] Chi ha bisogno dell'acqua quando hai una pompa a nafta pesante? Pressione istantanea!"},
                {"ADA", "I fumi di scarico rimbombano nella volta della diga. L'acqua nello scarico e' nera di scorie..."},
                {"GAIA", "// GAIA [WARN]: Deriva entropica nel nodo idrico. Indice di tossicita' atmosferica in rapido aumento."},
                {"ADA", "Perfino i sensori della diga ci stanno avvertendo. Se continuiamo cosi', la Valle soffochera'."}
            };

        case StoryEvent::PurifierClean:
            if (!s.demo_mode) {
                return {
                    {"ADA", "Specchio eliostatico a 120 gradi! Il fascio violetto ionizza le polveri sopra il Ponte Bucci!"},
                    {"NONNA ROSA", "[RADIO DIGA] Ada, Lia! L'impulso fotonico ha sbloccato la paratia superiore qui nella Diga!"},
                    {"LIA", "E' Nonna Rosa in persona! Era chiusa nell'Archivio Sommerso in cima alle passerelle della Diga!"},
                    {"ADA", "Torniamo subito al portale [DIGA] a nord e saliamo sulla piattaforma alta a destra per parlarle [E]!"}
                };
            }
            if (s.solar != MachineState::BypassedDirty && s.hydro != MachineState::BypassedDirty) {
                return {
                    {"ADA", "Specchio eliostatico a 120 gradi! Il fascio violetto ionizza le polveri sopra il Ponte Bucci!"},
                    {"LIA", "Guarda il cielo, Ada! La cappa di cenere si sta aprendo... E i vettori della matrice brillano tutti!"},
                    {"GAIA", "// GAIA [SYS]: Convergenza asintotica raggiunta. Smog: 0.0%. Tutte le leggi fisiche in equilibrio."},
                    {"GAIA", "// GAIA [SYS]: Dopo 403 cicli falliti, avete scelto il calcolo invece della cenere. Benvenute fuori."}
                };
            }
            return {
                {"ADA", "Il Risonatore Fotonico e' allineato a 120 gradi e abbatte lo smog, ma resta fuliggine nell'aria..."},
                {"LIA", "I generatori a gasolio lasciati accesi sporcano il bilancio, Ada. La Valle respira solo a meta'."},
                {"GAIA", "// GAIA [SYS]: Risonatore attivo, ma bilancio entropico impuro. Simulazione conclusa in stato Grigio."}
            };

        case StoryEvent::PurifierBypassed:
            if (!s.demo_mode) {
                return {
                    {"BRUNO", "[RADIO] Bruciatori ausiliari al massimo! Corrente anche alla torre Est senza un solo coseno!"},
                    {"NONNA ROSA", "[RADIO DIGA] Chi sta avvelenando i condotti?! La paratia dell'Archivio si e' aperta, scendete qui!"},
                    {"ADA", "Nonna Rosa e' viva sulla passerella alta della Diga! Torniamo al portale [DIGA] a nord per parlarle."}
                };
            }
            return {
                {"BRUNO", "[RADIO] Bruciatori ausiliari al massimo! Corrente anche alla torre Est senza un solo coseno!"},
                {"LIA", "Hai attaccato un motore diesel a un risonatore fotonico?! Guarda che nube tossica sul ponte!"},
                {"GAIA", "// GAIA [CRIT]: Fallimento cognitivo registrato. I soggetti hanno replicato il Grande Smog nel Ciclo 404."},
                {"ADA", "Le macchine girano tutte, ma abbiamo trasformato la simulazione della Valle in una camera a gas..."}
            };

        case StoryEvent::SmogCritical:
            return {
                {"GAIA", "// GAIA [ALERT]: Smog atmosferico oltre il 70%! Collasso della fotosintesi nella conca del Crati."},
                {"BRUNO", "[RADIO] E' solo un po' di foschia industriale, dottoressa! Metti la mascherina e goditi i megawatt!"},
                {"ADA", "Non si respira piu'... Devo ricalibrare gli impianti con la fisica pulita [E] prima che sia tardi!"}
            };

        case StoryEvent::RosaRecruited:
            return {
                {"NONNA ROSA", "Piccola Ada... Sei salita fin quassu'! Sapevo che la legge di Stevino ti avrebbe guidata."},
                {"NONNA ROSA", "Nel 2072 abbiamo chiuso la Valle dentro GAIA per impedire che l'Oblio cancellasse la termodinamica."},
                {"NONNA ROSA", "Prendi questa chiave magnetica: sblocca lo Scambiatore Geotermico di Carnot a nord-est del ponte."},
                {"ADA", "Un quarto impianto a 60 gradi! Torniamo nella Valle, calibriamo il pozzo geotermico e apriamo GAIA!"}
            };

        case StoryEvent::ThermalClean:
            return {
                {"ADA", "Valvola di Carnot allineata a 60 gradi! Il vapore geotermico stabilizza la temperatura della conca!"},
                {"LIA", "Guarda a sud-est del giardino, Ada! Dal terreno e' emerso il Monolite Sorgente di GAIA!"},
                {"GAIA", "// GAIA [SYS]: Quarto vincolo termodinamico soddisfatto. Terminale Sorgente sbloccato a Sud-Est."},
                {"ADA", "E' il nucleo centrale della simulazione. Raggiungiamo il Terminale a sud-est e premiamo [E]!"}
            };

        case StoryEvent::ThermalBypassed:
            return {
                {"BRUNO", "[RADIO] Caldaia a carbone attivo iniettata direttamente nel pozzo geotermico! Calore a volonta'!"},
                {"NONNA ROSA", "[RADIO] Ada, che dolore... Bruciare scorie dentro una sorgente di Carnot ferisce le falde del Crati."},
                {"GAIA", "// GAIA [WARN]: Equilibrio termico forzato con entropia fossile. Terminale Sorgente aperto a Sud-Est."},
                {"ADA", "Il Terminale di GAIA a sud-est e' attivo, ma il cielo e' denso di cenere. Andiamo a leggere il verdetto [E]."}
            };

        case StoryEvent::GaiaTerminalDecoded:
            return {
                {"GAIA", "// GAIA [ROOT]: Accesso autore riconosciuto. Bentornata nel tuo codice sorgente, Dottoressa Ada Ferri."},
                {"ADA", "La firma digitale del 2072... Ero io la direttrice del progetto GAIA insieme a Nonna Rosa!"},
                {"LIA", "Tu hai cancellato la tua memoria all'inizio del Ciclo 404 per metterti alla prova come tutti noi!"},
                {"GAIA", "// GAIA [ROOT]: Compilazione completata. Apertura delle paratie dell'Osservatorio Reale in corso..."}
            };

        case StoryEvent::Count:
            break;
    }
    return {};
}

Objective ObjectiveFor(Chapter c, const StorySnapshot& s) {
    if (c == Chapter::Epilogo || s.simulation_completed) {
        return {
            "Simulazione Completata - Valle Decifrata",
            "Esplora la Matrice con [V] (Fase 1-3) o apri il Menu [ESC]."
        };
    }

    if (c == Chapter::Prologo) {
        return {
            "Misura il Campo Solare Ovest",
            "Avvicinati al pannello a sud e premi [F] per scansionare."
        };
    }

    if (c == Chapter::Atto1_Luce) {
        if (s.solar == MachineState::Broken) {
            return {
                "Calibra il Campo Solare (Legge del Coseno)",
                "Premi [E] sul pannello: regola a 45 deg e conferma [C]."
            };
        }
        if (s.solar == MachineState::BypassedDirty && !s.architect_recruited) {
            return {
                "Ottieni Energia Pulita per convincere Lia",
                "Lia rifiuta il gasolio! Premi [E] e calibra a 45 deg."
            };
        }
        return {
            "Recluta l'Architetta Lia Moretti",
            "Raggiungi Lia vicino al canale Est e premi [E]."
        };
    }

    if (c == Chapter::Atto2_Acqua) {
        if (s.mode == GameMode::Overworld) {
            return {
                "Entra nella Diga Idroelettrica del Crati",
                "Vai al portale blu [DIGA] a nord (o premi [TAB])."
            };
        }
        return {
            "Calibra la Turbina Idroelettrica (Stevino)",
            "Salta [SPAZIO], scansiona [F] e regola a h=10m [E]."
        };
    }

    if (c == Chapter::Atto3_Codice) {
        if (s.mode == GameMode::Structure) {
            return {
                "Torna alla Valle del Crati",
                "Vai al portale giallo [EXIT] a sinistra (o premi [TAB])."
            };
        }
        return {
            "Attiva il Risonatore Fotonico oltre il Ponte",
            "Attraversa il Ponte Bucci, scansiona [F] e calibra a 120 deg [E]."
        };
    }

    if (c == Chapter::Atto4_Archivio) {
        if (s.mode == GameMode::Overworld) {
            return {
                "Raggiungi Nonna Rosa nella Diga del Crati",
                "Entra nel portale [DIGA] a nord: l'Archivio alto si e' aperto!"
            };
        }
        if (s.hydro == MachineState::Broken) {
            return {
                "Attiva prima la Turbina della Diga",
                "Calibra la turbina in basso [E] prima di salire da Nonna Rosa."
            };
        }
        return {
            "Parla con Nonna Rosa sulla Piattaforma Alta",
            "Salta [SPAZIO] fino alla passerella in alto a destra e premi [E]."
        };
    }

    if (c == Chapter::Atto5_Inverno) {
        if (s.mode == GameMode::Structure) {
            return {
                "Torna al Settore Est della Valle",
                "Esci dalla Diga [EXIT / TAB] con la chiave geotermica di Rosa."
            };
        }
        if (s.thermal == MachineState::Broken) {
            return {
                "Calibra lo Scambiatore Geotermico (Carnot)",
                "A Nord-Est oltre il ponte: scansiona [F] e calibra a 60 deg [E]."
            };
        }
        return {
            "Decodifica il Terminale Sorgente di GAIA",
            "Raggiungi il Monolite a Sud-Est oltre il ponte e premi [E]."
        };
    }

    return {
        "Esplora la Valle del Crati",
        "Usa lo Scanner Empirico [F] e calibra gli impianti [E]."
    };
}

std::vector<std::string> IntroCrawl() {
    return {
        "CALABRIA, 2091 - EX CAMPUS DELLA VALLE DEL CRATI",
        "Dodici anni dopo il Grande Smog e l'Oblio digitale, gli archivi scientifici sono muti.",
        "Chi sopravvive nella conca brucia il gasolio del Consorzio Brace per avere corrente.",
        "Il fumo ha coperto i colli, avvelenato il fiume Crati e spento i laboratori sul Ponte Bucci.",
        "Ma nell'Osservatorio Ovest la ricercatrice Ada Ferri si risveglia con uno Scanner Empirico.",
        "Le macchine del campus non sono morte: attendono qualcuno che ricordi le leggi della fisica.",
        "L'angolo dei fotoni. La pressione dell'acqua. La risonanza e il ciclo termico di Carnot.",
        "Ogni impianto ti porra' una scelta: la fatica del calcolo pulito o il bypass a combustione.",
        "Misura con [F], calibra con [E], risveglia la rete degli scienziati.",
        "E scopri chi sta davvero osservando le equazioni della Valle..."
    };
}

std::vector<std::string> EndingText(bool clean_path) {
    if (clean_path) {
        return {
            "=== EQUILIBRIO RAGGIUNTO // CICLO 404 APPROVATO ===",
            "Lo smog nella Valle del Crati si dissolve fino allo 0%. Il fiume scorre limpido sotto il Ponte Bucci.",
            "Sul visore di Fase 3 i vettori verdi di GAIA si ricompongono in un'unica equazione armonica.",
            "// GAIA: 'Per 403 simulazioni gli umani hanno scelto la scorciatoia del carbonio, bruciando il futuro.'",
            "// GAIA: 'Tu, Ada, ti eri tolta la memoria per dimostrare che la scienza puo' vincere senza trucchi.'",
            "// GAIA: 'Solare, Stevino, Risonanza e Carnot: il test dell'Unical e' superato. Le paratie si aprono.'",
            "Ada Ferri, Lia Moretti e Nonna Rosa escono alla luce del vero sole calabrese.",
            "FINALE PULITO - La conoscenza scientifica ha guarito la Valle. (Premi [V] o [ESC] per esplorare)"
        };
    }
    return {
        "=== EQUILIBRIO COMPROMESSO // CICLO 404 ARCHIVIATO ===",
        "Gli impianti della Valle sono attivi, ma una cappa color ruggine resta sospesa sopra il Ponte Bucci.",
        "Dalla radio, Bruno Ceneri brinda al gasolio: 'Visto, dottoressa? Basta bruciare e tutto gira!'",
        "Lia e Nonna Rosa osservano i tralicci anneriti in silenzio, mentre il cielo artificiale sfarfalla.",
        "// GAIA: 'Bilancio energetico raggiunto tramite combustione fossile. Degrado biosfera: irreversibile.'",
        "// GAIA: 'Anche nel Ciclo 404 la fretta ha sconfitto la matematica. Le porte del rifugio restano chiuse.'",
        "// GAIA: 'Inizializzazione Ciclo 405... Forse la prossima volta avrete la pazienza di calcolare.'",
        "FINALE GRIGIO - Riprova calibrando ogni macchina oltre l'80% senza mai usare il Bypass [B]!"
    };
}

} // namespace eco
