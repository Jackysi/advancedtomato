//Basic Setup
var hsetup = new Object();
hsetup.phase1="The <i>Setup</i> \
		screen is the first screen you will see when accessing the Router. Most users  \
		will be able to configure the Router and get it working properly using only the \
		settings on this screen. Some Internet Service Providers (ISPs) will require  \
		that you enter specific information, such as User Name, Password, IP Address,  \
		Default Gateway Address, or DNS IP Address. This information can be obtained  \
		from your ISP, if required.";
hsetup.phase2="Note: After \
		you have configured these settings, you should set a new password for the Router  \
		using the <i>Security</i> screen. This will increase security, protecting the  \
		Router from unauthorized changes. All users who try to access the Router \
		web-based utility or Setup Wizard will be prompted for the Router's password.";
hsetup.phase3="Select the \
		time zone for your location. If your location experiences daylight savings, \
    		leave the checkmark in the box next to <i>Automatically adjust clock for \
    		daylight saving changes</i>.";
hsetup.phase4="MTU is the  \
    		Maximum Transmission Unit. It specifies the largest packet size permitted \
    		for Internet transmission. Keep the default setting, <b>Auto</b>, to have \
    		the Router select the best MTU for your Internet connection. To specify a \
    		MTU size, select <b>Manual</b>, and enter the value desired (default is <b> \
    		1400</b>).&nbsp; You should leave this value in the 1200 to 1500 range.";
hsetup.phase5="This entry is necessary for some ISPs and can be provided by them.";
hsetup.phase6="The Router supports four connection types:";
hsetup.phase7="Automatic Configuration DHCP";
hsetup.phase8="(Point-to-Point Protocol over Ethernet)";
hsetup.phase9="(Point-to-Point Tunneling Protocol)";
hsetup.phase10="These types can be selected from the drop-down menu next to Internet Connection. \
    		The information required and available features will differ depending on \
    		what kind of connection type you select. Some descriptions of this \
    		information are included here:";		
hsetup.phase11="Internet IP Address and Subnet Mask";
hsetup.phase12="This is the Router's IP Address \
    		and Subnet Mask as seen by external users on the Internet (including your \
    		ISP). If your Internet connection requires a static IP address, then your \
    		ISP will provide you with a Static IP Address and Subnet Mask.";
hsetup.phase13="Your ISP will provide you with the Gateway IP Address."
hsetup.phase14="(Domain Name Server)";
hsetup.phase15="Your ISP will provide you with at least one DNS IP Address.";
hsetup.phase16="User name and Password";
hsetup.phase17="Enter the <b>User Name</b> and \
		<b>Password</b> you use when logging onto your ISP through a PPPoE or PPTP \
    		connection.";
hsetup.phase18="Connect on Demand";
hsetup.phase19="you can configure the Router to \
    		disconnect your Internet connection after a specified period of inactivity \
    		(Max Idle Time). If your Internet connection has been terminated due to \
    		inactivity, Connect on Demand enables the Router to automatically \
   		re-establish your connection as soon as you attempt to access the Internet \
   		again. If you wish to activate Connect on Demand, click the radio button. If \
    		you want your Internet connection to remain active at all times, enter <b>0</b> \
    		in the <i>Max Idle Time</i> field. Otherwise, enter the number of minutes \
    		you want to have elapsed before your Internet connection terminates.";
hsetup.phase20="Keep Alive Option ";
hsetup.phase21="This option keeps you connected \
    		to the Internet indefinitely, even when your connection sits idle. To use \
    		this option, click the radio button next to <i>Keep Alive</i>. The default \
    		Redial Period is 30 seconds (in other words, the Router will check the \
    		Internet connection every 30 seconds).";
hsetup.phase22="Note: Some \
    		cable providers require a specific MAC address for connection to the \
    		Internet. To learn more about this, click the <b>System</b> tab. Then click \
    		the <b>Help</b> button, and read about the MAC Cloning feature.";
