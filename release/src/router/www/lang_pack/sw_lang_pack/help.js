//Basic Setup
var hsetup = new Object();
hsetup.phase1="Skärmen <i>Inställningar</i> \
		är den första skärmen du ser vid åtkomst till routern. De flesta \
		användare kommer att kunna konfigurera routern och få den att fungera genom \
		inställningarna på denna skärm. Vissa Internetleverantörer kräver att du skriver in \
		specifik information som t.ex. användarnamn, lösenord, IP-adress, \
		standardadress till gateway eller DNS IP-adress. Denna information kan erhållas från \
		din Internetleverantör om det behövs.";
hsetup.phase2="Observera: Efter \
		du har konfigurerat dessa inställningar bör du ställa in ett nytt lösenord för routern \
		i skärmbilden <i>Säkerhet</i>. Detta ökar säkerheten och skyddar \
		routern mot obehöriga ändringar. Alla användare som försöker nå routerns \
		webbaserade verktyg eller installationsguide kommer att uppmanas om routerns lösenord.";
hsetup.phase3="Välj den \
		tidszon för där du befinner dig. Om sommartid används ska du se till att kryssrutan \
    		är markerad bredvid <i>Justera klockan automatiskt för sommartid \
    		</i>.";
hsetup.phase4="MTU står för \
   		största överföringsenhet. Den anger den största tillåtna paketstorleken \
    		för Internetöverföring. Behåll standardinställningen <b>Auto</b> för att \
    		låta routern välja bästa MTU för din Internetanslutning. För att ange en \
    		MTU-storlek väljer du <b>Manuell</b> och skriver in önskat värde (standardvärdet är <b> \
    		1400</b>).&nbsp; Detta värde bör vara i intervallet 1200 till 1500.";
hsetup.phase5="Detta värde är nödvändig för vissa Internetleverantörer och tillhandahålls av dem.";
hsetup.phase6="Routern stödjer fyra anslutningstyper:";
hsetup.phase7="Automatisk konfiguration DHCP";
hsetup.phase8="(Point-to-Point Protocol over Ethernet)";
hsetup.phase9="(Point-to-Point Tunneling Protocol)";
hsetup.phase10="Dessa typer kan väljas från den nedrullningsbara menyn bredvid Internetanslutning. \
    		Nödvändig information och tillgängliga funktioner varierar beroende på \
    		vilken anslutningstyp du väljer. Kort beskrivning av denna \
    		information finns här:";		
hsetup.phase11="Internet IP-adress och nätmask";
hsetup.phase12="Detta är routerns IP-adress \
    		och nätmask som visas för externa användare på Internet (inklusive din \
    		Internetleverantör). Om din Internetanslutning kräver en statisk IP-adress kommer din \
    		Internetleverantör tillhandahålla en statisk IP-adress och nätmask.";
hsetup.phase13="Din Internetleverantör kommer att tillhandahålla en gateway IP-adress."
hsetup.phase14="(Domännamnsserver)";
hsetup.phase15="Internetleverantören kommer att tillhandahålla minst en DNS IP-adress.";
hsetup.phase16="Användarnamn och lösenord";
hsetup.phase17="Skriv in det <b>Användarnamn </b> och \
		<b>Lösenord</b> som du använder vid inloggning till din Internetleverantör via en PPPoE- eller PPTP- \
    		anslutning.";
hsetup.phase18="Anslut på begäran";
hsetup.phase19="du kan konfigurera routern att \
    		koppla ifrån din Internetanslutning efter en viss period med inaktivitet \
    		(Max väntetid). Om Internetanslutningen har avbrutits på grund av \
    		inaktivitet gör Anslut på begäran att routern automatiskt kan \
   		återupprätta din anslutning så fort du försöker nå Internet \
   		igen. Om du vill aktivera Anslut på begäran klickar du på alternativknappen. Om \
    		du vill att din Internetanslutning alltid ska vara aktiv skriver du <b>0</b> \
    		i fältet <i>Max väntetid</i>. Annars skriver du in det antal minuter \
    		som du vill ska passera innan din Internetanslutning avslutas.";
