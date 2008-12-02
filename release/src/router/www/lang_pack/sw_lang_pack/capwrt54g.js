var firewall2 = new Object();
firewall2.natredir="Filter för Internet NAT omdirigering";
firewall2.ident="Filter för IDENT(Port 113)";
firewall2.multi="Filter för Multicast";

var hupgrade = new Object();
hupgrade.right1="Klicka på knappen bläddra och välj den fil för inbyggd programvara som ska skickas till routern.";
hupgrade.right2="Klicka på knappen Uppgradera för att påbörja uppgraderingsprocessen. Uppgraderingen får inte avbrytas.";
hupgrade.wrimage="Fel bildfil!";

var hfacdef = new Object();
hfacdef.right1="Detta kommer att återställa alla inställningar till fabriksinställningarna. Alla dina inställningar kommer att raderas.";
hfacdef.warning="Varning! Om du klickar på OK återställs enheten till fabriksinställningarna och alla tidigare inställningar försvinner.";

var hdiag = new Object();
hdiag.right1="Skriv in den IP-adress eller det domännamn som du vill pinga och klicka på knappen Ping.";
hdiag.right2="Skriv in den IP-adress eller det domännamn som du vill spåra och klicka därefter på knappen Traceroute.";

var hlog = new Object();
hlog.right1="Du kan aktivera eller inaktivera användningen av <b>Inkommande</b> eller <b>Utgående</b> loggar genom att välja lämplig alternativknappar.";

var manage2 = new Object();
manage2.webacc="Webbåtkomst";
manage2.accser="Åtkomstserver";
manage2.wlacc="Trådlös åtkomst &nbsp;Webb";
manage2.vapass="Det bekräftade lösenordet överensstämmer inte med lösenordet som angetts. Skriv in lösenordet igen";
manage2.passnot="Lösenorden överensstämmer inte.";
manage2.selweb="Du måste åtminstone välja en webbserver!";
manage2.changpass="Routern använder för närvarande standardlösenordet. Av säkerhetsskäl måste du ändra lösenordet innan funktionen Fjärrhantering kan aktiveras. Klicka på OK om du vill ändra lösenordet. Klicka på Avbryt om du vill lämna funktionen Fjärrhantering inaktiverad.";

var hmanage2 = new Object();
hmanage2.right1="<b>Lokal routeråtkomst: </b>Här kan du ändra routerns lösenord. Ange ett nytt routerlösenord och skriv in det igen i fältet Skriv in igen för att bekräfta.<br>";
hmanage2.right2="<b>Webbåtkomst: </b>Här kan du konfigurera åtkomstalternativen till routerns webbverktyg.";
hmanage2.right3="<b>Fjärråtkomst till routern: </b>Detta möjliggör fjärråtkomst till routern. Välj den port som du vill använda. Du måste ändra lösenordet till routern om den fortfarande använder standardlösenordet.";
hmanage2.right4="<b>UPnP: </b>Används av vissa program för att automatisk öppna portar för kommunikation.";

var hstatwl2 = new Object();
hstatwl2.right1="<b>MAC-adress</b>. Detta är routerns MAC-adress som den visas på ditt lokala trådlösa nätverk.";
hstatwl2.right2="<b>Läge</b>. Väljs under fliken Trådlös och visar det trådlösa läget (Blandat, G-enbart, eller Inaktiverad) som används av nätverket.";

var hstatlan2 = new Object();
hstatlan2.right1="<b>MAC-adress</b>. Detta är routerns MAC-adress som den visas på ditt lokala Ethernet-nätverk.";
hstatlan2.right2="<b>IP-adress</b>. Detta visar routerns IP-adress som den visas på ditt lokala Ethernet-nätverk.";
hstatlan2.right3="<b>Nätmask</b>. När routern använder en nätmask visas den här.";
hstatlan2.right4="<b>DHCP-server</b>. Om du använder routern som en DHCP-server så visas det här.";