hsetup.phase23="LAN";
hsetup.phase24="IP Address and Subnet Mask";
hsetup.phase25="This is\
    		the Router IP Address and Subnet Mask as seen on the internal LAN. The \
    		default value is 192.168.1.1 for IP Address and 255.255.255.0 for Subnet \
    		Mask.";
hsetup.phase26="DHCP";
hsetup.phase27="Keep the \
    		default, <b>Enable</b>, to enable the Router' DHCP server option. If you \
    		already have a DHCP server on your network or you do not want a DHCP server, \
    		then select <b>Disable</b>.";
hsetup.phase28="Enter a \
    		numerical value for the DHCP server to start with when issuing IP addresses. \
    		Do not start with 192.168.1.1 (the IP address of the Router).";
hsetup.phase29="Maximum Number of DHCP Users";
hsetup.phase30="Enter the \
    		maximum number of PCs that you want the DHCP server to assign IP addresses \
    		to. The absolute maximum is 253--possible if 192.168.1.2 is your starting IP \
    		address.";
hsetup.phase31="The Client \
    		Lease Time is the amount of time a network user will be allowed connection \
    		to the Router with their current dynamic IP address. Enter the amount of \
    		time, in minutes, that the user will be \"leased\" this dynamic IP address.";
hsetup.phase32="Static DNS 1-3 ";
hsetup.phase33="The Domain \
    		Name System (DNS) is how the Internet translates domain or website names \
    		into Internet addresses or URLs. Your ISP will provide you with at least one \
    		DNS Server IP Address. If you wish to utilize another, enter that IP Address \
    		in one of these fields. You can enter up to three DNS Server IP Addresses  \
    		here. The Router will utilize these for quicker access to functioning DNS  \
    		servers.";
hsetup.phase34="The Windows Internet Naming Service (WINS) manages each PC's interaction with \
    		the Internet. If you use a WINS server, enter that server's IP Address here. \
    		Otherwise, leave this blank.";
hsetup.phase35="Check all the \
		values and click <b>Save Settings</b> to save your settings. Click <b>Cancel \
		Changes</b> to \
		cancel your unsaved changes. You can test the settings by connecting to the  \
		Internet. ";    		    		    		

//DDNS
var helpddns = new Object();
helpddns.phase1="The Router offers a Dynamic Domain Name System (DDNS) feature. DDNS lets you assign a fixed \
		host and domain name to a dynamic Internet IP address. It is useful when you are \
		hosting your own website, FTP server, or other server behind the Router. Before \
		using this feature, you need to sign up for DDNS service at <i>www.dyndns.org</i>, \
		a DDNS service provider.";
helpddns.phase2="DDNS Service";
helpddns.phase3="To disable DDNS Service, keep the default setting, <b>Disable</b>. To enable DDNS \
		Service, follow these instructions:";
helpddns.phase4="Sign up for DDNS service at <i>www.dyndns.org</i>, and write down \
		your User Name,<i> </i>Password, and<i> </i>Host Name information.";
helpddns.phase5="On the <i>DDNS</i> screen, select <b>Enable.</b>";
helpddns.phase6="Complete the <i> User Name</i>,<i> Password</i>, and<i> Host Name</i> fields.";
helpddns.phase7="Click the <b>Save Settings</b> button to save your changes. Click the <b>Cancel Changes</b> button to \
		cancel unsaved changes.";
helpddns.phase8="The Router current Internet IP Address is displayed here.";
helpddns.phase9="The status of the DDNS service connection is displayed here.";
		
//MAC Address Clone
var helpmac =  new Object();
helpmac.phase1="MAC Cloning";
helpmac.phase2="The Router¡¦s MAC address is a 12-digit code assigned to a unique piece of \
    		hardware for identification. Some ISPs require that you register the MAC \
    		address of your network card/adapter, which was connected to your cable or \
    		DSL modem during installation. If your ISP requires MAC address \
    		registration, find your adapter¡¦s MAC address by following the \
    		instructions for your PC¡¦s operating system.";
