var firewall2 = new Object();
firewall2.natredir="Filtrer la redirection NAT Internet";
firewall2.ident="IDENT filtre (port 113)";
firewall2.multi="Filtrer multidiffusion";

var hupgrade = new Object();
//hupgrade.right1="Click on the browse button to select the firmware file to be uploaded to the router.";
hupgrade.right1="Cliquez sur le bouton Parcourir pour sélectionner le fichier du micrologiciel à charger sur le routeur.";
//hupgrade.right2="Click the Upgrade button to begin the upgrade process.  Upgrade must not be interrupted.";
hupgrade.right2="Cliquez sur le bouton Mettre à niveau pour lancer la procédure de mise à niveau. La mise à niveau ne doit pas être interrompue.";
hupgrade.wrimage="Fichier image incorrect";

var hfacdef = new Object();
//hfacdef.right1="This will reset all settings back to factory defaults.  All of your settings will be erased.";
hfacdef.right1="<b>Restaurer les paramètres d'usine&nbsp;:  </b>Permet de restaurer tous les paramètres d'usine. Tous vos paramètres seront effacés.";
hfacdef.warning="Avertissement ! Si vous cliquez sur OK, les paramètres d\'usine du périphérique seront réinitialisés et tous les paramètres précédents seront supprimés.";

var hdiag = new Object();
//hdiag.right1="Enter the IP address or domain name you would like to ping and click the Ping button.";
hdiag.right1="Entrez l'adresse IP ou le nom de domaine pour lequel vous voulez exécuter le ping et cliquez sur le bouton Ping.";
//hdiag.right2="Enter the IP address or domain name you would like to trace, then click the Traceroute button";
hdiag.right2="Entrez l'adresse IP ou le nom de domaine dont vous voulez effectuer le suivi et cliquez sur le bouton Déterminer l'itinéraire.";

var hlog = new Object();
//hlog.right1="You may enable or disable the use of <b>Incoming</b> and <b>Outgoing</b> logs by selecting the appropriate radio button.";
hlog.right1="Vous pouvez activer ou désactiver l'utilisation de fichiers journaux  <b>entrants</b> et  <b>sortants</b> en sélectionnant le bouton d'option approprié.";

var manage2 = new Object();
manage2.webacc="Accès au Web";
manage2.accser="Accès au serveur";
manage2.wlacc="Accès au Web &nbsp;sans fil";
manage2.vapass="Les mots de passe ne correspondent pas. Entrez-les de nouveau.";
manage2.passnot="Le mot de passe de confirmation ne correspond pas.";
manage2.selweb="Vous devez au moins sélectionner un serveur Web.";
manage2.changpass="Le mot de passe par défaut est déjà défini pour le routeur. Par mesure de sécurité, vous devez modifier le mot de passe pour que la fonction Gestion distante puisse être activée. Cliquez sur le bouton OK pour changer votre mot de passe. Cliquez sur le bouton Annuler pour maintenir la fonction Gestion distante désactivée.";

var hmanage2 = new Object();
//hmanage2.right1="<b>Local Router Access: </b>You can change the Router's password from here. Enter a new Router password and then type it again in the Re-enter to confirm field to confirm.<br>";
hmanage2.right1="<b>Accès local au routeur&nbsp;:  </b>Vous pouvez modifier le mot de passe du routeur dans ce champ. Entrez un nouveau mot de passe de routeur et retapez-le dans le champ de confirmation.";
//hmanage2.right2="<b>Web Access: </b>Allows you to configure access options to the router's web utility.";
hmanage2.right2="";
//hmanage2.right3="<b>Accès distant au routeur&nbsp;:  </b>Permet d'accéder au routeur à distance. Choisissez le port à utiliser. Vous devez modifier le mot de passe d'accès au routeur s'il utilise toujours son mot de passe par défaut.";
hmanage2.right3="<b>Accès distant au routeur&nbsp;:  </b>Permet d'accéder au routeur à distance. Choisissez le port à utiliser. Vous devez modifier le mot de passe d'accès au routeur s'il utilise toujours son mot de passe par défaut.";
//hmanage2.right4="<b>UPnP: </b>Used by certain programs to automatically open ports for communication.";
hmanage2.right4="<b>UPnP&nbsp;:  </b>Utilisé par certains programmes pour l'ouverture automatique de ports pour la communication.";

