var firewall2 = new Object();
firewall2.natredir="Filtra reindirizzamento NAT Internet";
firewall2.ident="Filtra IDENT(Porta 113)";
firewall2.multi="Filtra multicast";

var hupgrade = new Object();
hupgrade.right1="Fare clic sul pulsante Browse per selezionare il file del firmware da caricare sul router.";
hupgrade.right2="Fare clic sul pulsante Aggiorna per iniziare il processo di aggiornamento. L'aggiornamento non deve essere interrotto.";
hupgrade.wrimage="File immagine non valido.";

var hfacdef = new Object();
hfacdef.right1="verranno ripristinate tutte le impostazioni predefinite. Tutte le impostazioni dell'utente verranno annullate.";
hfacdef.warning="Avviso! Se si fa clic su OK, il dispositivo ripristina le impostazioni predefinite, annullando tutte le impostazioni precedenti.";

var hdiag = new Object();
hdiag.right1="immettere l'indirizzo IP o il nome del dominio da sottoporre al ping e fare clic sul pulsante Ping.";
hdiag.right2="immettere l'indirizzo IP o il nome del dominio da rintracciare e fare clic sul pulsante Traceroute.";

var hlog = new Object();
hlog.right1="È possibile attivare o disattivare i registri <b>In ingresso</b> e <b>In uscita</b> selezionando il pulsante di opzione appropriato.";

var manage2 = new Object();
manage2.webacc="Accesso Web";
manage2.accser="Accesso al &nbsp;server";
manage2.wlacc="Accesso Web &nbsp;wireless";
manage2.vapass="La password di conferma non corrisponde a quella già immessa. Immettere di nuovo la password";
manage2.passnot="La password di conferma non corrisponde.";
manage2.selweb="È necessario selezionare almeno un server Web.";
manage2.changpass="Per il router attualmente è impostata la password predefinita. Come misura protettiva, è necessario modificare la password per attivare la funzione Gestione remota. Fare clic sul pulsante OK per modificare la password. Fare clic sul pulsante Annulla per lasciare disattivata la funzione Gestione remota.";

var hmanage2 = new Object();
hmanage2.right1="<b>Accesso router locale: </b>da qui è possibile modificare la password del router. Immettere una nuova password quindi, per confermare, digitarla anche nel campo Ripeti digitazione per conferma.<br> ";
hmanage2.right2="<b>Accesso Web: </b>permette di configurare le opzioni di accesso all'utilità Web del router.";
hmanage2.right3="<b>Accesso router remoto: </b>consente di accedere al router in remoto. Scegliere la porta da utilizzare. È necessario modificare la password del router, se ancora utilizza quella predefinita.";
hmanage2.right4="<b>UPnP: </b>utilizzato da determinati programmi per aprire automaticamente le porte per le comunicazioni.";

var hstatwl2 = new Object();
hstatwl2.right1="<b>Indirizzo MAC</b>. L'indirizzo MAC del router visibile nella rete wireless locale.";
hstatwl2.right2="<b>Modalità</b>. In base alla selezione effettuata nella scheda Wireless, visualizza la modalità wireless (Mista, Solo G o Disattivato) utilizzata nella rete.";

var hstatlan2 = new Object();
hstatlan2.right1="<b>Indirizzo MAC</b>. L'indirizzo MAC del router visibile nella rete Ethernet locale.";
hstatlan2.right2="<b>Indirizzo IP</b>. L'indirizzo IP del router visibile nella rete Ethernet locale.";
hstatlan2.right3="<b>Maschera di sottorete</b>. Visualizza la maschera di sottorete eventualmente utilizzata dal router.";
hstatlan2.right4="<b>Server DHCP</b>. Visualizza il router come server DHCP, se lo si utilizza in questo modo.";

