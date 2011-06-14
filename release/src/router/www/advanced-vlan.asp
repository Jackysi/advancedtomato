<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2007 Jonathan Zarate
	http://www.polarcloud.com/tomato/
	For use with Tomato Firmware only.
	No part of this file may be used without permission.
	VLAN admin module by Augusto Bott
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] Advanced: VLAN</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<% css(); %>
<script type='text/javascript' src='tomato.js'></script>
<style type='text/css'>
#vlan-grid .co1,
#vlan-grid .co2,
#vlan-grid .co3,
#vlan-grid .co4,
#vlan-grid .co5,
#vlan-grid .co6,
#vlan-grid .co7,
#vlan-grid .co8,
#vlan-grid .co9,
#vlan-grid .co10,
#vlan-grid .co11,
#vlan-grid .co12,
#vlan-grid .co13 {
  text-align: center;
}
#vlan-grid .centered {
  text-align: center;
}
</style>
<script type='text/javascript'>
<% nvram ("vlan0ports,vlan1ports,vlan2ports,vlan3ports,vlan4ports,vlan5ports,vlan6ports,vlan7ports,vlan8ports,vlan9ports,vlan10ports,vlan11ports,vlan12ports,vlan13ports,vlan14ports,vlan15ports,vlan0hwname,vlan1hwname,vlan2hwname,vlan3hwname,vlan4hwname,vlan5hwname,vlan6hwname,vlan7hwname,vlan8hwname,vlan9hwname,vlan10hwname,vlan11hwname,vlan12hwname,vlan13hwname,vlan14hwname,vlan15hwname,wan_ifnameX,lan_ifnames,manual_boot_nv,boardtype");%> 

//http://www.dd-wrt.com/wiki/index.php/Hardware#Boardflags
// router/shared/id.c
//					HW_*                  boardtype    boardnum  boardrev  boardflags  others
//WRT54G 2.0		BCM4712               0x0101       42        0x10      0x0188
//WRT54GL 1.0, 1.1	BCM5352E              0x0467       42        0x10      0x2558

var port_vlan_supported = 0;
var trunk_vlan_supported = 0;

if(nvram['boardtype'] == '0x0101')
  port_vlan_supported = 1;
if(nvram['boardtype'] == '0x0467') {
  port_vlan_supported = 1;
  trunk_vlan_supported = 1;
}

MAX_PORT_ID = 4;
MAX_VLAN_ID = 15;

COL_VID = 0;
COL_P0  = 1;
COL_P0T = 2;
COL_P1  = 3;
COL_P1T = 4;
COL_P2  = 5;
COL_P2T = 6;
COL_P3  = 7;
COL_P3T = 8;
COL_P4  = 9;
COL_P4T = 10;
COL_VID_DEF = 11;
COL_BRI = 12;

// TESTED ONLY ON WRT54G v2 (boardtype 0x0101) and WRT54GL v1.1 (boardtype 0x0467)
switch(nvram['boardtype']) {
// WRT54GL v1.x, but should also work on WRT54G v4, WRT54GS v3/v4
  case '0x0467': 
    COL_P0N = '3';
    COL_P1N = '2';
    COL_P2N = '1';
    COL_P3N = '0';
    COL_P4N = '4';
    break;
// should work on WRT54G v2/v3, WRT54GS v1/v2 
  case '0x0101':
    COL_P0N = '1';
    COL_P1N = '2';
    COL_P2N = '3';
    COL_P3N = '4';
    COL_P4N = '0';
    break;
// other models are unsupported
  default:
    COL_P0N = '0';
    COL_P1N = '0';
    COL_P2N = '0';
    COL_P3N = '0';
    COL_P4N = '1';
    break;
}