var hstatwl2 = new Object();
//hstatwl2.right1="<b>MAC Address</b>. This is the Router's MAC Address, as seen on your local, wireless network.";
hstatwl2.right1="<b>Adresse MAC</b>. Il s'agit de l'adresse&nbsp;MAC du routeur telle qu'elle apparaît sur votre réseau sans fil.";
//hstatwl2.right2="<b>Mode</b>. As selected from the Wireless tab, this will display the wireless mode (Mixed, G-Only, or Disabled) used by the network.";
hstatwl2.right2="<b>Mode</b>. Cette option sélectionnée dans l'onglet Sans fil affiche le mode sans fil (Mixte, G&nbsp;uniquement ou Désactiver) utilisé sur le réseau.";

var hstatlan2 = new Object();
//hstatlan2.right1="<b>MAC Address</b>. This is the Router's MAC Address, as seen on your local, Ethernet network.";
hstatlan2.right1="<b>Adresse MAC</b>. Il s'agit de l'adresse&nbsp;MAC du routeur telle qu'elle apparaît sur votre réseau local Ethernet.";
//hstatlan2.right2="<b>IP Address</b>. This shows the Router's IP Address, as it appears on your local, Ethernet network.";
hstatlan2.right2="<b>Adresse IP</b>. Il s'agit de l'adresse&nbsp;IP du routeur telle qu'elle apparaît sur votre réseau local Ethernet.";
//hstatlan2.right3="<b>Subnet Mask</b>. When the Router is using a Subnet Mask, it is shown here.";
hstatlan2.right3="<b>Masque de sous-réseau</b>. Lorsque le routeur utilise un masque de sous-réseau, cette information s'affiche sur cette ligne.";
//hstatlan2.right4="<b>DHCP Server</b>. If you are using the Router as a DHCP server, that will be displayed here.";
hstatlan2.right4="<b>Serveur DHCP</b>. Si vous utilisez le routeur en qualité de serveur&nbsp;DHCP, cette information s'affiche sur cette ligne.";

var hstatrouter2 = new Object();
hstatrouter2.wan_static="statique";
hstatrouter2.l2tp="L2TP";
hstatrouter2.hb="Telstra Cable";
hstatrouter2.connecting="Connexion en cours";
hstatrouter2.disconnected="Déconnecté";
hstatrouter2.disconnect="Déconnecter";
//hstatrouter2.right1="<b>Firmware Version. </b>This is the Router's current firmware.";
hstatrouter2.right1="<b>Version du micrologiciel.  </b>Il s'agit du micrologiciel du routeur.";
//hstatrouter2.right2="<b>Current Time. </b>This shows the time, as you set on the Setup Tab.";
hstatrouter2.right2="<b>Heure actuelle.  </b>Affiche l'heure, définie dans l'onglet Configuration.";
//hstatrouter2.right3="<b>MAC Address. </b>This is the Router's MAC Address, as seen by your ISP.";
hstatrouter2.right3="<b>Adresse&nbsp;MAC.  </b>Adresse MAC du routeur, accessible par votre FAI.";
//hstatrouter2.right4="<b>Router Name. </b>This is the specific name for the Router, which you set on the Setup Tab.";
hstatrouter2.right4="<b>Nom du routeur.  </b>Il s'agit ici du nom spécifique du routeur que vous avez défini dans l'onglet Configuration.";
//hstatrouter2.right5="<b>Configuration Type. </b>This shows the information required by your ISP for connection to the Internet. This information was entered on the Setup Tab. You can <b>Connect</b> or <b>Disconnect</b> your connection here by clicking on that button.";
hstatrouter2.right5="<b>Type de configuration.  </b>Ce champ affiche les informations requises par votre FAI pour la connexion à Internet. Ces informations ont été saisies dans l'onglet Configuration. Vous pouvez <b>Connecter</b> ou <b>Déconnecter</b> votre connexion en cliquant sur ce bouton.";
hstatrouter2.authfail=" échec de l'authentification";
hstatrouter2.noip="Impossible d'obtenir une adresse IP ";
hstatrouter2.negfail=" échec de la négociation";
hstatrouter2.lcpfail=" échec de la négociation LCP";
hstatrouter2.tcpfail="Création d'une connexion TCP impossible ";
hstatrouter2.noconn="Connexion impossible ";
hstatrouter2.server=" Serveur";

