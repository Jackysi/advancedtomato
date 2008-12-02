var firewall2 = new Object();
firewall2.natredir="Internet-NAT-Umleitung filtern";
firewall2.ident="Filter-IDENT. (Port 113)";
firewall2.multi="Multicast filtern";

var hupgrade = new Object();
hupgrade.right1="Klicken Sie auf die Schaltfläche <b>Durchsuchen</b>, um die Firmwaredatei auszuwählen, die auf den Router hochgeladen werden soll.";
hupgrade.right2="Klicken Sie auf die Schaltfläche <b>Aktualisieren</b>, um mit dem Aktualisierungsvorgang zu beginnen. Der Aktualisierungsvorgang darf nicht unterbrochen werden.";
//hupgrade.wrimage="Falsche Bilddatei";
hupgrade.wrimage="Keine gültige Konfigurationsdatei." //Michael modify

var hfacdef = new Object();
hfacdef.right1="<b>Werkseinstellungen wiederherstellen: </b> Dadurch werden alle Einstellungen auf die Werkseinstellungen zurückgesetzt. Alle Einstellungen werden gelöscht.";
hfacdef.warning="Warnung! Wenn Sie auf OK klicken, wird das Gerät auf die Werkseinstellungen zurückgesetzt und alle bisherigen Einstellungen werden gelöscht.";

var hdiag = new Object();
hdiag.right1="Geben Sie die IP-Adresse oder den Domänennamen ein, für die/den Sie einen Ping-Test durchführen möchten, und klicken Sie auf die Schaltfläche <b>Ping</b>.";
hdiag.right2="Geben Sie die IP-Adresse oder den Domänennamen, die/den Sie verfolgen möchten, ein, und klicken Sie dann auf die Schaltfläche <b>Routenverfolgung</b>.";

var hlog = new Object();
hlog.right1="Sie können eingehenden und ausgehenden Datenverkehr protokollieren. Aktivieren Sie dazu die entsprechende Optionsschaltfläche.";

var manage2 = new Object();
manage2.webacc="Web-Zugriff";
manage2.accser="Zugriffsserver";
manage2.wlacc="Wireless Web-&nbsp;Zugriff";
manage2.vapass="Die von Ihnen eingegebenen Kennwörter stimmen nicht überein. Geben Sie die Kennwörter erneut ein.";
manage2.passnot="Das BestätigungsPasswort stimmt nicht mit dem Passwort überein.";
manage2.selweb="Sie mussen mindestens einen Webserver auswahlen.";
manage2.changpass="Für den Router ist derzeit das StandardPasswort festgelegt. Sie müssen das Passwort aus Sicherheitsgründen ändern, bevor die Funktion Remote-Management aktiviert werden kann. Klicken Sie auf die Schaltfläche OK, um Ihr Passwort zu ändern. Klicken Sie auf die Schaltfläche Abbrechen, um die Funktion Remote-Management deaktiviert zu lassen.";

var hmanage2 = new Object();
hmanage2.right1="<b>Lokaler Routerzugriff: </b>  Hier können Sie das Passwort des Routers ändern. Geben Sie ein neues Passwort für den Router ein, und geben Sie dieses zur Bestätigung erneut im Feld <b>Zur Bestätigung erneut eingeben</b> ein.<br>";
//hmanage2.right2="<b>Web Access: </b>Allows you to configure access options to the router's web utility.";*
hmanage2.right2="<b>Web-Zugriff: </b>Ermöglicht die Konfiguration von Zugriffsoptionen auf das webbasierte Dienstprogramm des Routers.";
hmanage2.right3="<b>Entfernter Routerzugriff: </b> Mit dieser Funktion können Sie von einem entfernten Standort auf den Router zugreifen. Wählen Sie den Port, den Sie verwenden möchten. Wenn für den Router noch das Standard-Passwort verwendet wird, müssen Sie dieses ändern.";
hmanage2.right4="<b>UPnP:  </b>Wird von bestimmten Programmen verwendet, um Ports automatisch für Kommunikationszwecke zu öffnen.";

