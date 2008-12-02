//Basic Setup
var hsetup = new Object();
hsetup.phase1="<i>设置</i>\
		屏幕是访问路由器时看到的第一个屏幕。大多数用户\
		可以在此配置路由器并仅使用该屏幕上的设置\
		正确运行。一些 Internet 服务提供商 (ISP) 将要求\
		您输入指定信息，如用户名、口令、IP 地址、\
		默认网关地址或 DNS IP 地址。如果需要，可以从 ISP 获得\
		该信息。";
hsetup.phase2="注意：配置这些设置之后，\
		应该使用<i>安全性</i>屏幕\
		为该路由器设置新口令。从而可以增加安全性，以\
		防止路由器进行未授权的更改。对于尝试访问该路由器基于 web 的实用程序或安装向导的所有用户，\
		将提示他们输入路由器的口令。";
hsetup.phase3="选择\
		您所在位置的时区。如果您所在的位置使用夏令时，\
		则选中<i>自动调整\
		夏令时更改时钟</i>旁边框中的复选标记。";
hsetup.phase4="MTU 为\
		最大传输单元。该设置指定 Internet 传输允许的\
		最大包大小。保持默认设置“自动”，\
		以使路由器为 Internet 连接选择最佳 MTU。要指定\
		MTU 大小，请选择“手动”，然后输入所需的值（默认设置为\
		“1400”）。&nbsp;应该使该值在 1200 到 1500 范围之间。";
hsetup.phase5="该项目对于某些 ISP 是必要的，并且可以由他们提供。";
hsetup.phase6="路由器支持四种连接类型：";
hsetup.phase7="自动配置 DHCP";
hsetup.phase8="（以太网上的点到点协议）";
hsetup.phase9="（点到点隧道协议）";
hsetup.phase10="可以从“Internet 连接”旁边的下拉菜单中选择这些类型。 \
		必需的信息和可用功能取决于\
		所选择的连接类型。此处包含该信息的\
		一些说明：";		
hsetup.phase11="Internet IP 地址和子网掩码";
hsetup.phase12="这是路由器的 IP 地址\
		和子网掩码，Internet 上的外部用户\
		（包括 ISP）可以看到这些信息。如果 Internet 连接需要静态 IP 地址，则\
		ISP 将为您提供静态 IP 地址和子网掩码。";
hsetup.phase13="ISP 将为您提供网关 IP 地址。"
hsetup.phase14="（域名服务器）";
hsetup.phase15="ISP 将至少为您提供一个 DNS IP 地址。";
hsetup.phase16="用户名和口令";
hsetup.phase17="通过 PPPoE 或 PPTP 连接登录到 ISP时，\
		输入所使用的“用户名”和\
		“口令”。";
hsetup.phase18="按需连接";
hsetup.phase19="可以将路由器配置为\
		在指定的不活动时段（最大空闲时间）之后，断开 Internet\
		连接。如果 Internet 连接由于\
		不活动而终止，则只要再次访问 Internet，“按需连接”将使路由器自动启用\
		重新建立\
		连接。如果要激活“按需连接”，请单击单选按钮。如果\
		要使 Internet 连接一直保持活动状态，请\
		在<i>最大空闲时间</i>字段中输入“0”。否则，输入\
		在 Internet 连接终止之前经过的时间（分钟数）。";
hsetup.phase20="保持活跃选项 ";
hsetup.phase21="该选项可以确保您持续\
		连接到 Internet，即使连接站点处于空闲状态也保持连接状态。要使用\
		该选项，请单击<i>保持活跃</i>旁边的单选按钮。默认\
		重新拨号周期为 30 秒（换句话说，路由器将每 30 秒检查一次\
		Internet 连接）。";
hsetup.phase22="注意：一些\
		电缆提供商需要特定的 MAC 地址才能连接到\
		Internet 上。要了解这种情况的详细信息，请单击“系统”选项卡。然后单击\
		“帮助”按钮，并读取关于 MAC 克隆功能的信息。";
