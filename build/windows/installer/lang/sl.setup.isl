[Messages]
WizardInfoBefore=Licen�na pogodba
AboutSetupNote=Namestitveni program je pripravil Jernej Simon�i�, jernej-gimp@ena.si%n%nSlika na prvi strani namestitvenega programa: Alexia_Death%nSlika na zadnji strani namestitvenega programa: Jakub Steiner
WinVersionTooLowError=Ta razli�ica programa GIMP potrebuje Windows XP s servisnim paketom 3, ali novej�o razli�ico programa Windows.

[CustomMessages]
DevelopmentWarningTitle=Razvojna razli�ica
;DevelopmentWarning=To je razvojna razli�ica programa GIMP, zaradi �esar nekateri deli programa morda niso dokon�ani, poleg tega pa je program lahko tudi nestabilen. V primeru te�av preverite, da le-te niso bile �e odpravljene v GIT-u, preden stopite v stik z razvijalci.%nZaradi nestabilnosti ta razli�ica ni namenjena za vsakodnevno delo, saj lahko kadarkoli izgubite svoje delo. Ali �elite vseeno nadaljevati z namestitvijo?
DevelopmentWarning=To je razvojna razli�ica namestitvenega programa za GIMP, ki �e ni tako preizku�ena kot obi�ajna razli�ica. �e naletite na kakr�ne koli te�ave pri namestitvi, jih prosim sporo�ite v Bugzilli (komponenta Installer):%n_https://bugzilla.gnome.org/enter_bug.cgi?product=GIMP%n%nV tem namestitvenem programu je nekaj znanih napak:%n- odpiranje TIFF datotek ne deluje%n- prikazane velikosti datotek so napa�ne%nProsimo, da teh napak ne sporo�ate.%n%nAli �elite vseeno nadaljevati z namestitvijo?
DevelopmentButtonContinue=&Nadaljuj
DevelopmentButtonExit=Prekini

;XPSP3Recommended=Opozorilo: uporabljate nepodprto razli�ico sistema Windows. Prosimo, namestite servisni paket 3 za Windows XP ali novej�o razli�ico sistema Windows preden nas obve��ate o kakr�nih koli te�avah.
SSERequired=Ta razli�ica programa GIMP potrebuje procesor, ki ima podporo za SSE ukaze.

Require32BPPTitle=Te�ava z zaslonskimi nastavitvami
Require32BPP=Namestitveni program je zaznal, da Windows ne deluje v 32-bitnem barvnem na�inu. Tak�ne nastavitve lahko povzro�ijo nestabilnost programa GIMP, zato je priporo�ljivo da pred nadaljevanjem spremenite barvno globino zaslona na 32 bitov.
Require32BPPContinue=&Nadaljuj
Require32BPPExit=I&zhod

InstallOrCustomize=GIMP je pripravljen na namestitev. Kliknite gumb Namesti za namestitev s privzetimi nastavitvami, ali pa kliknite gumb Po meri, �e bi radi imeli ve� nadzora nad mo�nostmi namestitve.
Install=Namest&i
Customize=&Po meri

TypeCompact=Minimalna namestitev
TypeCustom=Namestitev po meri
TypeFull=Polna namestitev

;components
ComponentsDescription=Opis
ComponentsGimp=GIMP
ComponentsGimpDescription=GIMP z vsemi privzetimi vti�niki
ComponentsDeps=Podporne knji�nice
ComponentsDepsDescription=Podporne knji�nice za GIMP, vklju�no z okoljem GTK+
ComponentsGtkWimp=Tema MS-Windows za GTK+
ComponentsGtkWimpDescription=Windows izgled za GIMP
ComponentsCompat=Podpora za stare vti�nike
ComponentsCompatDescription=Podporne knji�nice za stare zunanje vti�nike za GIMP
ComponentsTranslations=Prevodi
ComponentsTranslationsDescription=Prevodi
ComponentsPython=Podpora za Python
ComponentsPythonDescription=Omogo�a izvajanje vti�nikov za GIMP, napisanih v programskem jeziku Python
ComponentsGhostscript=Podpora za PostScript
ComponentsGhostscriptDescription=Omogo�i nalaganje PostScript datotek
ComponentsGimp32=Podpora za 32-bitne vti�nike
ComponentsGimp32Description=Omogo�a uporabo 32-bitnih vti�nikov.%nPotrebno za uporabo podpore za Python