var hdmz2 = new Object();
//hdmz2.right1="<b>DMZ: </b>Enabling this option will expose your router to the Internet.  All ports will be accessible from the Internet";
hdmz2.right1="<b>DMZ&nbsp;:  </b>Lorsque cette option est activée, le routeur est exposé à Internet. Tous les ports sont accessibles depuis Internet.";

var hforward2 = new Object();
//hforward2.right1="<b>Port Range Forwarding: </b>Certain applications may require to open specific ports in order for it to function correctly.  Examples of these applications include servers and certain online games.  When a request for a certain port comes in from the Internet, the router will route the data to the computer you specify.  Due to security concerns, you may want to limit port forwarding to only those ports you are using, and uncheck the <b>Enable</b> checkbox after you are finished.";
hforward2.right1="<b>Transfert de connexion&nbsp;:  </b>Certaines applications peuvent requérir l'ouverture de certains ports pour que le transfert de connexion puisse fonctionner correctement. Ces applications incluent les serveurs et certains jeux en ligne. Lorsque la demande d'un certain port provient d'Internet, le routeur achemine les données vers l'ordinateur indiqué. En raison de problémes de sécurité, vous pouvez limiter le transfert de connexion aux ports que vous utilisez et désactiver l'option <b>Activer</b> lorsque vous avez terminé.";

var hfilter2 = new Object();
//hfilter2.delpolicy="Delete the Policy?";
hfilter2.delpolicy="Supprimer la stratégie ?";
//hfilter2.right1="<b>Richtlinien für Internetzugriff: </b> Sie können bis zu 10 Zugriffsrichtlinien definieren. Zum Löschen einer Richtlinie klicken Sie auf <b>Löschen</b>, oder klicken Sie auf <b>Zusammenfassung</b>, um eine Zusammenfassung der Richtlinie anzuzeigen.";
hfilter2.right1="<b>Stratégie d'accès à Internet&nbsp;:  </b>Vous pouvez définir jusqu'à 10&nbsp;stratégies d'accès. Cliquez sur  <b>Supprimer</b> pour supprimer une stratégie ou  <b>Récapitulatif</b> pour afficher un récapitulatif de la stratégie.";
//hfilter2.right2="<b>Status: </b>Enable or disable a policy.";
hfilter2.right2="<b>Etat&nbsp;:  </b>Activer ou désactiver une stratégie.";
//hfilter2.right3="<b>Policy Name: </b>You may assign a name to your policy.";
hfilter2.right3="<b>Nom de la stratégie&nbsp;:  </b>Vous pouvez attribuer un nom à votre statégie.";
//hfilter2.right4="<b>Policy Type: </b>Choose from Internet Access or Inbound Traffic.";
hfilter2.right4="<b>Type de stratégie&nbsp;:  </b>Choisissez Accès à Internet ou Trafic entrant.";
//hfilter2.right5="<b>Days: </b>Choose the day of the week you would like your policy to be applied.";
hfilter2.right5="<b>Jours&nbsp;:  </b>Choisissez le jour d'application de votre stratégie.";
//hfilter2.right6="Times: </b>Enter the time of the day you would like your policy to apply.";
hfilter2.right6="<b>Heures&nbsp;:  </b>Entrez l'heure d'application de votre stratégie.";
//hfilter2.right7="<b>Blocked Services: </b>You may choose to block access to certain services.  Click <b>Add/Edit</b> Services to modify these settings.";
hfilter2.right7="<b>Services bloqués&nbsp;:  </b>Vous pouvez choisir de bloquer l'accès à certains services. Cliquez sur  <b>Ajouter/Modifier un service</b> pour modifier ces paramètres.";
//hfilter2.right8="<b>Website Blocking by URL: </b>You can block access to certain websites by entering their URL.";
hfilter2.right8="<b>Blocage du site Web par URL&nbsp;:  </b>Vous pouvez bloquer l'accès à certains sites Web en entrant leur URL.";
//hfilter2.right9="<b>Website Blocking by Keyword: </b>You can block access to certain website by the keywords contained in their webpage.";
hfilter2.right9="<b>Blocage du site Web par mot-clé&nbsp;:  </b>Vous pouvez bloquer l'accès à certains sites Web en utilisant les mots-clés contenus dans leur page Web.";


var hportser2 = new Object();
hportser2.submit="Appliquer";

