<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Router Scripts</title>
<content>
	<style type="text/css">
		.as-script {
			font: 12px monospace;
			width: 85%;
			height: 400px;
			overflow: scroll;
			border: 1px solid #eee;
			border-top: none;
		}
	</style>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,script_init,script_shut,script_fire,script_wanup"); %>

		tabs = [['as-init', 'Init'],['as-shut', 'Shutdown'],['as-fire','Firewall'],['as-wanup', 'WAN Up']];

		function tabSelect(name)
		{
			tabHigh(name);
			for (var i = 0; i < tabs.length; ++i) {
				var on = (name == tabs[i][0]);
				elem.display(tabs[i][0] + '-text', on);
			}
			if (i >= tabs.length) return;
			E(name + '-text').focus();
			cookie.set('scripts_tab', name)
		}

		function wordWrap()
		{
			for (var i = 0; i < tabs.length; ++i) {
				var e = E(tabs[i][0] + '-text');
				var s = e.value;
				var c = e.cloneNode(false);
				wrap = E('as-wordwrap').checked;
				c.setAttribute('wrap', wrap ? 'virtual' : 'off');
				e.parentNode.replaceChild(c, e);
				c.value = s;
			}
		}

		function save()
		{
			var i, t, n, x;

			for (i = 0; i < tabs.length; ++i) {
				t = tabs[i];
				n = E(t[0] + '-text').value.length;
				x = (t[0] == 'as-fire') ? 8192 : 4096;
				if (n > x) {
					tabSelect(t[0]);
					alert(t[1] + ' script is too long. Maximum allowed is ' + x + ' bytes.');
					return;
				}
			}
			form.submit('_fom', 1);
		}

		function earlyInit()
		{
			for (var i = 0; i < tabs.length; ++i) {
				var t = tabs[i][0];
				E(t + '-text').value = nvram['script_' + t.replace('as-', '')];
			}
			tabSelect(cookie.get('scripts_tab') || 'as-init');
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">
	<input type="hidden" name="_nextpage" value="/#admin-scripts.asp">

	<script type="text/javascript">
		$('.scriptsetc').append(tabCreate.apply(this, tabs));

		wrap = cookie.get('scripts_wrap') || 0;
		y = Math.floor(docu.getViewSize().height * 0.65);
		s = 'height:' + ((y > 300) ? y : 300) + 'px;display:none';
		for (i = 0; i < tabs.length; ++i) {
			t = tabs[i][0];
			$('.scriptsetc').append('<textarea class="as-script" name="script_' + t.replace('as-', '') + '" id="' + t + '-text" wrap=' + (wrap ? 'virtual' : 'off') + ' style="max-width:100%; min-width: 100%; margin: 0; ' + s + '"></textarea>');
		}
		$('.scriptsetc').append('<br /><br /><div class="checkbox c-checkbox"><label><input type="checkbox" id="as-wordwrap" onclick="wordWrap()" onchange="wordWrap()" ' +
			(wrap ? 'checked' : '') + '><span class="icon-check"></span> &nbsp; Word Wrap</label></div> ');
	</script>

	<div class="box">
		<div class="heading">Event Scripts</div>
		<div class="content scriptsetc" style="display: block;"></div>
	</div>

	<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
	<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
	&nbsp; <span id="footer-msg" class="alert warning" style="visibility: hidden;"></span>

	<script type="text/javascript">earlyInit();</script>
</content>