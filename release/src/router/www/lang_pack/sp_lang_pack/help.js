//Basic Setup
var hsetup = new Object();
hsetup.phase1="La pantalla <i>Configuración</i> \
		es la primera que aparece al acceder al enrutador. La mayoría de los usuarios  \
		podrán configurar el enrutador y hacer que funcione correctamente con tan solo \
		los parámetros de esta pantalla. Algunos proveedores de servicios de Internet (ISPs) necesitarán  \
		que introduzca información específica, como el usuario, la contraseña, la dirección IP,  \
		la dirección de puerta de enlace predeterminada o la dirección IP de DNS. Podrá obtener esta información \
		de su ISP, si es necesario.";
hsetup.phase2="Nota: después \
		de configurar estos parámetros, debería establecer una nueva contraseña para el enrutador  \
		mediante la pantalla <i>Seguridad</i>. De este modo habrá mayor seguridad ya que el \
		enrutador estará protegido de modificaciones no autorizadas. Todos los usuarios que intenten acceder a \
		la utilidad basada en Web o al asistente para instalación del enrutador deberán introducir la contraseña del mismo.";
hsetup.phase3="Seleccione la \
		zona horaria correspondiente a su residencia. Si en su zona se aplica el horario de verano, \
    		active la casilla de verificación situada junto a <i>Ajustar reloj automáticamente \
    		al horario de verano</i>.";
hsetup.phase4="MTU es la  \
    		unidad máxima de transmisión y especifica el tamaño de paquete más grande que se permite \
    		en la transmisión por Internet. Mantenga el valor predeterminado, <b>Automático</b>, o permita \
    		que el enrutador seleccione la mejor MTU para su conexión a Internet. Para especificar un \
    		tamaño de MTU, seleccione <b>Manual</b> e introduzca el valor deseado (el valor predeterminado es <b> \
    		1400</b>).&nbsp; Debería dejar este valor en un intervalo de 1200 a 1500.";
hsetup.phase5="Algunos ISP necesitan este valor y se lo pueden proporcionar.";
hsetup.phase6="El enrutador admite cuatro tipos de conexión:";
hsetup.phase7="Configuración automática DHCP";
hsetup.phase8="PPPoE (Protocolo punto a punto por Ethernet)";
hsetup.phase9="PPTP (Protocolo de túnel punto a punto)";
hsetup.phase10="Estos tipos se pueden seleccionar del menú desplegable situado junto a COnexión a Internet. \
    		La información necesaria y las funciones disponibles variarán en función \
    		del tipo de conexión seleccionado. A continuación se \
    		incluyen algunas descripciones de esta información:";		
hsetup.phase11="Dirección IP de Internet y máscara de subred";
hsetup.phase12="Es la dirección IP y la máscara de subred del enrutador \
    		tal como la observan los usuarios externos de Internet (incluso su \
    		ISP). Si su conexión a Internet requiere una dirección IP fija, su ISP \
    		le proporcinará entonces una dirección IP y una máscara de subred fijas.";
hsetup.phase13="Su ISP le proporcionará la dirección IP de la puerta de enlace."
hsetup.phase14="(Servidor de nombres de dominio)";
hsetup.phase15="Su ISP le proporcionará al menos una dirección IP de DNS.";
hsetup.phase16="Usuario y contraseña";
hsetup.phase17="Introduzca el <b>usuario</b> y la \
		<b>contraseña</b> que utiliza para conectarse a su ISP a través de una \
    		 conexión PPPoE o PPTP.";
hsetup.phase18="Conectar a petición";
hsetup.phase19="Puede configurar el enrutador para que \
    		desconecte la conexión a Internet después de un período de inactividad especificado \
    		(Tiempo máximo de inactividad). Si la conexión a Internet se ha cerrado porque ha estado \
    		inactiva, Conectar a petición permite al enrutador volver a establecer la \
   		conexión automáticamente en el momento en el que intenta volver a acceder a Internet.\
   		Si desea activar Conectar a petición, active el botón de opción. Si \
    		desea que la conexión a Internet siga activa todo el tiempo, introduzca <b>0</b> \
    		en el campo <i>Tiempo máximo de inactividad</i>. De lo contrario, introduzca el número de minutos \
    		que desea que transcurran antes de cerrar la conexión a Internet.";