var hstatrouter2 = new Object();
hstatrouter2.wan_static="Static";
hstatrouter2.l2tp="L2TP";
//hstatrouter2.hb="HeartBeat-signal";
hstatrouter2.hb="Telstra Cable";
hstatrouter2.connecting="Ansluter";
hstatrouter2.disconnected="Frånkopplad";
hstatrouter2.disconnect="Koppla från";
hstatrouter2.right1="<b>Version på inbyggd programvara. </b>Detta är routerns nuvarande inbyggda programvara.";
hstatrouter2.right2="<b>Aktuell tid. </b>Här visas tiden som du ställer in i fliken Inställning.";
hstatrouter2.right3="<b>MAC-adress. </b>Detta är routerns MAC-adress som den visas för din Internetleverantör.";
hstatrouter2.right4="<b>Routernamn. </b>Detta är det specifika namn för routern som du ställer in i fliken Inställning.";
hstatrouter2.right5="<b>Konfigurationstyp. </b>Här visas den information som krävs av din Internetleverantör för anslutning till Internet. Denna information skrevs in i fliken Inställning. Du kan <b>Ansluta</b> eller <b>Koppla från</b> din anslutning här genom att klicka på knappen.";
hstatrouter2.authfail=" verifiering misslyckades";
hstatrouter2.noip="Kan inte hämta en IP-adress från ";
hstatrouter2.negfail=" förhandlingen misslyckades";
hstatrouter2.lcpfail=" LCP-förhandlingen misslyckades";
hstatrouter2.tcpfail="Kan inte skapa en TCP-anslutning till ";
hstatrouter2.noconn="Kan inte ansluta till ";
hstatrouter2.server=" server";

var hdmz2 = new Object();
hdmz2.right1="<b>DMZ: </b>Om detta alternativ aktiveras kommer du att exponera din router mot Internet. Alla portar kommer att kunna nås från Internet";

var hforward2 = new Object();
hforward2.right1="<b>Vidarebefordran av portintervall: </b>Vissa tillämpningar kan eventuellt kräva att specifika portar öppnas för att det ska kunna fungera ordentligt. Exempel på sådana tillämpningar omfattar servrar och vissa online-spel. När en begäran för en viss port kommer in från Internet kommer routern att skicka data till den dator som du anger. Av säkerhetsskäl bör du begränsa portvidarebefordran till enbart de portar som du använder och avmarkera kryssrutan <b>Aktivera</b> när du är klar.";

var hfilter2 = new Object();
hfilter2.delpolicy="Ta bort policyn?";
hfilter2.right1="<b>Policy för Internetåtkomst: </b>Du kan definiera upp till 10 åtkomstpolicy. Klicka på <b>Ta bort</b> för att ta bort en policy eller på <b>Sammanfattning</b> för att se en sammanfattning av policyn.";
hfilter2.right2="<b>Status: </b>Aktivera eller inaktivera en policy.";
hfilter2.right3="<b>Policynamn: </b>Du kan tilldela ett namn till din policy.";
hfilter2.right4="<b>Policytyp: </b>Välj mellan Internetåtkomst eller Inkommande trafik.";
hfilter2.right5="<b>Dagar: </b>Välj den veckodag som du vill att din policy ska tillämpas.";
hfilter2.right6="<b>Tid: </b>Välj den tidpunkt på dagen som du vill att din policy ska tillämpas.";
hfilter2.right7="<b>Blockerade tjänster: </b>Du kan välja att blockera åtkomst till vissa tjänster. Klicka på <b>Lägg till/Redigera</b> tjänster för att ändra dessa inställningar.";
hfilter2.right8="<b>Blockering av webbplats med URL: </b>Du kan blockera åtkomsten till vissa webbplatser genom att skriva in URL-adressen.";
hfilter2.right9="<b>Blockering av webbplats med nyckelord: </b>Du kan blockera åtkomsten till vissa webbplatser med nyckelord som finns på webbsidan.";

var hportser2 = new Object();
hportser2.submit="Verkställ";

var hwlad2 = new Object();
hwlad2.authtyp="Verifieringstyp";
hwlad2.basrate="Grundläggande hastighet";
hwlad2.mbps="Mbps";
hwlad2.def="Standardinst";
hwlad2.all="Alla";
hwlad2.defdef="(Standardinst: Standardinställning)";
hwlad2.fburst="Frame Burst";
hwlad2.milli="Millisekunder";
hwlad2.range="Område";
hwlad2.frathrh="Tröskelvärde för fragmentering";
hwlad2.apiso="AP-isolation";
hwlad2.off="Av";
hwlad2.on="På";
hwlad2.right1="<b>Verifieringstyp: </b>Du kan välja mellan Auto eller Delad nyckel. Verifiering med delad nyckel är säkrare men alla enheter på ditt nätverk måste också stödja verifiering med delad nyckel.";