hsetup.phase20="Alternativet Behåll aktiv ";
hsetup.phase21="Detta alternativ håller dig ansluten \
    		till Internet under obestämd tid även om din anslutning är inaktiv. För att använda \
    		detta alternativ klickar du på alternativknappen bredvid <i>Behåll aktiv</i>. Standardinställningen \
    		för återuppringningsperiod är 30 sekunder (dvs. routern kontrollerar \
    		Internetanslutningen var 30:e sekund).";
hsetup.phase22="Observera: Vissa \
    		kabelleverantörer kräver en specifik MAC-adress för anslutning till \
    		Internet. Mer information finns under fliken <b>System</b>. Klicka därefter \
    		på knappen <b>Hjälp</b> och läs om funktionen för MAC-kloning.";
hsetup.phase23="LAN";
hsetup.phase24="IP-adress och nätmask";
hsetup.phase25="Detta är\
    		routerns IP-adress och nätmask som visas på det interna LAN. Standardvärdet \
    		är 192.168.1.1 för IP-adress och 255.255.255.0 för nät\
    		mask.";
hsetup.phase26="DHCP";
hsetup.phase27="Behåll \
		standardinställningen <b>Aktivera</b> för att aktivera routeralternativet DHCP-server. Om du \
		redan har en DHCP-server i nätverket eller inte vill ha en DHCP-server \
		väljer du <b>Inaktivera</b>.";
hsetup.phase28="Ange ett \
    		numeriskt värde till DHCP-servern för start vid utfärdandet av IP-adresser. \
    		Börja inte med 192.168.1.1 (IP-adressen till routern).";
hsetup.phase29="Maximalt antal DHCP-användare";
hsetup.phase30="Skriv in \
    		det maximala antalet datorer som du vill att DHCP-servern ska tilldela IP-adresser \
    		till. Absolut maximum är 253--möjligt om 192.168.1.2 är första IP- \
    		adress.";
hsetup.phase31="Klientens \
    		lånetid är den tid som en nätverksanvändare tillåts ansluta till \
    		till routern men dess aktuella dynamiska IP-adress. Skriv in tiden \
    		i minuter som användaren kan \"leased\" denna dynamiska IP-adress.";
hsetup.phase32="Statisk DNS 1-3 ";
hsetup.phase33="Domännamnsystemet \
    		(DNS) är hur Internet översätter namn på domäner eller webbplatser \
    		till Internet-adresser eller URL. Internetleverantören tillhandahåller minst en \
    		IP-adress för DNS-server. Om du vill använda en annan anger du den IP-adressen \
    		i ett av dessa fält. Du kan ange upp till tre IP-adresser för DNS-server \
    		här. Routern använder dessa för snabbare åtkomst till fungerande DNS-servrar.";
hsetup.phase34="WINS (Windows Internet Naming Service) hanterar varje dators interaktion med \
    		Internet. Om du använder en WINS-server skriver du in den serverns IP-adress är. \
    		I annat fall lämnar du detta tomt.";
hsetup.phase35="Kontrollera alla \
		värden och klicka på <b>Spara inställningar</b> för att spara dina inställningar. Klicka på <b>Avbryt \
		ändringar</b> för \
		att avbryta dina osparade ändringar. Du kan testa inställningarna genom att ansluta till \
		Internet. ";    		    		    		

//DDNS
var helpddns = new Object();
helpddns.phase1="Routern tillhandahåller en DDNS-funktion (Dynamic Domain Name System). DDNS låter dig tilldela ett fast \
		värd- och domännamn till en dynamisk Internet IP-adress. Detta är användbart när du \
		är värd för din egen webbplats, FTP-server eller annan server bakom routern. Innan du \
		använder denna funktion måste du anmäla dig för en DDNS-tjänst hos <i>www.dyndns.org</i> \
		som är en leverantör av DDNS-tjänster.";
