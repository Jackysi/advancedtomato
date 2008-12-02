//Basic Setup
var hsetup = new Object();
hsetup.phase1="La schermata <i>Impostazione</i> \
		è la prima visualizzata quando si accede al router. La maggior parte degli utenti  \
		può configurare il router e farlo funzionare correttamente utilizzando solo le \
		impostazioni disponibili in questa schermata. Alcuni provider di servizi Internet (ISP) richiedono \
		l'immissione di informazioni specifiche, quali Nome utente, Password, Indirizzo IP, \
		Gateway predefinito o Indirizzo IP DNS. Se necessario, l'ISP può fornire queste informazioni.";
hsetup.phase2="Nota: dopo \
		aver configurato queste impostazioni, è opportuno definire una nuova password per il router \
		nella schermata <i>Protezione</i>. In tal modo il router sarà più protetto \
		dalle modifiche non autorizzate. A tutti gli utenti che proveranno ad accedere \
		all'utilità basata sul Web o all'Impostazione guidata verrà richiesta la password del router.";
hsetup.phase3="Selezionare il \
		proprio fuso orario. Se il proprio paese aderisce all'ora legale, \
    		lasciare selezionata la casella accanto a <i>Regola l'orologio automaticamente per \
    		l'ora legale</i>.";
hsetup.phase4="MTU indica \
    		l'unità di trasmissione massima, ovvero le dimensioni più grandi consentite per i pacchetti \
    		nelle trasmissioni attraverso Internet. Lasciare invariata l'impostazione predefinita, <b>Auto</b>, affinché \
    		il router selezioni la MTU più appropriata per la connessione a Internet in uso. Per specificare le \
    		dimensioni MTU, selezionare <b>Manuale</b> e immettere il valore desiderato (l'impostazione predefinita è <b> \
    		1400</b>).&nbsp; Si consiglia di mantenere il valore nell'intervallo fra 1200 e 1500.";
hsetup.phase5="Questa voce è necessaria solo per alcuni ISP, che normalmente la forniscono agli utenti.";
hsetup.phase6="Il router supporta quattro tipi di connessioni:";
hsetup.phase7="Configurazione automatica - DHCP";
hsetup.phase8="(Point-to-Point Protocol over Ethernet)";
hsetup.phase9="(Point-to-Point Tunneling Protocol)";
hsetup.phase10="È possibile selezionare il tipo dal menu a discesa accanto a Connessione a Internet. \
    		Le informazioni richieste e le funzioni disponibili variano in \
    		base al tipo di connessione selezionato. Di seguito si riportano \
    		alcune descrizioni di tali informazioni:";		
hsetup.phase11="Indirizzo IP e maschera di sottorete per Internet";
hsetup.phase12="Queste informazioni corrispondono all'indirizzo IP \
    		e alla maschera di sottorete del router, visibili agli utenti esterni su Internet (compreso \
    		l'ISP). Se la connessione a Internet utilizzata richiede un indirizzo IP statico, \
    		l'ISP fornirà un indirizzo IP statico e una maschera di sottorete.";
hsetup.phase13="L'ISP fornirà l'indirizzo IP del gateway."
hsetup.phase14="(Server dei nomi di dominio)";
hsetup.phase15="L'ISP fornirà almeno un indirizzo IP DNS.";
hsetup.phase16="Nome utente e Password";
hsetup.phase17="Immettere il <b>Nome utente</b> e la \
		<b>Password</b> da utilizzare per accedere all'ISP tramite una connessione \
    		PPPoE o PPTP.";
hsetup.phase18="Connessione su richiesta";
hsetup.phase19="è possibile configurare il router in modo che \
    		interrompa la connessione a Internet dopo uno specifico intervallo di inattività \
    		(Tempo di inattività max.). Se la connessione a Internet è stata interrotta poiché \
    		era inattiva, Connessione su richiesta permette al router di ristabilirla automaticamente \
   		appena si tenta di accedere di nuovo a \
   		Internet. Se si desidera attivare Connessione su richiesta, fare clic sul pulsante di opzione. Se\
    		si preferisce che la connessione a Internet rimanga sempre attiva, immettere <b>0</b> \
    		nel campo <i>Tempo di inattività max</i>. In caso contrario, specificare il numero di minuti \
    		che dovranno trascorrere prima che la connessione a Internet si interrompa.";