hsetup.phase20="Opción Mantener conexión ";
hsetup.phase21="Esta opción le mantiene conectado \
    		a Internet de forma indefinida, aunque la conexión permanezca inactiva. Para utiliar \
    		esta opción, active el botón de opción situado junto a <i>Mantener conexión</i>. El \
    		período de marcado predeterminado es de 30 segundos (es decir, el enrutador comprobará la \
    		conexión a Internet cada 30 segundos).";
hsetup.phase22="Nota: algunos \
    		proveedores de cable requieren una dirección MAC específica poara realizar la conexión a \
    		Internet. Para obtener más información, haga clic en la ficha <b>Sistema</b> . A continuación, \
    		haga clic en el botón <b>Ayuda</b> y lea la información que se proporciona sobre la función de clonación MAC.";
hsetup.phase23="LAN";
hsetup.phase24="Dirección IP y Máscara de subred";
hsetup.phase25="Se trata de\
    		la dirección IP y la máscara de subred del enrutador tal y como se ve en la LAN interna. El \
    		valor predeterminado es 192.168.1.1 para dirección IP y 255.255.255.0 para la máscara de subnet.";
hsetup.phase26="DHCP";
hsetup.phase27="Mantenga el valor \
    		predeterminado, <b>Activar</b>, para activar la opción DHCP del enrutador. Si ya \
    		tiene un servidor DHCP en la red y no desea uno, \
    		seleccione <b>Desactivar</b>.";
hsetup.phase28="Introduzca un \
    		valor numérico con el que debe comenzar el servidor DHCP cuando emita direcciones IP. \
    		No empiece con 192.168.1.1 (la dirección IP del enrutador).";
hsetup.phase29="Número máximo de usuarios de DHCP";
hsetup.phase30="Introduzca el \
    		número máximo de ordenadores al que el servidor DHCP debe asignar direcciones IP.\
    		El máximo absoluto es 253--es posible si 192.168.1.2 es su dirección IP inicial.";
hsetup.phase31="El tiempo de \
    		concesión de cliente es la cantidad de tiempo durante el que un usuario de la red tiene permiso \
    		para conectarse al entutador con su dirección IP dinámica actual. Introduzca la cantidad de\
    		tiempo en minutos que se \"concederá\" al usuario esta dirección IP dinámica.";
hsetup.phase32="DNS 1-3 fijo ";
hsetup.phase33="El sistema \
    		de nombres de dominio (DNS) es la forma en la que Internet traduce nombres de dominio o de sitios Web \
    		en direcciones de Internet o direcciones URL. Su ISP le proporcionará al menos una \
    		dirección IP del servidor DNS. Si desea utilizar otra, introduzca esa dirección IP \
    		en uno de estos campos. Puede introducir hasta tres direcciones IP de servidor DNS \
    		aquí. El enrutador las utilizará para tener un acceso más rápido a los servidores DNS \
    		en funcionamiento.";
hsetup.phase34="El servicio de nombres de Internet de Windows (WINS) gestiona la intereacción de cada uno de los PC con \
    		Internet. Si utiliza un servidor WINS, introduzca su dirección IP aquí. \
    		De lo contrario, deje el campo en blanco.";
hsetup.phase35="Compruebe todos los \
		valores y haga clic en <b>Guardar configuración</b> para guardar la configuración. Haga clic en el botón <b>Cancelar \
		cambios</b> para \
		cancelar los cambios sin guardar. Conéctese a Internet para probar la \
		configuración. ";    		    		    		

//DDNS
var helpddns = new Object();
helpddns.phase1="El enrutador ofrece una función DDNS (Dynamic Domain Name System). DDNS permite asignar un host \
		y un nombre de dominio fijos a una dirección IP dinámica de Internet. Resulta útil cuando \
		aloja su propio sitio Web, servidor FTP y otro dispositivo detrás del enrutador. Antes \
		de utilizar esta función, debe suscribirse al servicio DDNS en <i>www.dyndns.org</i>, \
		un proveedor de servicios DDNS.";