var hstatwl2 = new Object();
hstatwl2.right1="<b>MAC-Adresse: </b> Hierbei handelt es sich um die MAC-Adresse des Routers, vom Standpunkt Ihres lokalen Wireless-Netzwerks aus gesehen.";
hstatwl2.right2="<b>Modus: </b> Hier wird der auf der Registerkarte <b>Wireless</b> ausgewählte und im Netzwerk verwendete Wireless-Modus (<b>Gemischt</b>, <b>Nur G</b> oder <b>Deaktiviert</b>) angezeigt.";

var hstatlan2 = new Object();
hstatlan2.right1="<b>MAC-Adresse: </b> Hierbei handelt es sich um die MAC-Adresse des Routers, vom Standpunkt Ihres lokalen Ethernet-Netzwerks aus gesehen.";
hstatlan2.right2="<b>IP-Adresse: </b> Hier wird die IP-Adresse des Routers angezeigt, so wie sie in Ihrem lokalen Ethernet-Netzwerk angezeigt wird.";
hstatlan2.right3="<b>Subnetzmaske: </b> Wenn der Router eine Subnetzmaske verwendet, wird diese hier angezeigt.";
hstatlan2.right4="<b>DHCP-Server: </b> Wenn Sie den Router als DHCP-Server verwenden, wird dies hier angezeigt.";

var hstatrouter2 = new Object();
hstatrouter2.wan_static="Statisches";
hstatrouter2.l2tp="L2TP";
hstatrouter2.hb="HeartBeat-Server";
hstatrouter2.connecting="Verbindung wird hergestellt";
hstatrouter2.disconnected="Getrennt";
hstatrouter2.disconnect="Trennen";
hstatrouter2.right1="<b>Firmware-Version:  </b>Hierbei handelt es sich um die aktuelle Firmware-Version des Routers.";
hstatrouter2.right2="<b>Aktuelle Uhrzeit:  </b>Hier wird die Uhrzeit so angezeigt, wie Sie sie auf der Registerkarte <b>Einrichtung</b> festgelegt haben.";
hstatrouter2.right3="<b>MAC-Adresse:  </b>Hierbei handelt es sich um die MAC-Adresse des Routers, vom Standpunkt des ISP aus gesehen.";
hstatrouter2.right4="<b>Routername:  </b>Hierbei handelt es sich um den Namen für den Router, den Sie auf der Registerkarte <b>Einrichtung</b> festgelegt haben.";
hstatrouter2.right5="<b>Konfigurationstyp:  </b>Hier werden die für eine Internetverbindung von Ihrem ISP benötigten Informationen angezeigt. Diese Informationen wurden auf der Registerkarte <b>Einrichtung</b> eingegeben. Hier können Sie die Verbindung herstellen bzw. trennen, indem Sie auf die Schaltfläche <b>Verbinden</b> bzw. <b>Trennen</b> klicken.";
hstatrouter2.authfail=" Authentifizierung fehlgeschlagen";
hstatrouter2.noip="IP-Adresse kann nicht ermittelt werden ";
hstatrouter2.negfail=" Verhandlung fehlgeschlagen";
hstatrouter2.lcpfail=" LCP-Verhandlung fehlgeschlagen";
hstatrouter2.tcpfail="TCP-Verbindung kann nicht aufgebaut werden ";
hstatrouter2.noconn="Verbindung kann nicht hergestellt werden ";
//hstatrouter2.server=" server";*
hstatrouter2.server=" server";
hstatrouter2.pppoenoip="Es kann keine IP-Adresse vom PPPOE-Server abgerufen werden.";

var hdmz2 = new Object();
hdmz2.right1="<b>DMZ:  </b>Das Aktivieren dieser Funktion schaltet den Firewall-Schutz für den Rechner mit der hier angegebenen IP-Adresse ab. Es kann aus dem Internet auf alle Ports an diesem Rechner zugegriffen werden.";

