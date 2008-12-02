<html>

<head>
<!-- Inserted by TRADOS: --><META HTTP-EQUIV="content-type" CONTENT="text/html; charset=GB2312">

<title>端口转发帮助</title>
<style>
<!--
 p.MsoNormal
	{mso-style-parent:"";
	margin-bottom:.0001pt;
	font-size:12.0pt;
	font-family:"Times New Roman";
	margin-left:0cm; margin-right:0cm; margin-top:0cm}
 table.MsoNormalTable
	{mso-style-parent:"";
	font-size:10.0pt;
	font-family:"Times New Roman"}
-->
</style>
</head>

<body bgColor=white>
<form>
<p class="MsoNormal"><font color="#3333FF"><b><span lang="ZH-CN" style="font-size: 16.0pt; font-family: Arial">端口转发</span><span style="font-size: 16.0pt; font-family: Arial">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; <a href="javascript:print();"><img border="0" src="../image/Printer.gif" align="center" width="82" height="44"></a></span></b></font><span lang="ZH-CN" style="font-family: Arial">&nbsp;</span></p>
<p class="MsoNormal"><span lang="ZH-CN" style="font-family: Arial"><i>端口范围转发</i>屏幕用于设置网络上的公共服务，如 Web 服务器、ftp 服务器、电子邮件服务器或其他专用的 Internet 应用。（专用的 Internet 应用程序是指使用 Internet 访问来执行像视频会议或在线游戏这种功能的任何应用程序。一些 Internet 应用程序可能不要求任何转发。） </span></p> 
<p class="MsoNormal"><span lang="ZH-CN" style="font-family: Arial">&nbsp;</span></p>
<p class="MsoNormal"><span lang="ZH-CN" style="font-family: Arial">当用户通过 Internet 将这类请求发送到网络上时，路由器会将这些请求转发到适当的 PC。任何将转发其端口的 PC 必须禁用其 DHCP 客户端功能，并且必须指定新的静态 IP 地址，因为在使用 DHCP 功能时，其 IP 地址可能更改。</span></p>
<p class="MsoNormal"><span lang="ZH-CN" style="font-family: Arial">&nbsp;</span></p>
<table class="MsoNormalTable" border="1" cellspacing="0" cellpadding="0" style="border-collapse: collapse; border: medium none">
  <tr>
    <td width="115" valign="top" style="width: 86.4pt; border: 1.0pt solid windowtext; padding-left: 5.4pt; padding-right: 5.4pt; padding-top: 0cm; padding-bottom: 0cm">
<p class="MsoNormal"><b><span lang="ZH-CN" style="font-family:Arial">定制应用程序</span></b></td> 
    <td width="475" valign="top" style="width: 356.4pt; border-left: medium none; border-right: 1.0pt solid windowtext; border-top: 1.0pt solid windowtext; border-bottom: 1.0pt solid windowtext; padding-left: 5.4pt; padding-right: 5.4pt; padding-top: 0cm; padding-bottom: 0cm">
<p class="MsoNormal"><span lang="ZH-CN" style="font-family: Arial">在提供的字段中输入公共服务或其他 Internet 应用的名称。</span></td>
  </tr>
  <tr>
    <td width="115" valign="top" style="width: 86.4pt; border-left: 1.0pt solid windowtext; border-right: 1.0pt solid windowtext; border-top: medium none; border-bottom: 1.0pt solid windowtext; padding-left: 5.4pt; padding-right: 5.4pt; padding-top: 0cm; padding-bottom: 0cm">
<p class="MsoNormal"><b><span lang="ZH-CN" style="font-family:Arial">外部</span><span lang="ZH-CN" style="font-family:
   Arial"></span><span lang="ZH-CN" style="font-family:Arial">端口</span></b></p>
    <p class="MsoNormal"><b><span lang="ZH-CN" style="font-family:Arial">&nbsp;</span></b></td>
    <td width="475" valign="top" style="width: 356.4pt; border-left: medium none; border-right: 1.0pt solid windowtext; border-top: medium none; border-bottom: 1.0pt solid windowtext; padding-left: 5.4pt; padding-right: 5.4pt; padding-top: 0cm; padding-bottom: 0cm">
