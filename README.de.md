# mod-custom-races

[English](README.md) · **Deutsch** · [Français](README.fr.md) · [Español](README.es.md) · [Português (Brasil)](README.pt-BR.md) · [Русский](README.ru.md) · [简体中文](README.zh-Hans.md)

*Kurzfassung – die vollständige Beschreibung steht im [englischen README](README.md).*

Mehr spielbare Rassen für **Conquest of Azeroth** (CoA), ein AzerothCore-Modul.

Der Spielclient von Conquest of Azeroth bringt die Modelle, Häute und Frisuren vieler Rassen mit, die kein Spieler wählen kann: Goblins, Worgen, Pandaren, Vulpera, Vrykul und mehr. Dieses Modul macht sie spielbar. Jede neue Rasse lehnt sich an eine Standardrasse an – Startort, Werte, Volksfähigkeiten und Fraktion –, behält aber ihr eigenes Aussehen. Der Client-Patch wird aus dem eigenen Spielclient des Spielers gebaut; das Modul bringt keine Modelle mit. **Testversion.**

## Anpassen behält das sechste Aussehen

Das Fenster zum Anpassen eines vorhandenen Charakters schickt im Stock-Paket nur Geschlecht und die fünf
gewöhnlichen Aussehensbytes. Die sechste Esteria-Auswahl blieb deshalb beim Speichern auf dem alten Wert.
`patches/core-customize-extra.patch` lässt den Server den vom gepatchten Client angehängten Wert lesen;
`tools/customize_extra_patch.py` ergänzt dafür einen Thunk in CoA Customs bereits gepatchter `Ascension.exe`.
Die Werte kommen aus denselben `HighmountainFields` wie die Vorschau: ein Byte für Highmountain/Irdene, acht
Bytes plus `HRC1` für Haranir. Der Thunk setzt außerdem `EsteriaAppearanceContext` vor dem Charakterauswahl-Hook,
damit auch ein Kaltstart dieselben Esteria-Sections wie die Vorschau benutzt. Die Unit-Hooks bleiben absichtlich
unangetastet: Sie laufen auch für NPCs und Bots; ein falscher Charakterzeiger dort ließ den Client beim
Weltbetreten abstürzen.

AFK Realm 1.0.50 und neuer erledigen das automatisch für einen erkannten CoA-Custom-1.9-Client-Root. Die
fingerprintgeprüften Thunks landen auf temporären Kopien von `Ascension.exe` und `EsteriaAppearance.dll`; die
Dateien im Pack selbst bleiben unverändert. Sowohl der installierte Client als auch die exportierte ZIP benutzen
die gepatchten Kopien. Eine unbekannte oder bereits gepatchte Datei bleibt unangetastet. Die Werkzeuge bleiben zum
Prüfen, für einen anderen Client-Build oder zum manuellen Vorbereiten eines Packs.

Gegen den Start-Crash darf die `dxvk.conf` im Client-Root **kein**
`dxvk.enableGraphicsPipelineLibrary`/`dxvk.trackPipelineLifetime` setzen; damit stürzte der Esteria-Texturlader
beim Kaltstart ab. Die mitgelieferte `d3d9.dll` (DXVK 2.7.1) gehört daneben. Die DBCs bleiben unverändert.

Die `EsteriaAppearance.dll` von CoA Custom 1.9 startet außerdem ihren Textur-Worker, bevor das D3D9-Gerät und
die ersten Texturen fertig sind. Das ist der verbleibende Timing-Crash vor dem Login; eine zweite GPU-Anwendung
verdeckt ihn nur. `tools/esteria_start_delay.py` ergänzt eine Zwei-Sekunden-Wartezeit am Worker-Start
(DLL-RVA `0x41EF8`) als versionsgeprüfte Übergangslösung. Sauber wäre, den Worker erst nach dem Ready-Signal des
Renderers zu starten; das Werkzeug ist nur für die aktuelle 414.720-Byte-DLL und verweigert andere Builds.

AFK Realm 1.0.50 und neuer ergänzen diese Wartezeit automatisch auf einer temporären Kopie derselben DLL. Der
Client-Root liefert weiterhin `dxvk.conf`, die DXVK-2.7.1-`d3d9.dll` und die Equipment-Fitting-Dateien. Ersetzt CoA
Custom die DLL durch einen Build mit anderem Fingerprint, lässt AFK Realm sie absichtlich unverändert; dann sind
das Werkzeug oder ein Fix im CoA-Client nötig.

AFK Realm 1.0.52 und neuer haben für den 64-Rassen-Patch und die hohen Rassennummern sichtbare Schalter auf der
Rassenseite. Wird der CoA-Pack-Ordner nach der Installation gelöscht, benutzt AFK Realm die bereits neben
`Ascension.exe` liegende `dinput8.dll` und behält die installierten Client-Root-Dateien.

## Installation

Am einfachsten mit [AFK Realm](https://github.com/aspollon/AFK-Realm): *Module → Module verwalten* fügt das Modul hinzu, baut den Server neu und bringt alles Nötige mit; die Einstellungsseite des Moduls ist auf Deutsch. Ohne AFK Realm: siehe das englische README.

Die Texte, die AFK Realm von diesem Modul zeigt, liegen übersetzt in `lang/`.