var hstatrouter2 = new Object();
hstatrouter2.wan_static="Static";
hstatrouter2.l2tp="L2TP";
//hstatrouter2.hb="Segnale heartbeat";
hstatrouter2.hb="Cavo Telstra";
hstatrouter2.connecting="Connessione in corso";
hstatrouter2.disconnected="Disconnesso";
hstatrouter2.disconnect="Disconnetti";
hstatrouter2.right1="<b>Versione firmware. </b>Il firmware corrente del router.";
hstatrouter2.right2="<b>Ora attuale. </b>Visualizza l'ora definita nella scheda Impostazione.";
hstatrouter2.right3="<b>Indirizzo MAC. </b>L'indirizzo MAC del router visualizzato dall'ISP.";
hstatrouter2.right4="<b>Nome router. </b>Il nome specifico del router definito nella scheda Impostazione.";
hstatrouter2.right5="<b>Tipo di configurazione. </b>Mostra le informazioni richieste dall'ISP per la connessione a Internet. Queste informazioni sono state immesse nella scheda Impostazione. Qui è possibile attivare e disattivare la connessione facendo clic sui pulsanti <b>Connetti</b> e <b>Disconnetti</b>.";
hstatrouter2.authfail=" autenticazione non riuscita";
hstatrouter2.noip="Impossibile ottenere un indirizzo IP da ";
hstatrouter2.negfail=" negoziazione non riuscita";
hstatrouter2.lcpfail=" Negoziazione LCP non riuscita";
hstatrouter2.tcpfail="Impossibile creare una connessione TCP con ";
hstatrouter2.noconn="Impossibile connettersi a ";
hstatrouter2.server=" server";

var hdmz2 = new Object();
hdmz2.right1="<b>DMZ: </b>attivando questa opzione, il router viene esposto a Internet. Tutte le porte saranno accessibili da Internet";

var hforward2 = new Object();
hforward2.right1="<b>Inoltro intervallo porte: </b>determinate applicazioni possono richiedere di aprire porte specifiche per funzionare correttamente. Ad esempio, ciò accade con i server e alcuni tipi di giochi online. Quando si riceve una richiesta da Internet per una determinata porta, il router instrada i dati al computer specificato. Per motivi di protezione, può essere opportuno limitare l'inoltro alle sole porte utilizzate attualmente e deselezionare la casella di controllo <b>Attiva</b> al termine.";

var hfilter2 = new Object();
hfilter2.delpolicy="Eliminare la politica?";
hfilter2.right1="<b>Politica di accesso a Internet: </b>è possibile definire fino a 10 politiche di accesso. Fare clic su <b>Elimina</b> per eliminare una politica o su <b>Riepilogo</b> per visualizzare il riepilogo della politica.";
hfilter2.right2="<b>Stato: </b>consente di attivare o disattivare una politica.";
hfilter2.right3="<b>Nome politica: </b>è possibile assegnare un nome alla politica.";
hfilter2.right4="<b>Tipo di politica: </b>scegliere fra Accesso a Internet e Traffico in ingresso.";
hfilter2.right5="<b>Giorni: </b>scegliere il giorno della settimana in cui applicare la politica.";
hfilter2.right6="<b>Ore: </b>immettere l'ora del giorno in cui applicare la politica.";
hfilter2.right7="<b>Servizi bloccati: </b>è possibile bloccare l'accesso a servizi specifici. Fare clic su <b>Aggiungi/Modifica</b> servizi per modificare queste impostazioni.";
hfilter2.right8="<b>Blocco siti Web per URL: </b>è possibile bloccare l'accesso a siti Web specifici immettendone l'URL.";
hfilter2.right9="<b>Blocco siti Web per parola chiave: </b>è possibile bloccare l'accesso a siti Web specifici in base alle parole chiave contenute nelle pagine Web.";

var hportser2 = new Object();
hportser2.submit="Applica";

var hwlad2 = new Object();
hwlad2.authtyp="Tipo di autenticazione";
hwlad2.basrate="Velocità base";
hwlad2.mbps="Mbps";
hwlad2.def="Predef.";
hwlad2.all="Tutto";
hwlad2.defdef="(Predef.: Predefinito)";
hwlad2.fburst="Burst frame";
hwlad2.milli="Millisecondi";
hwlad2.range="Intervallo";
hwlad2.frathrh="Soglia frammentazione";
hwlad2.apiso="Isolamento AP";
hwlad2.off="Dis.";
hwlad2.on="Att.";
hwlad2.right1="<b>Tipo di autenticazione: </b>è possibile scegliere fra Auto o Chiave condivisa. L'autenticazione tramite chiave condivisa è più sicura, ma deve essere supportata da tutte le periferiche della rete.";

