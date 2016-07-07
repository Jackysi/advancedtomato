<title>Adblock</title>
<content>
	<!-- / / / -->
	<style type='text/css'>
		#adblockg-grid {
			width: 100%;
		}

		#adblockg-grid .co1 {
			width: 5%;
			text-align: center;
		}

		#adblockg-grid .co2 {
			width: 70%;
		}

		#adblockg-grid .co3 {
			width: 25%;
		}

		textarea {
			width: 98%;
			height: 15em;
		}
	</style>
	<script type="text/javascript">

		//<% nvram("adblock_enable,adblock_blacklist,adblock_blacklist_custom,adblock_whitelist,dnsmasq_debug"); %>

		var adblockg = new TomatoGrid();

		adblockg.exist = function( f, v ) {
			var data = this.getAllData();
			for ( var i = 0; i < data.length; ++i ) {
				if ( data[ i ][ f ] == v ) return true;
			}
			return false;
		}

		adblockg.dataToView = function( data ) {
			return [ (data[ 0 ] != '0') ? '<i class="icon-check icon-green"></i>' : '<i class="icon-cancel icon-red"></i>', data[ 1 ], data[ 2 ] ];
		}

		adblockg.fieldValuesToData = function( row ) {
			var f = fields.getAll( row );
			return [ f[ 0 ].checked ? 1 : 0, f[ 1 ].value, f[ 2 ].value ];
		}

		adblockg.verifyFields = function( row, quiet ) {
			var ok = 1;
			return ok;
		}
		function verifyFields( focused, quiet ) {
			var ok = 1;
			return ok;
		}

		adblockg.resetNewEditor = function() {
			var f;

			f = fields.getAll( this.newEditor );
			ferror.clearAll( f );
			f[ 0 ].checked = 1;
			f[ 1 ].value   = '';
			f[ 2 ].value   = '';
		}

		adblockg.setup = function() {
			this.init( 'adblockg-grid', '', 50, [
				{ type: 'checkbox' },
				{ type: 'text', maxlen: 90 },
				{ type: 'text', maxlen: 40 }
			] );
			this.headerSet( [ 'On', 'URL', 'Description' ] );
			var s = nvram.adblock_blacklist.split( '>' );
			for ( var i = 0; i < s.length; ++i ) {
				var t = s[ i ].split( '<' );
				if ( t.length == 3 ) this.insertData( -1, t );
			}
			this.showNewEditor();
			this.resetNewEditor();
		}

		function save() {
			var data      = adblockg.getAllData();
			var blacklist = '';
			for ( var i = 0; i < data.length; ++i ) {
				blacklist += data[ i ].join( '<' ) + '>';
			}

			var fom                     = E( '_fom' );
			fom.adblock_enable.value    = E( '_f_adblock_enable' ).checked ? 1 : 0;
			fom.dnsmasq_debug.value     = E( '_f_dnsmasq_debug' ).checked ? 1 : 0;
			fom.adblock_blacklist.value = blacklist;
			form.submit( fom, 1 );
		}

		// Initiate on full window load
		$( window ).on( 'load', function() {

			adblockg.recolor();
			verifyFields( null, 1 );

		});

	</script>

	<form id="_fom" method="post" action="tomato.cgi">

		<input type="hidden" name="_nextpage" value="/#advanced-adblock.asp">
		<input type="hidden" name="_service" value="adblock-restart">
		<input type="hidden" name="adblock_enable">
		<input type='hidden' name="dnsmasq_debug">
		<input type="hidden" name="adblock_blacklist">

		<div class="box">
			<div class="heading">Adblock Settings</div>
			<div class="content">

				<div class="adblock-setting"></div><hr><br>
				<script type="text/javascript">
					$( '.adblock-setting' ).forms([
								{ title: 'Enable', name: 'f_adblock_enable', type: 'checkbox', value: nvram.adblock_enable != '0' },
								{ title: 'Debug Mode', indent: 2, name: 'f_dnsmasq_debug', type: 'checkbox', value: nvram.dnsmasq_debug == '1' }
						]);
				</script>

				<h4>Blacklists</h4>
				<table class="line-table" cellspacing=1 id="adblockg-grid"></table>
				<script type="text/javascript">adblockg.setup();</script>

				<br><hr>
				<h4>Custom List</h4>
				<div class="blacklist_custom"></div><hr>
				<script type='text/javascript'>
					$( '.blacklist_custom' ).forms( [ { title: 'Blacklisted domains', name: 'adblock_blacklist_custom', type: 'textarea', value: nvram.adblock_blacklist_custom } ] );
				</script>

				<h4>Whitelist</h4>
				<div class="whitelist"></div><hr>
				<script type="text/javascript">
					$( '.whitelist' ).forms( [ { title: 'Whitelisted domains', name: 'adblock_whitelist', type: 'textarea', value: nvram.adblock_whitelist } ] );
				</script>

				<h5>Notes</h5>
				<div class='section'>
					<ul>
						<li><b>Adblock</b> - Autoupdate will be launch on 1:00AM every day
						<li><b>Debug Mode</b> - All queries to dnsmasq will be logged to syslog
						<li><b>Blacklist URL</b> - Correct file format: 0.0.0.0 domain.com or 127.0.0.1 domain.com, one domain per line
						<li><b>Blacklist Custom</b> - Optional, space separated: domain1.com domain2.com domain3.com
						<li><b>Whitelist</b> - Optional, space separated: domain1.com domain2.com domain3.com
					</ul>
				</div>

			</div>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert alert-warning" style="visibility: hidden;"></span>

	</form>


</content>