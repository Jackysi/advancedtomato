var firewall2 = new Object();
firewall2.natredir="筛选 Internet NAT 重定向";
firewall2.ident="筛选 IDENT（端口 113）";
firewall2.multi="筛选多播";

var hupgrade = new Object();
hupgrade.right1="单击“浏览”按钮可选择要上载到路由器的固件文件。";
hupgrade.right2="单击“升级”按钮可开始升级进程。升级不能中断。";
hupgrade.wrimage="不正确的图像文件！";

var hfacdef = new Object();
hfacdef.right1="这将使所有设置重置为出厂默认值。您的所有设置将被删除。";
hfacdef.warning="警告！如果单击“确定”，设备将重置为出厂默认值，并且将删除之前的所有设置。";

var hdiag = new Object();
hdiag.right1="输入要 ping 的IP 地址或域名，然后单击“Ping”按钮。";
hdiag.right2="输入要跟踪 的IP 地址或域名，然后单击“跟踪路由”按钮。";

var hlog = new Object();
hlog.right1="通过选择相应的单选按钮，可以启用或禁用“传入”和“传出”日志。";

var manage2 = new Object();
manage2.webacc="Web 访问";
manage2.accser="访问服务器";
manage2.wlacc="无线访问 Web";
manage2.vapass="确认的口令与输入的口令不匹配。请重新输入口令";
manage2.passnot="口令确认不匹配。";
manage2.selweb="至少必须选择一个 web 服务器！";
manage2.changpass="将当前路由器设置为其默认口令。出于安全考虑，在启用远程管理功能之前必须更改口令。单击“确定”按钮可更改口令。单击“取消”按钮可禁用远程管理功能。";

var hmanage2 = new Object();
hmanage2.right1="<b>本地路由器访问：</b>从此处可以更改路由器的口令。输入新的路由器口令，然后在“重新输入以进行确认”字段中重新输入以进行确认。<br>";
hmanage2.right2="<b>Web 访问：</b>允许您配置路由器 web 实用程序的访问选项。";
hmanage2.right3="<b>远程路由器访问：</b>允许您远程访问路由器。选择要使用的端口。如果路由器仍然使用其默认口令，则必须更改路由器的口令。";
hmanage2.right4="<b>UpnP：</b>由某些程序使用，以自动打开用于通信的端口。";

var hstatwl2 = new Object();
hstatwl2.right1="<b>MAC 地址</b>。这是在本地无线网络上看到的路由器的 MAC 地址。";
hstatwl2.right2="<b>模式</b>。当从“无线”选项卡中选中该选项时，将显示网络使用的无线模式（混合、仅限 G 或 禁用）。";

var hstatlan2 = new Object();
hstatlan2.right1="<b>MAC 地址</b>。这是在本地以太网上看到的路由器的 MAC 地址。";
hstatlan2.right2="<b>IP 地址</b>。该选项显示在本地以太网上看到的路由器的 IP 地址。";
hstatlan2.right3="<b>子网掩码</b>。当路由器使用子网掩码时，此处将显示子网掩码。";
hstatlan2.right4="<b>DHCP 服务器</b>。如果将路由器作为 DHCP 服务器，此处将进行显示。";

var hstatrouter2 = new Object();
hstatrouter2.wan_static="Static";
hstatrouter2.l2tp="L2TP";
//hstatrouter2.hb="心跳信号";
hstatrouter2.hb="Telstra 电缆";
hstatrouter2.connecting="正在连接";
hstatrouter2.disconnected="已断开连接";
hstatrouter2.disconnect="断开连接";
hstatrouter2.right1="<b>固件版本。</b>这是路由器的当前固件。";
hstatrouter2.right2="<b>当前时间。</b>此选项将显示在“设置”选项卡上设置的时间。";
hstatrouter2.right3="<b>MAC 地址。</b>这是您的 ISP 看到的路由器的 MAC 地址。";
hstatrouter2.right4="<b>路由器名称。</b>这是在“设置”选项卡上设置的路由器的特定名称。";
hstatrouter2.right5="<b>配置类型。</b>此选项将显示您的 ISP 连接到 Internet 所需的信息。在“设置”选项卡上输入该信息。通过单击“连接”或“断开连接”按钮，可以在此设置连接状态。";
hstatrouter2.authfail=" 身份验证失败";
hstatrouter2.noip="不能获取以下位置的 IP 地址 ";
hstatrouter2.negfail=" 协商失败";
hstatrouter2.lcpfail=" LCP 协商失败";
hstatrouter2.tcpfail="无法建立于以下对象的 TCP 连接 ";
hstatrouter2.noconn="无法连接 ";
hstatrouter2.server=" 服务器";