var hforward2 = new Object();
hforward2.right1="<b>Port-Bereich: </b> Manche Anwendungen müssen u. U. spezielle Ports öffnen, um richtig zu funktionieren. Zu diesen Anwendungen gehören Server und bestimmte Online-Spiele. Wenn aus dem Internet eine Anfrage für einen bestimmten Port eingeht, leitet der Router die Daten an einen von Ihnen festgelegten Computer weiter. Aufgrund von Sicherheitsrisiken sollten Sie Port-Weiterleitung auf die Ports beschränken, die Sie verwenden, und das Kontrollkästchen <b>Aktivieren</b> deaktivieren, wenn Sie den Vorgang beendet haben.";

var hfilter2 = new Object();
hfilter2.delpolicy="Richtlinie löschen?";
hfilter2.right1="<b>Richtlinien für Internetzugriff: </b> Sie können bis zu 10 Zugriffsrichtlinien definieren. Zum Löschen einer Richtlinie klicken Sie auf <b>Löschen</b>, oder klicken Sie auf <b>Zusammenfassung</b>, um eine Zusammenfassung der Richtlinie anzuzeigen.";
hfilter2.right2="<b>Status:  </b>Aktivieren oder deaktivieren Sie eine Richtlinie.";
hfilter2.right3="<b>Richtlinienname: </b> Sie können der Richtlinie einen Namen zuweisen.";
hfilter2.right4="<b>Richtlinientyp: </b> Wählen Sie zwischen <b>Internetzugriff</b> und <b>Eingehender Datenverkehr</b>.";
hfilter2.right5="<b>Tage:  </b>Wählen Sie den Wochentag, an dem die Richtlinie gelten soll.";
hfilter2.right6="<b>Zeiten:  </b>Geben Sie die Tageszeit ein, zu der die Richtlinie gelten soll.";
hfilter2.right7="<b>Blockierte Dienste:  </b>Sie können den Zugriff auf bestimmte Dienste blockieren. Klicken Sie auf <b>Dienst hinzufügen/bearbeiten</b>, um diese Einstellungen zu ändern.";
hfilter2.right8="<b>Website nach URL-Adresse blockieren: </b> Sie können bestimmte Websites durch Eingabe der URL blockieren.";
hfilter2.right9="<b>Website nach Schlüsselwort blockieren:  </b>Sie können bestimmte Websites nach Schlüsselwörtern, die auf den Webseiten enthalten sind, blockieren.";

var hportser2 = new Object();
hportser2.submit="Übernehmen";

var hwlad2 = new Object();
hwlad2.authtyp="Authentifizierungstyp";
hwlad2.basrate="Grundrate";
hwlad2.mbps="MBit/s";
hwlad2.def="Standard";
hwlad2.all="Alle";
hwlad2.defdef="(Standard: Standard)";
hwlad2.fburst="Rahmen-Burst";
hwlad2.milli="Millis";
//hwlad2.range="Range";*
hwlad2.range="Bereich";
hwlad2.frathrh="Fragmentierungs-schwelle";
hwlad2.apiso="AP-Isolierung";
hwlad2.off="Aus";
hwlad2.on="Ein";
hwlad2.right1="<b>Authentifizierungstyp:  </b>Sie können zwischen der Option <b>Auto</b> oder <b>Freigegebener Schlüssel</b> wählen. Der Authentifizierungstyp <b>Freigegebener Schlüssel</b> ist sicherer, es müssen jedoch alle Geräte in Ihrem Netzwerk den Authentifizierungstyp <b>Freigegebener Schlüssel</b> unterstützen.";

var hwlbas2 = new Object();
hwlbas2.right1="<b>Wireless-Netzwerkmodus: </b><% support_match(\"SPEED_BOOSTER_SUPPORT\", \"1\", \"Speedbooster wird in den Modi <b>Gemischt</b> und <b>Nur G</b> automatisch aktiviert.\"); %> </b> Wenn Sie Wireless-G-Clients ausschließen möchten, wählen Sie den Modus <b>Nur B</b> aus. Wenn Sie den Wireless-Zugriff deaktivieren möchten, wählen Sie <b>Deaktiviert</b>.";