var hwlbas2 = new Object();
hwlbas2.right1="<b>Trådlöst nätverksläge: </b>SpeedBooster är automatiskt aktiverat i <b>Blandat</b> läge och <b>G-enbart</b> läge. Om du vill utesluta klienter med trådlös-G väljer du läget <b>B-enbart</b>. Om du vill inaktivera trådlös åtkomst väljer du <b>Inaktivera</b>.";

var hwlsec2 = new Object();
hwlsec2.wpapsk="WPA i förväg utdelad nyckel";
hwlsec2.wparadius="WPA RADIUS";
hwlsec2.wpapersonal="WPA Personal";
hwlsec2.wpaenterprise="WPA Enterprise";
//new wpa2
hwlsec2.wpa2psk="Endast WPA2 i förväg utdelad nyckel";
hwlsec2.wpa2radius="Endast WPA2 RADIUS";
hwlsec2.wpa2pskmix="Blandat WPA2 i förväg utdelad nyckel";
hwlsec2.wpa2radiusmix="Blandat WPA2 RADIUS";
hwlsec2.wpa2personal="WPA2 Personal";
hwlsec2.wpa2enterprise="WPA2 Enterprise";
//new wpa2
hwlsec2.right1="<b>Säkerhetsläge: </b>Du kan välja mellan Inaktivera, WEP, WPA i förväg utdelad nyckel, WPA RADIUS eller RADIUS. Alla enheter på nätverket måste använda samma säkerhetsläge för att kunna kommunicera.";

var hwmac2 = new Object();
hwmac2.right1="<b>Klona MAC-adress: </b>Vissa Internetleverantörer kräver att du registrerar din MAC-adress. Om du inte vill registrera om din MAC-adress kan routern klona MAC-adressen som finns registrerad hos Internetleverantören.";

var hddns2 = new Object();
hddns2.right1="<b>DDNS-tjänst: </b>DDNS erbjuder möjligheten att nå ditt nätverk med hjälp av domännamn istället för IP-adresser. Tjänsten hanterar förändrade IP-adresser och uppdaterar din domäninformation dynamiskt. Du måste anmäla dig för tjänsten genom TZO.com eller DynDNS.org.";
hddns2.right2="Click <b><a target=_blank href=http://Linksys.tzo.com>here</a></b> to SIGNUP with a <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;TZO FREE TRIAL ACCOUNT";
hddns2.right3="Click <b><a target=_blank href=https://controlpanel.tzo.com>here</a></b> to Manage your <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;TZO Account";
hddns2.right4="Click <b><a target=_blank href=https://www.tzo.com/cgi-bin/Orders.cgi?ref=linksys>here</a></b> to Purchase a TZO <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;DDNS Subscription";
hddns2.right5="TZO DDNS <b><a target=_blank href=http://Linksys.tzo.com>Support/Tutorials</a></b>";

var hrouting2 = new Object();
hrouting2.right1="<b>Driftsläge: </b>Om routern är värd för din Internetanslutning väljer du läget <b>Gateway</b>. Om en annan router existerar i nätverket väljer du läget <b>Router</b>.";
hrouting2.right2="<b>Välj inställt nummer</b> </b>Detta är ett unikt vägnummer. Du kan ställa in upp till 20 vägar.";
hrouting2.right3="<b>Vägnamn: Skriv in det namn som du vill tilldela denna väg.";
hrouting2.right4="<b>Destinationen LAN-IP: </b>Detta är den fjärrvärd till vilken du önskar tilldela den statiska vägen.";
hrouting2.right5="<b>Nätmask: </b>Styr värden och nätverksdelen.";

var hindex2 = new Object();
hindex2.telstra="Telstra Cable";
hindex2.dns3="Statisk DNS 3";
hindex2.hbs="Heart Beat-server";
hindex2.l2tps="L2TP-server";
hindex2.hdhcp="<b>Automatisk konfiguration - DHCP: </b>Denna inställning är den vanligaste bland kabeloperatörer.<br><br>";
hindex2.hpppoe1="<b>PPPoE: </b>Denna inställning är den vanligaste bland DSL-leverantörer.<br>";
hindex2.hpppoe2="<b>Användarnamn: </b>Skriv in det användarnamn som tillhandahölls av din Internetleverantör.<br>";
hindex2.hpppoe3="<b>Lösenord: </b>Skriv in det lösenord som tillhandahölls av din Internetleverantör.<br>";