var hdmz2 = new Object();
hdmz2.right1="<b>DMZ：</b>启用该选项将在 Internet 上公开路由器。可以从 Internet 访问所有端口";

var hforward2 = new Object();
hforward2.right1="<b>端口范围转发：</b>某些应用程序可能要求打开特定的端口才能正确运行。这些应用程序的示例包括服务器和某些在线游戏。当从 Internet 发出对某个端口的请求时，路由器会将数据发送到指定的计算机。出于安全考虑，可能要将端口转发仅限制到正在使用的那些端口上，并且在完成后取消选中“启用”复选框。";

var hfilter2 = new Object();
hfilter2.delpolicy="是否要删除策略？";
hfilter2.right1="<b>Internet 访问策略：</b>最多可以定义 10 个访问策略。单击“删除”可删除策略，单击“摘要”可查看策略的摘要。";
hfilter2.right2="<b>状态：</b>启用或禁用策略。";
hfilter2.right3="<b>策略名称：</b>可以指定策略的名称。";
hfilter2.right4="<b>策略类型：</b>从“Internet 访问”或“入站流量”之间进行选择。";
hfilter2.right5="<b>天：</b>选择要应用策略的一周中的一天。";
hfilter2.right6="<b>时间：</b>输入要应用策略的当天的时间。";
hfilter2.right7="<b>阻止服务：</b>可以选择阻止对某些服务的访问。单击“添加/编辑服务”可修改这些设置。";
hfilter2.right8="<b>通过 URL 阻止网站：</b>通过输入网站的 URL 可以阻止对某些网站的访问。";
hfilter2.right9="<b>通过关键字阻止网站：</b>通过网页中包含的关键字可以阻止对某些网站的访问。";

var hportser2 = new Object();
hportser2.submit="应用";

var hwlad2 = new Object();
hwlad2.authtyp="身份验证类型";
hwlad2.basrate="基本速率";
hwlad2.mbps="Mbps";
hwlad2.def="默认值";
hwlad2.all="全部";
hwlad2.defdef="（默认值：默认值）";
hwlad2.fburst="帧猝发";
hwlad2.milli="毫秒";
hwlad2.range="范围";
hwlad2.frathrh="分片阈值";
hwlad2.apiso="AP 隔离";
hwlad2.off="禁用";
hwlad2.on="启用";
hwlad2.right1="<b>身份验证类型：</b>可以从自动或共享密钥进行选择。共享密钥身份验证更为安全，但是网络上的所有设备同时必须支持共享密钥身份验证。";

var hwlbas2 = new Object();
hwlbas2.right1="<b>无线网络模式：</b>在“混合”模式和“仅限 G”模式中自动启用 SpeedBooster。如果要排除 Wireless-G 客户端，请选择“仅限 B”模式。如果要禁用无线访问，请选择“禁用”。";

var hwlsec2 = new Object();
hwlsec2.wpapsk="WPA 预共享密钥";
hwlsec2.wparadius="WPA RADIUS";
hwlsec2.wpapersonal="WPA Personal";
hwlsec2.wpaenterprise="WPA Enterprise";
//new wpa2
hwlsec2.wpa2psk="仅限 WPA2 预共享密钥";
hwlsec2.wpa2radius="仅限 WPA2 RADIUS";
hwlsec2.wpa2pskmix="混合 WPA2 预共享密钥";
hwlsec2.wpa2radiusmix="混合 WPA2 RADIUS";
hwlsec2.wpa2personal="WPA2 Personal";
hwlsec2.wpa2enterprise="WPA2 Enterprise";
//new wpa2
hwlsec2.right1="<b>安全模式：</b>可以从禁用、WEP、WPA 预共享密钥、WPA RADIUS 或 RADIUS 进行选择。网络上的所有设备必须使用相同的安全模式才能进行通信。";