var hwlad2 = new Object();
hwlad2.authtyp="Type d\'authentification";
hwlad2.basrate="Taux de base";
hwlad2.mbps="Mbit/s";
hwlad2.def="Par défaut";
hwlad2.all="Tout";
hwlad2.defdef="(Par défaut&nbsp;: Par défaut)";
hwlad2.fburst="Rafale de trames";
hwlad2.milli="millisecondes";
hwlad2.range="Plage";
hwlad2.frathrh="Seuil de fragmentation";
hwlad2.apiso="Isolation AP";
hwlad2.off="Désactivée";
hwlad2.on="Activée";
//hwlad2.right1="<b>Authentication Type: </b>You may choose from Auto or Shared Key.  Shared key authentication is more secure, but all devices on your network must also support Shared Key authentication.";
hwlad2.right1="<b>Type d'authentification : </b>Vous avez le choix entre Auto et Clé partagée. L'authentification Clé  partagée est plus sûre mais tous les périphériques du réseau doivent également prendre en charge ce type d'authentification.";

var hwlbas2 = new Object();
//hwlbas2.right1="<b>Wireless Network Mode: </b><% support_match("SPEED_BOOSTER_SUPPORT", "1", "SpeedBooster is enabled automatically on <b>Mixed</b> Mode and <b>G-Only</b> mode."); %> If you wish to exclude Wireless-G clients, choose <b>B-Only</b> Mode.  If you would like to disable wireless access, choose <b>Disable</b>.";
hwlbas2.right1="<b>Mode réseau sans fil  : </b><% support_match(\"SPEED_BOOSTER_SUPPORT\", \"1\", \"SpeedBooster est activé automatiquement en mode <b>Mixte</b> et en mode<b>  G uniquement</b>. \"); %> Si vous voulez exclure les clients sans fil G, sélectionnez le mode<b> B  uniquement</b>. Si vous souhaitez désactiver l'accès sans fil, sélectionnez<b> Désactiver</b>.";

var hwlsec2 = new Object();
hwlsec2.wpapsk="Clé pré-partagée WPA";
hwlsec2.wparadius="WPA RADIUS";
hwlsec2.wpapersonal="WPA Personal";
hwlsec2.wpaenterprise="WPA Enterprise";
//new wpa2
hwlsec2.wpa2psk="Clé pré-partagée WPA2 seulement";
hwlsec2.wpa2radius="WPA2 RADIUS seulement";
hwlsec2.wpa2pskmix="Clé pré-partagée WPA2 mixte";
hwlsec2.wpa2radiusmix="WPA2 RADIUS mixte";
hwlsec2.wpa2personal="WPA2 Personal";
hwlsec2.wpa2enterprise="WPA2 Enterprise";
//new wpa2
//hwlsec2.right1="<b>Security Mode: </b>You may choose from Disable, WEP, WPA Pre-Shared Key, WPA RADIUS, or RADIUS.  All devices on your network must use the same security mode in order to communicate.";
hwlsec2.right1="<b>Mode de sécurité  : </b>Vous avez le choix entre Désactiver, WEP, Clé pré-partagée WPA, WPA RADIUS et RADIUS. Tous les périphériques du réseau doivent utiliser le même mode de sécurité pour pouvoir communiquer.";

var hwmac2 = new Object();
//hwmac2.right1="<b>MAC Address Clone: </b>Some ISP will require you to register your MAC address.  If you do not wish to re-register your MAC address, you can have the router clone the MAC address that is registered with your ISP.";
hwmac2.right1="<b>Adresse MAC dupliquée : </b>Certains FAI vous demandent d'enregistrer votre adresse MAC. Si vous ne souhaitez pas enregistrer de nouveau votre adresse MAC, le routeur peut dupliquer l'adresse MAC enregistrée par votre FAI.";

var hddns2 = new Object();
//hddns2.right1="<b>DDNS Service: </b>DDNS allows you to access your network using domain names instead of IP addresses.  The service manages changing IP address and updates your domain information dynamically.  You must sign up for service through TZO.com or DynDNS.org.";
hddns2.right1="<b>Service DDNS : </b>DDNS permet d'accéder au réseau à l'aide de noms de domaine au lieu d'adresses IP. Le service gère la modification de l'adresse IP et met à jour de manière dynamique les informations relatives à votre domaine. Vous devez vous connecter au service via TZO.com ou DynDNS.org.";
hddns2.right2="Click <b><a target=_blank href=http://Linksys.tzo.com>here</a></b> to SIGNUP with a <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;TZO FREE TRIAL ACCOUNT";
hddns2.right3="Click <b><a target=_blank href=https://controlpanel.tzo.com>here</a></b> to Manage your <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;TZO Account";
hddns2.right4="Click <b><a target=_blank href=https://www.tzo.com/cgi-bin/Orders.cgi?ref=linksys>here</a></b> to Purchase a TZO <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;DDNS Subscription";
hddns2.right5="TZO DDNS <b><a target=_blank href=http://Linksys.tzo.com>Support/Tutorials</a></b>";