hsetup.phase23="LAN";
hsetup.phase24="IP 地址和子网掩码";
hsetup.phase25="这是\
		在内部 LAN 上看到的路由器 IP 地址和子网掩码。IP 地址的默认值\
		为 192.168.1.1，子网掩码的默认值为 \
		255.255.255.0。";
hsetup.phase26="DHCP";
hsetup.phase27="保持\
		默认设置“启用”可启用路由器的 DHCP 服务器选项。如果\
		网络上已经存在一个 DHCP 服务器或者您不想使用 DHCP 服务器，\
		则选择“禁用”。";
hsetup.phase28="发布 IP 地址时，\
		应输入 DHCP 服务器开头的数值。 \
		不要以 192.168.1.1（路由器的 IP 地址）开头。";
hsetup.phase29="最大 DHCP 用户数";
hsetup.phase30="输入\
		要使 DHCP 服务器指定 IP 地址的\
		最大 PC 数。绝对最大值为 253 -- 如果起始 IP 地址为 192.168.1.2，\
		则该值可能为绝对最大值。";
hsetup.phase31="客户端\
		租用时间是允许网络用户使用其当前动态 IP 地址\
		连接到路由器的时间。输入\
		用户将使用该动态 IP 地址\"leased\" 的时间（以分钟为单位）。";
hsetup.phase32="静态 DNS 1-3 ";
hsetup.phase33="域名\
		系统 (DNS) 是 Internet 将域名或网站名称\
		转换为 Internet 地址或 URL 的方式。ISP 将为您至少提供一个 \
		DNS 服务器 IP 地址。如果要利用另一个地址，请在这些字段之一输入 IP \
		地址。此处最多可以输入三个 DNS 服务器 IP \
		地址。路由器将利用这些地址来更快速地访问正在运行的 DNS \
		服务器。";
hsetup.phase34="Windows Internet 命名服务 (WINS) 管理每个 PC 与\
		Internet 的交互。如果使用 WINS 服务器，请在此处输入服务器的 IP 地址。 \
		否则，保留此字段为空白。";
hsetup.phase35="检查所有\
		值并单击“保存设置”来保存您的设置。单击\
		“取消更改”\
		可取消未保存的更改。通过连接到 Internet 可以\
		测试这些设置。 ";    		    		    		

//DDNS
var helpddns = new Object();
helpddns.phase1="路由器提供动态域名系统 (DDNS) 功能。DDNS 可以向动态 Internet IP 地址\
		指定一个固定的主机和域名。当您在路由器后面托管您自己的\
		网站、FTP 服务器或其他服务器时，该功能非常有用。使用该功能之前，\
		需要在 <i>www.dyndns.org</i>（一个 DDNS 服务提供商）上\
		申请 DDNS 服务。";
helpddns.phase2="DDNS 服务";
helpddns.phase3="要禁用 DDNS 服务，请保持默认设置“禁用”。要启用 DDNS \
		服务，请遵循以下指导：";
helpddns.phase4="在 <i>www.dyndns.org</i> 上申请 DDNS 服务，并写下\
		您的用户名、<i></i>口令和<i></i>主机名信息。";
helpddns.phase5="在 <i>DDNS</i> 屏幕上，请选择“启用”。";
helpddns.phase6="填写<i>用户名</i>、<i>口令</i>和<i>主机名</i>字段。";
helpddns.phase7="单击“保存设置”按钮来保存所做的更改。单击“取消更改”按钮可\
		取消未保存的更改。";
helpddns.phase8="此处显示路由器当前的 Internet IP 地址。";
helpddns.phase9="此处显示 DDNS 服务连接的状态。";
		