var hwlbas2 = new Object();
hwlbas2.right1="<b>Modalità rete wireless: </b>SpeedBooster è attivo automaticamente in modalità <b>Mista</b> e <b>Solo G</b>. Per escludere i client Wireless-G, scegliere la modalità <b>Solo B</b>. Per disattivare l'accesso wireless, scegliere <b>Disattiva</b>.";

var hwlsec2 = new Object();
hwlsec2.wpapsk="Chiave precondivisa WPA";
hwlsec2.wparadius="RADIUS WPA";
hwlsec2.wpapersonal="WPA Personal";
hwlsec2.wpaenterprise="WPA Enterprise";
//new wpa2
hwlsec2.wpa2psk="Solo chiave precondivisa WPA2";
hwlsec2.wpa2radius="Solo RADIUS WPA2";
hwlsec2.wpa2pskmix="Combinazione chiave precondivisa WPA2";
hwlsec2.wpa2radiusmix="Combinazione RADIUS WPA2";
hwlsec2.wpa2personal="WPA2 Personal";
hwlsec2.wpa2enterprise="WPA2 Enterprise";
//new wpa2
hwlsec2.right1="<b>Modalità Protezione: </b>è possibile scegliere fra Disattiva, WEP, Chiave precondivisa WPA, RADIUS WPA o RADIUS. Tutte le periferiche della rete devono utilizzare la stessa modalità di protezione per comunicare tra loro.";

var hwmac2 = new Object();
hwmac2.right1="<b>Clone indirizzo MAC: </b>alcuni ISP richiedono di registrare l'indirizzo MAC. Se non si desidera ripetere la registrazione dell'indirizzo MAC, il router può clonare l'indirizzo MAC registrato presso l'ISP.";

var hddns2 = new Object();
hddns2.right1="<b>Servizio DDNS: </b>DDNS consente di accedere alla rete utilizzando nomi di dominio anziché indirizzi IP. Il servizio gestisce la modifica dell'indirizzo IP e aggiorna dinamicamente le informazioni del dominio. È necessario sottoscrivere il servizio tramite TZO.com o DynDNS.org.";
hddns2.right2="Click <b><a target=_blank href=http://Linksys.tzo.com>here</a></b> to SIGNUP with a <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;TZO FREE TRIAL ACCOUNT";
hddns2.right3="Click <b><a target=_blank href=https://controlpanel.tzo.com>here</a></b> to Manage your <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;TZO Account";
hddns2.right4="Click <b><a target=_blank href=https://www.tzo.com/cgi-bin/Orders.cgi?ref=linksys>here</a></b> to Purchase a TZO <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;DDNS Subscription";
hddns2.right5="TZO DDNS <b><a target=_blank href=http://Linksys.tzo.com>Support/Tutorials</a></b>";

var hrouting2 = new Object();
hrouting2.right1="<b>Modalità operativa: </b>se il router gestisce la connessione a Internet utilizzata, selezionare la modalità <b>Gateway</b>. Se nella rete esiste un altro router, selezionare la modalità <b>Router</b>.";
hrouting2.right2="<b>Seleziona numero impostato: </b>il numero di routing univoco; è possibile impostare fino a 20 percorsi.";
hrouting2.right3="<b>Nome routing: </b>immettere il nome da assegnare al percorso.";
hrouting2.right4="<b>IP LAN di destinazione: </b>l'host remoto al quale assegnare il percorso statico.";
hrouting2.right5="<b>Maschera di sottorete: </b>determina le porzioni dell'host e della rete.";

var hindex2 = new Object();
hindex2.telstra="Cavo Telstra";
hindex2.dns3="DNS statico 3";
hindex2.hbs="Server heartbeat";
hindex2.l2tps="Server L2TP";
hindex2.hdhcp="<b>Configurazione automatica - DHCP: </b>questa impostazione viene utilizzata più spesso dagli operatori via cavo.<br><br>";
hindex2.hpppoe1="<b>PPPoE: </b>questa impostazione viene utilizzata più spesso dai provider DSL.<br>";
hindex2.hpppoe2="<b>Nome utente: </b>immettere il nome utente fornito dall'ISP.<br>";
hindex2.hpppoe3="<b>Password: </b>immettere la password fornita dall'ISP.<br>";