var hwmac2 = new Object();
hwmac2.right1="<b>MAC 地址克隆：</b>一些 ISP 将要求注册您的 MAC 地址。如果不想重新注册 MAC 地址，可以让路由器克隆向 ISP 注册的 MAC 地址。";

var hddns2 = new Object();
hddns2.right1="<b>DDNS 服务：</b>DDNS 允许您使用域名（而不是使用 IP 地址）来访问网络。服务管理不断变化的 IP 地址并动态更新域信息。必须通过 TZO.com 或 DynDNS.org 申请服务。";
hddns2.right2="Click <b><a target=_blank href=http://Linksys.tzo.com>here</a></b> to SIGNUP with a <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;TZO FREE TRIAL ACCOUNT";
hddns2.right3="Click <b><a target=_blank href=https://controlpanel.tzo.com>here</a></b> to Manage your <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;TZO Account";
hddns2.right4="Click <b><a target=_blank href=https://www.tzo.com/cgi-bin/Orders.cgi?ref=linksys>here</a></b> to Purchase a TZO <br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;DDNS Subscription";
hddns2.right5="TZO DDNS <b><a target=_blank href=http://Linksys.tzo.com>Support/Tutorials</a></b>";

var hrouting2 = new Object();
hrouting2.right1="<b>操作模式：</b>如果路由器托管 Internet 连接，请选择“网关”模式。如果网络上存在另一个路由器，则选择“路由器”模式。";
hrouting2.right2="<b>选择组编号：</b>这是唯一的路由编号，最多可以设置 20 个路由。";
hrouting2.right3="<b>路由名称：</b>输入要指定给该路由的名称。";
hrouting2.right4="<b>目标 LAN IP：</b>这是要向其指定静态路由的远程主机。";
hrouting2.right5="<b>子网掩码：</b>决定主机和网络部分。";

var hindex2 = new Object();
hindex2.telstra="Telstra 电缆";
hindex2.dns3="静态 DNS 3";
//hindex2.hbs="心跳服务器";
hindex2.hbs="Telstra 电缆";
hindex2.l2tps="L2TP 服务器";
hindex2.hdhcp="<b>自动配置 - DHCP：</b>这是小区宽带和Cable运营商最常用的设置。<br><br>";
hindex2.hpppoe1="<b>PPPoE：</b>这是 DSL 提供商最常用的设置。<br>";
hindex2.hpppoe2="<b>用户名：</b>输入 ISP 提供的用户名。<br>";
hindex2.hpppoe3="<b>口令：</b>输入 ISP 提供的口令。<br>";

//hindex2.hpppoe4="<b><a target=_blank href=help/HSetup.asp>更多...</a></b><br><br><br><br><br>";

hindex2.hstatic1="<b>静态 IP：</b>这是商务类 ISP 最常用的设置。<br>";
hindex2.hstatic2="<b>Internet IP 地址：</b>输入 ISP 提供的 IP 地址。<br>";
hindex2.hstatic3="<b>子网掩码：</b>输入子网掩码<br>";

//hindex2.hstatic4="<b><a target=_blank href=help/HSetup.asp>更多...</a></b><br><br><br><br><br><br><br>";

hindex2.hpptp1="<b>PPTP：</b>这是 DSL 提供商最常用的设置。<br>";
hindex2.hpptp2="<b>Internet IP 地址：</b>输入 ISP 提供的 IP 地址。<br>";
hindex2.hpptp3="<b>子网掩码：</b>输入子网掩码<br>";
hindex2.hpptp4="<b>网关：</b>输入 ISP 的网关地址<br>";

//hindex2.hpptp5="<b><a target=_blank href=help/HSetup.asp>更多...</a></b><br><br><br><br><br><br><br><br>";

hindex2.hl2tp1="<b>L2TP:</b>欧洲的一些 ISP 使用该设置。<br>";
hindex2.hl2tp2="<b>用户名：</b>输入 ISP 提供的用户名。<br>";
hindex2.hl2tp3="<b>口令：</b>输入 ISP 提供的口令。<br>";

//hindex2.hl2tp4="<b><a target=_blank href=help/HSetup.asp>更多...</a></b><br><br><br><br><br><br><br><br>";