hsetup.phase20="Opzione Keep Alive ";
hsetup.phase21="Questa opzione mantiene attiva la \
    		connessione a Internet indefinitamente, anche se inutilizzata. Per utilizzare \
    		l'opzione, fare clic sul pulsante accanto a <i>Keep Alive</i>. Il Periodo \
    		richiamata predefinito è 30 secondi (il router verifica la \
    		connessione a Internet ogni 30 secondi).";
hsetup.phase22="Nota: alcuni \
    		provider via cavo richiedono uno specifico indirizzo MAC per la connessione a \
    		Internet. Per ulteriori informazioni, fare clic sulla scheda <b>Sistema</b>. Quindi fare clic \
    		sul pulsante della <b>guida</b> e leggere l'argomento sulla clonazione MAC.";
hsetup.phase23="LAN";
hsetup.phase24="Indirizzo IP e Maschera di sottorete";
hsetup.phase25="Queste\
    		informazioni corrispondono all'indirizzo IP e alla maschera di sottorete visualizzati nella LAN interna. Il\
    		valore predefinito è 192.168.1.1 per l'indirizzo IP e 255.255.255.0 per la maschera \
    		di sottorete.";
hsetup.phase26="DHCP";
hsetup.phase27="Mantenere \
    		l'impostazione predefinita, <b>Attiva</b>, per attivare l'opzione del server DHCP per il router. Se la \
    		rete è già dotata di un server DHCP o non si desidera un server di questo tipo, \
    		selezionare <b>Disattiva</b>.";
hsetup.phase28="Immettere un \
    		valore numerico dal quale il server DHCP dovrà iniziare a generare indirizzi IP. \
    		Non cominciare da 192.168.1.1 (l'indirizzo IP del router).";
hsetup.phase29="Numero massimo di utenti DHCP";
hsetup.phase30="Immettere il \
    		numero massimo di PC ai quali il server DHCP potrà assegnare indirizzi \
    		IP. Il massimo in assoluto è 253--possibile se l'indirizzo IP iniziale è \
    		192.168.1.2.";
hsetup.phase31="La durata \
    		lease client indica per quanto tempo un utente della rete potrà rimanere connesso \
    		al router mediante l'indirizzo IP dinamico corrente. Immettere la durata \
    		in minuti del lease, ovvero il tempo consentito per l'uso dell'indirizzo IP dinamico da parte dell'utente. ";
hsetup.phase32="DNS statico 1-3 ";
hsetup.phase33="DNS \
    		(Domain Name System) è il metodo in base al quale Internet traduce i nomi dei domini e dei siti Web \
    		in indirizzi Internet, o URL. L'ISP fornisce almeno un \
    		indirizzo IP per il server DNS. Se si preferisce utilizzarne un altro, immettere tale indirizzo IP \
    		in uno dei campi disponibili. È possibile specificare fino a tre indirizzi IP per i server \
    		DNS. Il router si baserà su questi indirizzi per accedere ai server DNS \
    		attivi.";
hsetup.phase34="WINS (Windows Internet Naming Service) gestisce l'interazione di ciascun PC \
    		con Internet. Se si utilizza un server WINS, immetterne qui l'indirizzo IP. \
    		In caso contrario, lasciare il campo vuoto.";
hsetup.phase35="Verificare tutti i \
		valori e fare clic su <b>Salva impostazioni</b> per confermare. Fare clic su <b>Annulla\
		modifiche</b> per \
		eliminare le modifiche non salvate. È possibile verificare le impostazioni connettendosi a \
		Internet. ";    		    		    		

//DDNS
var helpddns = new Object();
helpddns.phase1="Il router offre la funzione DDNS (Dynamic Domain Name System), che consente di assegnare un \
		nome host e un nome di dominio fissi a un indirizzo IP dinamico per Internet. Ciò è utile se \
		si gestisce in hosting il proprio sito Web, il server FTP o altri server dietro al router. Prima \
		di utilizzare questa funzione occorre sottoscrivere il servizio DDNS presso <i>www.dyndns.org</i>, \
		un provider di servizi DDNS.";