var hrouting2 = new Object();
//hrouting2.right1="<b>Operating Mode: </b>If the router is hosting your Internet connection, select <b>Gateway</b> mode.  If another router exists on your network, select <b>Router</b> mode.";
hrouting2.right1="<b>Mode opérationnel : </b>Si le routeur héberge votre connexion Internet, sélectionnez le mode <b>Passerelle</b>. Si votre réseau contient un autre routeur, sélectionnez le mode <b>Routeur</b>.";
//hrouting2.right2="<b>Select Set Number: </b>This is the unique route number, you may set up to 20 routes.";
hrouting2.right2="<b>Sélectionner un numéro d'itinéraire : </b>Il s'agit d'un numéro d'itinéraire unique. Vous pouvez définir 20 itinéraires au maximum.";
//hrouting2.right3="<b>Route Name: </b>Enter the name you would like to assign to this route.";
hrouting2.right3="<b>Nom de l'itinéraire : </b>Entrez le nom de cet itinéraire.";
//hrouting2.right4="<b>Destination LAN IP: </b>This is the remote host to which you would like to assign the static route.";
hrouting2.right4="<b>IP LAN de destination : </b>Hôte distant auquel vous voulez assigner l'itinéraire statique.";
//hrouting2.right5="<b>Subnet Mask: </b>Determines the host and the network portion.";
hrouting2.right5="<b>Masque de sous-réseau : </b>Détermine l'hôte et la portion du réseau.";

var hindex2 = new Object();
hindex2.telstra="Telstra Cable";
hindex2.dns3="DNS statique 3";
hindex2.hbs="Serveur &nbsp;HeartBeat";
hindex2.l2tps="Serveur L2TP";
hindex2.hdhcp="<b>Configuration automatique - DHCP : </b>Ce paramètre est très communément utilisé par les opérateurs du cable<br><br>";
hindex2.hpppoe1="<b>PPPoE : </b>Ce paramètre est très communément utilisé par les fournisseurs de comptes DSL.<br>";
hindex2.hpppoe2="<b>Nom d'utilisateur : </b>Entrez le nom d'utilisateur fourni par votre FAI.<br>";
hindex2.hpppoe3="<b>Mot de passe : </b>Entrez le mot de passe fourni par votre FAI.<br>";

//hindex2.hpppoe4="<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br><br><br>";

hindex2.hstatic1="<b>IP statique : </b>Ce paramètre est très communément utilisé par les fournisseurs de comptes professionnels.<br>";
hindex2.hstatic2="<b>Adresse IP Internet : </b>Entrez l'adresse IP fournie par votre FAI.<br>";
hindex2.hstatic3="<b>Masque de sous-réseau : </b>Entrez le masque de sous réseau.<br>";

//hindex2.hstatic4="<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br><br><br><br><br>";

hindex2.hpptp1="<b>PPTP : </b>Ce paramètre est très communément utilisé par les fournisseur de comptes DSL.<br>";
hindex2.hpptp2="<b>Adresse IP Internet : </b>Entrez l'adresse IP fournie par votre FAI.<br>";
hindex2.hpptp3="<b>Masque de sous-réseau : </b>Entrez le masque de sous réseau.<br>";
hindex2.hpptp4="<b>Passerelle : </b>Entrez l'adresse de la passerelle de votre FAI.<br>";

//hindex2.hpptp5="<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br><br><br><br><br><br>";

hindex2.hl2tp1="<b>L2TP : </b>Ce paramètre est utilisé par certains FAI en Europe.<br>";
hindex2.hl2tp2="<b>Nom d'utilisateur : </b>Entrez le nom d'utilisateur fourni par votre FAI.<br>";
hindex2.hl2tp3="<b>Mot de passe : </b>Entrez le mot de passe fourni par votre FAI.<br>";