helpmac.phase3="For Windows 98 and Millennium:";
helpmac.phase4="1.  Click the <b>Start</b> button, and select <b>Run</b>.";
helpmac.phase5="2.  Type <b>winipcfg </b>in the field provided, and press the <b>OK </b>key.";
helpmac.phase6="3.  Select the Ethernet adapter you are using.";
helpmac.phase7="4.  Click <b>More Info</b>.";
helpmac.phase8="5.  Write down your adapter¡¦s MAC address.";
helpmac.phase9="1.  Click the <b>Start</b> button, and select <b>Run</b>.";
helpmac.phase10="2.  Type <b>cmd </b>in the field provided, and press the <b>OK </b>key.";
helpmac.phase11="3.  At the command prompt, run <b>ipconfig /all</b>, and look at your adapter¡¦s physical address.";
helpmac.phase12="4.  Write down your adapter¡¦s MAC address.";
helpmac.phase13="To clone your network adapter¡¦s MAC address onto the Router and avoid calling your \
    		ISP to change the registered MAC address, follow these instructions:";
helpmac.phase14="For Windows 2000 and XP:";
helpmac.phase15="1.  Select <b>Enable</b>.";
helpmac.phase16="2.  Enter your adapter¡¦s MAC address in the <i>MAC Address</i> field.";
helpmac.phase17="3.  Click the <b>Save Settings </b>button.";
helpmac.phase18="To disable MAC address cloning, keep the default setting, <b>Disable</b>.";

//Advance Routing
var hrouting = new Object();
hrouting.phase1="Routing";
hrouting.phase2="On the <i>Routing</i> screen, you can set the routing mode and settings of the Router. \
		 Gateway mode is recommended for most users.";
hrouting.phase3="Choose the correct working mode. Keep the default setting, <b> \
    		 Gateway</b>, if the Router is hosting your network’s connection to the Internet (Gateway mode is recommended for most users). Select <b> \
    		 Router </b>if the Router exists on a network with other routers.";
hrouting.phase4="Dynamic Routing (RIP)";
hrouting.phase5="Note: This feature is not available in Gateway mode.";
hrouting.phase6="Dynamic Routing enables the Router to automatically adjust to physical changes in \
    		 the network layout and exchange routing tables with other routers. The \
    		 Router determines the network packets route based on the fewest number of \
    		 hops between the source and destination. ";
hrouting.phase7="To enable the Dynamic Routing feature for the WAN side, select <b>WAN</b>. \
    		 To enable this feature for the LAN and wireless side, select <b>LAN &amp; Wireless</b>. \
    		 To enable the feature for both the WAN and LAN, select <b> \
    		 Both</b>. To disable the Dynamic Routing feature for all data transmissions, keep the \
    		 default setting, <b>Disable</b>. ";
hrouting.phase8="Static Routing,&nbsp; Destination IP Address, Subnet Mask, Gateway, and Interface";
hrouting.phase9="To set up a static route between the Router and another network, \
    		 select a number from the <i>Static Routing</i> drop-down list. (A static \
    		 route is a pre-determined pathway that network information must travel to \
    		 reach a specific host or network.)";
hrouting.phase10="Enter the following data:";
hrouting.phase11="Destination IP Address </b>- \
		  The Destination IP Address is the address of the network or host to which you want to assign a static route.";
hrouting.phase12="Subnet Mask </b>- \
		  The Subnet Mask determines which portion of an IP address is the network portion, and which \
    		  portion is the host portion. ";
hrouting.phase13="Gateway </b>- \
		  This is the IP address of the gateway device that allows for contact between the Router and the network or host.";
hrouting.phase14="Depending on where the Destination IP Address is located, select \
    		  <b>LAN &amp; Wireless </b>or <b>WAN </b>from the <i>Interface</i> drop-down menu.";