var hwlsec2 = new Object();
hwlsec2.wpapsk="WPA Vorläufiger gemeinsamer Schlüssel";
hwlsec2.wparadius="WPA-RADIUS";
hwlsec2.wpapersonal="WPA-PSK";
hwlsec2.wpaenterprise="WPA-RADIUS";
//new wpa2
hwlsec2.wpa2psk="Nur WPA2-vorinstallierter Schlüssel";
hwlsec2.wpa2radius="Nur WPA2 RADIUS";
hwlsec2.wpa2pskmix="WPA2-vorinstallierter Schlüssel gemischt";
hwlsec2.wpa2radiusmix="WPA2 RADIUS gemischt";
hwlsec2.wpa2personal="WPA2-PSK";
hwlsec2.wpa2enterprise="WPA2-RADIUS";
//new wpa2
hwlsec2.right1="<b>Sicherheitsmodus: </b>Sie können zwischen Deaktiviert, WEP, WPA-PSK (Vorläufiger gemeinsamer Schlüssel), WPA-RADIUS, WPA2-PSK, WPA2-RADIUS  oder RADIUS wählen.";

var hwmac2 = new Object();
//hwmac2.right1="<b>MAC Address Clone: </b>Some ISP will require you to register your MAC address.  If you do not wish to re-register your MAC address, you can have the router clone the MAC address that is registered with your ISP.";*
hwmac2.right1="<b>Kopieren der MAC-Adresse: </b> Bei einigen ISPs ist es erforderlich, dass Sie Ihre MAC-Adresse registrieren. Wenn Sie Ihre MAC-Adresse nicht erneut registrieren möchten, kann der Router die bei Ihrem ISP registrierte MAC-Adresse kopieren.";

var hddns2 = new Object();
hddns2.right1="<b>DDNS-Service: </b> Mit DDNS können Sie statt IP-Adressen Domänennamen verwenden, um auf Ihr Netzwerk zuzugreifen. Dieser Dienst verwaltet Änderungen der IP-Adresse und aktualisiert Ihre Domäneninformationen dynamisch. Sie müssen sich für diesen Dienst über TZO.com oder DynDNS.org anmelden.";
hddns2.right2="Click <b><a target=_blank href=http://Linksys.tzo.com>here</a></b> to SIGNUP with a <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;TZO FREE TRIAL ACCOUNT";
hddns2.right3="Click <b><a target=_blank href=https://controlpanel.tzo.com>here</a></b> to Manage your <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;TZO Account";
hddns2.right4="Click <b><a target=_blank href=https://www.tzo.com/cgi-bin/Orders.cgi?ref=linksys>here</a></b> to Purchase a TZO <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;DDNS Subscription";
hddns2.right5="TZO DDNS <b><a target=_blank href=http://Linksys.tzo.com>Support/Tutorials</a></b>";

var hrouting2 = new Object();
hrouting2.right1="<b>Betriebsmodus:  </b>Wenn der Router als Host der Internetverbindung fungiert, wählen Sie den Modus <b>Gateway</b> aus. Wenn sich ein weiterer Router in Ihrem Netzwerk befindet, wählen Sie den Modus <b>Router</b> aus.";
hrouting2.right2="<b>Set-Nummer auswählen: </b> Dies ist die eindeutige Routennummer, die Sie für bis zu 20 Routen einrichten können.";
hrouting2.right3="<b>Routenname: </b> Geben Sie den Namen ein, den Sie dieser Route zuweisen möchten.";
hrouting2.right4="<b>IP-Adresse des Ziel-LANs: </b> Dies ist der entfernte Host, dem Sie eine statische Route zuweisen möchten.";
hrouting2.right5="<b>Subnetzmaske: </b> Bestimmt den Host und den Netzwerkbereich.";