//hindex2.hl2tp4="<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br><br><br><br><br><br>";

//hindex2.hhb1="<b>Telstra Cable: </b>This setting is most commonly used by DSL providers.<br>";
hindex2.hhb1="<b>Câble Telstra: </b>Ce paramètre est très communément utilisé par les fournisseurs de comptes DSL.<br>";
hindex2.hhb2="<b>Nom d'utilisateur : </b>Entrez le nom d'utilisateur fourni par votre FAI.<br>";
hindex2.hhb3="<b>Mot de passe : </b>Entrez le mot de passe fourni par votre FAI.<br>";

//hindex2.hhb4="<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br><br><br><br>";

hindex2.right1="<b>Nom de l'hôte : </b> Entrez le nom de l'hôte fourni par votre FAI. <br>";
hindex2.right2="<b>Nom du domaine : </b>Entrez le nom de domaine fourni par votre FAI. <br>";
hindex2.right3="<b>Adresse IP locale : </b>Adresse du routeur. <br>";
hindex2.right4="<b>Masque de sous-réseau : </b>Masque de sous-réseau du routeur. <br><br><br>";
hindex2.right5="<b>Serveur DHCP : </b>Permet au routeur de gérer vos adresses IP. <br>";
hindex2.right6="<b>Adresse IP de départ : </b>Adresse que vous allez utiliser en premier. <br>";
//hindex2.right7="<b>Time Setting: </b>You may set the time manually.  Or choose Automatically if you wish to use a NTP server to keep the most accurate time.  Choose the time zone you are in.  The router can also adjust automatically for daylight savings time.";
hindex2.right7="<b>Réglage de l'heure : </b>Sélectionnez le fuseau horaire de votre pays d'origine. Le routeur peut également régler automatiquement le passage à l'heure d'été.";
//hindex2.hdhcps1="<b>Maximum number of DHCP Users: </b>You may limit the number of addresses your router hands out.<br>";
hindex2.hdhcps1="<b>Nombre maximum d'utilisateurs DHCP  : </b>Vous pouvez limiter le nombre d'adresses gérées par le routeur.<br>";

var errmsg2 = new Object();
//errmsg2.err0="The HeartBeat Server IP Address is invalid!";
errmsg2.err0="The HeartBeat Server IP Address is invalid!";
errmsg2.err1="Supprimer l'entrée";
errmsg2.err2="Vous devez entrer un SSID.";
errmsg2.err3="Entrez une clé partagée.";
errmsg2.err4=" Utilise des chiffres hexadécimaux non autorisés ou comporte plus de 63 caractères.";
errmsg2.err5="La valeur de la clé est non valide. Elle doit comprendre entre 8 et 63 caractères ou 64 chiffres hexadécimaux.";
errmsg2.err6="Entrez une valeur pour la clé. ";
errmsg2.err7="Longueur incorrecte de la clé ";
errmsg2.err8="Entrez une phrase mot de passe.";
errmsg2.err9="Le nombre de vérifications est supérieur à 40.";
errmsg2.err10=" L\'adresse MAC ne doit pas contenir d\'espaces";
errmsg2.err11="Après avoir effectué toutes les opérations, cliquez sur le bouton Appliquer pour enregistrer les paramètres.";
errmsg2.err12="Vous devez entrer un nom de service.";
errmsg2.err13="Le nom de service existe.";
errmsg2.err14="Adresse IP LAN ou masque de sous-réseau non valide.";

var trigger2 = new Object();
trigger2.ptrigger="Déclenchement de connexion";
trigger2.qos="QoS";
trigger2.trirange="Connexion déclenchée";
trigger2.forrange="Connexion transférée";
trigger2.sport="Port de début";
trigger2.eport="Port de fin";
//trigger2.right1="Application Enter the application name of the trigger. <b>Triggered Range</b> For each application, list the triggered port number range.  Check with the Internet application documentation for the port number(s) needed.<b>Start Port</b> Enter the starting port number of the Triggered Range.<b>End Port</b> Enter the ending port number of the Triggered Range.  <b>Forwarded Range</b> For each application, list the forwarded port number range.  Check with the Internet application documentation for the port number(s) needed.  <b>Start Port</b> Enter the starting port number of the Forwarded Range.  <b>End Port</b> Enter the ending port number of the Forwarded Range.";
trigger2.right1="<b>Application :</b> Entrez le nom d'application du déclencheur.<b>Connexion déclenchée :</b> Pour chaque application, répertoriez la plage des numéros de ports déclenchés. Vérifiez le(s) numéro(s) de port nécesssaire(s) dans la documentation de l'application Internet. <b>Port de début :</b> Entrez le numéro du port de début de la connexion déclenchée. <b>Port de fin :</b> Entrez le numéro du port de fin de la connexion déclenchée.<b>Connexion transférée :</b> Pour chaque application, répertoriez la plage des numéros de port transférés. Vérifiez le(s) numéro(s) de port nécesssaire(s) dans la documentation de l'application Internet. <b>Port de début :</b> Entrez le numéro du port de début de la connexion transférée. <b>Port de fin :</b> Entrez le numéro du port de fin de la connexion transférée.";

