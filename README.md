# Überwachungstool für Fröling P2 / Lambdatronic S 3100

## Vorgeschichte

Fast alle Geräte in unserem Haushalt haben einen eingebauten Signalgeber, um uns zu informieren, wenn sie etwas von uns brauchen: Die Spülmaschine piept, wenn sie fertig ist, die Mikrowelle hat eine laute Klingel, die Waschmaschine ebenso, wie der Trockner. Das einzige Gerät, das keinen Ton von sich gibt, wenn es eine Störung erkennt, ist die Fröling P2 Pellet-Heizung im Keller. Es ist nicht so, dass die Heizung keine Störungen hätte oder diese nicht erkennt, nein: Wenn zum Beispiel ein zu großes Pellet den Saugschlauch blockiert, dann erkennt die Heizung das und ihre Status-LED blinkt nicht länger grün sondern rot. Dass wir das nicht bemerken, weil die Heizung hinter verschlossener Türe im Heizungskeller steht, daran hat bei Fröling wohl niemand gedacht.

Mein erster Ansatz war, eine kleine Überwachung zu bauen, die die Farbe der LED erkennt und darauf basierend einen Alarm auslöst. Ich bin Softwerker, kein Hardware-Profi. An der Erkennung der Farbe (zu kleinem Preis) bin ich gescheitert. Noch während ich daran gearbeitet habe, bin ich darauf gestoßen, dass die P2 bzw. die darin verbaute Lambdatronic S 3100 eine serielle Schnittstelle hat, über die sie praktisch voll bedienbar ist. Da ich zunächst eine Lösung wollte, für die ich mich nicht direkt mit der Heizung verbinden muss (Gewährleistung, Diskussionen mit dem Service, ...), habe ich diese Möglichkeit zunächst nicht weiter verfolgt.

Nachdem wir in den letzten Wochen mehrmals (Pellets-Lager wird leer) mehrmals durch kaltes Wasser auf die Farbe der LED hingewiesen wurden, habe ich das Projekt wiederbelebt. Dieses Mal ist mir der Mitarbeiter der Wartungsfirma egal.

## Protokollanalyse

