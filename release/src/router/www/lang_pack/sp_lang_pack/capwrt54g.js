var firewall2 = new Object();
firewall2.natredir="Filtrar redirección NAT de Internet";
firewall2.ident="Filtrar IDENT (puerto 113)";
firewall2.multi="Filtrar multidifusión";

var hupgrade = new Object();
hupgrade.right1="Haga clic en el botón Examinar para seleccionar el archivo de firmware que desea cargar en el enrutador.";
hupgrade.right2="Haga clic en el botón Actualizar para iniciar el proceso de actualización. Dicho proceso no puede ser interrumpido.";
hupgrade.wrimage="Archivo de imagen incorrecto.";

var hfacdef = new Object();
hfacdef.right1="Se restablecerán todos los valores predeterminados de fábrica y se borrarán todos los valores que haya establecido el usuario.";
hfacdef.warning="Aviso. Si hace clic en Aceptar, el dispositivo restablecerá todos los valores predeterminados de fábrica y se borrará cualquier configuración anterior.";

var hdiag = new Object();
hdiag.right1="Introduzca la dirección IP o el nombre de dominio que desea sondear y haga clic en el botón Sondear.";
hdiag.right2="Introduzca la dirección IP o el nombre de dominio en los que desea realizar el seguimiento y haga clic en el botón Ruta de seguimiento.";

var hlog = new Object();
hlog.right1="Puede activar o desactivar el uso de los registros <b>de entrada</b> y <b>de salida</b> mediante la selección del botón de opción correspondiente.";

var manage2 = new Object();
manage2.webacc="Acceso Web";
manage2.accser="Servidor de &nbsp;acceso";
manage2.wlacc="Web de acceso &nbsp;inalámbrico";
manage2.vapass="La contraseña confirmada no coincide con la introducida. Vuelva a introducir la contraseña";
manage2.passnot="Las contraseñas no coinciden.";
manage2.selweb="Debe seleccionar al menos un servidor Web.";
manage2.changpass="Actualmente el enrutador tiene establecida la contraseña predeterminada. Como medida de seguridad, deberá cambiar la contraseña antes de poder activar la función Administración remota. Haga clic en el botón Aceptar para cambiar la contraseña. Haga clic en el botón Cancelar para dejar la función Administración remota desactivada.";

var hmanage2 = new Object();
hmanage2.right1="<b>Acceso a enrutador local: </b>Desde aquí puede cambiar la contraseña del enrutador. Introduzca una nueva contraseña y, a continuación, vuelva a escribirla en el campo Confirmar contraseña.<br>";
hmanage2.right2="<b>Acceso Web: </b>Permite configurar las opciones de acceso a la utilidad Web del enrutador.";
hmanage2.right3="<b>Acceso a enrutador remoto: </b>Permite acceder al enrutador de forma remota.  Seleccione el puerto que desea utilizar. Debe cambiar la contraseña si el enrutador sigue utilizando la contraseña predeterminada.";
hmanage2.right4="<b>UPnP: </b>Utilizado por determinados programas para abrir puertos automáticamente para realizar la comunicación.";

var hstatwl2 = new Object();
hstatwl2.right1="<b>Dirección MAC</b>. Dirección MAC del enrutador, tal y como se observa en la red local inalámbrica.";
hstatwl2.right2="<b>Modo</b>. Al seleccionarlo desde la ficha Inalámbrica, mostrará el modo inalámbrico (Mixto, Sólo G o Desactivado) que se utiliza en la red.";

var hstatlan2 = new Object();
hstatlan2.right1="<b>Dirección MAC</b>. Dirección MAC del enrutador, tal y como se observa en la red local Ethernet.";
hstatlan2.right2="<b>Dirección IP</b>. Muestra la dirección IP del enrutador, tal y como aparece en la red local Ethernet.";
hstatlan2.right3="<b>Máscara de subred</b>. Muestra la máscara de subred cuando el enrutador utiliza una.";
hstatlan2.right4="<b>Servidor DHCP</b>. Muestra el servidor DHCP si el enrutador utiliza uno.";

