// List of available ciphers
var vpnciphers = ['DES-CFB','DES-CBC','IDEA-CBC','IDEA-CFB','RC2-CBC','RC2-CFB','RC2-OFB','DES-EDE-CBC','DES-EDE3-CBC','DES-OFB','IDEA-OFB','DES-EDE-CFB','DES-EDE3-CFB','DES-EDE-OFB','DES-EDE3-OFB','DESX-CBC','BF-CBC','BF-CFB','BF-OFB','RC2-40-CBC','CAST5-CBC','CAST5-CFB','CAST5-OFB','RC5-CBC','RC5-CFB','RC5-OFB','RC2-64-CBC'];

// Handles parsing status files and displaying the information
function StatusUpdater(clients, routes, stats, time, cDiv, ncDiv, eDiv)
{
	this.init(clients, routes, stats, time, cDiv, ncDiv, eDiv);
	return this;
}
StatusUpdater.prototype =
{
	init: function(clients, routes, stats, time, cDiv, ncDiv, eDiv)
	{
		this.clientTable = clients? new TomatoGrid(clients,'sort',0,null): null;
		this.routeTable = routes? new TomatoGrid(routes,'sort',0,null): null;
		this.statTable = stats? new TomatoGrid(stats,'sort',0,null): null;
		this.statusTime = time? E(time): null;
		this.content = cDiv? E(cDiv): null;
		this.noContent = ncDiv? E(ncDiv): null;
		this.errors = eDiv? E(eDiv): null;
	},
	update: function(text)
	{
		if(this.errors) this.errors.innerHTML = '';
		if(this.noContent) this.noContent.style.display = (text==''?'':'none');
		if(this.content) this.content.style.display = (text==''?'none':'');

		if(this.clientTable) this.clientTable.tb.parentNode.style.display = 'none';
		if(this.clientTable) this.clientTable.removeAllData();
		if(this.routeTable) this.routeTable.tb.parentNode.style.display = 'none';
		if(this.routeTable) this.routeTable.removeAllData();
		if(this.statTable) this.statTable.tb.parentNode.style.display = 'none';
		if(this.statTable) this.statTable.removeAllData();

		if(this.statTable) this.statTable.headerSet(['Name','Value']);

		var lines = text.split('\n');
		var staticStats = false;
		for (i = 0; text != '' && i < lines.length; ++i)
		{
			var done = false;

			var fields = lines[i].split(',');
			if ( fields.length == 0 ) continue;
			switch ( fields[0] )
			{
			case "TITLE":
				break;
			case "TIME":
				if(this.statusTime) this.statusTime.innerHTML = fields[1];
				break;
			case "HEADER":
				switch ( fields[1] )
				{
				case "CLIENT_LIST":
					if(this.clientTable) this.clientTable.headerSet(fields.slice(2,fields.length-1));
					break;
				case "ROUTING_TABLE":
					if(this.routeTable) this.routeTable.headerSet(fields.slice(2,fields.length-1));
					break;
				default:
					if(this.errors) this.errors.innerHTML += 'Unknown header: '+lines[i]+'<br>';
					break;
				}
				break;
			case "CLIENT_LIST":
				if(this.clientTable) this.clientTable.tb.parentNode.style.display = '';
				if(this.clientTable) this.clientTable.insertData(-1, fields.slice(1,fields.length-1))
				break;
			case "ROUTING_TABLE":
				if(this.routeTable) this.routeTable.tb.parentNode.style.display = '';
				if(this.routeTable) this.routeTable.insertData(-1, fields.slice(1,fields.length-1))
				break;
			case "GLOBAL_STATS":
				if(this.statTable) this.statTable.tb.parentNode.style.display = '';
				if(this.statTable) this.statTable.insertData(-1, fields.slice(1));
				break;
			case "OpenVPN STATISTICS":
				staticStats = true;
				break;
			case "Updated":
				if(staticStats)
					if(this.statusTime) this.statusTime.innerHTML = fields[1];
				break;
			case "END":
				done = true;
				break;
			default:
				if(staticStats)
				{
					if(this.statTable) this.statTable.tb.parentNode.style.display = '';
					if(this.statTable) this.statTable.insertData(-1, fields);
				}
				else if(this.errors) this.errors.innerHTML += 'Unknown: '+lines[i]+'<br>';
				break;
			}
			if ( done ) break;
		}
	}
}

