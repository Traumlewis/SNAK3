# SNAK3 1.0.6: Kompakte Lichtauswahl

Entwickler: Traumlewis. SDK 4.44, pebble-tool 5.1.2.

Die Texte ON und SYSTEM wurden aus draw_light_control entfernt.
Glühbirne und Abwärtspfeil bleiben erhalten. Der Pfeil wurde direkt neben die
Glühbirne verschoben; die schwarze Hintergrundfläche ist 34 statt 80 Pixel breit.
Die gesamte Lichtsteuerung, gespeicherte Auswahl und Tastenbelegung bleiben unverändert.

Alle sieben Plattformen erfolgreich gebaut: aplite, basalt, chalk, diorite,
flint, emery, gabbro. PBW-Metadaten SNAK3 / Traumlewis / 1.0.6 und ursprüngliche
Store-UUID b83a2022-7b26-45ff-bceb-dfee33249857 geprüft.

Die 44 vorhandenen nativen Tests bestehen weiterhin (11 je Variante Farbe,
Schwarzweiß, Runddisplay, emery), mit ASan und UBSan. LeakSanitizer ist wegen
der ptrace-Beschränkung der Umgebung deaktiviert.

Gezielte Emulatorprüfung auf Time 2 (Firmware 4.44.3) und Time Round (4.3.0):
- Startbildschirm mit beiden Glühbirnenzuständen.
- Pausebildschirm mit beiden Glühbirnenzuständen.
- Wechsel durch Runter, tatsächliche gefüllte/umrandete Glühbirne im Screenshot geprüft.
- Darstellung in rechteckiger und runder sicherer Spielfläche visuell geprüft.

Die Lichtlogik wurde in 1.0.5 auf allen sieben Emulatorplattformen geprüft und
für diese rein visuelle Änderung nicht verändert. Tests auf echten Uhren stehen aus.