function save()
{
  if (vlg.isEditing()) return;

  var fom = E('_fom');

// wipe out relevant fields just in case this is not the first time we try to submit
  for (var i = 0 ; i <= MAX_VLAN_ID ; i++) {
    fom['vlan' + i + 'ports'].value = '';
    fom['vlan' + i + 'hwname'].value = '';
  }
  fom['wan_ifnameX'].value = '';
  fom['lan_ifnames'].value = '';

  var v = '';
  var d = vlg.getAllData();

  for (var i = 0; i < d.length; ++i) {
    var p = '';
    p += (d[i][COL_P0].toString() != '0') ? COL_P0N : '';
    p += ((trunk_vlan_supported) && (d[i][COL_P0T].toString() != '0')) ? 't' : '';
    p += trailingSpace(p);

    p += (d[i][COL_P1].toString() != '0') ? COL_P1N : '';
    p += ((trunk_vlan_supported) && (d[i][COL_P1T].toString() != '0')) ? 't' : '';
    p += trailingSpace(p);

    p += (d[i][COL_P2].toString() != '0') ? COL_P2N : '';
    p += ((trunk_vlan_supported) && (d[i][COL_P2T].toString() != '0')) ? 't' : '';
    p += trailingSpace(p);

    p += (d[i][COL_P3].toString() != '0') ? COL_P3N : '';
    p += ((trunk_vlan_supported) && (d[i][COL_P3T].toString() != '0')) ? 't' : '';
    p += trailingSpace(p);

    p += (d[i][COL_P4].toString() != '0') ? COL_P4N : '';
    p += ((trunk_vlan_supported) && (d[i][COL_P4T].toString() != '0')) ? 't' : '';
    p += trailingSpace(p);

// the CPU/internal switch port (port 5) is always a member of any vlan (even if no 'regular' port is a members)
// TODO: should this be port *8* on some (gigabit) routers?
    p += (d[i][COL_VID_DEF].toString() != '0') ? '5*' : '5';

// arrange port numbers in ascending order just to be safe (not sure if this is really needed... mostly, cosmetics?)
    p = p.split(" ");
    p = p.sort(cmpInt);
    p = p.join(" ");

    v += (d[i][COL_VID_DEF].toString() != '0') ? d[i][0] : '';

    fom['vlan'+d[i][COL_VID]+'ports'].value = p;
    fom['vlan'+d[i][COL_VID]+'hwname'].value = 'et0';

    fom['wan_ifnameX'].value += (d[i][COL_BRI] == '2') ? 'vlan'+d[i][0] : '';
    fom['lan_ifnames'].value += (d[i][COL_BRI] == '1') ? 'vlan'+d[i][0] : '';
    fom['lan_ifnames'].value += trailingSpace(fom['lan_ifnames'].value);
//    alert('vlan'+d[i][0]+'ports='+fom['vlan'+d[i][0]+'ports'].value+'\nvlan'+d[i][0]+'hwname='+fom['vlan'+d[i][0]+'hwname'].value);
  }

  var lif = nvram['lan_ifnames'].split(' ');
  for (var j = 0; j < lif.length; j++) {
    fom['lan_ifnames'].value += (lif[j].indexOf('vlan') != -1) ? '' : lif[j];
    fom['lan_ifnames'].value += trailingSpace(fom['lan_ifnames'].value);
  }
//  alert(fom['lan_ifnames'].value);

  if((fom['vlan0ports'].value.length < 1) || (fom['vlan0hwname'].value.length < 1))
    fom['manual_boot_nv'].value = '1';
  else
    fom['manual_boot_nv'].value = nvram['manual_boot_nv'];

  var e = E('footer-msg');

  if(vlg.countWanPorts() != 1) {
    e.innerHTML = 'Cannot proceed: one VID must be assigned to WAN.';
    e.style.visibility = 'visible';
    setTimeout(
      function() {
        e.innerHTML = '';
        e.style.visibility = 'hidden';
      }, 5000);
    return;
  }

  if (v.length < 1) {
    e.innerHTML = 'Cannot proceed without setting a default VID';
    e.style.visibility = 'visible';
    setTimeout(
      function() {
        e.innerHTML = '';
        e.style.visibility = 'hidden';
      }, 5000);
//    alert ('One VID must be set as default');
    return;
  }

//  alert ('postaria...');
//  return;

  if (confirm("Router must be rebooted to proceed. Commit changes to NVRAM and reboot now?"))
    form.submit(fom, 0);
}

function trailingSpace(s)
{
  return ((s.length>0)&&(s.charAt(s.length-1) != ' ')) ? ' ' : '';
}

