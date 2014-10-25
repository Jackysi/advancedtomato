<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Basic IPv6 Forwarding</title>
<content>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,ipv6_portforward"); %>

		var fog = new TomatoGrid();

		fog.sortCompare = function(a, b) {
			var col = this.sortColumn;
			var da = a.getRowData();
			var db = b.getRowData();
			var r;

			switch (col) {
				case 0:	// on
				case 1:	// proto
				case 4:	// ports
					r = cmpInt(da[col], db[col]);
					break;
				default:
					r = cmpText(da[col], db[col]);
					break;
			}

			return this.sortAscending ? r : -r;
		}

		fog.dataToView = function(data) {
			return [(data[0] != '0') ? '<i class="icon-check icon-green"></i>' : '<i class="icon-cancel icon-red"></i>', ['TCP', 'UDP', 'Both'][data[1] - 1], (data[2].match(/(.+)-(.+)/)) ? (RegExp.$1 + ' -<br>' + RegExp.$2) : data[2], data[3], data[4], data[5]];
		}

		fog.fieldValuesToData = function(row) {
			var f = fields.getAll(row);
			return [f[0].checked ? 1 : 0, f[1].value, f[2].value, f[3].value, f[4].value, f[5].value];
		}

		fog.verifyFields = function(row, quiet) {
			var f = fields.getAll(row);

			f[2].value = f[2].value.trim();
			if ((f[2].value.length) && (!_v_iptaddr(f[2], quiet, 0, 0, 1))) return 0;

			f[3].value = f[3].value.trim();
			if ((f[3].value.length) && !v_hostname(f[3], 1)) {
				if (!v_ipv6_addr(f[3], quiet)) return 0;
			}

			if (!v_iptport(f[4], quiet)) return 0;

			f[5].value = f[5].value.replace(/>/g, '_');
			if (!v_nodelim(f[5], quiet, 'Description')) return 0;
			return 1;
		}

		fog.resetNewEditor = function() {
			var f = fields.getAll(this.newEditor);
			f[0].checked = 1;
			f[1].selectedIndex = 0;
			f[2].value = '';
			f[3].value = '';
			f[4].value = '';
			f[5].value = '';
			ferror.clearAll(fields.getAll(this.newEditor));
		}

		fog.setup = function() {
			this.init('fo-grid', 'sort', 50, [
				{ type: 'checkbox' },
				{ type: 'select', options: [[1, 'TCP'],[2, 'UDP'],[3,'Both']], class : 'input-small' },
				{ type: 'text', maxlen: 140, class : 'input-medium' },
				{ type: 'text', maxlen: 140, class : 'input-medium' },
				{ type: 'text', maxlen: 16, class : 'input-small' },
				{ type: 'text', maxlen: 32 }]);
			this.headerSet(['On', 'Proto', 'Src Address', 'Dest Address', 'Dest Ports', 'Description']);
			var nv = nvram.ipv6_portforward.split('>');
			for (var i = 0; i < nv.length; ++i) {
				var r;

				if (r = nv[i].match(/^(\d)<(\d)<(.*)<(.*)<(.+?)<(.*)$/)) {
					r[1] *= 1;
					r[2] *= 1;
					r[5] = r[5].replace(/:/g, '-');
					fog.insertData(-1, r.slice(1, 7));
				}
			}
			fog.sort(5);
			fog.showNewEditor();
		}

		function srcSort(a, b)
		{
			if (a[2].length) return -1;
			if (b[2].length) return 1;
			return 0;
		}

		function save()
		{
			if (fog.isEditing()) return;

			var data = fog.getAllData().sort(srcSort);
			var s = '';
			for (var i = 0; i < data.length; ++i) {
				data[i][4] = data[i][4].replace(/-/g, ':');
				s += data[i].join('<') + '>';
			}
			var fom = E('_fom');
			fom.ipv6_portforward.value = s;
			form.submit(fom, 0, 'tomato.cgi');
		}

		function init()
		{
			fog.recolor();
			fog.resetNewEditor();
		}
	</script>

	<form id="_fom" method="post" action="javascript:{}">
		<input type="hidden" name="_nextpage" value="/#forward-basic-ipv6.asp">
		<input type="hidden" name="_service" value="firewall-restart">
		<input type="hidden" name="ipv6_portforward">

		<div class="box">
			<div class="heading">Basic IPv6 Port-forwarding</div>
			<div class="content">
				<script type="text/javascript">show_notice1('<% notice("ip6tables"); %>');</script>
				<table class="line-table" id="fo-grid"></table><br /><hr>

				<h4>Notes</h4>
				Opens access to ports on machines inside the LAN, but does <b>not</b> re-map ports.
				<ul>
					<li><b>Src Address</b> <i>(optional)</i> - Forward only if from this address. Ex: "2001:4860:800b::/48", "me.example.com".
					<li><b>Dest Address</b> <i>(optional)</i> - The destination address inside the LAN.
					<li><b>Dest Ports</b> - The ports to be opened for forwarding. Ex: "2345", "200,300", "200-300,400".
				</ul>

			</div>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert success" style="visibility: hidden;"></span>
	</form>

	<script type="text/javascript">fog.setup(); init();</script>
</content>