var bakres2 = new Object();
bakres2.conman="Gestion de configuration";
bakres2.bakconf="Configuration de sauvegarde";
bakres2.resconf="Restaurer la configuration";
bakres2.file2res="Veuillez sélectionner un fichier à restaurer";
bakres2.bakbutton="Sauvegarder";
bakres2.resbutton="Restaurer";
//bakres2.right1="You may backup your current configuration in case you need to reset the router back to its factory default settings.";
bakres2.right1="Vous pouvez sauvegarder votre configuration actuelle pour pouvoir réinitialiser votre routeur, en cas de besoin, aux paramètres d'usine par défaut.";
//bakres2.right2="You may click the Back up button to backup your current configuration.";
bakres2.right2="Pour sauvegarder votre configuration actuelle, cliquez sur le bouton Sauvegarder.";
//bakres2.right3="Click the Browse button to browse for a configuration file that is currently saved on your PC.";
bakres2.right3="Cliquez sur le bouton Parcourir pour rechercher un fichier de configuration enregistré sur votre ordinateur.";
//bakres2.right4="Click Restore to overwrite all current configurations with the ones in the configuration file.";
bakres2.right4="Cliquez sur le bouton Restaurer pour remplacer toutes les configurations actuelles par celles du fichier de configuration.";

var qos = new Object();
qos.uband="Upstream Bandwidth";
qos.bandwidth="Bandwidth";
qos.dpriority="Priorité du périphérique";
qos.priority="Priorité";
qos.dname="Nom du périphérique";
qos.low="Basse";
qos.medium="Moyen";
qos.high="Elevée";
qos.highest="Maximum";
//qos.eppriority="Ethernet Port Priority";
qos.eppriority="Ethernet Port Priority";
qos.flowctrl="Contrôle de flux";
qos.appriority="Priorité de l\'application";
qos.specport="Numéro de port spécifique (N)";
//qos.appname="Application Name";
qos.appname="Nom de l'application";
qos.alert1="La valeur du port est hors-limite [0 - 65535]";
qos.alert2="La valeur du port de début est plus élevée que la valeur du port de fin";
//qos.confirm1="Setting multiple devices, ethernet port or application to high priority may negate the effect of QoS.\nAre you sure you want to proceed?";
qos.confirm1="Setting multiple devices, ethernet port or application to high priority may negate the effect of QoS.\nAre you sure you want to proceed?";
/*
//qos.right1="The WRT54G offers two types of Quality of Service features, Application-based and Port-based.  Choose the appropriate offering for your needs.";
qos.right1="Le routeur WRT54G offre deux types de qualité de service : les fonctions basées sur l'application et les fonctions basées sur le port. Choisissez les fonctions qui correspondent le plus à vos besoins.";
//qos.right2="<b>Application-based Qos: </b>You may control your bandwidth with respect to the application that is consuming bandwidth.  There are several pre-configured applications.  You may also customize up to three applications by entering the port number they use.";
qos.right2="<b>QoS basé sur l'application : </b>Vous pouvez contrôler votre bande passante en fonction de l'application qui l'utilise. Il existe plusieurs applications configurées. Vous pouvez également personnaliser jusqu'à trois applications en entrant le numéro de port qu'elles utilisent.";
//qos.right3="<b>Port-based QoS: </b>You may control your bandwidth according to which physical LAN port your device is plugged into.  You may assign High or Low priority to devices connected on LAN ports 1 through 4.";
qos.right3="<b>QoS basé sur le port : </b>Vous pouvez contrôler votre bande passante en fonction du port du réseau local physique auquel est connecté votre périphérique. Vous pouvez attribuer une priorité élevée ou basse aux périphériques connectés aux ports 1 à 4 du réseau local.";
*/
//wireless qos
qos.optgame="Optimiser les applications de jeux";
qos.wqos="QS câblé";
qos.wlqos="QS sans fil";
qos.edca_ap="Paramètres EDCA AP";
qos.edca_sta="Paramètres EDCA STA";
qos.wme="Support de WMM";
qos.noack="Aucun accusé de réception";
qos.defdis="(Défaut : Désactiver)";
qos.cwmin="CWmin";
qos.cwmax="CWmax";
qos.aifsn="AIFSN";
qos.txopb="TXOP(b)";
qos.txopag="TXOP(a/g)";
qos.admin="Admin";
qos.forced="Forcé";
qos.ac_bk="AC_BK";
qos.ac_be="AC_BE";
qos.ac_vi="AC_VI";
qos.ac_vo="AC_VO";