//hindex2.hpppoe4="<b><a target=_blank href=help/HSetup.asp>Mera...</a></b><br><br><br><br><br>";

hindex2.hstatic1="<b>Statisk IP: </b>Denna inställning är den vanligaste bland företagsinriktade Internetleverantörer.<br>";
hindex2.hstatic2="<b>Internet IP-adress: </b>Skriv in den IP-adress som tillhandahölls av din Internetleverantör.<br>";
hindex2.hstatic3="<b>Nätmask: </b>Skriv in din nätmask<br>";

//hindex2.hstatic4="<b><a target=_blank href=help/HSetup.asp>Mera...</a></b><br><br><br><br><br><br><br>";

hindex2.hpptp1="<b>PPTP: </b>Denna inställning är den vanligaste bland DSL-leverantörer.<br>";
hindex2.hpptp2="<b>Internet IP-adress: </b>Skriv in den IP-adress som tillhandahölls av din Internetleverantör.<br>";
hindex2.hpptp3="<b>Nätmask: </b>Skriv in din nätmask<br>";
hindex2.hpptp4="<b>Gateway: </b>Skriv in gateway-adressen till din Internetleverantör<br>";

//hindex2.hpptp5="<b><a target=_blank href=help/HSetup.asp>Mera...</a></b><br><br><br><br><br><br><br><br>";

hindex2.hl2tp1="<b>L2TP: </b>Denna inställning används av vissa Internetleverantörer i Europa.<br>";
hindex2.hl2tp2="<b>Användarnamn: </b>Skriv in det användarnamn som tillhandahölls av din Internetleverantör.<br>";
hindex2.hl2tp3="<b>Lösenord: </b>Skriv in det lösenord som tillhandahölls av din Internetleverantör.<br>";

//hindex2.hl2tp4="<b><a target=_blank href=help/HSetup.asp>Mera...</a></b><br><br><br><br><br><br><br><br>";

hindex2.hhb1="<b>Telstra Cable: </b>Denna inställning är den vanligaste bland DSL-leverantörer.<br>";
hindex2.hhb2="<b>Användarnamn: </b>Skriv in det användarnamn som tillhandahölls av din Internetleverantör.<br>";
hindex2.hhb3="<b>Lösenord: </b>Skriv in det lösenord som tillhandahölls av din Internetleverantör.<br>";

//hindex2.hhb4="<b><a target=_blank href=help/HSetup.asp>Mera...</a></b><br><br><br><br><br><br>";

hindex2.right1="<b>Värdnamn: </b>Skriv in det värdnamn som tillhandahölls av din Internetleverantör.<br>";
hindex2.right2="<b>Domännamn: </b>Skriv in det domännamn som tillhandahölls av din Internetleverantör.<br>";
hindex2.right3="<b>Lokal IP-adress: </b>Detta är adressen till routern.<br>";
hindex2.right4="<b>Nätmask: </b>Detta är nätmasken till routern.<br><br><br>";
hindex2.right5="<b>DHCP-server: </b>Gör att routern kan hantera dina IP-adresser.<br>";
hindex2.right6="<b>Första IP-adress: </b>Den adress som du vill börja med.<br>";
hindex2.right7="<b>Tidsinställning: </b>Välj den tidszon som du befinner dig i. Routern kan även justera automatiskt för sommartid.";
hindex2.hdhcps1="<b>Maximalt antal DHCP-användare: </b>Du kan begränsa antalet adresser som din router delar ut.<br>";

var errmsg2 = new Object();
errmsg2.err0="IP-adressen för HeartBeat-servern är ogiltig!";
errmsg2.err1="Ta bort posten?";
errmsg2.err2="Du måste ange ett SSID!";
errmsg2.err3="Ange en utdelad nyckel!";
errmsg2.err4=" har otillåtna hexadecimaltecken eller mer än 63 tecken!";
errmsg2.err5="Ogiltig nyckel, måste vara mellan 8 och 63 ASCII-tecken eller 64 hexadecimaltecken";
errmsg2.err6="Du måste ange en nyckel för Nyckel ";
errmsg2.err7="Ogiltig längd i nyckel ";
errmsg2.err8="Skriv in en lösenfras!";
errmsg2.err9="Det totala antalet kontroller överstiger 40!";
errmsg2.err10=" tillåter inte mellanslag!";
errmsg2.err11="När du är klar med alla åtgärder klickar du på Verkställ för att spara inställningarna.";
errmsg2.err12="Du måste ange ett tjänstnamn!";
errmsg2.err13="Tjänstnamnet finns redan!";
errmsg2.err14="Ogiltig IP-adress eller nätmask för LAN";