var hindex2 = new Object();
hindex2.telstra="HeartBeat-Server";
hindex2.dns3="Statisches DNS 3";
hindex2.hbs="HeartBeat-Server";
hindex2.l2tps="L2TP-Server";
hindex2.hdhcp="<b>Automatische Konfiguration - DHCP: </b>Diese Einstellung wird gewöhnlich von Kabelanbietern verwendet.<br><br>";
hindex2.hpppoe1="<b>PPPoE: </b>Diese Einstellung wird gewöhnlich von DSL-Anbietern verwendet.<br>";
hindex2.hpppoe2="<b>Benutzername: </b>Geben Sie den Benutzernamen ein, der Ihnen von Ihrem ISP zugeteilt wird.<br>";
hindex2.hpppoe3="<b>Passwort: </b>Geben Sie das Passwort ein, das Ihnen von Ihrem ISP zugeteilt wird.<br>";

//hindex2.hpppoe4="<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br><br><br>";

hindex2.hstatic1="<b>Statische IP-Adresse: </b>Diese Einstellung wird gewöhnlich von Business-Class-ISPs verwendet.<br>";
hindex2.hstatic2="<b>Internet-IP-Adresse: </b>Geben Sie die IP-Adresse ein, die Ihnen von Ihrem ISP zugeteilt wird.<br>";
hindex2.hstatic3="<b>Subnetzmaske: </b>Geben Sie Ihre Subnetzmaske ein.<br>";

//hindex2.hstatic4="<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br><br><br><br><br>";

hindex2.hpptp1="<b>PPTP: </b>Diese Einstellung wird gewöhnlich von DSL-Anbietern verwendet.<br>";
hindex2.hpptp2="<b>Internet-IP-Adresse: </b>Geben Sie die IP-Adresse ein, die Ihnen von Ihrem ISP zugeteilt wird.<br>";
hindex2.hpptp3="<b>Subnetzmaske: </b>Geben Sie Ihre Subnetzmaske ein.<br>";
hindex2.hpptp4="<b>Gateway: </b>Geben Sie die Gateway-Adresse Ihres ISP ein.<br>";

//hindex2.hpptp5="<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br><br><br><br><br><br>";

hindex2.hl2tp1="<b>L2TP: </b>Diese Einstellung wird von einigen europäischen ISPs verwendet.<br>";
hindex2.hl2tp2="<b>Benutzername: </b>Geben Sie den Benutzernamen ein, der Ihnen von Ihrem ISP zugeteilt wird.<br>";
hindex2.hl2tp3="<b>Passwort: </b>Geben Sie das Passwort ein, das Ihnen von Ihrem ISP zugeteilt wird.<br>";

//hindex2.hl2tp4="<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br><br><br><br><br><br>";

hindex2.hhb1="<b>HeartBeat: </b>Diese Einstellung wird gewöhnlich von DSL-Anbietern verwendet.<br>";
hindex2.hhb2="<b>Benutzername: </b>Geben Sie den Benutzernamen ein, der Ihnen von Ihrem ISP zugeteilt wird.<br>";
hindex2.hhb3="<b>Passwort: </b>Geben Sie das Passwort ein, das Ihnen von Ihrem ISP zugeteilt wird.<br>";

//hindex2.hhb4="<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br><br><br><br>";