var hstatrouter2 = new Object();
hstatrouter2.wan_static="Static";
hstatrouter2.l2tp="L2TP";
//hstatrouter2.hb="Señal de latido";
hstatrouter2.hb="Cable Telstra";
hstatrouter2.connecting="Conectando";
hstatrouter2.disconnected="Desconectado";
hstatrouter2.disconnect="Desconectar";
hstatrouter2.right1="<b>Versión del firmware. </b>Versión actual del firmware del enrutador.";
hstatrouter2.right2="<b>Hora actual. </b>Muestra la hora, tal y como ha establecido en la ficha Instalar.";
hstatrouter2.right3="<b>Dirección MAC. </b>Dirección MAC del enrutador, como la observa su ISP.";
hstatrouter2.right4="<b>Nombre del enrutador. </b>Nombre específico del enrutador establecido en la ficha Instalar.";
hstatrouter2.right5="<b>Tipo de configuración. </b>Muestra información que necesita el ISP para realizar la conexión a Internet. Esta información se introdujo en la ficha Instalar. Puede <b>Conectar</b> o <b>Desconectar</b> desde aquí haciendo clic en el botón correspondiente.";
hstatrouter2.authfail=" fallo de autenticación";
hstatrouter2.noip="No se puede obtener una dirección IP de ";
hstatrouter2.negfail=" fallo de negociación";
hstatrouter2.lcpfail=" fallo de negociación LCP";
hstatrouter2.tcpfail="No se puede crear una conexión TCP a ";
hstatrouter2.noconn="No se puede conectar a ";
hstatrouter2.server=" servidor";

var hdmz2 = new Object();
hdmz2.right1="<b>DMZ: </b>Al activar esta opción, el enrutador quedará expuesto a Internet.  Se podrá acceder a todos los puertos desde Internet";

var hforward2 = new Object();
hforward2.right1="<b>Reenvío a intervalo de puertos: </b>Puede que determinadas aplicaciones necesiten abrir puertos específicos para que funcionen correctamente, como por ejemplo, servidores y determinados juegos en línea. Cuando se recibe una solicitud de un puerto concreto desde Internet, el enrutador dirigirá los datos al ordenador especificado.  Por razones de seguridad, tal vez desee limitar el reenvío de puertos sólo a los puertos que esté utilizando, y desactivar la casilla de verificación <b>Activar</b> cuando termine.";

var hfilter2 = new Object();
hfilter2.delpolicy="¿Desea eliminar la directiva?";
hfilter2.right1="<b>Directiva de acceso a Internet: </b>Puede definir hasta 10 directivas de acceso. Haga clic en <b>Eliminar</b> para eliminar una directiva, o en <b>Resumen</b> para ver un resumen de la directiva.";
hfilter2.right2="<b>Estado: </b>Activa o desactiva una directiva.";
hfilter2.right3="<b>Nombre de la directiva: </b>Si lo desea, puede asignar un nombre a la directiva.";
hfilter2.right4="<b>Tipo de directiva: </b>Seleccione entre Acceso a Internet o Tráfico de entrada.";
hfilter2.right5="<b>Días: </b>Seleccione el día de la semana en el que desea aplicar la directiva.";
hfilter2.right6="<b>Horas: </b>Introduzca la hora del día en la que desea aplicar la directiva.";
hfilter2.right7="<b>Servicios bloqueados: </b>Puede elegir bloquear el acceso a determinados servicios. Haga clic en <b>Agregar/Editar</b> servicios para modificar estos valores.";
hfilter2.right8="<b>Bloqueo de sitios Web por dirección URL: </b>Puede bloquear el acceso a determinados sitios Web mediante la introducción de sus direcciones URL.";
hfilter2.right9="<b>Bloqueo de sitios Web por palabra clave: </b>Puede bloquear el acceso a determinados sitios Web por las palabras clave contenidas en su página Web.";

var hportser2 = new Object();
hportser2.submit="Aplicar";

var hwlad2 = new Object();
hwlad2.authtyp="Tipo de autenticación";
hwlad2.basrate="Velocidad básica";
hwlad2.mbps="Mbps";
hwlad2.def="Predet.";
hwlad2.all="Todo";
hwlad2.defdef="(Predet.: Valor predeterminado)";
hwlad2.fburst="Ráfaga de tramas";
hwlad2.milli="Milisegundos";
hwlad2.range="Intervalo";
hwlad2.frathrh="Umbral de fragmentación";
hwlad2.apiso="Aislamiento de PA";
hwlad2.off="Desactivado";
hwlad2.on="Activado";
hwlad2.right1="<b>Tipo de autenticación: </b>Puede elegir entre Automático o Clave compartida.  La autenticación por clave compartida es más segura, pero todos los dispositivos de la red deberán admitir este tipo de autenticación.";