//hindex2.hpppoe4="<b><a target=_blank href=help/HSetup.asp>Altro...</a></b><br><br><br><br><br>";

hindex2.hstatic1="<b>IP statico: </b>questa impostazione viene utilizzata più spesso dagli ISP per l'utenza business.<br>";
hindex2.hstatic2="<b>Indirizzo IP Internet: </b>immettere l'indirizzo IP fornito dall'ISP.<br>";
hindex2.hstatic3="<b>Maschera di sottorete: </b>immettere la maschera di sottorete<br>";

//hindex2.hstatic4="<b><a target=_blank href=help/HSetup.asp>Altro...</a></b><br><br><br><br><br><br><br>";

hindex2.hpptp1="<b>PPTP: </b>questa impostazione viene utilizzata più spesso dai provider DSL.<br>";
hindex2.hpptp2="<b>Indirizzo IP Internet: </b>immettere l'indirizzo IP fornito dall'ISP.<br>";
hindex2.hpptp3="<b>Maschera di sottorete: </b>immettere la maschera di sottorete<br>";
hindex2.hpptp4="<b>Gateway: </b>immettere l'indirizzo del gateway dell'ISP.<br>";

//hindex2.hpptp5="<b><a target=_blank href=help/HSetup.asp>Altro...</a></b><br><br><br><br><br><br><br><br>";

hindex2.hl2tp1="<b>L2TP: </b>questa impostazione viene utilizzata da alcuni ISP in Europa.<br>";
hindex2.hl2tp2="<b>Nome utente: </b>immettere il nome utente fornito dall'ISP.<br>";
hindex2.hl2tp3="<b>Password: </b>immettere la password fornita dall'ISP.<br>";

//hindex2.hl2tp4="<b><a target=_blank href=help/HSetup.asp>Altro...</a></b><br><br><br><br><br><br><br><br>";

hindex2.hhb1="<b>Cavo Telstra: </b>questa impostazione viene utilizzata più spesso dai provider DSL.<br>";
hindex2.hhb2="<b>Nome utente: </b>immettere il nome utente fornito dall'ISP.<br>";
hindex2.hhb3="<b>Password: </b>immettere la password fornita dall'ISP.<br>";

//hindex2.hhb4="<b><a target=_blank href=help/HSetup.asp>Altro...</a></b><br><br><br><br><br><br>";

hindex2.right1="<b>Nome host: </b>immettere il nome host fornito dall'ISP.<br>";
hindex2.right2="<b>Nome dominio: </b>immettere il nome di dominio fornito dall'ISP.<br>";
hindex2.right3="<b>Indirizzo IP locale: </b>indirizzo del router.<br>";
hindex2.right4="<b>Maschera di sottorete: </b>la maschera di sottorete del router.<br><br><br>";
hindex2.right5="<b>Server DHCP: </b>consente al router di gestire gli indirizzi IP.<br>";
hindex2.right6="<b>Indirizzo IP iniziale: </b>l'indirizzo dal quale iniziare.<br>";
hindex2.right7="<b>Impostazione ora: </b>Scegliere il proprio fuso orario. Il router supporta inoltre la regolazione automatica dell'ora legale. ";
hindex2.hdhcps1="<b>Numero max. utenti DHCP: </b>è possibile limitare il numero di indirizzi gestiti dal router.<br>";

var errmsg2 = new Object();
errmsg2.err0="Indirizzo IP del server heartbeat non valido.";
errmsg2.err1="Eliminare la voce?";
errmsg2.err2="È necessario immettere un SSID.";
errmsg2.err3="Immettere una chiave condivisa.";
errmsg2.err4=" includono cifre esadecimali non valide o superano 63 caratteri.";
errmsg2.err5="Chiave non valida, deve essere compresa fra 8 e 63 caratteri ASCII o 64 cifre esadecimali";
errmsg2.err6="Immettere una chiave nel campo corrispondente ";
errmsg2.err7="Lunghezza chiave non valida ";
errmsg2.err8="Immettere una passphrase.";
errmsg2.err9="Le voci immesse in totale superano il limite di 40.";
errmsg2.err10=" non consente spazi.";
errmsg2.err11="Completate tutte le operazioni, fare clic sul pulsante Applica per salvare le impostazioni.";
errmsg2.err12="È necessario immettere il nome del servizio.";
errmsg2.err13="Il nome del servizio esiste già.";
errmsg2.err14="Indirizzo IP LAN o della maschera di sottorete non valido.";