helpddns.phase2="DDNS-tjänst";
helpddns.phase3="Om du vill inaktivera DDNS-tjänsten behåller du standardinställningen <b>Inaktivera</b>. För att aktivera DDNS-tjänsten följer du dessa instruktioner:";
helpddns.phase4="Anmäl dig för DDNS-tjänst på <i>www.dyndns.org</i> och skriv ned \
		informationen om ditt Användarnamn,<i> </i>Lösenord, och<i> </i>Värdnamn.";
helpddns.phase5="På skärmbilden <i>DDNS</i> väljer du <b>Aktivera.</b>";
helpddns.phase6="Fyll i fälten <i> Användarnamn</i>,<i> Lösenord</i> och<i> Värdnamn</i>.";
helpddns.phase7="Klicka på knappen <b>Spara inställningar</b> för att spara dina ändringar. Klicka på knappen <b>Avbryt ändringar</b> för att \
		avbryta osparade ändringar.";
helpddns.phase8="Routerns aktuella Internet IP-adress visas här.";
helpddns.phase9="Status för DDNS-tjänstanslutningen visas här.";
		
//MAC Address Clone
var helpmac =  new Object();
helpmac.phase1="MAC-kloning";
helpmac.phase2="Routerns MAC-adress är en kod på 12 siffror som tilldelas en unik \
    		maskinvara för identifiering. Vissa Internetleverantörer kräver att du registrerar MAC-\
    		adressen på ditt nätverkskort/adapter som var anslutet till ditt kabel- eller \
    		DSL-modem under installationen. Om din Internetleverantörer kräver MAC-\
    		adressregistrering hittar du adapterns MAC-adress genom att följa \
    		instruktionerna till din dators operativsystem.";
helpmac.phase3="För Windows 98 och Millennium:";
helpmac.phase4="1.  Klicka på knappen <b>Start</b> och välj <b>Kör</b>.";
helpmac.phase5="2.  Skriv <b>winipcfg </b>i fältet och tryck på <b>OK </b>.";
helpmac.phase6="3.  Välj den Ethernet-adapter som du använder.";
helpmac.phase7="4.  Klicka på <b>Mer Info</b>.";
helpmac.phase8="5.  Skriv ned din adapters MAC-adress.";
helpmac.phase9="1.  Klicka på knappen <b>Start</b> och välj <b>Kör</b>.";
helpmac.phase10="2.  Skriv <b>cmd </b>i fältet och tryck på <b>OK </b>.";
helpmac.phase11="3.  Vid kommandoprompten kör du <b>ipconfig /all</b> och letar efter din adapters fysiska adress.";
helpmac.phase12="4.  Skriv ned din adapters MAC-adress.";
helpmac.phase13="För att klona din nätverksadapters MAC-adress till routern och undvika att ringa till \
    		Internetleverantören för att ändra den registrerade MAC-adressen följer du dessa instruktioner:";
helpmac.phase14="För Windows 2000 och XP:";
helpmac.phase15="1.  Välj <b>Aktivera</b>.";
helpmac.phase16="2.  Skriv in din adapters MAC-adress i fältet <i>MAC-adress</i>.";
helpmac.phase17="3.  Klicka på knappen <b>Spara inställningar</b>.";
helpmac.phase18="Om du vill inaktivera kloning av MAC-adress behåller du standardinställningen <b>Inaktivera</b>.";

//Advance Routing
var hrouting = new Object();
hrouting.phase1="Routing";
hrouting.phase2="På skärmbilden <i>Routing</i> kan du ställa in routingläge och göra inställningar för routern. \
		 Gateway-läget rekommenderas för de flesta användare.";
hrouting.phase3="Välj rätt arbetsläge. Behåll standardinställningen <b> \
    		 Gateway</b>, om routern är värd för din nätverksanslutning till Internet (Gateway-läget rekommenderas för de flesta användare). Välj <b> \
    		 Router </b>om routern finns i ett nätverk med andra routers.";