hindex2.hhb1="<b>Telstra 电缆：</b>这是 DSL 提供商最常用的设置。<br>";
hindex2.hhb2="<b>用户名：</b>输入 ISP 提供的用户名。<br>";
hindex2.hhb3="<b>口令：</b>输入 ISP 提供的口令。<br>";

//hindex2.hhb4="<b><a target=_blank href=help/HSetup.asp>更多...</a></b><br><br><br><br><br><br>";

hindex2.right1="<b>主机名：</b>输入 ISP 提供的用户名。<br>";
hindex2.right2="<b>域名：</b>输入 ISP 提供的域名。<br>";
hindex2.right3="<b>本地 IP 地址：</b>这是路由器的地址。<br>";
hindex2.right4="<b>子网掩码：</b>这是路由器的子网掩码。<br><br><br>";
hindex2.right5="<b>DHCP 服务器：</b>允许路由器管理 IP 地址。<br>";
hindex2.right6="<b>起始 IP 地址：</b>作为起点的地址。<br>";
hindex2.right7="<b>时间设置：</b>选择您所在的时区。路由器也可以自动调整夏令时时间。";
hindex2.hdhcps1="<b>最大 DHCP 用户数：</b>可以限制路由器分发的地址数。<br>";

var errmsg2 = new Object();
errmsg2.err0="心跳服务器的 IP 地址无效！";
errmsg2.err1="是否要删除此项目？";
errmsg2.err2="必须输入 SSID！";
errmsg2.err3="请输入共享密钥！";
errmsg2.err4=" 非法的十六进制数字或超过 63 个字符！";
errmsg2.err5="无效的密钥，必须介于 8 个和 63 个 ASCII 字符之间或为 64 个十六进制数字";
errmsg2.err6="必须输入密钥的关键字。 ";
errmsg2.err7="无效的密钥长度 ";
errmsg2.err8="请输入密码！";
errmsg2.err9="总检查数超过 40！";
errmsg2.err10=" 不允许使用空格！";
errmsg2.err11="所有操作完成后，单击“应用”按钮可保存这些设置。";
errmsg2.err12="必须输入服务名称！";
errmsg2.err13="服务名称已经存在！";
errmsg2.err14="无效的 LAN IP 地址或子网掩码! ";

var trigger2 = new Object();
trigger2.ptrigger="端口触发";
trigger2.qos="QoS";
trigger2.trirange="触发范围";
trigger2.forrange="转发范围";
trigger2.sport="起始端口";
trigger2.eport="结束端口";
trigger2.right1="应用程序输入触发器的应用程序名称。<b>触发范围</b>列出每个应用程序触发的端口号范围。检查 Internet 应用程序文档需要的端口号。<b>起始端口</b>输入触发范围的起始端口号。<b>结束端口</b>输入触发范围的结束端口号。<b>转发范围</b>列出每个应用程序转发的端口号范围。检查 Internet 应用程序文档需要的端口号。<b>起始端口</b>输入转发范围的起始端口号。<b>结束端口</b>输入转发范围的结束端口号。";

var bakres2 = new Object();
bakres2.conman="配置管理";
bakres2.bakconf="备份配置";
bakres2.resconf="恢复配置";
bakres2.file2res="请选择要恢复的文件";
bakres2.bakbutton="备份";
bakres2.resbutton="恢复";
bakres2.right1="您可以备份当前配置，以防需要将路由器重置为其出厂默认设置。";
bakres2.right2="可以单击“备份”按钮来备份当前配置。";
bakres2.right3="单击“浏览”按钮来浏览当前保存在 PC 上的配置文件。";
bakres2.right4="单击“恢复”可以使用配置文件中的配置覆盖所有当前配置。";