hrouting.phase15="To save your changes, click the <b>Apply </b>button. To cancel your unsaved changes, click the <b> \
    		  Cancel </b>button.";
hrouting.phase16="For additional static routes, repeat steps 1-4.";
hrouting.phase17="Delete This Entry";
hrouting.phase18="To delete a static route entry:";
hrouting.phase19="From the <i>Static Routing</i> drop-down list, select the entry number of the static route.";
hrouting.phase20="Click the <b>Delete This Entry </b>button.";
hrouting.phase21="To save a deletion, click the <b>Apply </b>button. To cancel a deletion, click the <b> \
    		  Cancel </b>button. ";
hrouting.phase22="Show Routing Table";
hrouting.phase23="Click the \
    		  <b>Show Routing Table </b>button to view all of the valid route entries in use. The Destination IP address, Subnet Mask, \
    		  Gateway, and Interface will be displayed for each entry. Click the <b>Refresh </b>button to refresh the data displayed. Click the <b> \
    		  Close</b>button to return to the <i>Routing</i> screen.";
hrouting.phase24="Destination IP Address</b> - \
		  The Destination IP Address is the address of the network or host to which the static route is assigned. ";
hrouting.phase25="Subnet Mask</b> - \
		  The Subnet Mask determines which portion of an IP address is the network portion, and which portion is the host portion.";
hrouting.phase26="Gateway</b> - This is the IP address of the gateway device that allows for contact between the Router and the network or host.";
hrouting.phase27="Interface</b> - This interface tells you whether the Destination IP Address is on the \
    		  <b> LAN &amp; Wireless </b>(internal wired and wireless networks), the <b>WAN</b> (Internet), or <b> \
    		  Loopback</b> (a dummy network in which one PC acts like a network ecessary for certain software programs). ";

//Firewall
var hfirewall = new Object();
hfirewall.phase1="Block WAN Request";
hfirewall.phase2="By enabling the Block WAN Request feature, you can prevent your network from \
    		 being \"pinged,\" or detected, by other Internet users. The Block WAN Request \
    		 feature also reinforces your network security by hiding your network ports. \
    		 Both functions of the Block WAN Request feature make it more difficult for \
    		 outside users to work their way into your network. This feature is enabled \
    		 by default. Select <b>Disable</b> to disable this feature.";
hfirewall.right="Enable or disable the SPI firewall.";

//VPN
var helpvpn = new Object();
helpvpn.phase1="VPN Pass-Through";
helpvpn.phase2="Virtual Private Networking (VPN) is typically used for work-related networking. For \
    		VPN tunnels, the Router supports IPSec Pass-Through and PPTP Pass-Through.";
helpvpn.phase3="<b>IPSec</b> - Internet Protocol Security (IPSec) is a<b> </b>suite of protocols used to implement \
      		secure exchange of packets at the IP layer. To allow IPSec tunnels to pass \
      		through the Router, IPSec Pass-Through is enabled by default. To disable \
      		IPSec Pass-Through, uncheck the box next to <i>IPSec</i>.";
helpvpn.phase4="<b>PPTP </b>- Point-to-Point Tunneling Protocol is the method used to enable VPN \
      		sessions to a Windows NT 4.0 or 2000 server. To allow PPTP tunnels to pass \
      		through the Router, PPTP Pass-Through is enabled by default. To disable \
      		PPTP Pass-Through, uncheck the box next to <i>PPTP</i>.";

helpvpn.right="You may choose to enable PPTP, L2TP of IPSec passthrough to allow your network \
		devices to communicate via VPN.";
//Internet Access
var hfilter = new Object();
hfilter.phase1="Filters";
hfilter.phase2="The <i>Filters</i> screen allows you to block or allow specific kinds of Internet \
		usage. You can set up Internet access policies for specific PCs and set up \
		filters by using network port numbers.";