hrouting.phase4="Dynamisk routing (RIP)";
hrouting.phase5="Observera: Den här funktionen är inte tillgänglig i gateway-läge.";
hrouting.phase6="Dynamisk routing gör att routern automatiskt kan justeras för fysiska förändringar i \
    		 nätverksutformningen och routingtabeller för utväxling med andra routrar. Routern \
    		 avgör nätverkspaketens väg baserat på minsta antalet \
    		 hopp mellan källan och destinationen. ";
hrouting.phase7="För att aktivera funktionen dynamisk routing för WAN-sidan väljer du <b>WAN</b>. \
    		 För att aktivera denna funktion för sidan för LAN och trådlöst väljer du <b>LAN &amp; Trådlös</b>. \
    		 För att aktivera funktionen för både WAN och LAN väljer du <b> \
    		 Båda</b>. För att inaktivera funktionen dynamisk routing för alla överföringar behåller du \
    		 standardinställningen <b>Inaktivera</b>. ";
hrouting.phase8="Statisk routing,&nbsp; destinationens IP-adress, nätmask, gateway och gränssnitt";
hrouting.phase9="För att upprätta en statisk väg mellan routern och ett annat nätverk \
    		 väljer du ett nummer från den nedrullningsbara menyn <i>Statisk routing</i>. (En statisk \
    		 väg är en förutbestämd sökväg som nätverksinformationen måste färdas för att \
    		 nå en specifik värd eller nätverk.)";
hrouting.phase10="Skriv in följande information:";
hrouting.phase11="Destinationens IP-adress </b>- \
		  Destinationens IP-adress är den nätverksadress eller värd till vilken du vill tilldela en statisk väg.";
hrouting.phase12="Nätmask </b>- \
		  Nätmasken styr vilken del av IP-adressen som är nätverksdelen och vilken \
    		  del som är en värddelen. ";
hrouting.phase13="Gateway </b>- \
		  Detta är IP-adressen för den gateway-enhet som möjliggör kontakt mellan routern och nätverket eller värden.";
hrouting.phase14="Beroende på var destinationens IP-adress finns placerad väljer du \
    		  <b>LAN &amp; trådlös </b>eller <b>WAN </b>från den nedrullningsbara menyn <i>Gränssnitt</i>.";
hrouting.phase15="För att spara dina ändringar klickar du på knappen <b>Verkställ</b>. Om du vill avbryta dina osparade ändringar klickar du <b> \
    		  på knappen Avbryt</b>.";
hrouting.phase16="För ytterligare statiska vägar upprepar du stegen 1-4.";
hrouting.phase17="Ta bort denna post";
hrouting.phase18="Borttagning av en post för statisk väg:";
hrouting.phase19="I den nedrullningsbara menyn <i>Statisk routing</i> väljer du postnumret för den statiska vägen.";
hrouting.phase20="Klicka på knappen <b>Ta bort denna post</b>.";
hrouting.phase21="För att spara en borttagning klickar du på knappen <b>Verkställ</b>. Om du vill avbryta en borttagning klickar du <b> \
    		  på knappen Avbryt</b>. ";
hrouting.phase22="Visa routingtabell";
hrouting.phase23="Klicka på knappen \
    		  <b>Visa routingtabell </b>för att se alla giltiga vägposter som används. Destinationens IP-adress, nätmask, \
    		  gateway och gränssnitt visas för varje post. Klicka på knappen <b>Uppdatera </b>för att uppdatera informationen som visas. Klicka på knappen <b> \
    		  Stäng</b>för att återgå till skärmbilden <i>Routing</i>.";
hrouting.phase24="Destinationens IP-adress </b>- \
		  Destinationens IP-adress är den nätverksadress eller värd till vilken den statisk vägen är tilldelad. ";
hrouting.phase25="Nätmask </b>- \
		  Nätmasken styr vilken del av IP-adressen som är nätverksdelen och vilken del som är värddelen.";
hrouting.phase26="Gateway</b> - Detta är IP-adressen för den gateway-enhet som möjliggör kontakt mellan routern och nätverket eller värden.";
hrouting.phase27="Gränssnitt</b> - Detta gränssnitt anger om destinationens IP-adress är på \
    		  <b> LAN &amp; trådlös </b>(interna trådbundna och trådlösa nätverk), <b>WAN</b> (Internet) eller <b> \
    		  Loopback</b> (ett modellnätverk där en dator fungerar som ett nätverk vilket fordras av vissa programvaror). ";