//MAC Address Clone
var helpmac =  new Object();
helpmac.phase1="MAC 克隆";
helpmac.phase2="路由器的 MAC 地址是 12 个数字的代码，该代码指定给硬件的唯一片段，\
		以便进行标识。一些 ISP 要求注册网卡/适配器的 MAC \
		地址，该网卡/适配器在安装期间连接到电缆或\
		DSL 调制解调器。如果您的 ISP 要求进行 MAC 地址\
		注册，则按照 PC 操作系统的指导来查找适配器\
		的 MAC 地址。";
helpmac.phase3="对于 Windows 98 和 Millennium：";
helpmac.phase4="1.  单击“开始”按钮，并选择“运行”。";
helpmac.phase5="2.  在提供的字段中输入“winipcfg”，然后按“确定”键。";
helpmac.phase6="3.  选择所使用的以太网适配器。";
helpmac.phase7="4.  单击“更多信息”。";
helpmac.phase8="5.  写下适配器的 MAC 地址。";
helpmac.phase9="1.  单击“开始”按钮，并选择“运行”。";
helpmac.phase10="2.  在提供的字段中输入“cmd”，然后按“确定”键。";
helpmac.phase11="3.  在命令提示符上，运行“ipconfig/all”，并查看适配器的物理地址。";
helpmac.phase12="4.  写下适配器的 MAC 地址。";
helpmac.phase13="要将网络适配器的 MAC 地址克隆到路由器上，并且不想请求\
		ISP 来更改注册的 MAC 地址，请遵循以下指导：";
helpmac.phase14="对于 Windows 2000 和 XP：";
helpmac.phase15="1.  选择“启用”。";
helpmac.phase16="2.  在 <i>MAC 地址</i>字段中输入适配器的 MAC 地址。";
helpmac.phase17="3.  单击“保存设置”按钮。";
helpmac.phase18="要禁用 MAC 地址克隆，请保持默认设置“禁用”。";

//Advance Routing
var hrouting = new Object();
hrouting.phase1="路由";
hrouting.phase2="在<i>路由</i>屏幕上，可以设置路由模式和路由器的设置。 \
		 对于大多数用户，建议使用网关模式。";
hrouting.phase3="选择正确的工作模式。如果路由器托管网络与 Internet 的连接，则保持默认设置\
		 “网关”（对于大多数用户，建议使用网关模式）。如果\
		 网络上存在该路由器和其他路由器，则选择“路由器”。";
hrouting.phase4="动态路由 (RIP)";
hrouting.phase5="注意：该功能在网关模式下不可用。";
hrouting.phase6="动态路由使路由器能够针对网络布局中的物理更改进行自动调整，\
		 并且与其他路由器交换路由表。路由器\
		 基于源和目标之间的最小跳数确定\
		 网络包路由。 ";
hrouting.phase7="要对 WAN 端启用动态路由功能，请选择“WAN”。 \
		 要对 LAN 和无线端启用该功能，请选择“LAN 和无线”。 \
		 要对 WAN 和 LAN 同时启用该功能，请选择\
		 “两者”。要对所有数据传输启用动态路由功能，请保持\
		 默认设置“禁用”。 ";
hrouting.phase8="静态路由、目标 IP 地址、子网掩码、网关和接口";
hrouting.phase9="要在路由器和另一个网络之间设置静态路由，\
		 请从<i>静态路由</i>下拉列表中选择一个编号。（静态路由\
		 是网络信息必须传输到特定主机或网络而\
		 预先确定的路径。）";
hrouting.phase10="输入以下数据：";
hrouting.phase11="目标 IP 地址- \
		  目标 IP 地址是要指定静态路由的网络或主机的地址。";
hrouting.phase12="子网掩码 - \
		  子网掩码确定 IP 地址的哪个部分是网络部分，\
		  哪个部分是主机部分。 ";
hrouting.phase13="网关 - \
		  这是允许路由器和网络或主机之间进行联系的网关设备的 IP 地址。";
hrouting.phase14="根据目标 IP 地址所在的位置，\
		  从<i>接口</i>下拉菜单中，选择“LAN 和无线”或“WAN”。";