var hwlbas2 = new Object();
hwlbas2.right1="<b>Modo de red inalámbrica: </b>SpeedBooster se activa automáticamente en los modos <b>Mixto</b> y <b>Sólo G</b>. Si desea excluir clientes Wireless-G, seleccione el modo <b>Sólo B</b>.  Si desea desactivar el acceso inalámbrico, seleccione <b>Desactivar</b>.";

var hwlsec2 = new Object();
hwlsec2.wpapsk="Clave previamente compartida de WPA";
hwlsec2.wparadius="RADIUS WPA";
hwlsec2.wpapersonal="WPA Personal";
hwlsec2.wpaenterprise="WPA Enterprise";
//new wpa2
hwlsec2.wpa2psk="Sólo clave previamente compartida de WPA2";
hwlsec2.wpa2radius="Sólo WPA2 RADIUS";
hwlsec2.wpa2pskmix="Clave previamente compartida de WPA2 mixta";
hwlsec2.wpa2radiusmix="WPA2 RADIUS mixto";
hwlsec2.wpa2personal="WPA2 Personal";
hwlsec2.wpa2enterprise="WPA2 Enterprise";
//new wpa2
hwlsec2.right1="<b>Modo de seguridad: </b>Puede optar entre Desactivar, WEP, Clave previamente compartida de WPA, RADIUS WPA o RADIUS. Todos los dispositivos de la red deben usar el mismo modo de seguridad para poder comunicarse.";

var hwmac2 = new Object();
hwmac2.right1="<b>Clonación de direcciones MAC: </b>Algunos proveedores de servicios de Internet (ISP) le pedirán que registre la dirección MAC de su ordenador. Si no desea volver a registrar su dirección MAC, puede hacer que el enrutador clone la dirección MAC que está registrada con su ISP.";

var hddns2 = new Object();
hddns2.right1="<b>Servicio DDNS: </b>DDNS le permite acceder a su red mediante nombres de dominio, en lugar de usar direcciones IP. El servicio se encarga del cambio de dirección IP y actualiza la información del dominio de forma automática.  Debe suscribirse al servicio a través de TZO.com o de DynDNS.org.";
hddns2.right2="Click <b><a target=_blank href=http://Linksys.tzo.com>here</a></b> to SIGNUP with a <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;TZO FREE TRIAL ACCOUNT";
hddns2.right3="Click <b><a target=_blank href=https://controlpanel.tzo.com>here</a></b> to Manage your <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;TZO Account";
hddns2.right4="Click <b><a target=_blank href=https://www.tzo.com/cgi-bin/Orders.cgi?ref=linksys>here</a></b> to Purchase a TZO <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;DDNS Subscription";
hddns2.right5="TZO DDNS <b><a target=_blank href=http://Linksys.tzo.com>Support/Tutorials</a></b>";

var hrouting2 = new Object();
hrouting2.right1="<b>Modo de funcionamiento: </b>Si el enrutador se encarga de la conexión a Internet, seleccione el modo <b>Puerta de enlace</b>.  Si hay otro enrutador en la red, seleccione el modo <b>Enrutador</b>.";
hrouting2.right2="<b>Seleccionar número de conjunto: </b>Es el número de ruta único; puede configurar un máximo de 20 rutas.";
hrouting2.right3="<b>Nombre de ruta:</b>  Introduzca el nombre que desea asignar a esta ruta.";
hrouting2.right4="<b>IP de LAN de destino: </b>Es el host remoto al que desea asignar la ruta fija.";
hrouting2.right5="<b>Máscara de subred: </b>Determina el host y la parte de red.";

var hindex2 = new Object();
hindex2.telstra="Cable Telstra";
hindex2.dns3="DNS 3 fijo";
hindex2.hbs="Servidor de &nbsp;latidos";
hindex2.l2tps="Servidor L2TP";
hindex2.hdhcp="<b>Configuración automática - DHCP: </b>este valor se utiliza principalmente con operadores de cable.<br><br>";
hindex2.hpppoe1="<b>PPPoE: </b>este valor se utiliza principalmente con proveedores DSL.<br>";
hindex2.hpppoe2="<b>Usuario: </b>Introduzca el nombre de usuario proporcionado por su ISP.<br>";
hindex2.hpppoe3="<b>Contraseña: </b>Introduzca la contraseña proporcionada por su ISP.<br>";