var trigger2 = new Object();
trigger2.ptrigger="Portutlösning";
trigger2.qos="QoS";
trigger2.trirange="Utlöst intervall";
trigger2.forrange="Vidarebefordrat intervall";
trigger2.sport="Startport";
trigger2.eport="Slutport";
trigger2.right1="Tillämpning Skriv in tillämpningsnamnet för utlösningen. <b>Utlöst intervall</b> För varje tillämpning, lista nummerintervallet för portutlösning. Kontrollera i Internettillämpningens dokumentation för de portnummer som behövs.<b>Startport</b> Skriv in startportnummer för utlöst intervall.<b>Slutport</b> Skriv in slutportnummer för utlöst intervall. <b>Vidarebefordrat intervall</b> För varje tillämpning, lista intervallet för vidarebefordrade portnummer. Kontrollera i Internettillämpningens dokumentation för de portnummer som behövs. <b>Startport</b> Skriv in startportnumret för vidarebefordrat intervall. <b>Slutport</b> Skriv in slutportnumret för vidarebefordrat intervall.";

var bakres2 = new Object();
bakres2.conman="Konfighantering";
bakres2.bakconf="Säkerhetskopiera konfiguration";
bakres2.resconf="Återställ konfiguration";
bakres2.file2res="Välj en fil att återställa";
bakres2.bakbutton="Säkerhetskopiera";
bakres2.resbutton="Återställ";
bakres2.right1="Du kan säkerhetskopiera den aktuella konfigurationen om du skulle behöva återställa routern till fabriksinställningarna.";
bakres2.right2="Du kan klicka på knappen Säkerhetskopiera för att skapa en säkerhetskopia av din aktuella konfiguration.";
bakres2.right3="Klicka på knappen Bläddra för att leta upp en konfigurationsfil som för närvarande finns sparad på din dator.";
bakres2.right4="Klicka på Återställ för att skriva över alla aktuella konfigurationer men det som finns i konfigurationsfilen.";

var qos = new Object();
qos.uband="Bandbredd uppströms";
qos.bandwidth="Bandbredd";
qos.dpriority="Enhetsprioritet";
qos.priority="Prioritet";
qos.dname="Enhetsnamn";
qos.low="Låg";
qos.medium="Medium";
qos.high="Hög";
qos.highest="Högsta";
qos.eppriority="Prioritet för Ethernet-port";
qos.flowctrl="Flödesreglering";
qos.appriority="Tillämpningsprioritet";
qos.specport="Specifik port";
//qos.appname="Tillämpningsnamn";
qos.appname="Tillämpningsnamn";
qos.alert1="Portvärdet ligger utanför intervallet [0 - 65535]";
qos.alert2="Startportvärdet är högre än slutportvärdet";
qos.confirm1="Genom att ställa in flera enheter, ethernet-port eller tillämpning på hög prioritet motverkar du effekterna av QoS.\nÄr du säker på att du vill fortsätta?";
/*
qos.right1="WRT54G erbjuder två typer av servicekvalitetsfunktioner, tillämpningsbaserad och portbaserad. Välj lämpligt alternativ som passar dina behov.";
qos.right2="<b>Tillämpningsbaserad Qos: </b>Du kan styra din bandbredd med utgångspunkt från den tillämpning som konsumerar bandbredd. Det finns flera förkonfigurerade tillämpningar. Du kan även specialanpassa upp till tre tillämpningar genom att skriva in det portnummer som de använder.";
qos.right3="<b>Portbaserad QoS: </b>Du kan styra din bandbredd i enlighet med vilken fysisk LAN-port din enhet är inpluggad i. Du kan tilldela Hög eller Låg prioritet till enheter anslutna till LAN-port 1 till 4.";
*/
//wireless qos
qos.optgame="Optimera speltillämpningar";
qos.wqos="Trådbunden QoS";
qos.wlqos="Trådlös QoS";
qos.edca_ap="Parametrar för EDCA AP";
qos.edca_sta="Parametrar för EDCA STA";
qos.wme="WMM-stöd";
qos.noack="Ingen bekräftelse";
qos.defdis="(Standardinställning: Inaktivera)";
qos.cwmin="CWmin";
qos.cwmax="CWmax";
qos.aifsn="AIFSN";
qos.txopb="TXOP(b)";
qos.txopag="TXOP(a/g)";
qos.admin="Admin";
qos.forced="Tvingad";
qos.ac_bk="AC_BK";
qos.ac_be="AC_BE";
qos.ac_vi="AC_VI";
qos.ac_vo="AC_VO";