hindex2.right1="<b>Hostname: </b> Geben Sie den Hostnamen ein, der Ihnen von Ihrem ISP zugeteilt wird.<br>";
hindex2.right2="<b>Domänenname:  </b>Geben Sie den Domänennamen ein, der Ihnen von Ihrem ISP zugeteilt wird.<br>";
hindex2.right3="<b>Lokale IP-Adresse: </b> Dies ist die Adresse des Routers.<br>";
hindex2.right4="<b>Subnetzmaske: </b> Dies ist die Subnetzmaske des Routers.<br><br><br>";
hindex2.right5="<b>DHCP-Server:  </b>Hiermit verwaltet der Router Ihre IP-Adressen.<br>";
hindex2.right6="<b>Start-IP-Adresse: </b> Dies ist die Adresse, mit der Sie beginnen möchten.<br>";
hindex2.right7="<b>Zeiteinstellung: </b>Wählen Sie die Zeitzone aus, in der Sie sich befinden. Der Router kann auch automatisch an die Sommerzeit angepasst werden.";
hindex2.hdhcps1="<b>Maximale Anzahl der DHCP-Benutzer: </b>Sie können die Anzahl der Adressen, die der Router zuweist, einschränken.<br>";

var errmsg2 = new Object();
//errmsg2.err0="The HeartBeat Server IP Address is invalid!";*
errmsg2.err0="The HeartBeat Server IP Address is invalid!";
errmsg2.err1="Eintrag löschen";
errmsg2.err2="Geben Sie eine SSID ein.";
errmsg2.err3="Geben Sie einen freigegebenen Schlüssel ein.";
errmsg2.err4=" enthalten unzulässige Hexadezimalziffern bzw. mehr als 63 Zeichen.";
errmsg2.err5="Ungültiger Schlüssel. Schlüssel muss zwischen 8 und 63 ASCII-Zeichen bzw. 64 Hexadezimalziffern umfassen.";
errmsg2.err6="Sie müssen einen Schlüssel eingeben. ";
errmsg2.err7="Ungültige Schlüssellänge ";
errmsg2.err8="Geben Sie eine Passphrase ein.";
errmsg2.err9="Die Anzahl der Häkchen übersteigt 40.";
errmsg2.err10=" Die MAC-Adresse darf keine Leerzeichen enthalten.";
errmsg2.err11="Klicken Sie nach Abschluss aller Vorgänge auf die Schaltfläche \'Anwenden\', um die Einstellungen zu speichern.";
errmsg2.err12="Sie müssen einen Dienstnamen eingeben.";
errmsg2.err13="Der Dienstname ist bereits vorhanden.";
errmsg2.err14="Ungültige LAN-IP-Adresse oder Subnetzmaske.";

var trigger2 = new Object();
trigger2.ptrigger="Port-Triggering";
trigger2.qos="QoS";
trigger2.trirange="Triggering-Bereich";
trigger2.forrange="Weiterleitungsbereich";
trigger2.sport="Start-Port";
trigger2.eport="Ende-Port";
trigger2.right1="Geben Sie den Anwendungsnamen des Triggers ein. <b>Triggering-Bereich:</b> Geben Sie für jede Anwendung den Anschlussbereich des Triggers ein. Die erforderlichen Port-Nummern finden Sie in der Dokumentation zu Ihrer Internet-Anwendung. <b>Start-Port:</b> Geben Sie die Start-Anschlussnummer des Triggering-Bereichs ein. <b>Ende-Port:</b> Geben Sie die End-Anschlussnummer des Triggering-Bereichs ein. <b>Weiterleitungsbereich:</b> Geben Sie für jede Anwendung den Anschlussbereich für die Weiterleitung ein. Die erforderlichen Port-Nummern finden Sie in der Dokumentation zu Ihrer Internet-Anwendung. <b>Start-Port:</b> Geben Sie die Start-Anschlussnummer des Weiterleitungsbereichs ein. <b>Ende-Port:</b> Geben Sie die End-Anschlussnummer des Weiterleitungsbereichs ein.";