helpddns.phase2="Servizio DDNS";
helpddns.phase3="Per disattivare il servizio DDNS, mantenere l'impostazione predefinita <b>Disattiva</b>. Per attivare il servizio \
		DDNS, procedere come segue:";
helpddns.phase4="Sottoscrivere il servizio DDNS presso <i>www.dyndns.org</i> e prendere nota \
		di Nome utente,<i> </i>Password e<i> </i>Nome host.";
helpddns.phase5="Nella schermata <i>DDNS</i>, selezionare <b>Attiva</b>.";
helpddns.phase6="Compilare i campi <i>Nome utente</i>,<i> Password</i> e<i> Nome host</i>.";
helpddns.phase7="Fare clic sul pulsante <b>Salva impostazioni</b> per confermare. Fare clic sul pulsante <b>Annulla modifiche</b> per \
		eliminare le modifiche non salvate.";
helpddns.phase8="Qui viene visualizzato l'attuale indirizzo IP per Internet del router.";
helpddns.phase9="Qui viene visualizzato lo stato della connessione al servizio DDNS.";
		
//MAC Address Clone
var helpmac =  new Object();
helpmac.phase1="Clonazione MAC";
helpmac.phase2="L'indirizzo MAC del router è un codice univoco a 12 cifre che identifica un \
    		componente hardware. Alcuni ISP richiedono di registrare l'indirizzo \
    		MAC della scheda di rete, collegata al modem via cavo o \
    		DSL durante l'installazione. Se l'ISP richiede di registrare l'indirizzo \
    		MAC, è possibile individuarlo seguendo le istruzioni \
    		relative al sistema operativo del proprio PC.";
helpmac.phase3="Per Windows 98 e Millennium:";
helpmac.phase4="1.  Fare clic sul pulsante <b>Start</b> e selezionare <b>Esegui</b>.";
helpmac.phase5="2.  Digitare <b>winipcfg </b>nel campo disponibile e premere <b>OK</b>.";
helpmac.phase6="3.  Selezionare la scheda Ethernet in uso.";
helpmac.phase7="4.  Fare clic su <b>Ulteriori informazioni</b>.";
helpmac.phase8="5.  Prendere nota dell'indirizzo MAC della scheda.";
helpmac.phase9="1.  Fare clic sul pulsante <b>Start</b> e selezionare <b>Esegui</b>.";
helpmac.phase10="2.  Digitare <b>cmd </b>nel campo disponibile e premere <b>OK</b>.";
helpmac.phase11="3.  Al prompt dei comandi, eseguire <b>ipconfig /all</b> e individuare l'indirizzo fisico della scheda.";
helpmac.phase12="4.  Prendere nota dell'indirizzo MAC della scheda.";
helpmac.phase13="Per clonare sul router l'indirizzo MAC della scheda di rete in uso ed evitare di \
    		contattare l'ISP per modificare l'indirizzo MAC registrato, procedere come segue:";
helpmac.phase14="Per Windows 2000 e XP:";
helpmac.phase15="1.  Selezionare <b>Attiva</b>.";
helpmac.phase16="2.  Immettere l'indirizzo MAC della scheda nel campo <i>Indirizzo MAC</i>.";
helpmac.phase17="3.  Fare clic sul pulsante <b>Salva impostazioni</b>.";
helpmac.phase18="Per disattivare la clonazione dell'indirizzo MAC, mantenere l'impostazione predefinita <b>Disattiva</b>.";

//Advance Routing
var hrouting = new Object();
hrouting.phase1="Routing";
hrouting.phase2="La schermata <i>Routing</i> consente di specificare la modalità di routing e le impostazioni del router. \
		 La modalità Gateway è ottimale per la maggior parte degli utenti.";