qos.right1="Det finns två typer av QoS-funktioner (Quality of Service) tillgängliga: Trådbunden QoS som styr enheter inpluggade i routern med en Ethernet-kabel samt Trådlös QoS som styr enheter som är trådlöst anslutna till routern."
qos.right2="<b>Enhetsprioritet:</b> Du kan ange prioritet för all trafik från en enhet i ditt nätverk genom att ge enheten ett enhetsnamn, ange prioritet och skriva in MAC-adressen."
qos.right3="<b>Prioritet för Ethernet-port:</b> Du kan styra din datahastighet i enlighet med vilken fysisk LAN-port din enhet är inpluggad i. Du kan tilldela Hög eller Låg prioritet till datatrafik från enheter anslutna till LAN-port 1 till 4."
qos.right4="<b>Tillämpningsprioritet :</b> Du kan styra din datahastighet med utgångspunkt från den tillämpning som konsumerar bandbredd. Markera <b>Optimera speltillämpningar</b> för att automatiskt tillåta att portar för vanligt förekommande speltillämpningar får en högre prioritet. Du kan specialanpassa upp till åtta tillämpningar genom att skriva in det portnummer som de använder."
qos.right5="Trådlös QoS kallas även <b>Wi-Fi MultiMedia<sup>TM</sup> (WMM)</b> av Wi-Fi Alliance<sup>TM</sup>. Välj Aktivera för att utnyttja WMM om du använder andra trådlösa enheter som även är WMM-certifierade."
qos.right6="<b>Ingen bekräftelse:</b> Aktivera detta alternativ om du vill inaktivera bekräftelse. Om detta alternativ är aktiverat kommer routern inte att sända om data om ett fel uppstår."


var vpn2 = new Object();
vpn2.right1="Du kan välja att aktivera PPTP-, L2TP- eller IPSec-genomströmning för att låta dina nätverksenheter kommunicera via VPN.";

// for parental control

var pactrl = new Object();
pactrl.pactrl ="Föräldrastyrning";
pactrl.accsign ="Kontoanmälan";
pactrl.center1 ="Linksys föräldrastyrningslösning hjälper till att hålla din familj säker<br> när de surfar på Internet";
pactrl.center2 ="<li>Enkelt att anpassa</li><br><li>Skydda varje dator i ditt hushåll från din Linksys Router</li><br><li>Rapporterna hjälper dig att övervaka användningen av webben, e-post och IM</li>";
pactrl.center3 ="** Om du anmäler dig för denna tjänst kommer du att inaktivera routerns inbyggda Internetåtkomstpolicy";
pactrl.manageacc ="Administrera konton";
pactrl.center4 ="Administrera ditt föräldrastyrningskonto";
pactrl.signparental ="Anmäl dig för föräldrastyrningstjänsten";
pactrl.moreinfo ="Mer info";
pactrl.isprovision ="enheten är reserverad";
pactrl.notprovision ="enheten är inte reserverad";
pactrl.right1 ="Skärmbilden för föräldrastyrning ger dig möjligheten att anmäla dig och administrera ditt Linksys-konto för föräldrastyrning. Linksys tjänst för föräldrastyrning tillhandahåller kraftfulla verktyg för styrning av tillgängligheten av Internettjänster, åtkomst och funktioner och kan anpassas för varje enskild familjemedlem.";

var satusroute = new Object();
satusroute.localtime ="Ej tillgänglig";

var succ = new Object();
succ.autoreturn ="Du kommer tillbaka till föregående sida efter ett antal sekunder.";
