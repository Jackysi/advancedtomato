<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>UPnP / NAT-PMP Forwarding</title>
<content>
    <style>textarea { width: 100%; }</style>
    <script type="text/javascript">

        /* REMOVE-BEGIN
        !!TB - additional miniupnp settings
        REMOVE-END */
        //	<% nvram("at_update,tomatoanon_answer,upnp_enable,upnp_mnp,upnp_clean,upnp_secure,upnp_clean_interval,upnp_clean_threshold,upnp_custom,upnp_lan,upnp_lan1,upnp_lan2,upnp_lan3,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,"); %>
		// <% upnpinfo(); %>

		nvram.upnp_enable = fixInt(nvram.upnp_enable, 0, 3, 0);


		function submitDelete(proto, eport)
		{
			form.submitHidden('upnp.cgi', {
				remove_proto: proto,
				remove_eport: eport,
				_redirect: '/#forward-upnp.asp' });
		}

		function deleteData(data)
		{
			if (!confirm('Delete ' + data[3] + ' ' + data[0] + ' -> ' + data[2] + ':' + data[1] + ' ?')) return;
			submitDelete(data[3], data[0]);
		}

		var ug = new TomatoGrid();

		ug.onClick = function(cell) {
			deleteData(cell.parentNode.getRowData());
		}

		ug.rpDel = function(e) {
			deleteData(PR(e).getRowData());
		}


		ug.setup = function() {
			this.init('upnp-grid', 'sort delete');
			this.headerSet(['External', '<b>Internal', '<b>Internal Address', '<b>Protocol', '<b>Description']);
			ug.populate();
		}

		ug.populate = function() {
			var i, j, r, row, data;

			if (nvram.upnp_enable != 0) {
				var data = mupnp_data.split('\n');
				for (i = 0; i < data.length; ++i) {
					r = data[i].match(/^(UDP|TCP)\s+(\d+)\s+(.+?)\s+(\d+)\s+\[(.*)\](.*)$/);
					if (r == null) continue;
					row = this.insertData(-1, [r[2], r[4], r[3], r[1], r[5]]);

					if (!r[0]) {
						for (j = 0; j < 5; ++j) {
							elem.addClass(row.cells[j], 'disabled');
						}
					}
					for (j = 0; j < 5; ++j) {
						row.cells[j].title = 'Click to delete';
					}
				}
				this.sort(2);
			}
			E('upnp-delete-all').disabled = (ug.getDataCount() == 0);
		}

		function deleteAll()
		{
			if (!confirm('Delete all entries?')) return;
			submitDelete('*', '0');
		}

		function toggleFiltersVisibility(){
			if(E('upnpsettings').style.display=='')
				E('upnpsettings').style.display='none';
			else
				E('upnpsettings').style.display='';
		}

		function verifyFields(focused, quiet)
		{
			/* REMOVE-BEGIN
			!!TB - additional miniupnp settings
			REMOVE-END */
			var enable = E('_f_enable_upnp').checked || E('_f_enable_natpmp').checked;
			var bc = E('_f_upnp_clean').checked;

			E('_f_upnp_clean').disabled = (enable == 0);
			E('_f_upnp_secure').disabled = (enable == 0);
			E('_f_upnp_mnp').disabled = (E('_f_enable_upnp').checked == 0);
			E('_upnp_clean_interval').disabled = (enable == 0) || (bc == 0);
			E('_upnp_clean_threshold').disabled = (enable == 0) || (bc == 0);
			elem.display(PR(E('_upnp_clean_interval')), (enable != 0) && (bc != 0));
			elem.display(PR(E('_upnp_clean_threshold')), (enable != 0) && (bc != 0));

			if ((enable != 0) && (bc != 0)) {
				if (!v_range('_upnp_clean_interval', quiet, 60, 65535)) return 0;
				if (!v_range('_upnp_clean_threshold', quiet, 0, 9999)) return 0;
			}
			else {
				ferror.clear(E('_upnp_clean_interval'));
				ferror.clear(E('_upnp_clean_threshold'));
			}
			/* VLAN-BEGIN */
			E('_f_upnp_lan').disabled = ((nvram.lan_ifname.length < 1) || (enable == 0));
			if (E('_f_upnp_lan').disabled)
				E('_f_upnp_lan').checked = false;
			E('_f_upnp_lan1').disabled = ((nvram.lan1_ifname.length < 1) || (enable == 0));
			if (E('_f_upnp_lan1').disabled)
				E('_f_upnp_lan1').checked = false;
			E('_f_upnp_lan2').disabled = ((nvram.lan2_ifname.length < 1) || (enable == 0));
			if (E('_f_upnp_lan2').disabled)
				E('_f_upnp_lan2').checked = false;
			E('_f_upnp_lan3').disabled = ((nvram.lan3_ifname.length < 1) || (enable == 0));
			if (E('_f_upnp_lan3').disabled)
				E('_f_upnp_lan3').checked = false;
			if ((enable) && (!E('_f_upnp_lan').checked) && (!E('_f_upnp_lan1').checked) && (!E('_f_upnp_lan2').checked) && (!E('_f_upnp_lan3').checked)) {
				if ((E('_f_enable_natpmp').checked) || (E('_f_enable_upnp').checked)) {
					var m = 'NAT-PMP or UPnP must be enabled in least one LAN bridge. You may continue but remember to configure an access to UPnP service by Custom Configuration';
					ferror.set('_f_enable_natpmp', m, quiet);
					ferror.set('_f_enable_upnp', m, 1);
					ferror.set('_f_upnp_lan', m, 1);
					ferror.set('_f_upnp_lan1', m, 1);
					ferror.set('_f_upnp_lan2', m, 1);
					ferror.set('_f_upnp_lan3', m, 1);
				}
				return 1;
			} else {
				ferror.clear('_f_enable_natpmp');
				ferror.clear('_f_enable_upnp');
				ferror.clear('_f_upnp_lan');
				ferror.clear('_f_upnp_lan1');
				ferror.clear('_f_upnp_lan2');
				ferror.clear('_f_upnp_lan3');
			}
			/* VLAN-END */
			return 1;
		}

		function save()
		{
			/* REMOVE-BEGIN
			!!TB - miniupnp
			REMOVE-END */
			if (!verifyFields(null, 0)) return;

			var fom = E('_fom');
			fom.upnp_enable.value = 0;
			if (fom.f_enable_upnp.checked) fom.upnp_enable.value = 1;
			if (fom.f_enable_natpmp.checked) fom.upnp_enable.value |= 2;

			/* REMOVE-BEGIN
			!!TB - additional miniupnp settings
			REMOVE-END */
			fom.upnp_mnp.value = E('_f_upnp_mnp').checked ? 1 : 0;
			fom.upnp_clean.value = E('_f_upnp_clean').checked ? 1 : 0;
			fom.upnp_secure.value = E('_f_upnp_secure').checked ? 1 : 0;

			/* VLAN-BEGIN */
			fom.upnp_lan.value = E('_f_upnp_lan').checked ? 1 : 0;
			fom.upnp_lan1.value = E('_f_upnp_lan1').checked ? 1 : 0;
			fom.upnp_lan2.value = E('_f_upnp_lan2').checked ? 1 : 0;
			fom.upnp_lan3.value = E('_f_upnp_lan3').checked ? 1 : 0;
			/* VLAN-END */
			form.submit(fom, 0);
		}

		function init()
		{
			ug.recolor();
		}

		/* REMOVE-BEGIN
		!!TB - miniupnp
		REMOVE-END */
		function submit_complete()
		{
			reloadPage();
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#forward-upnp.asp">
		<input type="hidden" name="_service" value="upnp-restart">

		<input type="hidden" name="upnp_enable">
		<input type="hidden" name="upnp_mnp">
		<input type="hidden" name="upnp_clean">
		<input type="hidden" name="upnp_secure">
		<!-- VLAN-BEGIN -->
		<input type="hidden" name="upnp_lan">
		<input type="hidden" name="upnp_lan1">
		<input type="hidden" name="upnp_lan2">
		<input type="hidden" name="upnp_lan3">
		<!-- VLAN-END -->

		<div class="box">
			<div class="heading">UPnP / NAT-PMP Port Forwarding</div>
			<div class="content">
				<table id="upnp-grid" class="line-table"></table><br />
				<div style="width: 100%; text-align: right"><button type="button" value="Delete All" onclick="deleteAll();" id="upnp-delete-all" class="btn btn-danger">Delete All <i class="icon-cancel"></i></button>
					<button type="button" value="Refresh" onclick="javascript:reloadPage();" class="btn"><i class="icon-refresh"></i> Refresh</button></div>
			</div>
		</div>

		<div class="box" data-box="forward-upnp-settings">
			<div class="heading">Settings</div>
			<div class="content" id="upnpsettings"></div>
			<script type="text/javascript">
				$('#upnpsettings').forms([
					{ title: 'Enable UPnP', name: 'f_enable_upnp', type: 'checkbox', value: (nvram.upnp_enable & 1) },
					{ title: 'Enable NAT-PMP', name: 'f_enable_natpmp', type: 'checkbox', value: (nvram.upnp_enable & 2) },
					/* REMOVE-BEGIN
					!!TB - additional miniupnp settings
					REMOVE-END */
					{ title: 'Inactive Rules Cleaning', name: 'f_upnp_clean', type: 'checkbox', value: (nvram.upnp_clean == '1') },
					{ title: 'Cleaning Interval', indent: 2, name: 'upnp_clean_interval', type: 'text', maxlen: 5, size: 7,
						suffix: ' <small>seconds</small>', value: nvram.upnp_clean_interval },
					{ title: 'Cleaning Threshold', indent: 2, name: 'upnp_clean_threshold', type: 'text', maxlen: 4, size: 7,
						suffix: ' <small>redirections</small>', value: nvram.upnp_clean_threshold },
					{ title: 'Secure Mode', name: 'f_upnp_secure', type: 'checkbox',
						suffix: ' &nbsp; <small>(when enabled, UPnP clients are allowed to add mappings only to their IP)</small>',
						value: (nvram.upnp_secure == '1') },
					/* VLAN-BEGIN */
					{ title: 'Enabled on' },
					{ title: 'LAN', indent: 2, name: 'f_upnp_lan', type: 'checkbox', value: (nvram.upnp_lan == '1') },
					{ title: 'LAN1', indent: 2, name: 'f_upnp_lan1', type: 'checkbox', value: (nvram.upnp_lan1 == '1') },
					{ title: 'LAN2', indent: 2, name: 'f_upnp_lan2', type: 'checkbox', value: (nvram.upnp_lan2 == '1') },
					{ title: 'LAN3', indent: 2, name: 'f_upnp_lan3', type: 'checkbox', value: (nvram.upnp_lan3 == '1') },
					/* VLAN-END */
					{ title: 'Show In My Network Places',  name: 'f_upnp_mnp',  type: 'checkbox',  value: (nvram.upnp_mnp == '1')},
                    null,
	                { title: 'Miniupnpd</a><br>Custom configuration', name: 'upnp_custom', style: 'min-height: 120px;', type: 'textarea', value: nvram.upnp_custom }
				]);
			</script>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert alert-warning" style="visibility: hidden;"></span>

	</form>
	<script type="text/javascript">ug.setup(); init(); verifyFields(null, 1);</script>
</content>