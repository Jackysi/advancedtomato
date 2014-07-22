<!--
Tomato GUI
Copyright (C) 2007-2011 Shibby
http://openlinksys.info
For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>NFS Server</title>
<content>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,nfs_enable,nfs_exports"); %>

		var access = [['rw', 'Read/Write'], ['ro', 'Read only']];
		var sync = [['sync', 'Yes'], ['async', 'No']];
		var subtree = [['subtree_check', 'Yes'], ['no_subtree_check', 'No']];
		var nfsg = new TomatoGrid();
		nfsg.exist = function(f, v)
		{
			var data = this.getAllData();
			for (var i = 0; i < data.length; ++i) {
				if (data[i][f] == v) return true;
			}
			return false;
		}
		nfsg.dataToView = function(data) {
			return [data[0], data[1], data[2],data[3], data[4], data[5]];
		}
		nfsg.verifyFields = function(row, quiet)
		{
			var ok = 1;
			return ok;
		}
		nfsg.resetNewEditor = function() {
			var f;
			f = fields.getAll(this.newEditor);
			ferror.clearAll(f);
			f[0].value = '';
			f[1].value = '';
			f[2].selectedIndex = 0;
			f[3].selectedIndex = 0;
			f[4].selectedIndex = 1;
			f[5].value = 'no_root_squash';
		}
		nfsg.setup = function()
		{
			this.init('nfsg-grid', '', 50, [
				{ type: 'text', maxlen: 50 },
				{ type: 'text', maxlen: 30 },
				{ type: 'select', options: access },
				{ type: 'select', options: sync },
				{ type: 'select', options: subtree },
				{ type: 'text', maxlen: 50 }
			]);
			this.headerSet(['<b>Directory</b>', '<b>IP Address/Subnet</b>', '<b>Access</b>', '<b>Sync</b>', '<b>Subtree Check</b>', '<b>Other Options</b>']);
			var s = nvram.nfs_exports.split('>');
			for (var i = 0; i < s.length; ++i) {
				var t = s[i].split('<');
				if (t.length == 6) this.insertData(-1, t);
			}
			this.showNewEditor();
			this.resetNewEditor();
		}
		function save()
		{
			var data = nfsg.getAllData();
			var exports = '';
			var i;
			if (data.length != 0) exports += data[0].join('<');
			for (i = 1; i < data.length; ++i) {
				exports += '>' + data[i].join('<');
			}
			var fom = E('_fom');
			fom.nfs_enable.value = E('_f_nfs_enable').checked ? 1 : 0;
			fom.nfs_exports.value = exports;
			form.submit(fom, 1);
		}
		function init()
		{
			nfsg.recolor();
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#admin-nfs.asp">
		<input type="hidden" name="_service" value="nfs-start">
		<input type="hidden" name="nfs_enable">
		<input type="hidden" name="nfs_exports">

		<div class="box">
			<div class="heading">NFS Server</div>
			<div class="content">
				<div id="nfs-server"></div><hr><br />
				<script type="text/javascript">
					$('#nfs-server').forms([
						{ title: 'Enable NFS Server', name: 'f_nfs_enable', type: 'checkbox', value: nvram.nfs_enable != '0' }
					]);
				</script>

				<h4>Exports</h4>
				<table class="line-table" id="nfsg-grid"></table><br><hr>

				<h4>Notes</h4>
				<ul>
					<li>You can find more information on proper NFS configuration at the following website: <a href="http://nfs.sourceforge.net/nfs-howto/" target="_blanc"><b>http://nfs.sourceforge.net</b></a>.
					<li>If you want to mount an NFS share from other NFS Server, you can use the mount.nfs tool via telnet/ssh.
				</ul>
			</div>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		&nbsp; <span id="footer-msg" class="alert warning" style="visibility: hidden;"></span>

	</form>

	<script type="text/javascript">nfsg.setup(); init();</script>
</content>