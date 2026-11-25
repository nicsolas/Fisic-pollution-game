#include "core/collectible.h"

#include <array>
#include <cstddef>

namespace eco {

namespace {

// Tutti i testi usano esclusivamente caratteri ASCII stampabili (32..126)
// con apostrofi (e', piu', civilta', perche', cosi') per compatibilita' universale.
const std::array<CollectibleMeta, kTotalCollectibles>& AllCollectibleMetadata() {
    static const std::array<CollectibleMeta, kTotalCollectibles> kCatalog = {{
        // ==================== ERA I - ALBA DELLE OMBRE ====================
        {
            CollectibleId::Dawn_BrassGnomon,
            CollectibleEra::Era1_Dawn,
            CollectibleKind::Astrolabe,
            "I.1",
            "Gnomone d'Ottone",
            EraName(CollectibleEra::Era1_Dawn),
            "Terrazze dell'Alba",
            "Elena Valli",
            "Un mirino gnomonico d'ottone graduato al mezzo grado! Innestandolo sulla Lente [Q] potro' leggere l'azimut delle ombre e seguire ogni rimbalzo degli specchi.",
            {
                "Forgiato nei primi decenni dell'Alba delle Ombre, questo quadrante portatile serviva agli agrimensori per fissare l'altezza del sole prima di posare un solo concio di travertino.",
                "Sull'anello interno sono incise le tacche dell'angolo retto e la legge del coseno: quando l'ombra dello stilo eguaglia la sua altezza, il raggio incide a meta' quadrante senza disperdere calore.",
                "USO SUL CAMPO (Mirino Gnomonico): montato sulla Lente Empirica [Q], amplia il raggio di scansione, traccia le traiettorie ottiche complete tra gli specchi di bronzo e risveglia un rilievo bussola temporaneo."
            }
        },
        {
            CollectibleId::Dawn_FirstMeasureStele,
            CollectibleEra::Era1_Dawn,
            CollectibleKind::Stele,
            "I.2",
            "Stele della Prima Misura",
            EraName(CollectibleEra::Era1_Dawn),
            "Terrazze dell'Alba",
            "Elena Valli",
            "Il patto fondativo di Meridia inciso col regolo a nonio: sul banco di calibrazione [E] mi dara' passo fine e svelera' la finestra armonica dopo una prova!",
            {
                "Eretta sul margine delle Terrazze dell'Alba dopo l'incendio delle foreste di pianura, la stele conserva il giuramento dei primi maestri di pietra: 'Nessuna lampada si accenda a spese del respiro di chi verra' dopo.'",
                "Lungo il fianco destro corre un nonio millimetrico in bronzo, usato dagli apprendisti per smorzare le oscillazioni meccaniche dopo il primo impulso sperimentale.",
                "USO SUL CAMPO (Nonio della Prima Misura): nei banchi di calibrazione [E] riduce il passo di regolazione fine, allarga del 30% la finestra dello stabilizzatore pulito e rivela la banda armonica dopo un Impulso di Prova [T]."
            }
        },
        {
            CollectibleId::Dawn_SolsticeCylinder,
            CollectibleEra::Era1_Dawn,
            CollectibleKind::Cylinder,
            "I.3",
            "Cilindro dei Solstizi",
            EraName(CollectibleEra::Era1_Dawn),
            "Altopiano dei Venti",
            "Elena Valli",
            "Un cilindro anemometrico inciso a spirale: leggendo il passo delle tacche sapro' anticipare le raffiche sull'Altopiano dei Venti.",
            {
                "Gli osservatori dell'Altopiano dei Venti tracciavano su cilindri di pietra compatta le altezze solari e le correnti d'alta quota lungo dodici inverni consecutivi.",
                "Le spire piu' fitte segnano il respiro periodico delle raffiche da nord-ovest e mostrano il cono di quiete dietro i monoliti frangivento in basalto.",
                "USO SUL CAMPO (Bussola Anemometrica): attenua la spinta laterale delle raffiche improvvise e rende leggibile in anticipo il preavviso del vento lungo la salita alla Torre del Pendolo."
            }
        },
        {
            CollectibleId::Dawn_WhiteOliveSeed,
            CollectibleEra::Era1_Dawn,
            CollectibleKind::FossilSeed,
            "I.4",
            "Seme d'Ulivo Bianco",
            EraName(CollectibleEra::Era1_Dawn),
            "Quartiere dei Canali",
            "Elena Valli",
            "Un seme d'ulivo bianco ancora vivo nella resina chiara! Posso innestarlo nelle vasche di pietra arida [E] per sciogliere i rovi di pece e purificare l'aria.",
            {
                "Selezionato dai botanici dell'Era I per trattenere il calcare lungo i canali, l'ulivo bianco rifletteva la vampa estiva con le foglie argentate e filtrava le scorie acide con le radici.",
                "Sigillato nella resina balsamica prima dello Scisma, il germe interno conserva intatta la simbiosi micorrizica capace di disgregare le croste di pece solidificata.",
                "USO SUL CAMPO (Innesto Botanico Vivo): interagisci con le Vasche Botaniche spente [E] nelle Terrazze e nei Canali per far germogliare l'ulivo, aprire i varchi serrati dai Rovi di Pece, ridurre lo smog e guadagnare slancio sui sentieri fioriti."
            }
        },

        // ==================== ERA II - SECOLO DELLE ACQUE E DEL SUONO ====================
        {
            CollectibleId::Waters_HydraulicDiary,
            CollectibleEra::Era2_WatersAndSound,
            CollectibleKind::Cylinder,
            "II.1",
            "Diario dell'Idraulico",
            EraName(CollectibleEra::Era2_WatersAndSound),
            "Quartiere dei Canali",
            "Clara Sereni",
            "Il cilindro di cantiere di Valerio degli Archi! Qui c'e' lo schema del regime laminare: usandolo sulle leve delle chiuse [E] apriremo il Guado degli Idraulici!",
            {
                "Cilindro d'archivio inciso da Valerio degli Archi, capomastro del Quartiere dei Canali e progettista delle campate a secco del Ponte degli Archi durante la Seconda Era.",
                "Dimostra come una paratoia inclinata sul profilo di minima turbolenza trasformi una piena vorticosa in un velo laminare silenzioso, scoprendo i camminamenti sommersi di ispezione.",
                "USO SUL CAMPO (Codice delle Chiuse Laminari): potenzia le leve delle chiuse [E] nel Quartiere dei Canali, calmando i vortici pericolosi e prosciugando il Guado degli Idraulici come scorciatoia diretta tra Canali e Cisterne."
            }
        },
        {
            CollectibleId::Waters_SluiceStele,
            CollectibleEra::Era2_WatersAndSound,
            CollectibleKind::Stele,
            "II.2",
            "Stele delle Chiusure",
            EraName(CollectibleEra::Era2_WatersAndSound),
            "Cisterne Ipogee",
            "Elena Valli",
            "La tavola piezometrica dei maestri idraulici! Mi mostra dal vivo il carico di Stevino nelle Cisterne e sblocca le valvole di equilibrio ausiliarie.",
            {
                "Murata un tempo accanto ai pozzi di calma delle Cisterne Ipogee, questa stele insegnava agli apprendisti che la pressione sul fondo dipende solo da densita', gravita' e altezza della colonna d'acqua.",
                "Incide due limiti vitali: sotto il battente armonico la girante stalla; oltre la soglia supercritica l'acqua entra in cavitazione, scuote le volte e spalanca a forza gli sfioratori alti.",
                "USO SUL CAMPO (Tavola Piezometrica di Stevino): proietta la lettura in tempo reale di quota e kilopascal vicino alle vasche delle Cisterne e abilita la Valvola di Equilibrio Ausiliaria per attraversare all'asciutto."
            }
        },
        {
            CollectibleId::Waters_JadeTuningFork,
            CollectibleEra::Era2_WatersAndSound,
            CollectibleKind::Prism,
            "II.3",
            "Diapason di Giada",
            EraName(CollectibleEra::Era2_WatersAndSound),
            "Giardini Sospesi",
            "Miriam Ferro",
            "Vibra ancora alla frequenza pura dei Giardini! Avvicinandolo alle piastre armoniche e al Risonatore [E] sentirai subito quando il tono e' in accordo.",
            {
                "Intagliato in un singolo cristallo di vetro-giada delle cave orientali, questo diapason era il campione con cui i maestri accordavano le quattro piastre del ponte sospeso.",
                "Quando gli anelli del Risonatore oscillavano in fase, l'onda stazionaria faceva precipitare al suolo la polvere sottile prima che potesse velare gli stomi delle foglie.",
                "USO SUL CAMPO (Campione Acustico di Giada): evidenzia l'ordine crescente delle piastre di risonanza nei Giardini Sospesi e stabilizza la curva Lorentziana durante la taratura del Risonatore."
            }
        },
        {
            CollectibleId::Waters_ThreadChronometer,
            CollectibleEra::Era2_WatersAndSound,
            CollectibleKind::Astrolabe,
            "II.4",
            "Cronometro a Filo",
            EraName(CollectibleEra::Era2_WatersAndSound),
            "Torre del Pendolo",
            "Elena Valli",
            "Un bilanciere a filo tarato sull'isocronismo di Galileo: con questo riferimento posso leggere al centesimo lo scarto delle campane e del grande Pendolo.",
            {
                "Strumento tascabile dei custodi della Torre del Pendolo, usato per sincronizzare l'apertura ritmica delle chiuse lungo tutti i sei distretti della Conca.",
                "Ricorda con eleganza geometrica che il periodo di oscillazione dipende soltanto dalla radice della lunghezza del braccio e dalla gravita', mai dal peso della lente sospesa.",
                "USO SUL CAMPO (Scappamento a Filo): assiste la sequenza armonica delle campane nella Torre del Pendolo e amplia il margine di stabilita' isocrona sul banco del Pendolo."
            }
        },

        // ==================== ERA III - INVERNO DI PECE ====================
        {
            CollectibleId::Pitch_SchismMinutes,
            CollectibleEra::Era3_PitchWinter,
            CollectibleKind::Cylinder,
            "III.1",
            "Verbale dello Scisma",
            EraName(CollectibleEra::Era3_PitchWinter),
            "Caldera delle Fumarole",
            "Elena Valli",
            "Il verbale della notte in cui il Consiglio autorizzo' le fornaci d'emergenza... Contiene anche gli schemi per spegnere i bruciatori di bypass senza riflussi!",
            {
                "Nel terzo anno della Grande Siccita', col livello delle Cisterne dimezzato e il gelo alle porte, il Consiglio si spacco' tra chi chiedeva due stagioni di scavo geotermico e chi esigeva calore immediato.",
                "Per un solo voto passarono le fornaci a pece 'fino al primo disgelo'. Ma la fuliggine oscuro' gli specchi delle Terrazze, trasformando il rimedio temporaneo in dipendenza permanente.",
                "USO SUL CAMPO (Registro di Smorzamento): riduce il contraccolpo di smog durante gli Impulsi di Prova [T] fuori bersaglio e agevola la riconversione pulita dei monumenti forzati a pece."
            }
        },
        {
            CollectibleId::Pitch_EmberStele,
            CollectibleEra::Era3_PitchWinter,
            CollectibleKind::Stele,
            "III.2",
            "Stele delle Braci",
            EraName(CollectibleEra::Era3_PitchWinter),
            "Camera Geotermica",
            "Lorenzo Altieri",
            "I termodinamici dissidenti incisero qui le soglie critiche della roccia: usala sui banchi di calibrazione per distinguere il regime efficiente dal collasso!",
            {
                "Scolpita in segreto nel basalto nero dagli ingegneri allontanati durante l'Inverno di Pece, questa tavola documenta la fatica termica delle condotte spinte oltre il limite elastico.",
                "Dimostra come forzare una macchina in sovraccarico apra varchi rapidi nella roccia a caro prezzo strutturale, mentre bruciare scorie avvelena per decenni le falde sottostanti.",
                "USO SUL CAMPO (Regolo di Stress Critico): nei banchi di calibrazione [E] rende trasparente il confine tra la banda Rischiosa Supercritica [R] e il surriscaldamento distruttivo."
            }
        },
        {
            CollectibleId::Pitch_CharredSeed,
            CollectibleEra::Era3_PitchWinter,
            CollectibleKind::FossilSeed,
            "III.3",
            "Seme Annerito",
            EraName(CollectibleEra::Era3_PitchWinter),
            "Giardini Sospesi",
            "Miriam Ferro",
            "Un seme chiuso sotto un guscio di fuliggine, eppure ancora integro dentro: ci insegna come risvegliare le radici soffocate dalla cenere.",
            {
                "Raccolto da una botanica dell'Era III il giorno in cui le prime piogge acide macchiarono di ruggine le vasche dei Giardini Sospesi e costrinsero a sigillare le serre.",
                "Sulla capsula di vetro si legge: 'La scorza annerisce per difendere il cuore. Non strappate i rami secchi: togliete il fumo dal cielo e la linfa risalira' da sola.'",
                "USO SUL CAMPO (Catalizzatore di Ripresa): accelera la rigenerazione biologica di flora e fauna ogni volta che ricalibri in modo Pulito un impianto precedentemente sporco o sovraccarico."
            }
        },
        {
            CollectibleId::Pitch_BasaltThermometer,
            CollectibleEra::Era3_PitchWinter,
            CollectibleKind::Astrolabe,
            "III.4",
            "Termometro di Basalto",
            EraName(CollectibleEra::Era3_PitchWinter),
            "Camera Geotermica",
            "Elena Valli",
            "Una sonda bimetallica in rame e basalto tarata sul ciclo di Carnot: legge in anticipo i geyser della Caldera e il gradiente della faglia!",
            {
                "Costruito negli ultimi anni dell'Inverno di Pece, questo strumento a dilatazione provava che il salto termico naturale tra le Cisterne fredde e la faglia calda bastava a scaldare l'intera valle.",
                "Quando gli autori lo portarono in superficie, le valvole della Caldera erano ormai incrostate di catrame e nessuno volle fermare le caldaie per ascoltarli.",
                "USO SUL CAMPO (Sonda Termica di Faglia): prolunga il segnale di preavviso prima dell'eruzione dei geyser nella Caldera e stabilizza la lettura Kelvin nello Scambiatore Geotermico."
            }
        },

        // ==================== ERA IV - SIGILLO DELL'ARMONIA ====================
        {
            CollectibleId::Harmony_FarewellPrism,
            CollectibleEra::Era4_HarmonySeal,
            CollectibleKind::Prism,
            "IV.1",
            "Prisma del Congedo",
            EraName(CollectibleEra::Era4_HarmonySeal),
            "Cisterne Ipogee",
            "Eco di Meridia",
            "Un prisma di risonanza dei Custodi! Emettendo un impulso con la Lente [Q] posso far scattare a distanza specchi, chiuse e interruttori oltre l'acqua!",
            {
                "Deposto sull'ala alta delle Cisterne Ipogee dagli ultimi custodi dell'Era IV prima di chiudere le paratoie senza rancore, conserva nel cristallo la frequenza d'innesto dei servomeccanismi in bronzo.",
                "Invece di spezzare gli ingranaggi per paura dei saccheggiatori, i Meridiani accordarono ogni leva a questo prisma affinche' chi avesse studiato le camere alte potesse governare le chiuse da lontano.",
                "USO SUL CAMPO (Prisma di Risonanza Remota): premendo [Q] con la Lente Empirica entro 220 passi metrici da uno specchio solare, una chiusa idraulica o uno switch delle Cisterne, il raggio cristallino lo commuta a distanza!"
            }
        },
        {
            CollectibleId::Harmony_CurrentsPrism,
            CollectibleEra::Era4_HarmonySeal,
            CollectibleKind::Prism,
            "IV.2",
            "Prisma delle Correnti",
            EraName(CollectibleEra::Era4_HarmonySeal),
            "Torre del Pendolo",
            "Eco di Meridia",
            "Questo cristallo intreccia i cinque flussi della Conca: attraverso la Trama Geodetica [V] vedro' correre insieme luce, acqua, tempo, suono e calore.",
            {
                "Custodito sotto la guglia della Torre del Pendolo, il Prisma delle Correnti mappa i condotti d'ottone e di giada che uniscono i cinque monumenti in un solo organismo energetico.",
                "Spiega perche' una scelta compiuta sulle Terrazze si ripercuote fino alle Cisterne e al Ponte degli Archi: nessuna macchina a Meridia lavora mai davvero da sola.",
                "USO SUL CAMPO (Accoppiatore Geodetico): arricchisce la lettura delle linee di forza nella Trama Geodetica [V] e smorza le oscillazioni residue tra distretti adiacenti."
            }
        },
        {
            CollectibleId::Harmony_AwakeningSeed,
            CollectibleEra::Era4_HarmonySeal,
            CollectibleKind::FossilSeed,
            "IV.3",
            "Seme del Risveglio",
            EraName(CollectibleEra::Era4_HarmonySeal),
            "Santuario dell'Armonia",
            "Miriam Ferro",
            "E' vivo, Elena! Un seme custodito nel vetro-giada del Santuario: la sua sola presenza richiama impollinatori e rondini dove l'aria torna limpida.",
            {
                "Sigillato al margine del mosaico astronomico nell'ultimo giorno dell'Era IV, questo seme proviene dalla riserva intatta della Valle dei Semi oltre i portali del Santuario.",
                "Nota incisa dai botanici del Congedo: 'Non chiediamo monumenti d'oro a chi aprira' questa capsula, ma acqua chiara nei canali e silenzio di fornaci sulle terrazze.'",
                "USO SUL CAMPO (Richiamo della Biosfera): innalza la vitalita' ecologica e il ritorno di rondini, carpe e lucciole di grotta in tutte le zone portate in equilibrio."
            }
        },
        {
            CollectibleId::Harmony_AstrolabeStele,
            CollectibleEra::Era4_HarmonySeal,
            CollectibleKind::Stele,
            "IV.4",
            "Stele dell'Astrolabio",
            EraName(CollectibleEra::Era4_HarmonySeal),
            "Santuario dell'Armonia",
            "Eco di Meridia",
            "L'ultima tavola dei Meridiani: racchiude la chiave interferometrica per accordare i cinque sigilli radiali e l'Astrolabio Centrale.",
            {
                "Posta davanti alla grande sfera armillare del Santuario dell'Armonia, questa stele raccoglie in un'unica proporzione l'eredita' delle quattro ere di Meridia.",
                "Ricorda che l'energia non e' mai mancata alla terra: cio' che distingue la cura dalla rovina e' l'umilta' di osservare, formulare un'ipotesi, provare sul campo e accettare il limite naturale.",
                "USO SUL CAMPO (Chiave Armillare di Fase): guida l'allineamento di fase dell'Astrolabio Centrale [E] quando i cinque monumenti della Conca sono stati risvegliati."
            }
        }
    }};
    return kCatalog;
}

} // namespace

const CollectibleMeta& CollectibleMetadata(CollectibleId id) {
    const auto& catalog = AllCollectibleMetadata();
    const int raw = static_cast<int>(id);
    if (raw < 0 || raw >= kTotalCollectibles) {
        return catalog[0];
    }
    return catalog[static_cast<std::size_t>(raw)];
}

} // namespace eco