var qos = new Object();
qos.uband="上行流带宽";
qos.bandwidth="带宽";
qos.dpriority="设备优先级";
qos.priority="优先级";
qos.dname="设备名称";
qos.low="低";
qos.medium="中等";
qos.high="高";
qos.highest="最高";
qos.eppriority="以太网端口优先级";
qos.flowctrl="流控制";
qos.appriority="应用程序优先级";
qos.specport="指定端口";
//qos.appname="应用程序名称";
qos.appname="应用程序名称";
qos.alert1="端口值超出范围 [0 - 65535]";
qos.alert2="起始端口值大于结束端口值";
qos.confirm1="将多个设备、以太网端口或应用程序设置为高优先级可能会使 QoS 不起作用。\n确实要继续吗？";
/*
qos.right1="WRT54G 提供两种类型的服务质量功能，即基于应用程序的服务质量和基于端口的服务质量。按照需要选择相应的类型。";
qos.right2="<b>基于应用程序的 Qos：</b>可以根据应用程序所使用的带宽来控制您的带宽。可以采用几种预先配置的应用程序。还可以通过输入应用程序使用的端口号，最多定制三个应用程序。";
qos.right3="<b>基于端口的 QoS：</b>可以根据设备插入的物理 LAN 端口控制带宽。可以对连接在 LAN 端口 1 到 端口 4 上的设备指定高优先级或低优先级。";
*/
//wireless qos
qos.optgame="优化游戏应用程序";
qos.wqos="有线 QoS";
qos.wlqos="无线 QoS";
qos.edca_ap="EDCA AP 参数";
qos.edca_sta="EDCA STA 参数";
qos.wme="WMM 支持";
qos.noack="不确认";
qos.defdis="（默认值：禁用）";
qos.cwmin="CWmin";
qos.cwmax="CWmax";
qos.aifsn="AIFSN";
qos.txopb="TXOP(b)";
qos.txopag="TXOP(a/g)";
qos.admin="管理";
qos.forced="强制";
qos.ac_bk="AC_BK";
qos.ac_be="AC_BE";
qos.ac_vi="AC_VI";
qos.ac_vo="AC_VO";


qos.right1="可以使用两种类型的服务质量功能，即有线 QoS 和无线 QoS，前者控制使用以太网电缆插入到路由器的设备，后者控制以无线方式连接到路由器的设备。"
qos.right2="<b>设备优先级：</b>您可以为网络上某个设备的所有流量指定优先级，方法是为设备指定设备名称、指定优先级及输入其 MAC 地址。"
qos.right3="<b>以太网端口优先级：</b>可以根据设备插入的物理 LAN 端口控制数据速率。可以对连接在 LAN 端口 1 到端口 4 上的设备的数据流量指定高优先级或低优先级。"
qos.right4="<b>应用程序优先级：</b>可以根据应用程序所使用的带宽来控制数据速率。选中“优化游戏应用程序”可使通用的游戏应用程序端口自动拥有较高的优先级。可以通过输入应用程序使用的端口号，最多定制八个应用程序。"
qos.right5="Wi-Fi Alliance<sup>TM</sup> 还将无线 QoS 称为 <b>Wi-Fi MultiMedia<sup>TM</sup> (WMM)</b>。如果您使用的是其他也经过 WMM 认证的无线设备，则选择“启用”即可使用 WMM。"
qos.right6="<b>不确认：</b>如果您要禁用确认，则启用此选项。如果启用此选项，则路由器在发生错误时不会重新发送数据。"


var vpn2 = new Object();
vpn2.right1="可以选择启用 PPTP、IPSec 通过的 L2TP，以允许网络设备通过 VPN 进行通信。";

// for parental control

var pactrl = new Object();
pactrl.pactrl ="父级控制";
pactrl.accsign ="帐户申请";
pactrl.center1 ="Linksys 父级控制解决方案可以确保<br>您的家人在 Internet 上冲浪时高枕无忧。";
pactrl.center2 ="<li>易于设置</li><br><li>通过 Linksys 路由器保护家中的每台计算机</li><br><li>报告有助于您监视 web、电子邮件和 IM 的使用情况</li>";
pactrl.center3 ="** 申请该服务将禁用路由器的内置 Internet 访问策略。";
pactrl.manageacc ="管理帐户";
pactrl.center4 ="管理父级控制帐户";
pactrl.signparental ="申请父级控制服务";
pactrl.moreinfo ="更多信息";
pactrl.isprovision ="设备已预配";
pactrl.notprovision ="设备未预配";
pactrl.right1 ="父级控制屏幕允许您申请和管理 Linksys 父级控制帐户。Linksys 父级控制服务为您提供了功能强大的工具，不仅可以控制 Internet 服务、访问和功能的可用性，而且还能够您家庭的每个成员进行定制。";

var satusroute = new Object();
satusroute.localtime ="不可用";

var succ = new Object();
succ.autoreturn ="几秒钟后，您将返回到前一页面。";