helpddns.phase2="Servicio DDNS";
helpddns.phase3="Para desactivar el servicio DDNS, mantenga el valor predeterminado, <b>Desactivar</b>. Para activar el servicio DDNS,\
		siga estas instrucciones:";
helpddns.phase4="Suscríbase al servicio DDNS en <i>www.dyndns.org</i> y anote \
		su nombre de usuario,<i> </i>contraseña y la información del <i> </i>nombre de host.";
helpddns.phase5="En la pantalla <i>DDNS</i>, seleccione <b>Activar.</b>";
helpddns.phase6="Rellene los campos <i>Nombre de usuario</i>,<i> Contraseña</i> y <i> Nombre de host</i>.";
helpddns.phase7="Haga clic en el botón <b>Guardar configuración</b> para guardar los cambios. Haga clic en el botón <b>Cancelar cambios</b> para \
		cancelar los cambios no guardados.";
helpddns.phase8="Aquí se muestra la dirección IP de Internet actual del enrutador.";
helpddns.phase9="Aquí se muestra la conexión al servicio DDNS.";
		
//MAC Address Clone
var helpmac =  new Object();
helpmac.phase1="Clonación MAC";
helpmac.phase2="La dirección MAC del enrutador es un código de 12 dígitos asignando a un componente \
    		de hardware para su identificación. Algunos ISP requieren que registre la dirección MAC \
    		de su tarjeta o adaptador de red, que estaba conectado a su módem DSL o de cable \
    		durante la instalación. Si su ISP require que registre la dirección MAC, \
    		busque la dirección MAC del adaptador siguiendo las \
    		instrucciones del sistema operativo de su ordenador.";
helpmac.phase3="Para Windows 98 y Millennium:";
helpmac.phase4="1.  Haga clic en el botón <b>Inicio</b> y seleccione <b>Ejecutar</b>.";
helpmac.phase5="2.  Escriba <b>winipcfg </b> en el campo proporcionado y pulse la tecla <b>Aceptar</b>.";
helpmac.phase6="3.  Seleccione el adaptador Ethernet que esté utilizando.";
helpmac.phase7="4.  Haga clic en <b>Más información</b>.";
helpmac.phase8="5.  Anote la dirección MAC del adaptador.";
helpmac.phase9="1.  Haga clic en el botón <b>Inicio</b> y seleccione <b>Ejecutar</b>.";
helpmac.phase10="2.  Escriba <b>cmd </b> en el campo proporcionado y pulse la tecla <b>Aceptar</b>.";
helpmac.phase11="3.  En el símbolo de comandos, ejecute <b>ipconfig /all</b> y observe la dirección física del adaptador.";
helpmac.phase12="4.  Anote la dirección MAC del adaptador.";
helpmac.phase13="Para clonar la dirección MAC del adaptador de red en el enrutador y evitar tener que llamar a \
    		su ISP para cambiar la dirección MAC registrada, siga estas instrucciones:";
helpmac.phase14="Para Windows 2000 y XP:";
helpmac.phase15="1.  Seleccione <b>Activar</b>.";
helpmac.phase16="2.  Introduzca la dirección MAC del adaptador en en campo <i>Dirección MAC</i>.";
helpmac.phase17="3.  Haga clic en el botón <b>Guardar configuración</b>.";
helpmac.phase18="Para desactivar la clonación de direcciones MAC, mantenga el valor predeterminado, <b>Desactivar</b>.";

//Advance Routing
var hrouting = new Object();
hrouting.phase1="Enrutamiento";
hrouting.phase2="En la pantalla <i>Enrutamiento</i> puede establecer el modo de enrutamiento y la configuración del enrutador. \
		 El modo de puerta de enlace es el recomendado para la mayoría de los usuarios.";
hrouting.phase3="Seleccione el modo de trabajo correcto. Mantenga el valor predeterminado, <b> \
    		 Puerta de enlace</b>, si el enrutador está alojando la conexión de red a Internet (el modo Puerta de enlace es el recomendado para la mayoría de los usuarios). Seleccione <b> \
    		 Enrutador</b> si el enrutador se encuentra en una red con otros enrutadores.";