hrouting.phase3="Scegliere la modalità operativa appropriata. Mantenere l'impostazione predefinita, <b> \
    		 Gateway</b>, se il router gestisce la connessione della rete a Internet (per la maggior parte degli utenti si consiglia la modalità Gateway). Selezionare <b> \
    		 Router </b>se nella rete esistono vari router.";
hrouting.phase4="Routing dinamico (RIP)";
hrouting.phase5="Nota: questa funzione non è disponibile in modalità Gateway.";
hrouting.phase6="Grazie al routing dinamico, il router può adattarsi automaticamente alle modifiche fisiche \
    		 nel layout della rete e scambiare tabelle di routing con altri router. Il\
    		 router stabilisce il percorso dei pacchetti nella rete in base al numero più piccolo di \
    		 hop fra origine e destinazione. ";
hrouting.phase7="Per attivare la funzione Routing dinamico sul lato WAN, selezionare <b>WAN</b>. \
    		 Per attivare questa funzione sui lati LAN e wireless, selezionare <b>LAN e wireless</b>. \
    		 Per attivare la funzione sia per la LAN che per la WAN, selezionare <b> \
    		 Entrambi</b>. Per disattivare la funzione Routing dinamico per tutte le trasmissioni di dati, mantenere \
    		 l'impostazione predefinita <b>Disattiva</b>. ";
hrouting.phase8="Routing statico,&nbsp; Indirizzo IP di destinazione, Maschera di sottorete, Gateway e Interfaccia";
hrouting.phase9="Per impostare un percorso statico fra il router e un'altra rete, \
    		 selezionare un numero dall'elenco a discesa <i>Routing statico</i> (si definisce \
    		 statico un percorso predeterminato che le informazioni della rete devono \
    		 attraversare per raggiungere un host o una rete specifici).";
hrouting.phase10="Immettere i dati seguenti:";
hrouting.phase11="Indirizzo IP di destinazione </b>- \
		  l'indirizzo della rete o dell'host al quale si desidera assegnare un percorso statico.";
hrouting.phase12="Maschera di sottorete </b>- \
		  determina quale parte di un indirizzo IP appartiene alla rete e quale \
    		  all'host. ";
hrouting.phase13="Gateway </b>- \
		  l'indirizzo IP del dispositivo gateway che consente i contatti fra il router e la rete o l'host.";
hrouting.phase14="In base all'ubicazione dell'indirizzo IP di destinazione, selezionare \
    		  <b>LAN e wireless </b>o <b>WAN </b>dal menu a discesa <i>Interfaccia</i>.";
hrouting.phase15="Per salvare le modifiche, fare clic sul pulsante <b>Applica</b>. Per annullare le modifiche non salvate, fare clic sul pulsante <b> \
    		  Annulla</b>.";
hrouting.phase16="Per impostare altri percorsi statici, ripetere i punti 1-4.";
hrouting.phase17="Elimina voce";
hrouting.phase18="Per eliminare una voce relativa a un percorso statico:";
hrouting.phase19="Dall'elenco a discesa <i>Routing statico</i>, selezionare il numero della voce del percorso statico.";
hrouting.phase20="Fare clic sul pulsante <b>Elimina voce</b>.";
hrouting.phase21="Per confermare un'eliminazione, fare clic sul pulsante <b>Applica</b>. Per annullare un'eliminazione, fare clic sul pulsante <b> \
    		  Annulla</b>. ";
hrouting.phase22="Mostra tabella di routing";
hrouting.phase23="Fare clic sul pulsante \
    		  <b>Mostra tabella di routing </b>per visualizzare tutte le voci valide dei percorsi in uso. Per ciascuna voce vengono visualizzate \
    		  le informazioni relative a Indirizzo IP di destinazione, Maschera di sottorete, Gateway e Interfaccia. Fare clic su <b>Aggiorna </b>per aggiornare i dati visualizzati. Fare clic sul pulsante <b>\
    		  Chiudi</b> per tornare alla schermata <i>Routing</i>.";
hrouting.phase24="Indirizzo IP di destinazione </b>- \
		  l'indirizzo della rete o dell'host al quale viene assegnato il percorso statico. ";