//hindex2.hpppoe4="<b><a target=_blank href=help/HSetup.asp>Más...</a></b><br><br><br><br><br>";

hindex2.hstatic1="<b>IP fija: </b>este valor se utiliza principalmente con ISP para empresas.<br>";
hindex2.hstatic2="<b>Dirección IP de Internet: </b>Introduzca la dirección IP proporcionada por su ISP.<br>";
hindex2.hstatic3="<b>Máscara de subred: </b>Introduzca su máscara de subred<br>";

//hindex2.hstatic4="<b><a target=_blank href=help/HSetup.asp>Más...</a></b><br><br><br><br><br><br><br>";

hindex2.hpptp1="<b>PPTP: </b>este valor se utiliza principalmente con proveedores DSL.<br>";
hindex2.hpptp2="<b>Dirección IP de Internet: </b>Introduzca la dirección IP proporcionada por su ISP.<br>";
hindex2.hpptp3="<b>Máscara de subred: </b>Introduzca su máscara de subred<br>";
hindex2.hpptp4="<b>Puerta de enlace: </b>Introduzca la dirección de la puerta de enlace de su ISP<br>";

//hindex2.hpptp5="<b><a target=_blank href=help/HSetup.asp>Más...</a></b><br><br><br><br><br><br><br><br>";

hindex2.hl2tp1="<b>L2TP: </b>Este valor lo utilizan algunos ISP en Europa.<br>";
hindex2.hl2tp2="<b>Usuario: </b>Introduzca el nombre de usuario proporcionado por su ISP.<br>";
hindex2.hl2tp3="<b>Contraseña: </b>Introduzca la contraseña proporcionada por su ISP.<br>";

//hindex2.hl2tp4="<b><a target=_blank href=help/HSetup.asp>Más...</a></b><br><br><br><br><br><br><br><br>";

hindex2.hhb1="<b>Cable Telstra: </b>Este valor se utiliza principalmente con proveedores DSL.<br>";
hindex2.hhb2="<b>Usuario: </b>Introduzca el nombre de usuario proporcionado por su ISP.<br>";
hindex2.hhb3="<b>Contraseña: </b>Introduzca la contraseña proporcionada por su ISP.<br>";

//hindex2.hhb4="<b><a target=_blank href=help/HSetup.asp>Más...</a></b><br><br><br><br><br><br>";

hindex2.right1="<b>Nombre de host: </b>Introduzca el nombre de host proporcionado por su ISP.<br>";
hindex2.right2="<b>Nombre de dominio: </b>Introduzca el nombre de dominio proporcionado por su ISP.<br>";
hindex2.right3="<b>Dirección IP local: </b>Es la dirección del enrutador.<br>";
hindex2.right4="<b>Máscara de subred: </b>Es la máscara de subred del enrutador.<br><br><br>";
hindex2.right5="<b>Servidor DHCP: </b>Permite al enrutador gestionar las direcciones IP.<br>";
hindex2.right6="<b>Dirección IP inicial: </b>Dirección con la que desea comenzar.<br>";
hindex2.right7="<b>Configuración horaria: </b>Seleccione la zona horaria donde se encuentra. El enrutador también se puede ajustar automáticamente al horario de verano.";
hindex2.hdhcps1="<b>Número máximo de usuarios DHCP: </b>Puede limitar el número de direcciones que gestiona el enrutador.<br>";

var errmsg2 = new Object();
errmsg2.err0="La dirección IP del servidor de latidos no es válida.";
errmsg2.err1="¿Eliminar esta entrada?";
errmsg2.err2="Debe introducir un SSID.";
errmsg2.err3="Introduzca la clave compartida.";
errmsg2.err4=" tienen dígitos hexadecimales no válidos o superan los 63 caracteres.";
errmsg2.err5="Clave no válida, debe tener entre 8 y 63 caracteres ASCII o 64 dígitos hexadecimales";
errmsg2.err6="Debe introducir una clave para Clave ";
errmsg2.err7="Longitud no válida en la clave ";
errmsg2.err8="Introduzca una frase secreta.";
errmsg2.err9="El total de comprobaciones excede las 40.";
errmsg2.err10=" no es un espacio permitido.";
errmsg2.err11="Después de realizar todas las acciones, haga clic en el botón Aplicar para guardar la configuración.";
errmsg2.err12="Debe introducir un nombre de servicio.";
errmsg2.err13="El nombre del servicio debe existir.";
errmsg2.err14="Dirección IP de LAN o máscara de subred no válidas.";