hfilter.phase3="This feature allows you to customize up to ten different Internet Access Policies \
    		for particular PCs, which are identified by their IP or MAC addresses. For \
    		each policy designated PCs, during the days and time periods specified.";
hfilter.phase4="To create or edit a policy, follow these instructions:";
hfilter.phase5="Select the policy number \(1-10\) in the drop-down menu.";
hfilter.phase6="Enter a name in the <i>Enter Profile Name</i> field.";
hfilter.phase7="Click the <b>Edit List of PCs</b> button.";
hfilter.phase8="Click the <b>Apply</b> button to save your changes. Click the <b>Cancel</b> button to \
    		cancel your unsaved changes. Click the <b>Close</b> button to return to the \
    		<i>Filters</i> screen.";
hfilter.phase9="If you want to block the listed PCs from Internet access during the designated days and \
    		time, then keep the default setting, <b>Disable Internet Access for Listed \
    		PCs</b>. If you want the listed PCs to be able to access the Internet during \
    		the designated days and time, then click the radio button next to<i> Enable \
    		Internet Access for Listed PCs</i>.";
hfilter.phase10="On the <i>List of PCs</i> screen, specify PCs by IP address or MAC address. Enter the  \
    		appropriate IP addresses into the <i>IP</i> fields. If you have a range of \
    		IP addresses to filter, complete the appropriate <i>IP Range</i> fields. \
    		Enter the appropriate MAC addresses into the <i>MAC</i> fields.";
hfilter.phase11="Set the time when access will be filtered. Select <b>24 Hours</b>,<b> </b>or check the box next to <i>From</i> \
    		and use the drop-down boxes to designate a specific time period. ";
hfilter.phase12="Set the days when access will be filtered. Select <b>Everyday</b> or the appropriate days of the week. ";
hfilter.phase13="Click the <b>Add to Policy</b> button to save your changes and active it.";
hfilter.phase14="To create or edit additional policies, repeat steps 1-9.";
hfilter.phase15="To delete an Internet Access Policy, select the policy number, and click the <b>Delete</b> button.";
hfilter.phase16="To see a summary of all the policies, click the <b>Summary</b> button. The <i> \
    		Internet Policy Summary</i> screen will show each policy number, Policy \
    		Name, Days, and Time of Day. To delete a policy, click its box, and then \
    		click the <b>Delete</b> button. Click the <b>Close</b> button to return to \
    		the <i>Filters</i> screen.";
hfilter.phase17="Filtered Internet Port Range";
hfilter.phase18="To filter PCs by network port number, select <b>Both</b>, <b>TCP</b>, or <b>UDP</b>, \
    		depending on which protocols you want to filter. Then<b> </b>enter the port  \
    		numbers you want to filter into the port number fields. PCs connected to the  \
    		Router will no longer be able to access any port number listed here. To  \
    		disable a filter, select <b>Disable</b>.";

//share of help string
var hshare = new Object();
hshare.phase1="Check all the values and click <b>Save Settings</b> to save your settings. Click the <b>Cancel \
		Changes</b> button to cancel your unsaved changes.";


//DMZ
var helpdmz = new Object();
helpdmz.phase1="The DMZ hosting feature allows one local user to be exposed to the Internet for use \
		of a special-purpose service such as Internet gaming or videoconferencing. \
		DMZ hosting forwards all the ports at the same time to one PC. The Port \
    		Forwarding feature is more secure because it only opens the ports you want \
    		to have opened, while DMZ hosting opens all the ports of one computer, \
    		exposing the computer so the Internet can see it. ";    		
helpdmz.phase2="Any PC whose port is being forwarded must have its DHCP client function disabled  \
    		and should have a new static IP address assigned to it because its IP  \
    		address may change when using the DHCP function.";
/***To expose one PC, select enable.***/
helpdmz.phase3="To expose one PC, select ";
helpdmz.phase4="Enter the computer's IP address in the <i>DMZ Host IP Address</i> field.";



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
 