Das Protokoll der S 3100 ist halbwegs gut analysiert. Die beste Quelle ist [dieser Thread bei mikrocontroller.net](http://www.mikrocontroller.net/topic/319990 "Fröling Heizungssteuerung auslesen"). Auch wertvoll ist [dieser Eintrag auf rigert.com](http://rigert.com/wiki-wiki/index.php?title=Fr%C3%B6ling). Die einzige Quelle, die „M3“ erwähnt ist [das hier verlinkte Archiv eines Foren-City-Threads](http://www.labviewforum.de/Thread-Mit-RS232-Daten-lesen-und-schreiben?pid=159201#pid159201).

Die drei Quellen scheinen größtenteils voneinander abzuschreiben (Forencity ist dabei der älteste, gefolgt von mikrocontroller und dann rigert.com), allen gemein ist, dass meiner Ansicht nach keiner der dort Mitschreibenden das Protokoll verstanden hat. Da wird davon gesprochen, dass 32 Mal ein bestimmter Befehl gesendet werden müsste, um die Kommunikation am Laufen zu halten, oder dass man abwechselnd „M1“ und „M2“ abfragen müsste. Ich halte das für Blödsinn.

Nach meinen Analysen sieht das Protokoll wie folgt aus:

### Datenpakete

Das Protokoll basiert auf Datenpaketen mit folgender Struktur:

| Bytes  | Beschreibung |
| ------ | ------------ |
| 2      | Befehl       |
| 1      | Länge (n)    |
| n      | Daten        |
| 2      | Prüfsumme    |

Dabei gilt:

 * Für eine Liste der bekannten Befehle siehe unten. Soweit ich sehen konnte, beginnen Befehle, die von der Heizung gesendet werden, mit „M“, Befehle, die vom Endgerät ausgehen, beginnen mit „R“. (Achtung: Das gilt nicht für Bestätigungen)
 * Die Länge zählt nur die Datenbytes, Header und Prüfsumme bleiben unberücksichtigt.
 * Die Prüfsumme ist eine einfache, vorzeichenlos berechnete, 16 Bit breite Summe über Befehl, Länge und Daten.

## Ablauf

Hier widerspreche ich den oben verlinkten Quellen: Meiner Meinung nach sieht das Protokoll vor, dass jedes Datenpaket mit einer Bestätigung quittiert wird. Dabei folgt das Bestätigungspaket folgendem Schema:

| Bytes  | Beschreibung                                       |
| ------ | -------------------------------------------------- |
| 2      | Befehl, der quittiert wird                         |
| 1      | Länge (immer 1)                                    |
| 1      | 0x01 für Bestätigung (ACK), 0x00 für Fehler (NACK) |
| 2      | Prüfsumme                                          |

Der Ablauf im Programm kann also wie folgt aussehen:

 1. Benutze einen Puffer für mindestens 260 Bytes (2 Bytes Befehl, 1 Byte Länge, max. 255 Bytes Daten, 2 Bytes Prüfsumme).
 2. 5 Bytes von der seriellen Schnittstelle lesen (2 Byte Befehl, 1 Byte Länge, 2 Bytes Prüfsumme)
 3. Wenn Länge > 0: Entsprechend weitere Bytes lesen.
 4. Die Prüfsumme prüfen.
    * Ist sie ungültig: NACK senden, weiter bei 1.
    * Ist sie gültig: ACK senden, weiter bei 5.
 5. Bestätigungen behandeln:
    * Wurde der entsprechende Befehl gesendet und ist die Länge 1 und ist der Wert 0x01? Dann ist der Befehl quittiert, weiter bei 1.
    * Wurde der entspechende Befehl gesendet und ist die Länge 1 und ist der Wert 0x00? Dann Befehl erneut senden, weiter bei 1.
    * Wurde der entsprechende Befehl nicht gesendet oder ist die Länge nicht 1, dann weiter bei 6.
 6. Befehl behandeln.

## Bekannte Befehle

Hier die Beschreibung der Befehle, so wie ich sie zusammengetragen und verstanden habe. Das unterscheidet sich zum Beispiel bei „M1“ von dem, was man in den Foren findet, dort wird immer von einer festen Reihenfolge ausgegangen. Für eine bestimmte Heizung stimmt das vermutlich auch, solange nichts neues angeschlossen wird.

Alle Texte, die übertragen werden, sind übrigens in „Codepage 850“ kodiert. Ein (Mapping zu Unicode findet sich z.B. bei der Wikipedia)[https://de.wikipedia.org/wiki/Codepage_850].

### Login („Ra“)

| Bytes  | Wert      | Beschreibung                                                        |
| ------ | --------- | ------------------------------------------------------------------- |
| 2      | 0x52 0x61 | Login-Befehl                                                        |
| 1      | 0x03      | Länge 3                                                             |
| 1      | 0x00      | unbekannt                                                           |
| 2      | 0x.. 0x.. | Benutzer-Kennung (-7: 0xff 0xf9 für Service, 1/0x00 0x01 für Kunde) |
| 2      | 0x.. 0x.. | Prüfsumme                                                           |

Die Heizung antwortet darauf mit einem ACK (siehe Ablauf) und sendet daraufhin die Befehlsblöcke MA, MB, MC, MD, ME, MF, MG, MK, ML, MM, MW, MS, MT und MU, jeweils beendet durch entsprechende MZ Befehle. Anschließend beginnt die Heizung regelmäßig M2 zu senden.

### Status („Rb“)

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x52 0x62      | Status-Befehl                                                       |
| 1      | 0x03           | Länge 3                                                             |
| 3      | 0x00 0x00 0x00 | unbekannt                                                           |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

Die Heizung antwortet darauf mit einem ACK (siehe Ablauf) und sendet ab sofort regelmäßig die Messwerte (M1).

### Parameter ändern („RI“)

Ändert einen der Werte der Heizung. Die verfügbaren Werte werden in „ME“ beschrieben.

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x52 0x49      | Befehl                                                              |
| 1      | 0x04           | Länge                                                               |
| 2      | 0x.. 0x..      | Parameter-ID                                                        |
| 2      | 0x.. 0x..      | Neuer Wert                                                          |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

Die Heizung antwortet darauf mit einem ACK/NACK (siehe Ablauf), anschließend kommt der neue Wert über „MI“.

### Messwerte („M1“)

Auch hier muss ich den Angaben aus den Foren widersprechen: Der Inhalt von M1 ist nicht fest vorgegeben, es hängt davon ab, was wie die Heizung installiert wurde. Welche Werte geliefert werden und wie diese zu interpretieren sind, kann man aus der „MA“- bzw. „MC“ Antworten ableiten, die man beim Login bekommt. Für meine Heizung sind das die folgenden Werte, für andere Heizungen können es andere Werte (oder Reihenfolgen) sein.

| Bytes  | Wert           | Beschreibung                        | Faktor | Einheit                |
| ------ | -------------- | ----------------------------------- |:------:|:----------------------:|
| 2      | 0x4D 0x31      | Status-Befehl                       |        |                        |
| 1      | 0x..           | Länge                               |        |                        |
| 2      | 0x.. 0x..      | 1. Displayzeile                     |        |                        |
| 2      | 0x.. 0x..      | 2. Displayzeile                     |        |                        |
| 2      | 0x.. 0x..      | Zustand                             |   1    | -/-                    |
| 2      | 0x.. 0x..      | Rost                                |   1    | -/-                    |
| 2      | 0x.. 0x..      | Kessel-Temperatur                   |   2    | °C                     |
| 2      | 0x.. 0x..      | Abgas-Temperatur                    |   1    | °C                     |
| 2      | 0x.. 0x..      | Abgas Schwellwert                   |   1    | °C                     |
| 2      | 0x.. 0x..      | KesselStellGr                       |   1    | %                      |
| 2      | 0x.. 0x..      | Saugzug                             |   1    | %                      |
| 2      | 0x.. 0x..      | Zuluft-Gebläse                      |   1    | %                      |
| 2      | 0x.. 0x..      | Einschub                            |   1    | %                      |
| 2      | 0x.. 0x..      | Rest-O2                             |  100   | %                      |
| 2      | 0x.. 0x..      | O2-Regler                           |  100   | %                      |
| 2      | 0x.. 0x..      | Füllstand                           |  207   | %                      |
| 2      | 0x.. 0x..      | Feuerraum-Temperatur                |   2    | °C                     |
| 2      | 0x.. 0x..      | Puffer oben                         |   2    | °C                     |
| 2      | 0x.. 0x..      | Puffer unten                        |   2    | °C                     |
| 2      | 0x.. 0x..      | Puffer Pu                           |   1    | %                      |
| 2      | 0x.. 0x..      | Boiler                              |   2    | °C                     |
| 2      | 0x.. 0x..      | Außentemperatur                     |   2    | °C                     |
| 2      | 0x.. 0x..      | Vorlauf 1 Schwellwert               |   2    | °C                     |
| 2      | 0x.. 0x..      | Vorlauf 1                           |   2    | °C                     |
| 2      | 0x.. 0x..      | Vorlauf 2 Schwellwert               |   2    | °C                     |
| 2      | 0x.. 0x..      | Vorlauf 2                           |   2    | °C                     |
| 2      | 0x.. 0x..      | KTY6_H2                             |   2    | °C                     |
| 2      | 0x.. 0x..      | KTY7_H2                             |   2    | °C                     |
| 2      | 0x.. 0x..      | Brennerstarts                       |   1    | -/-                    |
| 2      | 0x.. 0x..      | Betriebsstunden                     |   1    | h                      |
| 2      | 0x.. 0x..      | Board-Temperatur                    |   1    | °C                     |
| 2      | 0x.. 0x..      | Kessel Soll-Temperatur              |   2    | °C                     |
| 2      | 0x.. 0x..      | Prüfsumme                           |        |                        |

### Zeit („M2“)

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x32      | Befehl                                                              |
| 1      | 0x07           | Länge                                                               |
| 1      | 0x..           | Sekunden                                                            |
| 1      | 0x..           | Minuten                                                             |
| 1      | 0x..           | Stunden                                                             |
| 1      | 0x..           | Tag                                                                 |
| 1      | 0x..           | Monat                                                               |
| 1      | 0x..           | Wochentag (1: Montag, ..., 7: Sonntag)                              |
| 1      | 0x..           | Jahr                                                                |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

Alle Werte sind BCD, d.h. die oberen 4 Bit beschreiben die Zehner-Stelle, die unteren 4 Bit die Einer-Stelle.

### Fehlermeldung („M3“)

Wird gesendet, wenn ein Fehler auftritt. Vermutlich die interessanteste Meldung überhaupt, jedenfalls für meinen Anwendungszweck!

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x33      | Befehl                                                              |
| 1      | 0x0a           | Länge                                                               |
| 1      | 0x..           | Fehlertext-ID                                                       |
| 1      | 0x..           | Unbekannt                                                           |
| 1      | 0x..           | Unbekannt                                                           |
| 1      | 0x..           | Sekunden                                                            |
| 1      | 0x..           | Minuten                                                             |
| 1      | 0x..           | Stunden                                                             |
| 1      | 0x..           | Tag                                                                 |
| 1      | 0x..           | Monat                                                               |
| 1      | 0x..           | Wochentag (1: Montag, ..., 7: Sonntag)                              |
| 1      | 0x..           | Jahr                                                                |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

Der Inhalt entspricht dem von „MU“, die Fehlertext-ID ist analog denen in „MT“.

### Bezeichnung der Parameter in „M1“ („MA“)

Diese Nachrichten müssen ausgewertet werden, um den Inhalt von „M1“ auswerten zu können. Die Werte in „M1“ sind jeweils 2 Byte lang und werden in genau der Reihenfolge gesendet, in der auch die „MA“ Nachrichten gesendet wurden.

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x41      | Befehl                                                              |
| 1      | 0x..           | Länge                                                               |
| 1      | 0x..           | 'S': String, 'I': Messwerte                                         |
| 2      | 0x.. 0x..      | 'S': Typ in „MB“, 'I': Index in „MC“                                |
| 2      | 0x.. 0x..      | Unbekannt                                                           |
| n      | ...            | Parameterbezeichnung                                                |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

### Displaytexte („MB“)

Enthält die Texte, die in „MA“ referenziert werden. Bisher bekannt sind die beiden Typen 0 und 1, welche in der ersten bzw. zweiten Displayzeile angezeigt werden.

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x42      | Befehl                                                              |
| 1      | 0x..           | Länge                                                               |
| 2      | 0x.. 0x..      | Typ (0: obere Displayzeile, 1: untere Displayzeile)                 |
| 2      | 0x.. 0x..      | Index der Nachricht                                                 |
| n      | Text           | Nachrichtentext                                                     |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

### Datenformat der Werte in „M1“ („MC“)

Beschreibt, wie die Daten in „M1“ formatiert werden müssen. Da es bei mir deutlich mehr „MC“-Datensätze, als für „MA“ gibt, gehe ich davon aus, dass die übrigen Einträge anderweitig referenziert werden. Wo habe ich noch nicht herausgefunden.

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x43      | Befehl                                                              |
| 1      | 0x08           | Länge                                                               |
| 2      | 0x.. 0x..      | Index                                                               |
| 1      | 0x..           | Einheit (Codepage 850 beachten!)                                    |
| 1      | 0x..           | Anzahl der Nachkommastellen                                         |
| 2      | 0x.. 0x..      | Anzuwendender Teiler                                                |
| 2      | 0x.. 0x..      | Unbekannt (Häufig gleich Teiler, aber nicht immer)                  |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

### Einstellungsmenü („MD“)

Aus den Foren übernommen, Zweck unbekannt.

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x44      | Befehl                                                              |
| 1      | 0x..           | Länge                                                               |
| 1      | 0x..           | Unbekannt                                                           |
| 2      | 0x.. 0x..      | Unbekannt                                                           |
| 2      | 0x.. 0x..      | Unbekannt                                                           |
| 2      | 0x.. 0x..      | Unbekannt                                                           |
| n      |                | Text                                                                |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

### Parameterbeschreibung („ME“)

Aus den Foren übernommen, Zweck unbekannt. Hängt vermutlich mit „MD“ zusammen.

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x45      | Befehl                                                              |
| 1      | 0x..           | Länge                                                               |
| 2      | 0x.. 0x..      | Parameter-ID                                                        |
| 1      | 0x..           | Einheit                                                             |
| 1      | 0x..           | Nachkommastellen                                                    |
| 2      | 0x.. 0x..      | Faktor                                                              |
| 2      | 0x.. 0x..      | Minimalwert                                                         |
| 2      | 0x.. 0x..      | Maximalwert                                                         |
| 2      | 0x.. 0x..      | Standardwert                                                        |
| 5      | ...            | Unbekannt                                                           |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

### Texte Betriebsmodus („MF“)

Enthält je einen Eintrag für die Betriebsmodi („Sommerbetrieb“, „Winterbetrieb“, …). Wo die Einträge referenziert werden ist nicht mir noch nicht klar.

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x46      | Befehl                                                              |
| 1      | 0x..           | Länge                                                               |
| 2      | 0x.. 0x..      | Text-ID                                                             |
| n      | ...            | Text                                                                |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

### Wochenprogramm Heizkreise („MG“)

Noch nicht weiter analysiert.

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x47      | Befehl                                                              |
| 1      | 0x12           | Länge                                                               |
| 2      | 0x.. 0x..      | Heizkreis-Nummer                                                    |
| 1      | 0x..           | Unbekannt                                                           |
| 7      | ...            | Programmnummer für jeden Tag (Sonntag - Samstag), siehe MK          |
| 1      | 0x..           | Unbekannt                                                           |
| 7      | ...            | Programmnummer für jeden Tag (Sonntag - Samstag), siehe MK          |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

### Parameteränderung („MI“)

Wird gesendet, wenn einer der Heizungsparameter geändert wurde (z.B. durch Befehl „RI“).

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x49      | Befehl                                                              |
| 1      | 0x04           | Länge                                                               |
| 2      | 0x.. 0x..      | Parameter-ID                                                        |
| 2      | 0x.. 0x..      | Neuer Wert                                                          |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

### Tagesprogramm („MK“)

Noch nicht weiter analysiert.

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x4B      | Befehl                                                              |
| 1      | 0x06           | Länge                                                               |
| 2      | 0x.. 0x..      | Programmnummer                                                      |
| 2      | 0x.. 0x..      | Unbekannt                                                           |
| 2      | 0x.. 0x..      | Unbekannt                                                           |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

### Produktnamen („ML“)

Enthält diverse Namen von Heizungen. Wo diese Texte referenziert werden ist mir noch nicht klar.

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x4C      | Befehl                                                              |
| 1      | 0x..           | Länge                                                               |
| 10     | ...            | Unbekannt                                                           |
| n      | ...            | Name                                                                |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

### Features („MM“)

Enthält diverse Meldungen, welche Optionen eingebaut sind. Wo diese Texte referenziert werden ist mir noch nicht klar.

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x4C      | Befehl                                                              |
| 1      | 0x..           | Länge                                                               |
| 4      | ...            | Unbekannt                                                           |
| n      | ...            | Meldung                                                             |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

### Weitere texte („MW“)

Wo diese Texte referenziert werden ist mir noch nicht klar.

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x4C      | Befehl                                                              |
| 1      | 0x..           | Länge                                                               |
| 2      | ...            | Unbekannt                                                           |
| n      | ...            | Text                                                                |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

### Fehlerprotokoll-Texte („MT“)

Enthält Beschreibungen für mögliche Fehler im Fehlerprotokoll „MU“.

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x54      | Befehl                                                              |
| 1      | 0x..           | Länge                                                               |
| 1      | 0x..           | Text-ID                                                             |
| n      | ...            | Text                                                                |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

### Fehlerprotokoll („MU“)

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x55      | Befehl                                                              |
| 1      | 0x..           | Länge                                                               |
| 1      | 0x..           | Fehlertext-ID                                                       |
| 1      | 0x..           | Unbekannt                                                           |
| 1      | 0x..           | Unbekannt                                                           |
| 1      | 0x..           | Sekunden                                                            |
| 1      | 0x..           | Minuten                                                             |
| 1      | 0x..           | Stunden                                                             |
| 1      | 0x..           | Tag                                                                 |
| 1      | 0x..           | Monat                                                               |
| 1      | 0x..           | Unbekannt                                                           |
| 1      | 0x..           | Jahr                                                                |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |

### Fehlerprotokoll („MV“)

Dass es „MV“ gibt weiß ich, weil ich ein entsprechendes „MZ“ gesehen habe. Allerdings habe ich noch kein „MV“ gesehen.

### Blockende („MZ“)

Beendet die Übertragung eines der obigen Daten-Blöcke.

| Bytes  | Wert           | Beschreibung                                                        |
| ------ | -------------- | ------------------------------------------------------------------- |
| 2      | 0x4D 0x55      | Befehl                                                              |
| 1      | 0x01           | Länge                                                               |
| 1      | 0x..           | Beendeter Block                                                     |
| 2      | 0x.. 0x..      | Prüfsumme                                                           |






