hrouting.phase15="要保存所做的更改，请单击“应用”按钮。要取消未保存的更改，请单击\
		  “取消”按钮。";
hrouting.phase16="对于其他静态路由，请重复步骤 1 到 4。";
hrouting.phase17="删除此项目";
hrouting.phase18="要删除静态路由项目，请执行以下操作：";
hrouting.phase19="从<i>静态路由</i>下拉列表中，选择静态路由的项目编号。";
hrouting.phase20="单击“删除此项目”按钮。";
hrouting.phase21="要保存删除结果，请单击“应用”按钮。要取消删除，请单击\
		  “取消”按钮。 ";
hrouting.phase22="显示路由表";
hrouting.phase23="单击\
		  “显示路由表”按钮可查看正在使用的所有有效路由项目。将显示每个项目的目标 IP 地址、子网掩码、\
		  网关和接口。单击“刷新”按钮可刷新显示的数据。单击\
		  “关闭”按钮可返回到<i>路由</i>屏幕。";
hrouting.phase24="目标 IP 地址 - \
		  目标 IP 地址是指定静态路由的网络或主机的地址。 ";
hrouting.phase25="子网掩码 - \
		  子网掩码确定 IP 地址的哪个部分是网络部分，哪个部分是主机部分。";
hrouting.phase26="网关 - 这是允许路由器和网络或主机之间进行联系的网关设备的 IP 地址。";
hrouting.phase27="接口 - 该接口将告诉您目标 IP 地址是\
		  “LAN 和无线”（内部有线网络和无线网络）的地址、“WAN”（Internet）的地址，还是\
		  “回送”的地址（一个虚拟网络，其中 PC 的行为类似于某些软件程序所需的网络）。 ";

//Firewall
var hfirewall = new Object();
hfirewall.phase1="阻止 WAN 请求";
hfirewall.phase2="通过启用阻止 WAN 请求功能，可以防止网络\
		 由其他 Internet 用户执行 \"ping\" 操作或检测。阻止 WAN 请求\
		 功能通过隐藏网络端口也可以加强网络的安全性。 \
		 使用这两种阻止 WAN 请求的功能可以使\
		 外部用户以自己的方式进入您的网络难上加难。默认情况下，\
		 该功能处于启用状态。选择“禁用”可禁用该功能。";
hfirewall.right="启用或禁用 SPI 防火墙。";

//VPN
var helpvpn = new Object();
helpvpn.phase1="VPN 通过";
helpvpn.phase2="虚拟专用网 (VPN) 通常用于与工作相关的网络。对于\
		VPN 隧道，路由器支持 IPSec 通过和 PPTP 通过。";
helpvpn.phase3="<b>IPSec</b> - Internet 协议安全 (IPSec) 是一<b></b>套用于\
		在 IP 层上执行安全数据包交换的协议。要允许 IPSec 隧道\
		通过路由器，默认情况下应启用 IPSec 通过。要禁用\
		IPSec 通过，请取消选中 <i>IPSec</i> 旁边的框。";
helpvpn.phase4="<b>PPTP </b>- 点到点隧道协议是用于使 VPN \
		与 Windows NT 4.0 或 2000 Server 进行会话的方法。要允许 PPTP 隧道\
		通过路由器，默认情况下应启用 PPTP 通过。要禁用\
		PPTP 通过，请取消选中 <i>PPTP</i> 旁边的框。";

helpvpn.right="可以选择启用 PPTP、IPSec 通过的 L2TP，以允许网络\
		设备通过 VPN 进行通信。";
//Internet Access
var hfilter = new Object();
hfilter.phase1="筛选器";
hfilter.phase2="通过<i>筛选器</i>屏幕可以阻止或允许指定类型的 Internet \
		用法。通过使用网络端口号，可以设置特定 PC 的 Internet 访问策略和\
		筛选器。";