var trigger2 = new Object();
trigger2.ptrigger="Attivazione porte";
trigger2.qos="QoS";
trigger2.trirange="Intervallo attivato";
trigger2.forrange="Intervallo inoltrato";
trigger2.sport="Porta iniziale";
trigger2.eport="Porta finale";
trigger2.right1="Applicazione: immettere il nome dell'applicazione per il trigger. <b>Intervallo attivato</b> Per ciascuna applicazione, elenca l'intervallo dei numeri di porta attivati. Per i numeri di porta necessari, consultare la documentazione dell'applicazione per Internet.<b>Porta iniziale</b> Immettere il numero di porta iniziale dell'Intervallo attivato.<b>Porta finale</b> Immettere il numero di porta finale dell'Intervallo attivato. <b>Intervallo inoltrato</b> Per ciascuna applicazione, elenca l'intervallo dei numeri di porta inoltrati. Per i numeri di porta necessari, consultare la documentazione dell'applicazione per Internet. <b>Porta iniziale</b> Immettere il numero di porta iniziale dell'Intervallo inoltrato. <b>Porta finale</b> Immettere il numero di porta finale dell'Intervallo inoltrato.";

var bakres2 = new Object();
bakres2.conman="Gestione config";
bakres2.bakconf="Backup configurazione";
bakres2.resconf="Ripristino configurazione";
bakres2.file2res="Selezionare un file da ripristinare";
bakres2.bakbutton="Backup";
bakres2.resbutton="Ripristino";
bakres2.right1="È possibile sottoporre a backup la configurazione corrente; ciò è utile se occorre ripristinare le impostazioni predefinite del router. ";
bakres2.right2="Per eseguire il backup della configurazione corrente, fare clic sul pulsante Backup.";
bakres2.right3="Fare clic sul pulsante Browse per individuare un file di configurazione salvato attualmente nel PC.";
bakres2.right4="Fare clic su Ripristino per sovrascrivere tutte le configurazioni correnti con quelle del file di configurazione.";

var qos = new Object();
qos.uband="Larghezza di banda a monte";
qos.bandwidth="Larghezza di banda";
qos.dpriority="Priorità periferiche";
qos.priority="Priorità";
qos.dname="Nome periferica";
qos.low="Bassa";
qos.medium="Medio";
qos.high="Alta";
qos.highest="Massimo";
qos.eppriority="Priorità porte Ethernet";
qos.flowctrl="Controllo flusso";
qos.appriority="Priorità applicazioni";
qos.specport="Porta specifica";
//qos.appname="Nome applicazione";
qos.appname="Nome applicazione";
qos.alert1="Valore porta non compreso nell'intervallo [0 - 65535]";
qos.alert2="Il valore della porta iniziale è superiore a quello della porta finale";
qos.confirm1="Se si impostano più periferiche, porte Ethernet o applicazioni sulla priorità Alta, si può annullare l'effetto di QoS.\nProcedere?";
/*
qos.right1="WRT54G offre due tipi di funzioni QoS (qualità del servizio): basate sulle applicazioni o sulle porte. Scegliere quella adeguata per le proprie esigenze.";
qos.right2="<b>Qos basata sull'applicazione: </b>è possibile controllare la larghezza di banda relativamente all'applicazione che la sta utilizzando. Sono disponibili varie applicazioni preconfigurate. È inoltre possibile personalizzare fino a tre applicazioni specificando il numero di porta utilizzato.";
qos.right3="<b>QoS basata sulle porte: </b>è possibile controllare la larghezza di banda in base alla porta LAN fisica alla quale è collegata la periferica. Si può assegnare la priorità Alta o Bassa alle periferiche collegate alle porte LAN da 1 a 4.";
*/
//wireless qos
qos.optgame="Ottimizza applicazioni di gioco";
qos.wqos="QoS via cavo";
qos.wlqos="QoS wireless";
qos.edca_ap="Parametri AP EDCA";
qos.edca_sta="Parametri STA EDCA";
qos.wme="Supporto WMM";
qos.noack="Nessun riconoscimento";
qos.defdis="(Predefinito: Disattiva)";
qos.cwmin="CWmin";
qos.cwmax="CWmax";
qos.aifsn="AIFSN";
qos.txopb="TXOP(b)";
qos.txopag="TXOP(a/g)";
qos.admin="Admin";
qos.forced="Forzato";
qos.ac_bk="AC_BK";
qos.ac_be="AC_BE";
qos.ac_vi="AC_VI";
qos.ac_vo="AC_VO";