hrouting.phase25="Maschera di sottorete </b>- \
		  determina quale parte di un indirizzo IP appartiene alla rete e quale all'host.";
hrouting.phase26="Gateway</b> - l'indirizzo IP del dispositivo gateway che consente i contatti fra il router e la rete o l'host.";
hrouting.phase27="Interfaccia</b> - indica se l'indirizzo IP di destinazione è di tipo \
    		  <b> LAN e wireless </b>(reti interne cablate e wireless), <b>WAN</b> (Internet) o <b> \
    		  Loopback</b> (rete fittizia, nella quale un PC funge da rete; necessaria per alcuni software). ";

//Firewall
var hfirewall = new Object();
hfirewall.phase1="Blocca richieste WAN";
hfirewall.phase2="Attivando la funzione Blocca richieste WAN è possibile impedire che la rete \
    		 venga sottoposta a ping o rilevata da altri utenti di Internet. La funzione Blocca richieste WAN \
    		 inoltre aumenta la protezione della rete nascondendone le porte. \
    		 In entrambi i casi, la funzione Blocca richieste WAN rende più difficile per gli utenti \
    		 esterni penetrare nella rete. Questa funzione è attiva \
    		 per impostazione predefinita. Selezionare <b>Disattiva</b> per disattivarla.";
hfirewall.right="Attiva o disattiva il firewall SPI.";

//VPN
var helpvpn = new Object();
helpvpn.phase1="Pass-through VPN";
helpvpn.phase2="VPN (Virtual Private Networking) è comunemente utilizzato per il networking associato al lavoro. Per \
    		i tunnel VPN, il router supporta i tipi di pass-through IPSec e PPTP.";
helpvpn.phase3="<b>IPSec</b> - Internet Protocol Security, una<b> </b>suite di protocolli utilizzati per implementare \
      		lo scambio protetto di pacchetti al livello IP. Per consentire ai tunnel IPSec di \
      		attraversare il router, il pass-through IPSec è attivo per impostazione predefinita. Per disattivare \
      		il pass-through IPSec, deselezionare la casella accanto a <i>IPSec</i>.";
helpvpn.phase4="<b>PPTP </b>- Point-to-Point Tunneling Protocol, il metodo utilizzato per attivare le \
      		sessioni VPN sui server Windows NT 4.0 o 2000. Per consentire ai tunnel PPTP di \
      		attraversare il router, il pass-through PPTP è attivo per impostazione predefinita. Per disattivare \
      		il pass-through PPTP, deselezionare la casella accanto a <i>PPTP</i>.";

helpvpn.right="Per consentire alle periferiche della rete di comunicare tramite VPN, si può attivare \
		il pass-through PPTP, L2TP o IPSec.";
//Internet Access
var hfilter = new Object();
hfilter.phase1="Filtri";
hfilter.phase2="La schermata <i>Filtri</i> consente di bloccare o meno tipi specifici di utilizzo \
		di Internet. È possibile impostare politiche di accesso a Internet per determinati PC e definire \
		filtri basandosi sui numeri delle porte della rete.";
hfilter.phase3="Con questa funzione si possono personalizzare fino a dieci diverse politiche di accesso a Internet \
    		per PC specifici, identificati da indirizzi IP o MAC. Per \
    		ciascun PC inoltre è possibile specificare giorni e intervalli di tempo particolari per l'accesso a Internet. ";
hfilter.phase4="Per creare o modificare una politica, procedere come segue:";
hfilter.phase5="Selezionare il numero della politica \(1-10\) nel menu a discesa.";
hfilter.phase6="Immettere un nome nel campo <i>Immetti nome profilo</i>.";
hfilter.phase7="Fare clic sul pulsante <b>Modifica elenco PC</b>.";
hfilter.phase8="Fare clic sul pulsante <b>Applica</b> per confermare. Fare clic sul pulsante <b>Annulla</b> per \
    		eliminare le modifiche non salvate. Fare clic sul pulsante <b>Chiudi</b> per tornare \
    		alla schermata <i>Filtri</i>.";