AdditionalIcons=Dodatne ikone:
AdditionalIconsDesktop=Ustvari ikono na n&amizju
AdditionalIconsQuickLaunch=Ustvari ikono v vrstici &hitri zagon

RemoveOldGIMP=Odstrani prej�njo razli�ico programa GIMP

;%1 is replaced by file name; these messages should never appear (unless user runs out of disk space at the exact right moment)
ErrorChangingEnviron=Pri�lo je do te�av pri posodabljanju okolja za GIMP v datoteki %1. �e se pri nalaganju vti�nikov pojavijo sporo�ila o napakah, poizkusite odstraniti in ponovno namestiti GIMP.
ErrorExtractingTemp=Pri�lo je do napake pri raz�irjanju za�asnih datotek.
ErrorUpdatingPython=Pri�lo je do napake pri nastavljanju podpore za Python.
ErrorReadingGimpRC=Pri�lo je do napake pri branju datoteke %1.
ErrorUpdatingGimpRC=Pri�lo je do napake pri pisanju nastavitev v datoteko %1.

;displayed in Explorer's right-click menu
OpenWithGimp=Uredi z GIMP-om

SelectAssociationsCaption=Povezovanje vrst datotek
SelectAssociationsExtensions=Pripone:
SelectAssociationsInfo1=Izberite vste datotek, ki bi jih radi odpirali z GIMP-om
SelectAssociationsInfo2=Tu lahko izberete vrste datotek, ki se bodo odprle v GIMP-u, ko jih dvokliknete v Raziskovalcu
SelectAssociationsSelectAll=Izber&i vse
SelectAssociationsUnselectAll=Po�ist&i izbor
SelectAssociationsSelectUnused=Ne&uporabljene

ReadyMemoAssociations=Vrste datotek, ki jih bo odpiral GIMP:

RemovingOldVersion=Odstranjevanje starej�ih razli�ic programa GIMP:
;%1 = version, %2 = installation directory
;ran uninstaller, but it returned an error, or didn't remove everything
RemovingOldVersionFailed=Te razli�ice programa GIMP ne morete namestiti preko prej�nje razli�ice, namestitvenemu programu pa prej�nje razli�ice ni uspelo samodejno odstraniti.%n%nPred ponovnim name��anjem te razli�ice programa GIMP v mapo %2, odstranite prej�njo razli�ico, ali pa izberite namestitev po meri, in GIMP %1 namestite v drugo mapo.%n%nNamestitveni program se bo zdaj kon�al.
;couldn't find an uninstaller, or found several uninstallers
RemovingOldVersionCantUninstall=Te razli�ice programa GIMP ne morete namestiti preko prej�nje razli�ice, namestitvenemu programu pa ni uspelo ugotoviti, kako prej�njo razli�ico odstraniti samodejno.%n%nPred ponovnim name��anjem te razli�ice programa GIMP v mapo %2, odstranite prej�njo razli�ico, ali pa izberite namestitev po meri, in GIMP %1 namestite v drugo mapo.%n%nNamestitveni program se bo zdaj kon�al.

RebootRequiredFirst=Prej�nja razli�ica programa GIMP je bila uspe�no odstranjena, vendar je pred nadaljevanjem namestitve potrebno znova zagnati Windows.%n%nNamestitveni program bo dokon�al namestitev po ponovnem zagonu, ko se prvi� prijavi administrator.

;displayed while the files are being extracted
Billboard1=GIMP spada med prosto programje.%n%nObi��ite
;www.gimp.org (displayed between Billboard1 and Billboard2)
Billboard2=za brezpla�ne posodobitve.

;displayed if restart settings couldn't be read, or if the setup couldn't re-run itself
ErrorRestartingSetup=Pri�lo je do napake pri ponovnem zagonu namestitvenega programa. (%1)

SettingUpAssociations=Nastavljam povezane vrste datotek...
SettingUpPyGimp=Pripravljam okolje za GIMP Python...
SettingUpEnvironment=Pripravljam okolje za GIMP...
SettingUpGimpRC=Pripravljam nastavitve za 32-bitne vti�nike...

;displayed on last page
LaunchGimp=Za�eni GIMP

;shown during uninstall when removing add-ons
UninstallingAddOnCaption=Odstranjujem dodatek

InternalError=Notranja napaka (%1).

;used by installer for add-ons (currently only help)
DirNotGimp=GIMP o�itno ni name��en v izbrani mapi. �elite kljub temu nadaljevati?