var bakres2 = new Object();
bakres2.conman="Konfigurationsverwaltung";
bakres2.bakconf="Konfiguration sichern";
bakres2.resconf="Konfiguration wiederherstellen";
bakres2.file2res="Wählen Sie eine Datei zur Wiederherstellung aus";
bakres2.bakbutton="Sichern";
bakres2.resbutton="Wiederherstellen";
bakres2.right1="Sie können eine Sicherungskopie der aktuellen Konfigurationseinstellungen erstellen, wenn Sie den Router auf die Werkseinstellungen zurücksetzen müssen.";
bakres2.right2="Klicken Sie auf die Schaltfläche <b>Sichern</b>, um die aktuelle Konfiguration zu sichern.";
bakres2.right3="Klicken Sie auf die Schaltfläche <b>Durchsuchen</b>, um nach einer Konfigurationsdatei zu suchen, die bereits auf dem PC gespeichert ist.";
bakres2.right4="Klicken Sie auf <b>Wiederherstellen</b>, um alle aktuellen Konfigurationseinstellungen mit denen in der Konfigurationsdatei zu überschreiben.";

var qos = new Object();
qos.uband="Upstream-Bandbreite";
//qos.bandwidth="Bandwidth";*
qos.bandwidth="Bandbreite";
qos.dpriority="Gerätepriorität";
qos.priority="Priorität";
qos.dname="Gerätename";
qos.low="Niedrig";
qos.medium="Mittel";
qos.high="Hoch";
qos.highest="Am höchsten";
qos.eppriority="Ethernet-Port-Priorität";
qos.flowctrl="Flusssteuerung";
qos.appriority="Anwendungspriorität";
qos.specport="Spezifische Port-Nr.";
//qos.appname="Application Name";
qos.appname="Anwendungsname";
qos.alert1="Anschlusswert liegt auserhalb des zulassigen Bereichs [0 - 65535]";
qos.alert2="Start-Port-Wert ist größer als Ende-Port-Wert";
qos.confirm1="Durch das Einstellen mehrerer Geräte, eines Ethernet-Ports bzw. einer Anwendung auf eine hohe Priorität wird der Effekt des QoS u. U. beeinträchtigt. Möchten Sie trotzdem fortfahren?";
/*
qos.right1="Mit dem WRT54G stehen 2 Arten von Quality of Service zur Verfügung, <b>Anwendungsbasierter QoS</b> und <b>Anschlussbasierter QoS</b>. Wählen Sie die Ihren Anforderungen entsprechende Funktion aus.";
qos.right2="<b>Anwendungsbasierter QoS:</b> Mit dieser Option können Sie die Bandbreite auf die von der Anwendung verwendete Bandbreite einstellen. Es stehen mehrere vorkonfigurierte Anwendungen zur Verfügung. Sie können außerdem bis zu drei Anwendungen durch Eingabe der Anschlussnummer, die sie verwenden, persönlich anpassen.";
qos.right3="<b>Anschlussbasierter QoS:</b> Mit dieser Option können Sie die Bandbreite basierend auf dem physischen LAN-Port, an den das Gerät angeschlossen ist, einstellen. Sie können den an den LAN-Ports 1 - 4 angeschlossenen Geräten eine hohe bzw. niedrige Priorität zuweisen.";
*/
//wireless qos
qos.optgame="Game-Anwendungen optimieren";
qos.wqos="Wired QoS";
qos.wlqos="Wireless QoS";
qos.edca_ap="EDCA-AP-Parameter";
qos.edca_sta="EDCA-STA-Parameter";
qos.wme="WMM-Unterstützung";
qos.noack="Keine Bestätigung";
qos.defdis="(Standard: Deaktivieren)";
qos.cwmin="CWmin";
qos.cwmax="CWmax";
qos.aifsn="AIFSN";
qos.txopb="TXOP(b)";
qos.txopag="TXOP(a/g)";
qos.admin="Admin";
qos.forced="Gezwungen";
qos.ac_bk="AC_BK";
qos.ac_be="AC_BE";
qos.ac_vi="AC_VI";
qos.ac_vo="AC_VO";