hrouting.phase4="Enrutamiento dinámico (RIP)";
hrouting.phase5="Nota: esta función no está disponible en el modo Puerta de enlace.";
hrouting.phase6="El enrutamiento dinámico permite al enrutador ajustar de forma automática los cambios físicos en \
    		 el diseño de la red e intercambiar tablas de enrutamiento con otros enrutadores. El \
    		 enrutador determina la ruta de los paquetes de red según el menor número de \
    		 saltos entre el origen y el destino. ";
hrouting.phase7="Para activar la función de enrutamiento dinámico para el lado WAN, seleccione <b>WAN</b>. \
    		 Para activar esta función para el lado LAN e inalámbrico, seleccione <b>LAN e inalámbrica</b>. \
    		 Para activar la función tanto en WAN como en LAN, seleccione <b> \
    		 Ambos</b>. Para desactivar la función de enrutamiento dinámico para todas las transmisiones de datos, mantenga \
    		 el valor predeterminado, <b>Desactivar</b>. ";
hrouting.phase8="Enrutamiento estático,&nbsp; Dirección IP de destino, Máscara de subred, Puerta de enlace e Interfaz";
hrouting.phase9="Para configurar una ruta estática entre el enrutador y otra red, \
    		 seleccione un número en la lista desplegable <i>Enrutamiento estático</i>. (Una ruta \
    		 estática es un camino predetemrinado por la que debe viajar la información de red para \
    		 alcanzar un host o una red específicos.)";
hrouting.phase10="Introduzca estos datos:";
hrouting.phase11="Dirección IP de destino </b>- \
		  La dirección IP de destino es la dirección de la red o del host al que desea asignar una ruta estática.";
hrouting.phase12="Máscara de subred </b>- \
		  La máscara de subred determina qué sección de una dirección IP es la porción de red y qué \
    		  sección es la porción de host. ";
hrouting.phase13="Puerta de enlace </b>- \
		  Es la dirección IP del dispositivo de puerta de enlace que permite establecer el contacto entre el enrutador y la red o el host.";
hrouting.phase14="Dependiendo de la ubicación de la dirección IP de destino, seleccione \
    		  <b>LAN e Inalámbrica </b>o <b>WAN </b>en el menú desplegable <i>Interfaz</i>.";
hrouting.phase15="Para guardar los cambios, haga clic en el botón <b>Aplicar</b>. Para cancelar los cambios sin guardar, haga clic en el botón <b> \
    		  Cancelar</b>.";
hrouting.phase16="Para agregar más rutas estáticas, repita los pasos 1 al 4.";
hrouting.phase17="Eliminar esta entrada";
hrouting.phase18="Para eliminar una entrada de ruta estática:";
hrouting.phase19="En la lista lista desplegable <i>Enrutamiento estático</i>, seleccione un número de entrada de la ruta estática.";
hrouting.phase20="Haga clic en el botón <b>Eliminar esta entrada</b>.";
hrouting.phase21="Para guardar un elemento eliminado, haga clic en el botón <b>Aplicar</b>. Para cancelar un elemento eliminado, haga clic en el botón <b> \
    		  Cancelar</b>. ";
hrouting.phase22="Mostrar tabla de enrutamiento";
hrouting.phase23="Haga clic en el botón \
    		  <b>Mostrar tabla de enrutamiento</b> para ver todas las entradas de ruta válidas en uso. La dirección IP de destino, la máscara de subred, \
    		  la puerta de enlace y la interfaz se mostrarán en cada entrada. Haga clic en el botón <b>Actualizar</b> para actualizar los datos visualizados. Haga clic en el botón <b> \
    		  Cerrar</b> para volver a la pantalla <i>Enrutamiento</i>.";
hrouting.phase24="Dirección IP de destino </b>- \
		  La dirección IP de destino es la dirección de la red o del host a la que está asignada la ruta estática. ";
hrouting.phase25="Máscara de subred </b>- \
		  La máscara de subred determina qué sección de una dirección IP es la porción de red y qué sección es la porción de host.";