var trigger2 = new Object();
trigger2.ptrigger="Desencadenamiento de puertos";
trigger2.qos="QoS";
trigger2.trirange="Intervalo desencadenado";
trigger2.forrange="Intervalo reenviado";
trigger2.sport="Puerto inicial";
trigger2.eport="Puerto final";
trigger2.right1="Aplicación Introduzca el nombre de la aplicación del disparador. <b>Intervalo desencadenado</b> Para cada aplicación, muestra el intervalo de número de puertos desencadenados. Revise los números de puerto necesarios en la documentación de la aplicación de Internet. <b>Puerto inicial</b> Introduzca el número de puerto inicial del intervalo desencadenado. <b>Puerto final</b> Introduzca el número de puerto final del intervalo desencadenado. <b>Intervalo reenviado</b> Para cada aplicación, muestra el intervalo de números de puerto reenviados. Revise los números de puerto necesarios en la documentación de la aplicación de Internet. <b>Puerto inicial</b> Introduzca el número de puerto inicial del intervalo reenviado. <b>Puerto final</b> Introduzca el número de puerto final del intervalo reenviado.";

var bakres2 = new Object();
bakres2.conman="Administración de configuración";
bakres2.bakconf="Realizar copia de seguridad de configuración";
bakres2.resconf="Restaurar configuración";
bakres2.file2res="Seleccione un archivo para realizar la restauración";
bakres2.bakbutton="Copia de seguridad";
bakres2.resbutton="Restaurar";
bakres2.right1="Si lo desea, puede realizar una copia de seguridad de la configuración actual en caso de que necesite restablecer el enrutador a su configuración predeterminada de fábrica.";
bakres2.right2="Puede hacer clic en el botón Copia de seguridad para realizar una copia de seguridad de la configuración actual.";
bakres2.right3="Haga clic en el botón Examinar para buscar y seleccionar un archivo de configuración que esté guardado actualmente en su PC.";
bakres2.right4="Haga clic en Restaurar para sobrescribir la configuración actual con la del archivo de configuración.";

var qos = new Object();
qos.uband="Ancho de banda<br>de subida";
qos.bandwidth="Ancho de banda";
qos.dpriority="Prioridad de dispositivos";
qos.priority="Prioridad";
qos.dname="Nombre del dispositivo";
qos.low="Baja";
qos.medium="Media";
qos.high="Alta";
qos.highest="Máxima";
qos.eppriority="Prioridad de puertos Ethernet";
qos.flowctrl="Control de flujo";
qos.appriority="Prioridad de aplicaciones";
qos.specport="Puerto específico";
//qos.appname="Nombre de la aplicación";
qos.appname="Nombre de la aplicación";
qos.alert1="El valor del puerto está fuera del intervalo [0-65535]";
qos.alert2="El valor de puerto inicial es mayor que el valor de puerto final";
qos.confirm1="Configurar varios dispositivos, el puerto Ethernet o la aplicación con prioridad alta puede negar el efecto de QoS.\n¿Seguro que desea continuar?";
/*
qos.right1="WRT54G ofrece dos tipos de funciones QoS (calidad del servicio), basada en aplicación y basada en puerto. Seleccione la opción adecuada a sus necesidades.";
qos.right2="<b>QoS basada en aplicación: </b>Puede controlar el ancho de banda con respecto a la aplicación que lo consume. Existen varias aplicaciones ya configuradas. También puede personalizar hasta tres aplicaciones mediante la introducción del número de puerto que utilizan.";
qos.right3="<b>QoS basada en puerto: </b>Puede controlar el ancho de banda según el puerto de LAN físico al que está conectado su dispositivo. Puede asignar una prioridad Alta o Baja a los dispositivos conectados a los puertos de LAN 1 a 4.";
*/
//wireless qos
qos.optgame="Optimizar aplicaciones de juegos";
qos.wqos="QoS de cable";
qos.wlqos="QoS inalámbrico";
qos.edca_ap="Parámetros PA de EDCA";
qos.edca_sta="Parámetros STA de EDCA";
qos.wme="Soporte de WMM";
qos.noack="Sin acuse de recibo";
qos.defdis="(Predeterminado: Desactivar)";
qos.cwmin="CWmin";
qos.cwmax="CWmax";
qos.aifsn="AIFSN";
qos.txopb="TXOP(b)";
qos.txopag="TXOP(a/g)";
qos.admin="Admin";
qos.forced="Forzado";
qos.ac_bk="AC_BK";
qos.ac_be="AC_BE";
qos.ac_vi="AC_VI";
qos.ac_vo="AC_VO";