//Firewall
var hfirewall = new Object();
hfirewall.phase1="Blockera WAN-begäran";
hfirewall.phase2="Genom att aktivera funktionen Blockera WAN-begäran kan du förhindra att ditt nätverk \
    		 blir \"pingat\" eller upptäckt av andra Internetanvändare. Funktionen Blockera WAN-begäran \
    		 förstärker även din nätverkssäkerhet genom att dölja dina nätverksportar. \
    		 Båda funktionerna i blockera WAN-begäran gör det svårare för \
    		 utomstående användare att arbeta sig in i ditt nätverk. Denna funktion är aktiverad \
    		 som standard. Om du vill inaktivera denna funktion väljer du <b>Inaktivera</b>.";
hfirewall.right="Aktivera eller inaktivera SPI-brandvägg.";

//VPN
var helpvpn = new Object();
helpvpn.phase1="VPN-genomströmning";
helpvpn.phase2="VPN (Virtual Private Networking) används vanligtvis för arbetsrelaterad nätverksuppkoppling. För \
    		VPN-tunnlar stödjer routern IPSec-genomströmning och PPTP-genomströmning.";
helpvpn.phase3="<b>IPSec</b> - IPSec (Internet Protocol Security) är en<b> </b>uppsättning protokoll som används för implementering av \
      		säker utväxling av paket i IP-skiktet. För att tillåta IPSec-tunnlar att passera \
      		genom routern är IPSec-genomströmning aktiverad som standard. För att inaktivera \
      		IPSec-genomströmning avmarkerar du rutan bredvid <i>IPSec</i>.";
helpvpn.phase4="<b>PPTP </b>- Point-to-Point Tunneling Protocol är den metod som används för att möjliggöra VPN- \
      		sessioner till en Windows NT 4.0- eller 2000-server. För att tillåta PPTP-tunnlar att passera \
      		genom routern är PPTP-genomströmning aktiverad som standard. För att inaktivera  \
      		PPTP-genomströmning avmarkerar du rutan bredvid <i>PPTP</i>.";

helpvpn.right="Du kan välja att aktivera PPTP-, L2TP- eller IPSec-genomströmning för att låta dina \
		nätverksenheter kommunicera via VPN.";
//Internet Access
var hfilter = new Object();
hfilter.phase1="Filter";
hfilter.phase2="Skärmbilden <i>Filter</i> låter dig blockera eller tillåta specifika typer av Internet- \
		användning. Du kan upprätta Internetåtkomstpolicy för specifika datorer och upprätta \
		filter genom att använda nätverksportnummer.";
hfilter.phase3="Denna funktion ger dig möjligheten att anpassa upp till tio olika Internetåtkomstpolicy \
    		för speciella datorer som identifieras av deras IP- eller MAC-adresser. För \
    		varje policyberörd dator under de dagar och tidsperioder som anges.";
hfilter.phase4="Följ dessa instruktioner för att skapa och redigera en policy:";
hfilter.phase5="Välj ett policynummer \(1-10\) i den nedrullningsbara menyn.";
hfilter.phase6="Skriv in ett namn i fältet <i>Skriv in profilnamn</i>.";
hfilter.phase7="Klicka på knappen <b>Redigera datorlista</b>.";
hfilter.phase8="Klicka på knappen <b>Verkställ</b> för att spara ändringarna. Klicka på knappen <b>Avbryt</b> för \
    		att avbryta dina osparade ändringar. Klicka på knappen <b>Stäng</b> för att återgå till \
    		skärmen <i>Filter</i>.";
hfilter.phase9="Om du vill blockera datorerna i listan mot Internetåtkomst under de angivna dagarna och \
    		tidpunkterna behåller du standardinställningen <b>Inaktivera Internetåtkomst för \
    		datorerna i listan</b>. Om du vill att datorerna i listan ska få åtkomst till Internet under \
    		de angivna dagarna och tidpunkterna klickar du på alternativknappen bredvid <i> Aktivera \
    		Internetåtkomst för datorerna i listan</i>.";