hfilter.phase3="该功能允许您对特定 PC 最多定制十个不同的 Internet 访问策略，\
		使用其 IP 地址或 MAC 地址进行标识。对于\
		每个策略指定的 PC，将指定日期和时段。";
hfilter.phase4="要创建或编辑策略，请遵循以下指导：";
hfilter.phase5="在下拉菜单中选择策略编号 \(1-10\)。";
hfilter.phase6="在<i>输入配置文件名称</i>字段中输入名称。";
hfilter.phase7="单击“编辑 PC 列表”按钮。";
hfilter.phase8="单击“应用”按钮可保存所做的更改。单击“取消”按钮\
		可取消未保存的更改。单击“关闭”按钮可返回到\
		<i>筛选器</i>屏幕。";
hfilter.phase9="如果要在指定日期和时间期内阻止列出的 PC 进行 Internet 访问，\
		则保持默认设置“禁止列出的 PC 进行 Internet \
		访问”。如果要在指定日期和时间内使列出的 PC 可以访问 Internet，\
		则单击<i>使列出的 PC 可以进行\
		Internet 访问</i>旁边的单选按钮。";
hfilter.phase10="在 <i>PC 列表</i>屏幕上，通过 IP 地址或 MAC 地址指定 PC。在 <i>IP</i> 字段中输入\
		相应的 IP 地址。如果要筛选\
		IP 地址的范围，请完成相应的 <i>IP 范围</i>字段。 \
		在 <i>MAC</i> 字段中输入相应的 MAC 地址。";
hfilter.phase11="设置将筛选访问的时间。选择“24 小时”，<b></b>或选中<i>从</i>旁边的框\
		并且使用下拉框来指定特定时段。 ";
hfilter.phase12="设置将筛选访问的日期。选择“每天”或一周中适当的几天。 ";
hfilter.phase13="单击“添加到策略”按钮可保存所做的更改并将其激活。";
hfilter.phase14="要创建或编辑其他策略，请重复步骤 1 到 9。";
hfilter.phase15="要删除 Internet 访问策略，请选择策略编号，然后单击“删除”按钮。";
hfilter.phase16="要查看所有策略的摘要信息，请单击“摘要”按钮。</i>\
		Internet 策略摘要</i>屏幕将显示每个策略编号、策略名称、\
		日期和每天的时间。要删除策略，请单击其框，然后\
		单击“删除”按钮。单击“关闭”按钮可返回到\
		<i>筛选器</i>屏幕。";
hfilter.phase17="筛选 Internet 端口范围";
hfilter.phase18="要通过网络端口号筛选 PC，请选择“两者”、“TCP”或“UDP”，\
		具体情况取决于要筛选的协议。然后在端口号字段中<b></b>输入要\
		筛选的端口号。连接到路由器的 PC \
		无法再访问此处列出的任何端口号。要\
		禁用筛选器，请选择“禁用”。";

//share of help string
var hshare = new Object();
hshare.phase1="选中所有值并单击“保存设置”可保存您的设置。单击“取消更改”\
		按钮可取消未保存的更改。";


//DMZ
var helpdmz = new Object();
helpdmz.phase1="DMZ 托管功能可以针对特殊用途的服务（如 Internet 游戏和视频会议）\
		将一个本地用户在 Internet 上公开。 \
		DMZ 托管同时向一个 PC 转发所有端口。端口转发功能\
		更为安全，因为该功能只打开想要打开的端口，\
		而 DMZ 托管将打开一个计算机的所有端口，\
		从而将该计算机在 Internet 上公开。 ";    		
helpdmz.phase2="正在转发其端口的任何 PC 必须禁用其 DHCP 客户端功能，\
		并且应该指定一个新的静态 IP 地址，因为其 IP 地址\
		在使用 DHCP 功能时可能发生更改。";
/***To expose one PC, select enable.***/
helpdmz.phase3="要公开一个 PC，请选择 ";
helpdmz.phase4="在 <i>DMZ 主机 IP 地址</i>字段中输入计算机的 IP 地址。";



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
 