qos.right1="Están disponibles dos tipos de funciones QoS (Quality of Service), QoS de cable que controla los dispositivos conectados al enrutador mediante un cable Ethernet, y QoS inalámbrico, que controla los dispositivos conectados de forma inalámbrica al enrutador."
qos.right2="<b>Prioridad de dispositivos:</b> puede especificar la prioridad de todo el tráfico desde un dispositivo de la red mediante la asignación de un nombre de dispositivo a dicho dispositivo, especifique la prioridad e introduzca su dirección MAC."
qos.right3="<b>Prioridad de puertos Ethernet:</b> puede controlar la velocidad de los datos en función del puerto LAN físico al que esté conectado el dispositivo. Puede asignar una prioridad Alta o Baja al tráfico de datos de los dispositivos conectados a los puertos de LAN 1 a 4."
qos.right4="<b>Prioridad de aplicaciones:</b> puede controlar la velocidad de los datos con respecto a la aplicación que consume ancho de banda. Active <b>Optimizar aplicaciones de juegos</b> para permitir que los puertos comunes de aplicaciones de juegos tengan una prioridad más alta. Puede personalizar hasta ocho aplicaciones mediante la introducción del número de puerto que emplean."
qos.right5="QoS inalámbrico también se conoce como <b>Wi-Fi MultiMedia<sup>TM</sup> (WMM)</b>, denominación asignada por Wi-Fi Alliance<sup>TM</sup>. Seleccione Activar para usar WMM si emplea otros dispositivos inalámbricos que disponen también de certificación WMM."
qos.right6="<b>Sin acuse de recibo:</b> active esta opción si desea desactivar el acuse de recibo. Si esta opción está activada, el enrutador no reenviará los datos si se produce un error."


var vpn2 = new Object();
vpn2.right1="Si lo desea, puede activar la pasarela PPTP, L2TP o IPSec para permitir que sus dispositivos de red se comuniquen a través de VPN.";

// for parental control

var pactrl = new Object();
pactrl.pactrl ="Control parental";
pactrl.accsign ="Suscripción de cuenta";
pactrl.center1 ="La solución de control parental de Linksys permite mantener segura <br>a su familia mientras navegan por Internet";
pactrl.center2 ="<li>Fácil de configurar</li><br><li>Proteja cualquier ordenador de la casa desde el enrutador Linksys</li><br><li>Los informes le permiten controlar el uso del Web, del correo electrónico y de la mensajería instantánea</li>";
pactrl.center3 ="** La suscripción a este servicio desactivará las directivas de acceso a Internet integradas en el enrutador";
pactrl.manageacc ="Gestionar cuenta";
pactrl.center4 ="Gestión de la cuenta de control parental";
pactrl.signparental ="Suscribirse al servicio de control parental";
pactrl.moreinfo ="Más información";
pactrl.isprovision ="el dispositivo tiene una cuenta";
pactrl.notprovision ="el dispositivo no tiene ninguna cuenta ";
pactrl.right1 ="La pantalla Control parental permite suscribirse a una cuenta Control parental de Linksys y gestionarla. El servicio Parental Control de Linksys le ofrece unas potentes herramientas para controlar la disponibilidad de los servicios de Internet, el acceso y funciones, y permite personalizarlo para cada miembro de la familia.";

var satusroute = new Object();
satusroute.localtime ="No disponible";

var succ = new Object();
succ.autoreturn ="Volverá a la página anterior tras varios segundos.";