if(port_vlan_supported) {
  var vlg = new TomatoGrid();
  vlg.setup = function() {
    this.init('vlan-grid', '', (MAX_VLAN_ID + 1), [
    { type: 'select', options: [[0, '0'],[1, '1'],[2, '2'],[3, '3'],[4, '4'],[5, '5'],[6, '6'],[7, '7'],[8, '8'],[9, '9'],[10, '10'],[11, '11'],[12, '12'],[13, '13'],[14, '14'],[15, '15']], prefix: '<div class="centered">', suffix: '</div>' },
    { type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
    { type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
    { type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
    { type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
    { type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
    { type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
    { type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
    { type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
    { type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
    { type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
    { type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
    { type: 'select', options: [[1, 'LAN'],[2, 'WAN'],[3, 'none']], prefix: '<div class="centered">', suffix: '</div>' }]);
//    if (trunk_vlan_supported)
      this.headerSet(['VID', 'Port 1', 'Tagged', 'Port 2', 'Tagged', 'Port 3', 'Tagged', 'Port 4', 'Tagged', 'WAN Port', 'Tagged', 'Default', 'Role']);
//    else 
//      this.headerSet(['VID', 'Port 1', ' ', 'Port 2', ' ', 'Port 3', ' ', 'Port 4', ' ', 'WAN Port', ' ', 'Default', 'Role']);

// find out which vlans are supposed to be bridged to LAN
    var bridged = [];
    var l = nvram['lan_ifnames'].split(' ');
    for (var i = 0 ; i < l.length; i++) {
      if(l[i].indexOf('vlan') != -1)
        bridged[parseInt(l[i].replace('vlan',''))] = '1';
    }

// WAN port
    bridged[parseInt(nvram['wan_ifnameX'].replace('vlan',''))] = '2';

// go thru all possible VLANs
    for (var i = 0 ; i <= MAX_VLAN_ID ; i++) {
      var port = [];
      var tagged = [];
      if ((nvram['vlan' + i + 'hwname'].length > 0) || (nvram['vlan' + i + 'ports'].length > 0)) {
// (re)initialize our bitmap for this particular iteration
        for (var j=0; j <= MAX_PORT_ID ; j++) {
          port[j] = '0';
          tagged[j] = '0';
        }
// which ports are members of this VLAN?
        var m=nvram['vlan' + i + 'ports'].split(' ');
        for (var j = 0; j < (m.length) ; j++) {
          port[parseInt(m[j].charAt(0))] = '1';
          tagged[parseInt(m[j].charAt(0))] = ((trunk_vlan_supported) && (m[j].indexOf('t') != -1)) ? '1' : '0';
        }

        switch(nvram['boardtype']) {
// WRT54G v4, WRT54GL v1.x, WRT54GS v3/v4
          case '0x0467': 
            vlg.insertData(-1, [ i.toString(),
              port[3], tagged[3],
              port[2], tagged[2],
              port[1], tagged[1],
              port[0], tagged[0],
              port[4], tagged[4],
              (((nvram['vlan' + i + 'ports']).indexOf('*') != -1) ? '1' : '0' ),
              (bridged[i] != null) ? bridged[i] : '3' ]);
            break;
// WRT54G v2/v3, WRT54GS v1/v2
          case '0x0101':
            vlg.insertData(-1, [ i.toString(),
              port[1], tagged[1],
              port[2], tagged[2],
              port[3], tagged[3],
              port[4], tagged[4],
              port[0], tagged[0],
              (((nvram['vlan' + i + 'ports']).indexOf('*') != -1) ? '1' : '0' ),
              (bridged[i] != null) ? bridged[i] : '3' ]);
            break;
// other models are not supported
          default:
            break;
        }
      }
    }
    vlg.canDelete = false;
    vlg.sort(0);
    vlg.showNewEditor();
    vlg.resetNewEditor();
  }

  vlg.countElem = function(f, v)
  {
    var data = this.getAllData();
    var total = 0;
    for (var i = 0; i < data.length; ++i) {
      total += (data[i][f] == v) ? 1 : 0;
    }
    return total;
  }

  vlg.countDefaultVID = function()
  {
    return this.countElem(COL_VID_DEF,1);
  }

  vlg.countVID = function (v)
  {
    return this.countElem(COL_VID,v);
  }

  vlg.countWanPorts = function()
  {
    return this.countElem(COL_BRI,2);
  }

  vlg.verifyFields = function(row, quiet) {
    var valid = 1;
    var f = fields.getAll(row);
    if((trunk_vlan_supported) && (f[COL_P0].checked == 1)) {
      f[COL_P0T].disabled=0;
    } else {
      f[COL_P0T].disabled=1;
      f[COL_P0T].checked=0;
    }
    if((trunk_vlan_supported) && (f[COL_P1].checked == 1)) {
      f[COL_P1T].disabled=0;
    } else {
      f[COL_P1T].disabled=1;
      f[COL_P1T].checked=0;
    }
    if((trunk_vlan_supported) && (f[COL_P2].checked == 1)) {
      f[COL_P2T].disabled=0;
    } else {
      f[COL_P2T].disabled=1;
      f[COL_P2T].checked=0;
    }
    if((trunk_vlan_supported) && (f[COL_P3].checked == 1)) {
      f[COL_P3T].disabled=0;
    } else {
      f[COL_P3T].disabled=1;
      f[COL_P3T].checked=0;
    }
    if((trunk_vlan_supported) && (f[COL_P4].checked == 1)) {
      f[COL_P4T].disabled=0;
    } else {
      f[COL_P4T].disabled=1;
      f[COL_P4T].checked=0;
    }

    if(this.countDefaultVID() > 0) {
      f[COL_VID_DEF].disabled=1;
      f[COL_VID_DEF].checked=0;
    }

    if((this.countDefaultVID() > 0) && (f[11].checked ==1)) {
// we should never get into this part
      ferror.set(f[COL_VID_DEF], 'Only one VID can be selected as the default VID', quiet);
      valid = 0;
    } else {
      ferror.clear(f[COL_VID_DEF]);
    }

    if(this.countVID(f[COL_VID].selectedIndex) > 0) {
      ferror.set(f[COL_VID], 'Cannot add more than one entry with VID ' + f[0].selectedIndex, quiet);
      valid = 0;
    } else {
      ferror.clear(f[COL_VID]);
    }

    if ((this.countWanPorts() > 0) && (f[COL_BRI].selectedIndex == 1)) {
      ferror.set(f[COL_BRI],'Only one VID can be used as WAN at any time', quiet);
      valid = 0;
    } else {
      ferror.clear(f[COL_BRI]);
    }

    return valid;
  }

  vlg.dataToView = function(data) {
    return [data[COL_VID],
    (data[COL_P0].toString() != '0') ? 'Yes' : '',
    (data[COL_P0T].toString() != '0') ? 'On' : '',
    (data[COL_P1].toString() != '0') ? 'Yes' : '',
    (data[COL_P1T].toString() != '0') ? 'On' : '',
    (data[COL_P2].toString() != '0') ? 'Yes' : '',
    (data[COL_P2T].toString() != '0') ? 'On' : '',
    (data[COL_P3].toString() != '0') ? 'Yes' : '',
    (data[COL_P3T].toString() != '0') ? 'On' : '',
    (data[COL_P4].toString() != '0') ? 'Yes' : '',
    (data[COL_P4T].toString() != '0') ? 'On' : '',
    (data[COL_VID_DEF].toString() != '0') ? '*' : '',
    ['LAN', 'WAN', ''][data[COL_BRI] - 1]];
  }

  vlg.dataToFieldValues = function (data) {
    return [data[COL_VID],
    (data[COL_P0] != 0) ? 'checked' : '',
    (data[COL_P0T] != 0) ? 'checked' : '',
    (data[COL_P1] != 0) ? 'checked' : '',
    (data[COL_P1T] != 0) ? 'checked' : '',
    (data[COL_P2] != 0) ? 'checked' : '',
    (data[COL_P2T] != 0) ? 'checked' : '',
    (data[COL_P3] != 0) ? 'checked' : '',
    (data[COL_P3T] != 0) ? 'checked' : '',
    (data[COL_P4] != 0) ? 'checked' : '',
    (data[COL_P4T] != 0) ? 'checked' : '',
    (data[COL_VID_DEF] != 0) ? 'checked' : '',
    data[COL_BRI]];
  }

  vlg.fieldValuesToData = function(row) {
    var f = fields.getAll(row);
    return [f[COL_VID].value,
    f[COL_P0].checked ? 1 : 0,
    f[COL_P0T].checked ? 1 : 0,
    f[COL_P1].checked ? 1 : 0,
    f[COL_P1T].checked ? 1 : 0,
    f[COL_P2].checked ? 1 : 0,
    f[COL_P2T].checked ? 1 : 0,
    f[COL_P3].checked ? 1 : 0,
    f[COL_P3T].checked ? 1 : 0,
    f[COL_P4].checked ? 1 : 0,
    f[COL_P4T].checked ? 1 : 0,
    f[COL_VID_DEF].checked ? 1 : 0,
    f[COL_BRI].value];
  }

  vlg.onCancel = function() {
    this.removeEditor();
    this.showSource();
    this.disableNewEditor(false);

    this.resetNewEditor();
  }

  vlg.onAdd = function() {
    var data;

    this.moving = null;
    this.rpHide();

    if (!this.verifyFields(this.newEditor, false)) return;

    data = this.fieldValuesToData(this.newEditor);
    this.insertData(-1, data);

    this.disableNewEditor(false);
    this.resetNewEditor();

    this.resort();
  }

  vlg.onOK = function() {
    var i, data, view;

    if (!this.verifyFields(this.editor, false)) return;

    data = this.fieldValuesToData(this.editor);
    view = this.dataToView(data);

    this.source.setRowData(data);
    for (i = 0; i < this.source.cells.length; ++i) {
      this.source.cells[i].innerHTML = view[i];
    }

    this.removeEditor();
    this.showSource();
    this.disableNewEditor(false);

    this.resetNewEditor();
    this.resort();
  }

  vlg.onDelete = function() {
    this.removeEditor();
    elem.remove(this.source);
    this.source = null;
    this.disableNewEditor(false);

    this.resetNewEditor();
  }

  vlg.sortCompare = function(a, b) {
    var obj = TGO(a);
    var col = obj.sortColumn;
    if (this.sortColumn == 0) {
      var r = cmpInt(parseInt(a.cells[col].innerHTML), parseInt(b.cells[col].innerHTML));
    } else {
      var r = cmpText(a.cells[col].innerHTML, b.cells[col].innerHTML);
    }
    return obj.sortAscending ? r : -r;
  };

  vlg.resetNewEditor = function() {
    var f = fields.getAll(this.newEditor);
    if (f[COL_VID].selectedIndex < 0) 
      f[COL_VID].selectedIndex=0;
    while(this.countVID(f[COL_VID].selectedIndex) > 0)
      f[COL_VID].selectedIndex = (f[COL_VID].selectedIndex%(MAX_VLAN_ID+1))+1;
    f[COL_P0].checked = 0;
    f[COL_P0T].checked = 0;
    f[COL_P0T].disabled = 1;
    f[COL_P1].checked = 0;
    f[COL_P1T].checked = 0;
    f[COL_P1T].disabled = 1;
    f[COL_P2].checked = 0;
    f[COL_P2T].checked = 0;
    f[COL_P2T].disabled = 1;
    f[COL_P3].checked = 0;
    f[COL_P3T].checked = 0;
    f[COL_P3T].disabled = 1;
    f[COL_P4].checked = 0;
    f[COL_P4T].checked = 0;
    f[COL_P4T].disabled = 1;
    f[COL_VID_DEF].checked = 0;
    if (this.countDefaultVID()>0)
      f[COL_VID_DEF].disabled = 1;
    f[COL_BRI].selectedIndex = 2;
    ferror.clearAll(fields.getAll(this.newEditor));
  }
} // fim do tal if(supported_device)

function init()
{
  if(port_vlan_supported) {
    vlg.recolor();
    vlg.resetNewEditor();
  }
}

// TESTED ONLY ON WRT54G v2 (boardtype 0x0101) and WRT54GL v1.1 (boardtype 0x0467)
// WRT54G v4, WRT54GL v1.x, WRT54GS v3/v4 also have boardtype 0x0101, 
// so it seems reasonable to assume those models are also supported
function earlyInit()
{
  if(!port_vlan_supported) {
    E('save-button').disabled = 1;
    return;
  }
}

</script>
</head>
<body onload='init()'>
<form id='_fom' method='post' action='tomato.cgi'>
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
  <div class='title'>Tomato</div>
  <div class='version'>Version <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>
<input type='hidden' name='_nextpage' value='advanced-vlan.asp'>
<input type='hidden' name='_nextwait' value='10'>
<input type='hidden' name='_reboot' value='1'>
<input type='hidden' name='_nvset' value='1'>
<input type='hidden' name='_commit' value='1'>

<input type='hidden' name='vlan0ports'>
<input type='hidden' name='vlan1ports'>
<input type='hidden' name='vlan2ports'>
<input type='hidden' name='vlan3ports'>
<input type='hidden' name='vlan4ports'>
<input type='hidden' name='vlan5ports'>
<input type='hidden' name='vlan6ports'>
<input type='hidden' name='vlan7ports'>
<input type='hidden' name='vlan8ports'>
<input type='hidden' name='vlan9ports'>
<input type='hidden' name='vlan10ports'>
<input type='hidden' name='vlan11ports'>
<input type='hidden' name='vlan12ports'>
<input type='hidden' name='vlan13ports'>
<input type='hidden' name='vlan14ports'>
<input type='hidden' name='vlan15ports'>
<input type='hidden' name='vlan0hwname'>
<input type='hidden' name='vlan1hwname'>
<input type='hidden' name='vlan2hwname'>
<input type='hidden' name='vlan3hwname'>
<input type='hidden' name='vlan4hwname'>
<input type='hidden' name='vlan5hwname'>
<input type='hidden' name='vlan6hwname'>
<input type='hidden' name='vlan7hwname'>
<input type='hidden' name='vlan8hwname'>
<input type='hidden' name='vlan9hwname'>
<input type='hidden' name='vlan10hwname'>
<input type='hidden' name='vlan11hwname'>
<input type='hidden' name='vlan12hwname'>
<input type='hidden' name='vlan13hwname'>
<input type='hidden' name='vlan14hwname'>
<input type='hidden' name='vlan15hwname'>
<input type='hidden' name='wan_ifnameX'>
<input type='hidden' name='lan_ifnames'>
<input type='hidden' name='manual_boot_nv'>

<div id='sesdiv' style='display:none'>
<div class='section-title'>VLAN</div>
<div class='section'>
  <table class='tomato-grid' cellspacing=1 id='vlan-grid'></table>
  <script type='text/javascript'>if(port_vlan_supported) vlg.setup();</script>
</div>

<div>
<ul>
<li><b>VID</b> - Unique identifier of a VLAN.
<li><b>Ports 1-4 &amp; WAN</b> - Which ethernet ports on the router should be members of this VLAN.
<li><b>Tagged</b> - Enable 802.1q tagging of ethernet frames on a particular port/VLAN
<script type='text/javascript'>
if(!trunk_vlan_supported)
  document.write(' <i>(not supported on this model)</i>');
</script>
.
<li><b>Default</b> - VLAN ID assigned to untagged frames received by the router.
<li><b>Role</b> - Determines if this VLAN ID should be treated as WAN, part of the LAN bridge or neither (i.e. member of a 802.1q trunk, being managed manually via scripts, etc...).
</ul>
<small>
<ul>
<br>
<li><b>Other relevant notes/hints:</b>
<ul>
<li>One VID <i>must</i> be assigned to WAN.
<li>One VID <i>must</i> be selected as the default.
<li>WLAN is always/automatically bridged to LAN.
<script type='text/javascript'>
if(trunk_vlan_supported)
  document.write('<li>To prevent 802.1q compatibility issues, avoid using VID "0" as 802.1q specifies that frames with a tag of "0" do not belong to any VLAN.');
</script>
</ul>
</ul>
</small>
</div>

</div>
<script type='text/javascript'>
  if(!port_vlan_supported) 
    W('<i>This feature is not supported on this router.</i>');
  else 
    E('sesdiv').style.display = '';
</script>

</td></tr>
<tr><td id='footer' colspan=2>
 <span id='footer-msg'></span>
 <input type='button' value='Save' id='save-button' onclick='save()'>
 <input type='button' value='Cancel' id='cancel-button' onclick='javascript:reloadPage();'>
</td></tr>
</table>
</form>
<script type='text/javascript'>earlyInit()</script>
</body>
</html>