qos.right1="Es gibt zwei Typen von Quality of Service-Funktionen, Wired QoS, der Geräte kontrolliert, die über ein Ethernet-Kabel mit dem Router verbunden sind, und Wireless QoS, der Geräte kontrolliert, die drahtlos mit dem Router verbunden sind."
qos.right2="<b>Gerätepriorität:</b> Sie können die Priorität für den gesamten Verkehr von einem Gerät im Netzwerk festlegen, indem Sie dem Gerät einen Gerätenamen geben, die Priorität angeben und seine MAC-Adresse eingeben."
qos.right3="<b>Ethernet-Port-Priorität:</b> Sie können Ihre Datenrate entsprechend dem physikalischen LAN-Port steuern, an den Ihr Gerät angeschlossen ist. Sie können dem Datenverkehr von Geräten, die mit den LAN-Ports 1 bis 4 verbunden sind, die Priorität Hoch oder Niedrig zuweisen."
qos.right4="<b>Anwendungspriorität:</b> Sie können die Datenrate hinsichtlich der Anwendung steuern, die Bandbreite verbraucht. Wählen Sie <b>Game-Anwendungen optimieren</b> an, damit gewöhnliche Game-Anwendungs-Ports automatisch eine höhere Priorität erhalten.  Sie können auf diese Weise bis zu acht Anwendung einrichten, indem Sie die von ihnen verwendete Port-Nummer eingeben."
qos.right5="Wireless QoS wird von der Wi-Fi Alliance<sup>TM</sup> auch als <b>Wi-Fi MultiMedia<sup>TM</sup> (WMM) </b> bezeichnet. Wählen Sie Aktivieren, um WMM zu nutzen, wenn Sie andere Drahtlosgeräte verwenden, die auch WMM-zertifiziert sind."
qos.right6="<b>Keine Bestätigung:</b> Aktivieren Sie diese Option, wenn Sie Bestätigung deaktivieren möchten. Ist diese Option aktiviert, sendet der Router bei einem Fehler nicht nochmals die Daten."


var vpn2 = new Object();
vpn2.right1="Sie können <b>PPTP-Passthrough</b>, <b>L2TP-Passthrough</b> oder <b>IPSec-Passthrough</b> aktivieren, damit Ihre Netzwerkgeräte über VPN Daten austauschen können.";

var fail = new Object();
fail.msg="Die von Ihnen eingegebenen Werte sind ungultig. Versuchen Sie es erneut.";

// for parental control

var pactrl = new Object();
pactrl.pactrl ="Kinderschutzfunktionen";
pactrl.accsign ="Konto anmelden";
pactrl.center1 ="Die Kinderschutzfunktionen von Linksys machen es möglich, dass Ihre Familie sicher im Internet surfen kann.";
pactrl.center2 ="<li>Einfache Einrichtung</li><br><li>Schützen Sie jeden Computer, den Sie zuhause verwenden, mithilfe des  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Linksys-Routers.</li><br><li>Mit Berichten können Sie Internet-, E-Mail- und IM-Verwendung überwachen.</li>";
pactrl.center3 ="** Wenn Sie sich für diesen Dienst anmelden, werden die integrierten Sicherheitsrichtlinien für den Internetzugriff deaktiviert.";
pactrl.manageacc ="Konto verwalten";
pactrl.center4 ="Konto für Kinderschutzfunktionen verwalten";
pactrl.signparental ="Beim Parental Control-Dienst anmelden";
pactrl.moreinfo ="Weitere Informationen";
pactrl.isprovision ="Gerät ist registriert";
pactrl.notprovision ="Gerät nicht registriert";
pactrl.right1 ="Im Bildschirm des Parental Control-Dienstes können Sie sich bei einem Linksys Parental Control-Konto anmelden und dieses verwalten. Mit dem <b>Parental Control-Dienst</b> von Linksys erhalten Sie leistungsstarke Werkzeuge zur Steuerung der Verfügbarkeit von Internetdiensten, Internetzugang und Internetfunktionen, die für jedes Mitglied in Ihrem Haushalt individuell einstellbar sind.";

var satusroute = new Object();
satusroute.localtime ="Nicht verfügbar";

var succ = new Object();
succ.autoreturn ="Nach mehreren Sekunden werden Sie zur vorherigen Seiten zurückgebracht.";