hrouting.phase26="Puerta de enlace</b> - Es la dirección IP del dispositivo de puerta de enlace que permite establecer el contacto entre el enrutador y la red o el host.";
hrouting.phase27="Interfaz</b> - Esta interfaz le indica si la dirección IP de destino está en \
    		  <b> LAN e Inalámbrica </b>(redes con cables e inalámbricas internas), en <b>WAN</b> (Internet) o en <b> \
    		  Bucle invertido</b> (una red ficticia en la que un PC actúa como una red y que es necesaria para determinados programas de software). ";

//Firewall
var hfirewall = new Object();
hfirewall.phase1="Bloquear solicitud de WAN";
hfirewall.phase2="Mediante la activación de la función de bloqueo de solicitudes de WAN es posible evitar que se realicen \
    		 sondeos en su red o que ésta sea decectada por otros usuarios de Internet. La función Bloquear solicitud de WAN \
    		 también refuerza la seguridad de la red ya que oculta los puertos. \
    		 Ambas funciones de Bloquear solicutud de WAN hacen más dificil que usuarios \
    		 externos consigan acceder a la red. Esta función está activada \
    		 de forma predeterminada. Seleccione <b>Desactivar</b> para desactivarla.";
hfirewall.right="Activar o desactivar el servidor de seguridad SPI.";

//VPN
var helpvpn = new Object();
helpvpn.phase1="Pasarela de VPN";
helpvpn.phase2="Las redes privadas virtuales (VPN) normalmente se utilizan para crear redes relacionadas con trabajos. En los \
    		túneles VPN, en enrutador admite las pasarelas IPSec y PPTP.";
helpvpn.phase3="<b>IPSec</b> - Seguridad del protocolo de Internet (IPSec) es un<b> </b>conjunto de protocolos que se usan para implementar \
      		el intercambio seguro de paquetes a nivel de IP. Para permitir que los túneles IPSec \
      		atraviesen el enrutrador, la pasarela IPSec está activada de forma predetermianda. Para desactivarla, \
      		desactive la casilla de verificación situada junto a <i>IPSec</i>.";
helpvpn.phase4="<b>PPTP </b>- El protocolo de túnes punto a punto es el método utilizado para habilitar sesiones VPN \
      		para un servidor Windows NT 4.0 o 2000. Para permitir que los túneles PPTP atraviesen \
      		el enrutrador, la pasarela PPTP está activada de forma predetermianda. Para desactivarla, \
      		desactive la casilla de verificación situada junto a <i>PPTP</i>.";

helpvpn.right="Si lo desea, puede activar la pasarela PPTP, L2TP o IPSec para permitir que sus dispositivos de red\
		se comuniquen a través de VPN.";
//Internet Access
var hfilter = new Object();
hfilter.phase1="Filtros";
hfilter.phase2="La pantalla <i>Filtros</i> permite bloquear o permitir tipos concretos de usos de Internet. \
		Puede configurar directivas de acceso a Internet en ordenadores concretos y configurar \
		filtros mediante el empleo de números de puertos de red.";
hfilter.phase3="Esta función permite personalizar hasta diez directivas diferentes de acceso a Internet \
    		para ordenadores concretos, que se identifican mediante sus direcciones IP o MAC. Para \
    		cada PC con directiva, especifique los días y períodos de tiempo.";
hfilter.phase4="Para crear o editar una directiva, siga estas instrucciones:";
hfilter.phase5="Seleccione el número de dierctiva \(1-10\) en el menú desplegable.";
hfilter.phase6="Introduzca un nombre en el campo <i>Introducir nombre de perfil</i>.";
hfilter.phase7="Haga clic en el botón <b>Editar lista de equipos</b>.";
hfilter.phase8="Haga clic en el botón <b>Aplicar</b> para guardar los cambios. Haga clic en el botón <b>Cancelar</b> para \
    		cancelar los cambios no guardados. Haga clic en el botón <b>Cerrar</b> para volver a la pantalla \
    		<i>Filtros</i>.";