hfilter.phase9="Se si desidera impedire ai PC elencati di accedere a Internet durante i giorni e gli orari indicati \
    		mantenere l'impostazione predefinita <b>Disattiva accesso a Internet per i PC \
    		elencati</b>. Se si desidera che i PC elencati possano accedere a Internet durante \
    		i giorni e gli orari indicati, fare clic sul pulsante di opzione accanto a <i>Attiva \
    		accesso a Internet per i PC elencati</i>.";
hfilter.phase10="Nella schermata <i>Elenco PC</i>, specificare i PC in base all'indirizzo IP o MAC. Immettere gli \
    		indirizzi IP appropriati nei campi <i>IP</i>. Se occorre applicare il filtro a \
    		una serie di indirizzi IP, compilare i campi <i>Intervallo IP</i> appropriati. \
    		Immettere gli indirizzi MAC appropriati nei campi <i>MAC</i>.";
hfilter.phase11="Impostare l'orario in cui l'accesso verrà filtrato. Selezionare <b>24 ore</b><b> </b>o la casella accanto a <i>Da</i> \
    		e utilizzare le caselle a discesa per impostare un intervallo di tempo specifico. ";
hfilter.phase12="Impostare i giorni in cui l'accesso verrà filtrato. Selezionare <b>Ogni giorno</b> o i giorni della settimana appropriati. ";
hfilter.phase13="Fare clic sul pulsante <b>Aggiungi alla politica</b> per salvare le modifiche e attivare la politica.";
hfilter.phase14="Per creare o modificare altre politiche, ripetere i punti 1-9.";
hfilter.phase15="Per eliminare una politica di accesso a Internet, selezionarne il numero e fare clic sul pulsante <b>Elimina</b>.";
hfilter.phase16="Per visualizzare il riepilogo di tutte le politiche, fare clic sul pulsante <b>Riepilogo</b>. La schermata <i> \
    		Riepilogo politica Internet</i> visualizza il numero di ciascuna politica e le \
    		informazioni Nome politica, Giorni e Ora. Per eliminare una politica, fare clic sulla relativa casella, quindi \
    		sul pulsante <b>Elimina</b>. Fare clic sul pulsante <b>Chiudi</b> per tornare \
    		alla schermata <i>Filtri</i>.";
hfilter.phase17="Intervallo di porte Internet sottoposte al filtro";
hfilter.phase18="Per filtrare i PC in base al numero di porta di rete, selezionare <b>Entrambi</b>, <b>TCP</b> o <b>UDP</b>, \
    		in base ai protocolli da sottoporre al filtro. Quindi<b> </b>immettere nei campi \
    		appropriati i numeri di porta da sottoporre al filtro.  I PC connessi al \
    		router non potranno più accedere ai numeri di porta elencati. Per \
    		disattivare un filtro, selezionare <b>Disattiva</b>.";

//share of help string
var hshare = new Object();
hshare.phase1="Fare clic su tutti i valori e su <b>Salva impostazioni</b> per confermare. Fare clic sul pulsante <b>Annulla\
		modifiche</b> per annullare le modifiche non salvate.";


//DMZ
var helpdmz = new Object();
helpdmz.phase1="La funzione di hosting DMZ permette a un utente locale di esporsi a Internet per utilizzare \
		un servizio specifico, ad esempio i giochi e le videoconferenze online. \
		L'hosting DMZ inoltra simultaneamente tutte le porte a un unico PC. La funzione \
    		Inoltro porte è più sicura, poiché apre solo le porte desiderate \
    		mentre l'hosting DMZ apre tutte le porte disponibili, \
    		esponendo il computer a Internet. ";
helpdmz.phase2="Se si inoltra una porta di un PC, la funzione client DHCP del computer deve essere disattivata; \
    		inoltre è necessario assegnare al PC un nuovo indirizzo IP statico, poiché \
    		quando si utilizza DHCP l'indirizzo può cambiare.";
/* **To expose one PC, select enable.** */
helpdmz.phase3="Per esporre a Internet un PC, selezionare ";
helpdmz.phase4="Immettere l'indirizzo IP del computer nel campo <i>Indirizzo IP host DMZ</i>.";



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
 
