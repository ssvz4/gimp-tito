[Messages]
;InfoBefore page is used instead of license page because the GPL only concerns distribution, not use, and as such doesn't have to be accepted
WizardInfoBefore=Lizenzvereinbarung
AboutSetupNote=Setup erstellt von Jernej Simoncic, jernej-gimp@ena.si%n%nGrafik auf der Startseite der Installation von Alexia_Death%nGrafik auf der Abschlussseite der Installation von Jakub Steiner
WinVersionTooLowError=Diese Version von GIMP ben�tigt Windows XP Service Pack 3 oder jede neuere Version von Windows

[CustomMessages]
;shown before the wizard starts on development versions of GIMP
DevelopmentWarningTitle=Entwicklerversion
;DevelopmentWarning=Dies ist eine Entwicklerversion von GIMP. Diese kann instabil sein oder unvollendete Funktionen enthalten. Sollten Probleme auftreten, pr�fen Sie bitte zun�chst, ob diese bereits in GIT behoben wurden, bevor Sie die Entwickler kontaktieren.%nDiese Version von GIMP ist nicht f�r den tagt�glichen Einsatz bestimmt, weil sie abst�rzen kann und Sie dadurch Daten verlieren werden. Wollen Sie die Installation dennoch fortsetzen?
DevelopmentWarning=Dies ist eine Entwicklerversion des GIMP-Installers. Er wurde nicht so intensiv wie der stabile Installer getestet, was dazu f�hren kann, dass GIMP nicht sauber arbeitet. Bitte melden Sie Probleme, auf die Sie sto�en im GIMP Bugzilla (Installationskomponente):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nBekannte Probleme des Installers sind:%n- Laden von TIFF-Dateien funktioniert nicht%n- Dateigr��en werden nicht korrekt angezeigt%nBitte melden Sie diese Probleme nicht - wir kennen sie bereits.%n%nWollen Sie die Installation dennoch fortsetzen?
DevelopmentButtonContinue=&Weiter
DevelopmentButtonExit=&Abbrechen

;XPSP3Recommended=Achtung: Sie verwenden eine nicht unterst�tzte Version von Windows. Bitte aktualisieren Sie wenigstens auf Windows XP Service Pack 3 bevor Sie Probleme melden.
SSERequired=Diese Version von GIMP ben�tigt einen Prozessor, der �ber SSE-Erweiterungen verf�gt.

Require32BPPTitle=Problem mit Grafikeinstellungen
Require32BPP=Die Installationsroutine hat festgestellt, dass Ihr Windows nicht derzeit nicht mit 32 Bit Farbtiefe l�uft. Diese Einstellung ist bekannt daf�r, Stabilit�tsprobleme mit GIMP zu verursachen. Wir empfehlen deshalb, die Farbtiefe auf 32 Bit pro Pixel einzustellen, bevor Sie fortfahren.
Require32BPPContinue=&Weiter
Require32BPPExit=&Abbrechen

InstallOrCustomize=GIMP kann jetzt installiert werden. Klicken Sie auf Installieren, um mit den Standardeinstellungen zu installieren oder auf Anpassen, um festzulegen, welche Komponenten wo installiert werden.
Install=&Installieren
Customize=&Anpassen

;setup types
TypeCompact=Einfache Installation
TypeCustom=Benutzerdefinierte Installation
TypeFull=Komplette Installation

;text above component description
ComponentsDescription=Beschreibung
;components
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP und alle Standard-Plugins
ComponentsDeps=Laufzeitbibliotheken
ComponentsDepsDescription=Von GIMP ben�tigte Laufzeitbibliotheken inclusive der GTK+-Bibliothek
ComponentsGtkWimp=Windows-Engine f�r GTK+
ComponentsGtkWimpDescription=Natives Aussehen f�r GIMP
ComponentsCompat=Kompatibilit�tsmodus
ComponentsCompatDescription=Bibliotheken, die von �lteren Third-Party-Plug-Ins ben�tigt werden
ComponentsTranslations=�bersetzungen
ComponentsTranslationsDescription=�bersetzungen
ComponentsPython=Python Scriptumgebung
ComponentsPythonDescription=Erlaubt Ihnen, GIMP-Plug-Ins zu nutzen, die in der Scriptsprache Python geschrieben wurden.
ComponentsGhostscript=Postscript-Unterst�tzung
ComponentsGhostscriptDescription=erm�glicht es GIMP, Postscript- und PDF-dateien zu laden
;only when installing on x64 Windows
ComponentsGimp32=32-Bit-Unterst�tzung
ComponentsGimp32Description=Dateien installieren, die f�r die Nutzung von 32-Bit-Plug-Ins ben�tigt werden.%nF�r Python-Unterst�tzung erforderlich.