hfilter.phase10="I skärmbilden <i>Datorlista</i> anger du datorerna med IP-adress eller MAC-adress. Skriv in \
    		respektive IP-adresser i fälten <i>IP</i>. Om du har ett intervall med \
    		IP-adresser som ska filtreras fyller du i respektive fält i <i>IP-intervall</i>. \
    		Skriv in respektive MAC-adresser i fälten <i>MAC</i>.";
hfilter.phase11="Ange tidpunkt för filtrering av åtkomst. Välj <b>24 timmar</b><b> </b>eller markera kryssrutan bredvid <i>Från</i> \
    		och använd de nedrullningsbara rutorna för att ange en specifik tidsperiod.";
hfilter.phase12="Ange dagar för filtrering av åtkomst. Välj <b>Alla dagar</b> eller respektive dagar i veckan.";
hfilter.phase13="Klicka på knappen <b>Lägg till policy</b> för att spara ändringarna och aktivera.";
hfilter.phase14="Upprepa steg 1-9 för att skapa eller redigera ytterligare policy.";
hfilter.phase15="Om du vill ta bort en policy för Internetåtkomst väljer du policynumret och klickar på knappen <b>Ta bort</b>.";
hfilter.phase16="Om du vill se en sammanfattning av alla policy klickar du på knappen <b>Sammanfattning</b>. Skärmbilden <i> \
    		Sammanfattning av Internetpolicy</i> visar varje policynummer, policynamn \
    		dagar och tidpunkt på dagen. Om du vill ta bort en policy klickar du i rutan \
    		och sedan på knappen <b>Ta bort</b>. Klicka på knappen <b>Stäng</b> för att återgå till \
    		skärmen <i>Filter</i>.";
hfilter.phase17="Filtrera portintervall för Internet";
hfilter.phase18="Om du vill filtrera datorer efter nätverksportnummer väljer du <b>Båda</b>, <b>TCP</b> eller <b>UDP</b>, \
    		beroende på vilket protokoll du vill filtrera.  Skriv sedan in<b> </b>de \
    		portnummer som du vill filtrera i fälten portnummer.  Datorer anslutna till \
    		routern kan inte längre nå något portnummer som listas här. För att \
    		inaktivera ett filter väljer du <b>Inaktivera</b>.";

//share of help string
var hshare = new Object();
hshare.phase1="Kontrollera alla värden och klicka på <b>Spara inställningar</b> för att spara dina inställningar. Klicka på knappen <b>Avbryt \
		ändringar</b> för att avbryta dina osparade ändringar.";


//DMZ
var helpdmz = new Object();
helpdmz.phase1="Funktionen DMZ-värd tillåter att en lokal användare exponeras mot Internet vid användning av \
		en specialtjänst som t.ex. Internetspel eller videokonferens. \
		DMZ-värden vidarebefordrar alla portar samtidigt till en dator. Funktionen \
    		Portvidarebefordran är säkrare eftersom den endast öppnar de portar som du vill \
    		öppna medan DMZ-värden öppnar alla portar på en dator och exponerar \
    		datorn så att Internet kan se den. ";
helpdmz.phase2="En dator vars port vidarebefordras måste ha funktionen DHCP-klient inaktiverad \
    		och måste ha en ny statisk IP-adress tilldelade eftersom IP-adressen \
    		kan ändras vid användning av DHCP-funktionen.";
/***To expose one PC, select enable.***/
helpdmz.phase3="För att exponera en dator  ";
helpdmz.phase4="anger du datorns IP-adress i fältet <i>IP-adress för DMZ-värd</i>.";



//help number string
var hnum = new Object();
hnum.one="1."
hnum.two="2."
hnum.three="3."
hnum.four="4."
hnum.five="5."
hnum.six="5."
hnum.seven="6."
hnum.eight="7."
hnum.night="8."
 