hfilter.phase9="Si desea bloquear el acceso a Internet a los ordenadores de la lista durante los días y las horas \
    		designadas, mantenga el valor predeterminado, <b>Desactivar acceso a Internet para \
    		ordenadores de la lista</b>. Si desea permitir que los ordenadores de la lista accedan a Internet durante \
    		los días y las horas designadas, haga clic en el botón de opción situado junto a <i> Activar \
    		acceso a Internet para ordenadores de la lista</i>.";
hfilter.phase10="En la pantalla <i>Lista de equipos</i>, especifique los ordenadores mediante su dirección IP o MAC. Introduzca las \
    		direcciones IP adecuadas en los campos <i>IP</i>. Si tiene un intervalo de \
    		direcciones IP que desea filtrar, rellene los campos <i>Intervalo de IP</i> apropiados. \
    		Introduzca las direcciones MAC correspondientes en los campos <i>MAC</i>.";
hfilter.phase11="Defina la hora en la que se filtrará el acceso. Seleccione <b>24 horas</b>,<b> </b>o active la casilla situada junto a <i>De</i> \
    		u utilice los cuadros desplegables para designar un período de tiempo concreto. ";
hfilter.phase12="Defina los días en los que se filtrará el acceso. Seleccione <b>Cada día</b> o los días apropiados de la semana. ";
hfilter.phase13="Haga clic en el botón <b>Agregar a directiva</b> para guardar los cambios y activarla.";
hfilter.phase14="Para crear o editar más directivas, repita los pasos 1 al 9.";
hfilter.phase15="Para eliminar una directiva de acceso a Internet, seleccione el número de directiva y haga clic en el botón <b>Eliminar</b>.";
hfilter.phase16="Para ver un resumen de todas las directivas, haga clic en el botón <b>Resumen</b>. La pantalla <i> \
    		Resumen de directivas de Internet</i> mostrará cada número de directiva, el nombre \
    		de la directiva, los días y la hora del día. Para eliminar una directiva, haga clic en su casilla y, a continuación, \
    		haga clic en el botón <b>Eliminar</b>. Haga clic en el botón <b>Cerrar</b> para volver a la \
    		pantalla <i>Filtros</i>.";
hfilter.phase17="Intervalo de puertos de Internet filtrados";
hfilter.phase18="Para filtrar ordenadores por número de puerto de red, seleccione <b>Ambos</b>, <b>TCP</b> o <b>UDP</b>, \
    		dependiendo de los protocolos que desee filtrar. A continuación<b> </b>introduzca los números \
    		de puertos que desea filtrar en los campos de número de puerto. Los ordenadores conectados al \
    		enrutador no podrán acceder a ningún puerto que aparezca en esta lista. Para \
    		desactivar un filtro, seleccione <b>Desactivar</b>.";

//share of help string
var hshare = new Object();
hshare.phase1="Compruebe todos los valores y haga clic en <b>Guardar configuración</b> para guardar la configuración. Haga clic en el botón <b>Cancelar \
		cambios</b> para cancelar los cambios no guardados.";


//DMZ
var helpdmz = new Object();
helpdmz.phase1="La función de host DMZ permite exponer a un usuario local a Internet para que\
		pueda utilizar un servicio especial como juegos por Internet o videoconferencia. \
		El alojamiento DMZ reenvía todos los puertos al mismo tiempo a un ordenador. La función Reenvío de puertos \
    		es más segura porque sólo abre los puertos que el usuario \
    		desea tener abiertos, mientras que el alojamiento DMZ abre todos los puertos de un ordenador, \
    		deja expuesto al ordenador de modo que puede verse en Internet. ";    		
helpdmz.phase2="Cualquier ordenador cuyo puerto está siendo reenviado debe tene desactivada la función de cliente DHCP \
    		y debe tener asignada una nueva dirección IP fija porque tal vez \
    		cambie su dirección IP al utilizar la función DHCP.";
/***To expose one PC, select enable.***/
helpdmz.phase3="Para exponer un PC, seleccione ";
helpdmz.phase4="introduzca la dirección IP del prdenador en el campo <i>Dirección IP del host DMZ</i>.";



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
 