;additional installation tasks
AdditionalIcons=Zus�tzliche Verkn�pfungen:
AdditionalIconsDesktop=&Desktop-Verkn�pfung erstellen
AdditionalIconsQuickLaunch=&Quicklaunch-Verkn�pfung erstellen

RemoveOldGIMP=�ltere GIMP-Versionen entfernen

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Es gab ein Problem bei der Aktualisierung von GIMPs Umgebung in %1. Sollten Fehler beim Laden von Plug-Ins auftauchen, probieren Sie, GIMP zu deinstallieren und neu zu installieren. 
ErrorExtractingTemp=Fehler beim Entpacken tempor�rer Dateien.
ErrorUpdatingPython=Fehler bei der Aktualisierung des Python-Interpreters.
ErrorReadingGimpRC=Bei der Aktualisierung von %1 trat ein Fehler auf.
ErrorUpdatingGimpRC=Bei der Aktualisierung der Konfigurationsdatei %1 trat ein Fehler auf.

;displayed in Explorer's right-click menu
OpenWithGimp=Mit GIMP �ffnen

;file associations page
SelectAssociationsCaption=Dateizuordnungen ausw�hlen
SelectAssociationsExtensions=Erweiterungen:
SelectAssociationsInfo1=W�hlen Sie die Dateitypen, die Sie mit GIMP �ffnen wollen
SelectAssociationsInfo2=Ausgew�hlte Dateitypen werden nach Doppelklick im Explorer automatisch mit GIMP ge�ffnet.
SelectAssociationsSelectAll=&Alle ausw�hlen
SelectAssociationsUnselectAll=Auswahl auf&heben
SelectAssociationsSelectUnused=&Unbenutzte ausw�hlen

;shown on summary screen just before starting the install
ReadyMemoAssociations=Dateizuordnungen f�r GIMP:

RemovingOldVersion=Entfernung von �lteren GIMP-Installationen:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=GIMP %1 kann nicht �ber eine �ltere Version von GIMP installiert werden und die automatische Deinstallation schlug fehl.%n%nBitte entfernen Sie die vorhandene GIMP-Installation manuell bevor Sie diese Version nach %2 installieren, oder w�hlen Sie Benutzerdefinierte Installation und verwenden Sie einen anderen Installationsordner.%n%nDie Einrichtung wird nun beendet.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=GIMP %1 kann nicht �ber die derzeit installierte Version von GIMP installiert werden und die Installationsroutine konnte die vorhandene Version nicht automatisch deinstallieren.%n%nBitte entfernen Sie die vorhandene GIMP-Installation manuell bevor Sie diese Version nach %2 installieren, oder w�hlen Sie Benutzerdefinierte Installation und verwenden Sie einen anderen Installationsordner.%n%nDie Einrichtung wird nun beendet.

RebootRequiredFirst=Die vorhandene GIMP-Version wurde erfolgreich entfernt, aber Windows muss neu gestartet werden, bevor die Installation fortgef�hrt werden kann.%n%nNach dem Neustart wird die Installation automatisch fortgesetzt, sobald sich ein Administrator einloggt. 

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=Bei der Fortsetzung der Installation trat ein Fehler auf (%1).

;displayed while the files are being extracted; note the capitalisation!
Billboard1=Beachten Sie: GIMP ist Freie Software.%n%nBitte besuchen Sie
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=f�r kostenlose Aktualisierungen.

SettingUpAssociations=Richte Dateizuordnungen ein...
SettingUpPyGimp=Richte Umgebung f�r die GIMP Python-Erweiterung ein...
SettingUpEnvironment=Richte Umgebung f�r GIMP ein...
SettingUpGimpRC=Richte GIMP-Einstellungen f�r 32-Bit-Plug-Ins ein...

;displayed on last page
LaunchGimp=GIMP jetzt starten

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=Entferne Erweiterung

InternalError=Interner Fehler (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=GIMP scheint nicht im ausgew�hlten Ordner installiert zu sein. Dennoch fortfahren?