qos.right1="Sono disponibili due tipi di funzioni QoS (Quality of Service): QoS via cavo, che controlla le periferiche collegate al router tramite cavo Ethernet, e QoS wireless, per le periferiche connesse al router in modalità wireless. "
qos.right2="<b>Priorità periferiche:</b> è possibile specificare la priorità per tutto il traffico da una periferica nella rete, assegnando a tale periferica un nome, specificandone la priorità e immettendone l'indirizzo MAC."
qos.right3="<b>Priorità porte Ethernet:</b> è possibile controllare la velocità dei dati in base alla porta LAN fisica alla quale è collegata la periferica. Si può assegnare la priorità Alta o Bassa al traffico dati dalle periferiche collegate alle porte LAN da 1 a 4."
qos.right4="<b>Priorità applicazioni :</b> è possibile controllare la velocità di trasmissione dati per l'applicazione che sta consumando larghezza di banda. Selezionare <b>Ottimizza applicazioni di gioco</b> se si desidera che le porte comuni delle applicazioni di gioco assumano automaticamente una priorità superiore. È possibile personalizzare fino a otto applicazioni specificando il numero di porta utilizzato."
qos.right5="Wi-Fi Alliance<sup>TM</sup> definisce QoS wireless come <b>Wi-Fi MultiMedia<sup>TM</sup> (WMM)</b>. Selezionare Attiva per utilizzare WMM se si ricorre ad altre periferiche wireless certificate WMM."
qos.right6="<b>Nessun riconoscimento:</b> attivare questa opzione se si desidera disattivare il riconoscimento. Se l'opzione è attiva, il router non ripete l'invio dei dati in caso di errore."


var vpn2 = new Object();
vpn2.right1="Per consentire alle periferiche della rete di comunicare tramite VPN, si può attivare il pass-through PPTP, L2TP o IPSec.";

// for parental control

var pactrl = new Object();
pactrl.pactrl ="Controllo genitori";
pactrl.accsign ="Sottoscrizione account";
pactrl.center1 ="La soluzione Controllo genitori Linksys contribuisce a difendere i membri della famiglia mentre navigano su Internet";
pactrl.center2 ="<li>Facile da impostare</li><br><li>Per proteggere il computer di casa mediante il router Linksys</li><br><li>I rapporti consentono di monitorare l'utilizzo di e-mail, IM e Internet</li>";
pactrl.center3 ="** La sottoscrizione di questo servizio disattiva le politiche di accesso a Internet incorporate nel router";
pactrl.manageacc ="Gestisci account";
pactrl.center4 ="Gestione dell'account Controllo genitori";
pactrl.signparental ="Sottoscrivi il servizio Controllo genitori";
pactrl.moreinfo ="Ulteriori informazioni";
pactrl.isprovision ="periferica predisposta";
pactrl.notprovision ="periferica non predisposta";
pactrl.right1 ="La schermata Controllo genitori consente di sottoscrivere e gestire l'account Controllo genitori Linksys. Il servizio Controllo genitori Linksys offre strumenti avanzati per controllare la disponibilità di accesso, funzioni e servizi Internet ed è personalizzabile per ogni membro della famiglia.";

var satusroute = new Object();
satusroute.localtime ="Non disponibile";

var succ = new Object();
succ.autoreturn ="Dopo qualche secondo, verrà visualizzata la pagina precedente.";