<p class="MsoNormal"><span lang="ZH-CN" style="font-family: Arial">输入外部端口的编号（Internet 上的用户可以看到这些端口号）。</span></td>
  </tr>
  <tr>
    <td width="115" valign="top" style="width: 86.4pt; border-left: 1.0pt solid windowtext; border-right: 1.0pt solid windowtext; border-top: medium none; border-bottom: 1.0pt solid windowtext; padding-left: 5.4pt; padding-right: 5.4pt; padding-top: 0cm; padding-bottom: 0cm">
<p class="MsoNormal"><b><span lang="ZH-CN" style="font-family:Arial">TCP 协议</span></b></td>
    <td width="475" valign="top" style="width: 356.4pt; border-left: medium none; border-right: 1.0pt solid windowtext; border-top: medium none; border-bottom: 1.0pt solid windowtext; padding-left: 5.4pt; padding-right: 5.4pt; padding-top: 0cm; padding-bottom: 0cm">
<p class="MsoNormal"><span lang="ZH-CN" style="font-family: Arial">如果应用程序要求使用 TCP，请单击此复选框。</span></td>
  </tr>
  <tr>
    <td width="115" valign="top" style="width: 86.4pt; border-left: 1.0pt solid windowtext; border-right: 1.0pt solid windowtext; border-top: medium none; border-bottom: 1.0pt solid windowtext; padding-left: 5.4pt; padding-right: 5.4pt; padding-top: 0cm; padding-bottom: 0cm">
<p class="MsoNormal"><b><span lang="ZH-CN" style="font-family:Arial">UDP 协议</span></b></td>
    <td width="475" valign="top" style="width: 356.4pt; border-left: medium none; border-right: 1.0pt solid windowtext; border-top: medium none; border-bottom: 1.0pt solid windowtext; padding-left: 5.4pt; padding-right: 5.4pt; padding-top: 0cm; padding-bottom: 0cm">
<p class="MsoNormal"><span lang="ZH-CN" style="font-family: Arial">如果应用程序要求使用 UDP，请单击此复选框。</span></td>
  </tr>
  <tr>
    <td width="115" valign="top" style="width: 86.4pt; border-left: 1.0pt solid windowtext; border-right: 1.0pt solid windowtext; border-top: medium none; border-bottom: 1.0pt solid windowtext; padding-left: 5.4pt; padding-right: 5.4pt; padding-top: 0cm; padding-bottom: 0cm">
<p class="MsoNormal"><b><span lang="ZH-CN" style="font-family:Arial">IP 地址</span></b></td>
    <td width="475" valign="top" style="width: 356.4pt; border-left: medium none; border-right: 1.0pt solid windowtext; border-top: medium none; border-bottom: 1.0pt solid windowtext; padding-left: 5.4pt; padding-right: 5.4pt; padding-top: 0cm; padding-bottom: 0cm">
<p class="MsoNormal"><span lang="ZH-CN" style="font-family: Arial">输入运行该应用程序的 PC 的 IP 地址。</span></td>
  </tr>
  <tr>
    <td width="115" valign="top" style="width: 86.4pt; border-left: 1.0pt solid windowtext; border-right: 1.0pt solid windowtext; border-top: medium none; border-bottom: 1.0pt solid windowtext; padding-left: 5.4pt; padding-right: 5.4pt; padding-top: 0cm; padding-bottom: 0cm">
<p class="MsoNormal"><b><span lang="ZH-CN" style="font-family:Arial">启用</span></b></td>
    <td width="475" valign="top" style="width: 356.4pt; border-left: medium none; border-right: 1.0pt solid windowtext; border-top: medium none; border-bottom: 1.0pt solid windowtext; padding-left: 5.4pt; padding-right: 5.4pt; padding-top: 0cm; padding-bottom: 0cm">
<p class="MsoNormal"><span lang="ZH-CN" style="font-family: Arial">单击</span><b><span lang="ZH-CN" style="font-family:Arial">启用</span></b><span lang="ZH-CN" style="font-family: Arial">复选框可启用该应用程序的端口转发。</span></td>
  </tr>
</table>
<p class="MsoNormal"><span lang="ZH-CN" style="font-family: Arial">&nbsp;</span></p>
<p class="MsoNormal"><span lang="ZH-CN" style="font-family:Arial">选中所有值并单击<b>保存设置</b>可保存您的设置。单击<b>取消更改</b>按钮可取消未保存的更改。</span></p> 
<p class="MsoNormal">&nbsp;</p>
<p class="MsoNormal">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<center><input type="button" value="关闭" name="B3" style="font-family: Arial; font-size: 10pt" onclick=self.close()></center></p>
</form>
</body>

</html>
