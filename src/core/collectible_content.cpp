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
            "Un gnomone d'ottone ancora graduato al mezzo grado: i primi Meridiani misuravano la luce prima ancora di posare una pietra.",
            {
                "Forgiato nei primi decenni dell'Alba delle Ombre, questo quadrante portatile serviva agli agrimensori per fissare l'inclinazione del sole al mezzogiorno dei solstizi.",
                "Sul bordo interno sono incise le tacche dell'angolo retto e la bisettrice di 45 gradi, usata per orientare gli specchi mobili verso le piastre fotosensibili dei cancelli.",
                "Annotazione sul rovescio: 'Chi non sa leggere un'ombra costruisce contro il cielo. Chi segue il coseno riceve la luce senza chiedere tributo alla terra.'"
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
            "Qui e' inciso il patto fondativo di Meridia: nessuna opera puo' togliere all'aria piu' di quanto la luce sappia restituire.",
            {
                "Eretta sul margine delle Terrazze dell'Alba, la stele raccoglie il giuramento dei primi maestri di pietra, idraulici e osservatori del cielo.",
                "Dopo aver visto bruciare le foreste delle pianure lontane, i fondatori scelsero questo altopiano calcareo per erigere una citta' capace di vivere senza fumo.",
                "Iscrizione centrale: 'Nessuna pietra si posi senza misurarne l'ombra. Nessuna lampada si accenda a spese del respiro di chi verra' dopo di noi.'"
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
            "Un cilindro di calcare inciso a spirale: registra le altezze del sole e la direzione dei venti lungo dodici inverni.",
            {
                "Gli osservatori dell'Altopiano dei Venti tracciavano su cilindri di pietra compatta le stagioni della Conca, ruotandoli lungo i solchi delle rose dei venti.",
                "Le righe piu' fitte segnano il ritmo delle raffiche da nord-ovest e indicano dove furono eretti i monoliti frangivento per proteggere i viandanti diretti alla Torre.",
                "Da queste tavole nacque l'idea che la Conca non dovesse essere dominata, ma accordata come un unico strumento esposto alle stagioni."
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
            "Un nocciolo d'ulivo sigillato nella resina chiara: le prime radici che tennero salda la terra lungo i canali.",
            {
                "Prima degli acquedotti monumentali, le pendici calcaree di Meridia erano spoglie e battute dalla pioggia. I botanici dell'Era I selezionarono l'ulivo bianco per consolidare i terrazzamenti.",
                "Le sue foglie argentate riflettevano il calore estivo e le radici profonde impedivano al limo di intasare le prime chiuse di pietra.",
                "Miriam direbbe che Meridia non e' nata soltanto dai calcoli degli astronomi, ma dalla pazienza di chi ha atteso vent'anni per il primo raccolto."
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
            "Questi sono gli appunti del maestro che costrui' il Ponte degli Archi: guarda con quale rigore calcolava ogni spinta dell'acqua.",
            {
                "Cilindro d'archivio attribuito a Valerio degli Archi, capomastro del Quartiere dei Canali durante il pieno fiorire della Seconda Era.",
                "Nelle colonne incise descrive le chiuse a leva che deviavano la spinta delle correnti nei canali a gradini, abbassando i passaggi verso le Cisterne e il Ponte degli Archi.",
                "Passo finale: 'Un ponte ben calcolato non combatte il fiume: gli lascia spazio per cantare sotto gli archi e porta chi cammina verso i giardini.'"
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
            "La legge di Stevino scolpita nella pietra viva: dieci metri di colonna d'acqua, novantottomila e cento Pascal senza cavitazione.",
            {
                "Murata un tempo accanto alle paratoie principali delle Cisterne Ipogee, questa stele serviva da manuale eterno per gli apprendisti idraulici.",
                "Spiega che la pressione sul fondo dipende soltanto dalla densita' dell'acqua, dalla gravita' e dall'altezza della colonna, mai dalla forma della vasca.",
                "Monito inciso in calce: 'Se apri troppo la paratoia la girante ulula e si spezza; se la chiudi troppo l'acqua ristagna. Dieci metri e' la misura del respiro.'"
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
            "Toccandolo vibra ancora a centoventi Hertz esatti: era il campione con cui i maestri accordavano il Risonatore dei Giardini.",
            {
                "Intagliato in un singolo blocco di vetro-giada delle cave orientali, questo prisma fungeva da riferimento acustico per le quattro piastre di risonanza e il ponte di giada dei Giardini Sospesi.",
                "Quando i nodi venivano risvegliati in sequenza e gli anelli del Risonatore oscillavano a 120 Hertz, le polveri trasportate dai venti si depositavano al suolo prima di velare le foglie.",
                "I cronisti raccontano che nelle sere d'estate l'intero distretto orientale risuonava di un bordone limpido, appena udibile sotto il fruscio degli ulivi."
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
            "Un piccolo bilanciere d'ottone calibrato sul grande Pendolo: due metri di braccio per scandire il tempo di tutta la Conca.",
            {
                "Strumento da tasca dei custodi della Torre del Pendolo, usato per verificare l'isocronismo delle chiuse sincronizzate lungo l'altopiano.",
                "Seguendo la legge del pendolo, un braccio di due metri compie un'oscillazione completa ogni 2,84 secondi, indipendentemente dalla massa sospesa.",
                "Nota del costruttore: 'La fretta altera gli orologi e gli animi; soltanto la lunghezza del filo e il peso della terra dicono il vero tempo.'"
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
            "E' il verbale della notte in cui il Consiglio si spezzo': tre anni di siccita', e la scelta disperata di accendere le fornaci di pece.",
            {
                "Nel terzo anno della Grande Siccita', la portata delle Cisterne Ipogee scese sotto la meta' e l'inverno colpi' duramente i borghi a valle.",
                "Il Consiglio di Meridia si divise per la prima volta: gli ingegneri del calore proposero di scavare lo scambiatore geotermico profondo, ma servivano due stagioni di cantiere.",
                "La fazione dell'emergenza vinse per un solo voto, autorizzando le fornaci a pece nella Caldera 'solo fino alla prossima pioggia'. Le piogge tornarono, ma le fornaci non furono mai spente."
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
            "I maestri delle grotte scolpirono questo avvertimento prima di essere allontanati: bruciare scorie nelle faglie ferisce la pietra per secoli.",
            {
                "Incisa in fretta nel basalto scuro da un gruppo di termodinamici contrari allo Scisma, questa stele fu nascosta accanto alle valvole di sfogo che calmavano le fumarole della Caldera.",
                "Descrive il progressivo annerimento degli specchi solari causato dai fumi della Caldera: piu' la cenere oscurava il cielo, meno energia pulita arrivava, spingendo ad accendere altre fornaci.",
                "Riga conclusiva: 'La scorciatoia crea la propria necessita'. Ogni fornace accesa oggi rende piu' difficile spegnerla domani.'"
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
            "Un seme chiuso a meta' schiusa, coperto di fuliggine acida: la testimonianza muta del primo anno in cui i Giardini tacquero.",
            {
                "Raccolto e sigillato da una botanica dell'Era III quando il Risonatore Armonico venne spento per destinare le risorse alle fornaci settentrionali.",
                "Sulla capsula di vetro e' incisa una data e una sola frase: 'Oggi la pioggia ha lasciato macchie color ruggine sui petali bianchi. Chiudiamo le serre prima che muoia anche la radice.'",
                "Eppure, osservandolo attraverso la lente, il cuore del seme non e' distrutto: attendeva soltanto che qualcuno togliesse il velo di cenere dal cielo."
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
            "Uno strumento a dilatazione tarato su seicento Kelvin: gli ingegneri dell'Era III avevano trovato la via pulita, ma nessuno li ascolto'.",
            {
                "Costruito in segreto durante gli ultimi anni dell'Inverno di Pece, questo termometro a lamina di rame e basalto dimostra che la faglia profonda era stabile e generosa.",
                "Con la sorgente fredda delle cisterne a 300 Kelvin e il vapore regolato a 600 Kelvin, il ciclo di Carnot garantiva un rendimento del cinquanta per cento senza bruciare una goccia di pece.",
                "Quando il progetto venne presentato al Consiglio, la Caldera era gia' soffocata dalle scorie e gran parte degli abitanti aveva abbandonato l'altopiano."
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
            "Chiudiamo le chiuse senza rancore. L'acqua ricorda l'altezza delle proprie cadute e attendera' mani piu' pazienti delle nostre.",
            {
                "Deposto sull'ala alta delle Cisterne Ipogee dagli ultimi custodi prima che l'altopiano si svuotasse, questo prisma conserva la loro voce incisa nel cristallo.",
                "Invece di distruggere le macchine per impedirne l'abuso, i Meridiani scelsero di metterle a riposo, lasciando intatte le scale metriche e le formule nella pietra.",
                "Testamento inciso: 'Non lasciamo segreti per i potenti, ma misure aperte a chiunque abbia l'umilta' di osservare, provare e correggere.'"
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
            "Luce, acqua, tempo, suono e calore sono cinque volti della stessa bilancia: accordali insieme e l'Astrolabio tornera' a cantare.",
            {
                "Custodito tra i ballatoi luminosi della Torre del Pendolo, il Prisma delle Correnti descrive la rete geodetica completa della Conca di Meridia.",
                "Ogni monumento sostiene gli altri: l'Eliostato alimenta i rilievi, la Turbina rinnova le falde, il Pendolo sincronizza le chiuse, il Risonatore monda l'aria e lo Scambiatore dona calore pulito.",
                "Quando i cinque flussi convergono al Santuario dell'Armonia senza scorie di combustione, i quattro anelli dell'Astrolabio si allineano in un'unica fase."
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
            "E' vivo, Elena! Chiuso nel vetro-giada del Santuario, un seme della leggendaria Valle dei Semi pronto a schiudersi nell'aria limpida.",
            {
                "Sigillato al margine del mosaico astronomico nell'ultimo giorno dell'Era IV, questo campione appartiene alla riserva botanica celata oltre le porte del Santuario.",
                "I Meridiani raccolsero qui i semi piu' antichi dell'altopiano, proteggendoli dalla cenere finche' l'aria della Conca non fosse tornata pura.",
                "Nota dei botanici dell'Era IV: 'Quando questo vetro si aprira' sotto un cielo senza fumo, sapremo che l'Inverno di Pece e' finalmente finito.'"
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
            "Non e' mai mancata l'energia agli esseri umani, ma la pazienza della misura. Se hai letto tutte le nostre pietre, la Conca e' di nuovo viva.",
            {
                "L'ultima iscrizione della civilta' di Meridia, posta davanti ai cinque sigilli radiali e alla grande sfera armillare del Santuario dell'Armonia.",
                "Riassume le quattro ere in un unico arco: la meraviglia dell'Alba, la fioritura delle Acque e del Suono, la ferita dell'Inverno di Pece e la promessa del Sigillo.",
                "Epitaffio finale: 'Abbiamo chiuso il passo alla Valle dei Semi non per punire i nostri figli, ma per custodire un seme di verita': la terra guarisce quando la scienza impara ad ascoltarla.'"
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
