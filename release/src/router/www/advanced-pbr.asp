<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
Tomato GUI
Copyright (C) 2006-2008 Jonathan Zarate
http://www.polarcloud.com/tomato/
For use with Tomato Firmware only.
No part of this file may be used without permission.
-->
<title>MultiWAN Routing Policy</title>
<content>
	<script type="text/javascript">
		//	<% nvram("pbr_rules"); %>

		var class1 =  [[1,'WAN1'],[2,'WAN2']
			/* MULTIWAN-BEGIN */
			,[3,'WAN3'],[4,'WAN4']
			/* MULTIWAN-END */
		];

		function getOUTifdesc(outif)
		{
			var i;
			for (i=0; i<class1.length; ++i) {
				if (class1[i][0] == outif)
					return class1[i][1];
			}
			return 'unknown';
		}

		var pbr = new TomatoGrid();
		pbr.dataToView = function(data) {
			var a = [];
			var b = [];
			var c = [];
			var d = [];
			var s, i, outif;

			if (data[2] != '0') {

				if (data[3] != '') b.push(((data[2] == 1) ? 'IP ' : 'MAC ') + data[3]);
				else b.push('All');

			} else b.push('All');

			if (data[5] != '0') {
				if (data[6] != '') {
					switch (data[5]) {
						case '1':
							c.push('IP: ' + data[6]);
							break;
						case '2':
							c.push('MAC: ' + data[6]);
							break;
						case '3':
							c.push('Domain: ' + data[6]);
							break;
					}
				}
				else c.push('All');
			} else c.push('All');
			if (data[1] >= -1) {
				if (data[1] == -1) a.push('TCP/UDP');
				else if (data[1] == 6) a.push('TCP');
					else if (data[1] == 17) a.push('UDP');
						else if (data[1] == 2) a.push('ICMP');
				if (data[1] != 2) {
					if (data[4] != '') b.push('Port: ' + data[4].replace(/:/g, '-'));
					if (data[7] != '') c.push('Port: ' + data[7].replace(/:/g, '-'));
				}
			}	else a.push('All');
			d.push(getOUTifdesc(data[8] * 1));
			return [(data[0] != '0') ? 'On' : 'Off', a.join('<br>'), b.join('<br>'), c.join('<br>'), d.join('<br>'), escapeHTML(data[9])];
		}

		pbr.fieldValuesToData = function(row) {
			var f = fields.getAll(row);
			var proto = f[1].value;
			var dir = 'x';
			if ((proto != -1) && (proto != 6) && (proto != 17)) dir = 'a';
			return [f[0].checked ? 1 : 0, f[1].value, f[2].value, f[2].selectedIndex ? f[3].value : '',	(dir != 'a') ? f[4].value : '',
				f[5].value, f[5].selectedIndex ? f[6].value : '',	(dir != 'a') ? f[7].value : '',
				f[8].value, f[9].value];
		}

		// Reset Editor
		pbr.resetNewEditor = function() {

			var f = fields.getAll(this.newEditor);
			//Enable
			f[0].checked = 1;
			//Protocol
			f[1].selectedIndex = 0;
			//From
			f[2].selectedIndex = 0;
			f[3].value = '';
			f[4].value = '';
			if ((c = cookie.get('addassignout')) != null) {
				cookie.set('addassignout', '', 0);
				c = c.split(',');
				if (c.length == 1) {
					f[2].value = 1;
					f[3].value = c[0];
				}
			}
			//To
			f[5].selectedIndex = 0;
			f[6].value = '';
			f[7].value = '';
			//Interface
			f[8].selectedIndex = 0;
			f[9].value = '';
			this.enDiFields(this.newEditor);
			ferror.clearAll(fields.getAll(this.newEditor));

		}

		pbr._disableNewEditor = pbr.disableNewEditor;
		pbr.disableNewEditor = function(disable) {
			pbr._disableNewEditor(disable);
			if (!disable) {
				this.enDiFields(this.newEditor);
			}
		}

		pbr.enDiFields = function(row) {

			var f = fields.getAll(row);
			var x;
			//Enable
			//Protocol
			x = f[1].value;
			x = ((x != -1) && (x != 6) && (x != 17));
			f[4].disabled = x;
			f[7].disabled = x;
			//From
			f[3].disabled = (f[2].selectedIndex == 0);
			//To
			f[6].disabled = (f[5].selectedIndex == 0);

		}
		pbr.verifyFields = function(row, quiet) {

			var f = fields.getAll(row);
			var a, b, e;
			this.enDiFields(row);
			ferror.clearAll(f);
			//valid saddr
			a = f[2].value * 1;
			if (a == 1) {
				if (!v_iptip(f[3], quiet)) return 0;
			}
			else if ((a == 2) && (!v_mac(f[3], quiet))) return 0;
			//valid sport & dport
			b = f[1].selectedIndex;
			if ((b > 0) && (b <= 3) && (f[4].value != '') && (!v_iptport(f[4], quiet))) return 0;
			if ((b > 0) && (b <= 3) && (f[7].value != '') && (!v_iptport(f[7], quiet))) return 0;
			//valid daddr
			a = f[5].value * 1;
			switch (a ) {
				case 1: 
					if ( !v_iptip(f[6], quiet) ) return 0;
					break;
				case 2: 
					if ( !v_mac(f[6], quiet) ) return 0;
					break;
				case 3: 
					if ( !v_length(f[6], quiet) || !v_domain(f[6], quiet) ) return 0;
					break
			}

			return v_length(f[9], quiet);

		}

		pbr.setup = function() {

			var i, a, b;
			a = [[-2, 'All Protocols'],[-1,'TCP/UDP'],[6,'TCP'],[17,'UDP'],[2, 'ICMP']];
			// what a mess...
			this.init('qg', 'move', 100, [
				{ type: 'checkbox' },
				{ type: 'select', prefix: '<div class="x2a">', suffix: '</div>', options: a },
				{ multi: [
					{ type: 'select', options: [['0','All'],['1','IP'],['2','MAC']], prefix: '<div class="x1a">', suffix: '</div>' },
					{ type: 'text', prefix: '<div class="x1b">', suffix: '</div>' },
					{ type: 'text', prefix: '<div class="x1c">Port', suffix: '</div>' }
				] },
				{ multi: [
					{ type: 'select', options: [['0','All'],['1','IP'],['3','Domain']], prefix: '<div class="x1a">', suffix: '</div>' },
					{ type: 'text', prefix: '<div class="x1b">', suffix: '</div>' },
					{ type: 'text', prefix: '<div class="x1c">Port', suffix: '</div>' }
				] },
				{ type: 'select', options: class1, vtop: 1 },
				{ type: 'text', maxlen: 32, vtop: 1 }
			]);

			this.headerSet(['On', 'Protocol', 'Source Address', 'Destination Address', 'Select WAN', 'Description']);
			/*  Enable(0) < SAddrType(1) < SAddrValue(2) < ProtoType(3) < PortValue(4) < DAddrType(5) < DAddrValue(6) < ProtoType(7) <PortValue(8) < WANx(9) < Desc(10) */
			a = nvram.pbr_rules.split('>');
			if (a != '') {
				for (i = 0; i < a.length; ++i) {
					b = a[i].split('<');
					b[4] = b[4].replace(/:/g, '-');
					b[7] = b[7].replace(/:/g, '-');
					b[9] = unescape(b[9]);
					pbr.insertData(-1, b);
				}
			}

			this.showNewEditor();
			this.resetNewEditor();

		}

		function verifyFields(focused, quiet) {

			return 1;

		}

		function save() {

			if (pbr.isEditing()) return;
			var fom = E('_fom');
			var i, a, b, c;
			c = pbr.getAllData();
			a = [];
			for (i = 0; i < c.length; ++i) {
				b = c[i].slice(0);
				b[4] = b[4].replace(/-/g, ':');
				b[7] = b[7].replace(/-/g, ':');
				b[9] = escapeD(b[9]);
				a.push(b.join('<'));
			}
			fom.pbr_rules.value = a.join('>');
			form.submit(fom, 1);

		}

	</script>

    <form id="_fom" method="post" action="tomato.cgi">
        <input type="hidden" name="_nextpage" value="advanced-pbr.asp">
        <input type="hidden" name="_service" value="firewall-restart">
        <input type="hidden" name="pbr_rules">

        <div class="box">
            <div class="heading">MultiWAN Routing Policy</div>
            <div class="content">

                <table class="line-table" id="qg"></table>
                <br>Note: Policy routing traffic only work on the LAN to the Internet.

            </div>
        </div>

        <button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
        <button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
        <span id="footer-msg" class="alert alert-warning" style="visibility: hidden;"></span>

    </form>

	<script type="text/javascript">pbr.setup(); pbr.recolor();</script>
</content>