qos.right1="Deux types QS (qualité de service) sont disponibles : QS câblé, pour controler les périphériques branchés au routeur avec un câble Ethernet, et QoS sans fil, pour controler les périphériques sans fil connectés au routeur."
qos.right2="<b>Priorité du périphérique :</b> Vous pouvez spécifier la priorité pour le trafic d’un périphérique sur votre réseau en assignant un nom au périphérique et en spécifiant sa priorité et son adresse MAC."
qos.right3="<b>Priorité du port Ethernet :</b> Vous pouvez contrôler le taux de transfert selon le port LAN physique dans lequel votre périphérique est branché. Vous pouvez assigner une priorité Haute ou Basse au trafic de données des périphériques connectés au ports LAN 1 à 4."
qos.right4="<b>Priorité d'application :</b> Vous pouvez contrôler le taux de transfert de l'application utilisant la bande passante. Cochez <b>Optimiser les applications de jeux</b> pour assigner automatiquement une plus haute priorité aux ports des applications de jeux standards.  Vous pouvez personnaliser jusqu'à huit applications en entrant le numéro du port qu'elles utilisent."
qos.right5="Wi-Fi Alliance<sup>TM</sup> définit le QS sans fil comme <b>Wi-Fi MultiMedia<sup>TM</sup> (WMM)</b>.  Sélectionnez Activer pour utiliser WMM si vous avez d’autres périphériques sans fil qui sont aussi certifiés WMM."
qos.right6="<b>Aucun accusé de réception :</b> Activez cette option si vous voulez désactiver les accusés de réception.  Si cette option est activée, le routeur ne renvoie pas les données en cas d’erreur."


var vpn2 = new Object();
vpn2.right1="Vous pouvez choisir d'activer l'intercommunication PPTP, L2TP ou IPSec afin d'autoriser les périphériques de votre réseau à communiquer via VPN.";

// for parental control

var pactrl = new Object();
pactrl.pactrl ="Contrôle parental";
pactrl.accsign ="Se connecter à un compte";
pactrl.center1 ="La solution Contrôle parental de Linksys vous permet de protéger les membres de votre famille lorsqu\'ils surfent sur internet.";
pactrl.center2 ="<li>Facile à installer</li><br><li>Protégez les ordinateurs de votre domicile grâce au routeur Linksys.</li><br><li>Des rapports permettent de contrôler l\'utilisation du Web, de la messagerie et d\'IM.</li>";
pactrl.center3 ="** La connexion à ce service désactive les stratégies d\'accès à Internet intégrées au routeur.";
pactrl.manageacc ="Gérer le compte";
pactrl.center4 ="Gestion de votre compte Contrôle parental";
pactrl.signparental ="Inscription au service de contrôle";
pactrl.moreinfo ="Plus d\'infos";
pactrl.isprovision ="Le périphérique est approvisionné";
pactrl.notprovision ="Le périphérique n'est pas approvisionné";
pactrl.right1 ="L'écran Contrôle parental vous permet de vous connecter à votre compte Contrôle parental de Linksys et de le gérer. Le service de contrôle parental de Linksys fournit des outils puissants pour contrôler la disponibilité des services, de l'accès et des fonctions d'Internet, pouvant être personnalisés pour chaque membre de votre famille.";

var satusroute = new Object();
satusroute.localtime ="Non disponible";

var succ = new Object();
succ.autoreturn ="La page précédente sera réaffichée après quelques secondes.";
