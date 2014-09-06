// Bind Navi etc.
function AdvancedTomato () {

	/** Misc functions, calls, binds
	************************************************************************************************/

	// Call navi function in tomato.js to generate navigation
	navi();

	// Find current active link
	$('.navigation > ul > li').each(function(key) {

		if ($(this).hasClass('active')) {
			$(this).find('ul').slideDown('350', 'easeInQuad');
		} else {
			$(this).find('ul').slideUp(350, 'easeOutBounce');
		}

	});

	// First run, check hash for current page
	if (window.location.hash.match(/#/)) { loadPage(window.location.hash); } else { loadPage('#status-home.asp'); }

	// Bind for "back" state of browser
	$(window).hashchange(function(e) {

		// Prevent Missmatch on features page
		((location.hash.replace('#', '') != '') ? loadPage(location.hash.replace('#', '')) : '');
		return false;

	});


	/** Click handlers
	************************************************************************************************/

	// Navigation slides
	$('.navigation:not(.collapsed) > ul > li > a').on('click', function() {

		if ($('.navigation').hasClass('collapsed')) { return; }
		if ($(this).parent('li').hasClass('active')) { return false; }

		$('.navigation > ul > li').removeClass('active').find('ul').slideUp('150');
		$(this).parent('li').addClass('active');
		$(this).closest('li').find('ul').slideDown('150');

		return false;
	});

	// Close click handler for updates
	$('.ajaxwrap').on('click', '.alert .close', function() {
		if ($(this).attr('data-update')) { cookie.set('latest-update', $(this).attr('data-update')); }
		$(this).parent('.alert').slideUp();
		return false;
	});

	// Handle ajax loading
	$('.navigation li ul a, .header .links a[href!="#system"]').on('click', function(e) {
		loadPage($(this).attr('href'));
		return false;
	});

	// Toggle Navigation
	$('.toggle-nav').on('click', function() {

		if (!$('.navigation').hasClass('collapsed')) {

			$('#wrapper').find('.container').css('margin-left', '60px');
			$('#wrapper').find('.navigation').addClass('collapsed');
			$('#wrapper').find('.logo').addClass('collapsed');
			$('#wrapper').find('.nav-collapse-hide').hide();

		} else {

			$('#wrapper').find('.container').css('margin-left', '240px');
			$('#wrapper').find('.navigation').removeClass('collapsed');
			$('#wrapper').find('.logo').removeClass('collapsed');
			$('#wrapper').find('.nav-collapse-hide').show();

		}

	});

	// Handle Ajax Class Loading
	$('.ajaxwrap').on('click', '.ajaxload', function(e) {
		loadPage($(this).attr('href'));
		return false;
	});

	// System Info box
	$('#system-ui').on('click', function() {

		if ($(this).hasClass('active')) {

			$('#system-ui').removeClass('active');
			$('.system-ui').fadeOut(250);

		} else {

			$(this).addClass('active');
			$('.system-ui').fadeIn(250);
			systemUI();

			$(document).click(function() {$('#system-ui').removeClass('active'); $('.system-ui').fadeOut(250); $(document).unbind('click'); });
		}

		return false;
	});


	/** Handle NVRAM global functions and notifications
	************************************************************************************************/
	if (typeof nvram == 'undefined') { return false; }

	// Check for update
	if (typeof nvram.at_update !== "undefined" && nvram.at_update != '') {

		var n = cookie.get('latest-update');
		var lastUpdate = nvram['at_update'].replace('.', '');

		if (n < lastUpdate || n == null) {

			$updateNotification = $('<div class="alert info"><a href="#" class="close" data-update="' + nvram.at_update.replace('.','') + '"><i class="icon-cancel"></i></a>\
				AdvancedTomato <b>v' + nvram.at_update + '</b> is already available. <a target="_blank" href="http://advancedtomato.com/changelog/">Click here to find out more</a>.</div>');

			$($updateNotification).find('.close').on('click', function() {
				if ($(this).attr('data-update')) { cookie.set('latest-update', $(this).attr('data-update')); }
				$(this).parent('.alert').slideUp();
				return false;
			});

			$(".container").prepend($updateNotification);

		}
	}

	// Check if tomatoanon is configured
	if (typeof nvram.tomatoanon_answer !== "undefined") {

		if (nvram.tomatoanon_answer != '1') {

			$('.container').prepend('<div class="alert warning"><h5>Attention</h5> You did not configure <b>TomatoAnon project</b> setting.\
				Please go to <a onclick="loadPage(\'admin-tomatoanon.asp\')" href="#">TomatoAnon configuration page</a> and make a choice.</div>');

		}

	}

	if (typeof nvram.at_navi !== 'undefined') {

		if (nvram.at_navi == 'collapsed') {

			$('#wrapper').find('.container').css('margin-left', '60px');
			$('#wrapper').find('.navigation').addClass('collapsed');
			$('#wrapper').find('.logo').addClass('collapsed');
			$('#wrapper').find('.nav-collapse-hide').hide();

		}

	}

}

// Get status of router and fill system-ui with it
function systemUI () {

	$('.system-ui .datasystem').html('<img width="32" height="32" src="img/preloader.svg"><br /><br />').addClass('align center');

	systemAJAX = new XmlHttp();
	systemAJAX.onCompleted = function (data, xml) {

		stats = {};
		try { eval(data); } catch (ex) { stats = {}; }

		stats.wanstatus = '<a title="Go to Status Overview" href="#" onclick="loadPage(\'#status-home.asp\');">' + ((stats.wanstatus == 'Connected') ? '<span style="color: green;">' + stats.wanstatus + '</span>' : stats.wanstatus) + '</a>';
		$('.system-ui .datasystem').html('<div class="router-name">' + stats.routermodel + ' <small class="pull-right">(' + stats.uptime + ')</small></div>' +
			'<div class="inner-container row">' +
			'<div class="col-sm-2">CPU:</div><div class="col-sm-10">' + stats.cpuload + '</div>'+
			'<div class="col-sm-2">RAM:</div><div class="col-sm-10">' + stats.memory + '<div class="progress"><div class="bar" style="width: ' + stats.memoryperc + '"></div></div></div>' +
			((nvram.swap != null) ? '<div class="col-sm-2">SWAP:</div><div class="col-sm-10">' + stats.swap + '<div class="progress"><div class="bar" style="width: ' + stats.swapperc + '"></div></div></div>':'') +
			'<div class="col-sm-2">WAN:</div><div class="col-sm-10">' + stats.wanstatus + ' (' + stats.wanuptime + ')</div></div>').removeClass('align center');
	}
	systemAJAX.get('js/status-data.jsx');
}

// Ajax Function to load pages
function loadPage(page) {

	// Fix refreshers when switching pages
	if (typeof (ref) != 'undefined') {
		ref.destroy();
	}

	// Some things that need to be done here =)
	page = page.replace('#', '');
	if (page == 'status-home.asp' || page == '/') { page = 'status-home.asp'; }
	if (window.ajaxLoadingState) { return false; } else { window.ajaxLoadingState = true; }

	preloader('start');

	// Tomato XMLHTTP/AJAX
	TomatoAJAX = new XmlHttp();
	TomatoAJAX.onCompleted = function (resp, xml) {

		var dom = $(resp);
		var title = dom.filter('title').text();
		var html = dom.filter('content').html();
		$('title').text(window.routerName + title);
		$('h2.currentpage').text(title);
		$('.container .ajaxwrap').hide().html(html).fadeIn(400);

		// Push History
		if (history.pushState) { // Fix issue with IE9 or bellow
			window.history.pushState({"html":null,"pageTitle": window.routerName + title }, '#'+page, '#'+page);
		}

		// Go back to top
		$('.container').scrollTop(0);

		// Handle Navigation
		$('.navigation li ul li').removeClass('active'); // Reset all

		var naviLinks = $(".navigation a[href='#" + page + "']");
		$(naviLinks).parent('li').addClass('active');

		// Loaded, clear state
		window.ajaxLoadingState = false;

		// Custom file inputs
		$("input[type='file']").each(function() { $(this).customFileInput(); });

		// Function that allows easy implementation of content hide/show on boxes
		$('[data-box]').each(function() {

			var id 		= $(this).attr('data-box');
			var parent	= $(this);
			var status	= (((c = cookie.get(id + '_visibility')) != null) && (c != '1') || !$(this).is(':visible')) ? false : true;
			var html	= $('<a class="pull-right" href="#" data-toggle="tooltip" title="Hide/Show"><i class="icon-chevron-' + ((status) ? 'down' : 'up') + '"></i></a>');

			// Hide if hidden
			if (!status) { $(this).find('.content').hide(); }

			// Now click handler
			$(html).on('click', function() {

				if (status) {

					$(parent).find('.content').stop(true, true).slideUp(700, 'easeOutBounce');
					$(html).find('i').removeClass('icon-chevron-down').addClass('icon-chevron-up');
					cookie.set(id + '_visibility', 0); status = false;

				} else {

					$(parent).find('.content').stop(true, true).slideDown(350, 'easeInQuad');
					$(html).find('i').removeClass('icon-chevron-uo').addClass('icon-chevron-down');
					cookie.set(id + '_visibility', 1); status = true;

				}

				return false;

			});

			$(parent).find('.heading').prepend(html);


		});

		// Init Tooltips
		$('[data-toggle="tooltip"]').tooltip({ placement: 'top auto' });

		preloader('stop');
	}

	// ERROR Handler
	TomatoAJAX.onError = function (x) {

		console.log(x);

		$('h2.currentpage').text('Interface Error');
		$('.container .ajaxwrap').hide().html('<h2>ERROR occured!<i class="icon-cancel" style="font-size: 20px; color: red; vertical-align: top;"></i></h2>\
			<span style="font-size: 14px;">There has been error while loading a page, please review debug data bellow if this is isolated issue.<br />\
			Otherwise please leave a message at <a target="_blank" href="http://advancedtomato.com/contact/">http://advancedtomato.com</a>. <br /><br /><pre class="debug">' + x + '</pre><br /><a href="/">Refreshing</a> browser window might help.</span>').fadeIn(200);

		preloader('stop');
		// Loaded, clear state
		window.ajaxLoadingState = false;
	}

	// Execute Prototype
	TomatoAJAX.get(page);

}

// Function preloader (Shows preloader close to cursor)
function preloader (event) {

	if (event == 'start') {

		$('html,a,.btn').attr('style', 'cursor: wait !important');

	} else {

		$('html,a,.btn').removeAttr('style');

	}

}

// $.browser jquery addon
(function(a){(jQuery.browser=jQuery.browser||{}).mobile=/(android|bb\d+|meego).+mobile|avantgo|bada\/|blackberry|blazer|compal|elaine|fennec|hiptop|iemobile|ip(hone|od)|iris|kindle|lge |maemo|midp|mmp|mobile.+firefox|netfront|opera m(ob|in)i|palm( os)?|phone|p(ixi|re)\/|plucker|pocket|psp|series(4|6)0|symbian|treo|up\.(browser|link)|vodafone|wap|windows (ce|phone)|xda|xiino/i.test(a)||/1207|6310|6590|3gso|4thp|50[1-6]i|770s|802s|a wa|abac|ac(er|oo|s\-)|ai(ko|rn)|al(av|ca|co)|amoi|an(ex|ny|yw)|aptu|ar(ch|go)|as(te|us)|attw|au(di|\-m|r |s )|avan|be(ck|ll|nq)|bi(lb|rd)|bl(ac|az)|br(e|v)w|bumb|bw\-(n|u)|c55\/|capi|ccwa|cdm\-|cell|chtm|cldc|cmd\-|co(mp|nd)|craw|da(it|ll|ng)|dbte|dc\-s|devi|dica|dmob|do(c|p)o|ds(12|\-d)|el(49|ai)|em(l2|ul)|er(ic|k0)|esl8|ez([4-7]0|os|wa|ze)|fetc|fly(\-|_)|g1 u|g560|gene|gf\-5|g\-mo|go(\.w|od)|gr(ad|un)|haie|hcit|hd\-(m|p|t)|hei\-|hi(pt|ta)|hp( i|ip)|hs\-c|ht(c(\-| |_|a|g|p|s|t)|tp)|hu(aw|tc)|i\-(20|go|ma)|i230|iac( |\-|\/)|ibro|idea|ig01|ikom|im1k|inno|ipaq|iris|ja(t|v)a|jbro|jemu|jigs|kddi|keji|kgt( |\/)|klon|kpt |kwc\-|kyo(c|k)|le(no|xi)|lg( g|\/(k|l|u)|50|54|\-[a-w])|libw|lynx|m1\-w|m3ga|m50\/|ma(te|ui|xo)|mc(01|21|ca)|m\-cr|me(rc|ri)|mi(o8|oa|ts)|mmef|mo(01|02|bi|de|do|t(\-| |o|v)|zz)|mt(50|p1|v )|mwbp|mywa|n10[0-2]|n20[2-3]|n30(0|2)|n50(0|2|5)|n7(0(0|1)|10)|ne((c|m)\-|on|tf|wf|wg|wt)|nok(6|i)|nzph|o2im|op(ti|wv)|oran|owg1|p800|pan(a|d|t)|pdxg|pg(13|\-([1-8]|c))|phil|pire|pl(ay|uc)|pn\-2|po(ck|rt|se)|prox|psio|pt\-g|qa\-a|qc(07|12|21|32|60|\-[2-7]|i\-)|qtek|r380|r600|raks|rim9|ro(ve|zo)|s55\/|sa(ge|ma|mm|ms|ny|va)|sc(01|h\-|oo|p\-)|sdk\/|se(c(\-|0|1)|47|mc|nd|ri)|sgh\-|shar|sie(\-|m)|sk\-0|sl(45|id)|sm(al|ar|b3|it|t5)|so(ft|ny)|sp(01|h\-|v\-|v )|sy(01|mb)|t2(18|50)|t6(00|10|18)|ta(gt|lk)|tcl\-|tdg\-|tel(i|m)|tim\-|t\-mo|to(pl|sh)|ts(70|m\-|m3|m5)|tx\-9|up(\.b|g1|si)|utst|v400|v750|veri|vi(rg|te)|vk(40|5[0-3]|\-v)|vm40|voda|vulc|vx(52|53|60|61|70|80|81|83|85|98)|w3c(\-| )|webc|whit|wi(g |nc|nw)|wmlb|wonu|x700|yas\-|your|zeto|zte\-/i.test(a.substr(0,4))})(navigator.userAgent||navigator.vendor||window.opera);
// Hash Change Plugin (Workaround browser issues)
(function($,e,b){var c="hashchange",h=document,f,g=$.event.special,i=h.documentMode,d="on"+c in e&&(i===b||i>7);function a(j){j=j||location.href;return"#"+j.replace(/^[^#]*#?(.*)$/,"$1")}$.fn[c]=function(j){return j?this.bind(c,j):this.trigger(c)};$.fn[c].delay=50;g[c]=$.extend(g[c],{setup:function(){if(d){return false}$(f.start)},teardown:function(){if(d){return false}$(f.stop)}});f=(function(){var j={},p,m=a(),k=function(q){return q},l=k,o=k;j.start=function(){p||n()};j.stop=function(){p&&clearTimeout(p);p=b};function n(){var r=a(),q=o(m);if(r!==m){l(m=r,q);$(e).trigger(c)}else{if(q!==m){location.href=location.href.replace(/#.*/,"")+q}}p=setTimeout(n,$.fn[c].delay)}$.browser.msie&&!d&&(function(){var q,r;j.start=function(){if(!q){r=$.fn[c].src;r=r&&r+a();q=$('<iframe tabindex="-1" title="empty"/>').hide().one("load",function(){r||l(a());n()}).attr("src",r||"javascript:0").insertAfter("body")[0].contentWindow;h.onpropertychange=function(){try{if(event.propertyName==="title"){q.document.title=h.title}}catch(s){}}}};j.stop=k;o=function(){return a(q.location.href)};l=function(v,s){var u=q.document,t=$.fn[c].domain;if(v!==s){u.title=h.title;u.open();t&&u.write('<script>document.domain="'+t+'"<\/script>');u.close();q.location.hash=v}}})();return j})()})(jQuery,this);
// Custom FileInputs (coded by http://prahec.com)
(function(e){e.fn.customFileInput=function(){var t=e(this).addClass("customfile-input").mouseover(function(){n.addClass("customfile-hover")}).mouseout(function(){n.removeClass("customfile-hover")}).focus(function(){n.addClass("customfile-focus");t.data("val",t.val())}).blur(function(){n.removeClass("customfile-focus");e(this).trigger("checkChange")}).bind("disable",function(){t.attr("disabled",true);n.addClass("customfile-disabled")}).bind("enable",function(){t.removeAttr("disabled");n.removeClass("customfile-disabled")}).bind("checkChange",function(){if(t.val()&&t.val()!=t.data("val")){t.trigger("change")}}).bind("change",function(){var t=e(this).val().split(/\\/).pop();var n="customfile-ext-"+t.split(".").pop().toLowerCase();i.html('<i class="icon-file"></i> '+t).removeClass(i.data("fileExt")||"").addClass(n).data("fileExt",n);r.text("Change")}).click(function(){t.data("val",t.val());setTimeout(function(){t.trigger("checkChange")},100)});var n=e('<div class="customfile"></div>');var i=e('<span class="customfile-text" aria-hidden="true">No file selected...</span>').appendTo(n);var r=e('<a class="btn btn-primary browse" href="#">Browse</a>').appendTo(n);if(t.is("[disabled]")){t.trigger("disable")}n.mousemove(function(r){t.css({left:r.pageX-n.offset().left-t.outerWidth()+20,top:r.pageY-n.offset().top-15})}).insertAfter(t);t.appendTo(n);return e(this)}})(jQuery)
// Bootstrap Tooltips
if("undefined"==typeof jQuery)throw new Error("Bootstrap's JavaScript requires jQuery");+function(t){"use strict";function e(e){return this.each(function(){var i=t(this),s=i.data("bs.tooltip"),n="object"==typeof e&&e;(s||"destroy"!=e)&&(s||i.data("bs.tooltip",s=new o(this,n)),"string"==typeof e&&s[e]())})}var o=function(t,e){this.type=this.options=this.enabled=this.timeout=this.hoverState=this.$element=null,this.init("tooltip",t,e)};o.VERSION="3.2.0",o.DEFAULTS={animation:!0,placement:"top",selector:!1,template:'<div class="tooltip" role="tooltip"><div class="tooltip-arrow"></div><div class="tooltip-inner"></div></div>',trigger:"hover focus",title:"",delay:0,html:!1,container:!1,viewport:{selector:"body",padding:0}},o.prototype.init=function(e,o,i){this.enabled=!0,this.type=e,this.$element=t(o),this.options=this.getOptions(i),this.$viewport=this.options.viewport&&t(this.options.viewport.selector||this.options.viewport);for(var s=this.options.trigger.split(" "),n=s.length;n--;){var r=s[n];if("click"==r)this.$element.on("click."+this.type,this.options.selector,t.proxy(this.toggle,this));else if("manual"!=r){var a="hover"==r?"mouseenter":"focusin",l="hover"==r?"mouseleave":"focusout";this.$element.on(a+"."+this.type,this.options.selector,t.proxy(this.enter,this)),this.$element.on(l+"."+this.type,this.options.selector,t.proxy(this.leave,this))}}this.options.selector?this._options=t.extend({},this.options,{trigger:"manual",selector:""}):this.fixTitle()},o.prototype.getDefaults=function(){return o.DEFAULTS},o.prototype.getOptions=function(e){return e=t.extend({},this.getDefaults(),this.$element.data(),e),e.delay&&"number"==typeof e.delay&&(e.delay={show:e.delay,hide:e.delay}),e},o.prototype.getDelegateOptions=function(){var e={},o=this.getDefaults();return this._options&&t.each(this._options,function(t,i){o[t]!=i&&(e[t]=i)}),e},o.prototype.enter=function(e){var o=e instanceof this.constructor?e:t(e.currentTarget).data("bs."+this.type);return o||(o=new this.constructor(e.currentTarget,this.getDelegateOptions()),t(e.currentTarget).data("bs."+this.type,o)),clearTimeout(o.timeout),o.hoverState="in",o.options.delay&&o.options.delay.show?void(o.timeout=setTimeout(function(){"in"==o.hoverState&&o.show()},o.options.delay.show)):o.show()},o.prototype.leave=function(e){var o=e instanceof this.constructor?e:t(e.currentTarget).data("bs."+this.type);return o||(o=new this.constructor(e.currentTarget,this.getDelegateOptions()),t(e.currentTarget).data("bs."+this.type,o)),clearTimeout(o.timeout),o.hoverState="out",o.options.delay&&o.options.delay.hide?void(o.timeout=setTimeout(function(){"out"==o.hoverState&&o.hide()},o.options.delay.hide)):o.hide()},o.prototype.show=function(){var e=t.Event("show.bs."+this.type);if(this.hasContent()&&this.enabled){this.$element.trigger(e);var o=t.contains(document.documentElement,this.$element[0]);if(e.isDefaultPrevented()||!o)return;var i=this,s=this.tip(),n=this.getUID(this.type);this.setContent(),s.attr("id",n),this.$element.attr("aria-describedby",n),this.options.animation&&s.addClass("fade");var r="function"==typeof this.options.placement?this.options.placement.call(this,s[0],this.$element[0]):this.options.placement,a=/\s?auto?\s?/i,l=a.test(r);l&&(r=r.replace(a,"")||"top"),s.detach().css({top:0,left:0,display:"block"}).addClass(r).data("bs."+this.type,this),this.options.container?s.appendTo(this.options.container):s.insertAfter(this.$element);var p=this.getPosition(),h=s[0].offsetWidth,f=s[0].offsetHeight;if(l){var u=r,d=this.$element.parent(),c=this.getPosition(d);r="bottom"==r&&p.top+p.height+f-c.scroll>c.height?"top":"top"==r&&p.top-c.scroll-f<0?"bottom":"right"==r&&p.right+h>c.width?"left":"left"==r&&p.left-h<c.left?"right":r,s.removeClass(u).addClass(r)}var g=this.getCalculatedOffset(r,p,h,f);this.applyPlacement(g,r);var y=function(){i.$element.trigger("shown.bs."+i.type),i.hoverState=null};t.support.transition&&this.$tip.hasClass("fade")?s.one("bsTransitionEnd",y).emulateTransitionEnd(150):y()}},o.prototype.applyPlacement=function(e,o){var i=this.tip(),s=i[0].offsetWidth,n=i[0].offsetHeight,r=parseInt(i.css("margin-top"),10),a=parseInt(i.css("margin-left"),10);isNaN(r)&&(r=0),isNaN(a)&&(a=0),e.top=e.top+r,e.left=e.left+a,t.offset.setOffset(i[0],t.extend({using:function(t){i.css({top:Math.round(t.top),left:Math.round(t.left)})}},e),0),i.addClass("in");var l=i[0].offsetWidth,p=i[0].offsetHeight;"top"==o&&p!=n&&(e.top=e.top+n-p);var h=this.getViewportAdjustedDelta(o,e,l,p);h.left?e.left+=h.left:e.top+=h.top;var f=h.left?2*h.left-s+l:2*h.top-n+p,u=h.left?"left":"top",d=h.left?"offsetWidth":"offsetHeight";i.offset(e),this.replaceArrow(f,i[0][d],u)},o.prototype.replaceArrow=function(t,e,o){this.arrow().css(o,t?50*(1-t/e)+"%":"")},o.prototype.setContent=function(){var t=this.tip(),e=this.getTitle();t.find(".tooltip-inner")[this.options.html?"html":"text"](e),t.removeClass("fade in top bottom left right")},o.prototype.hide=function(){function e(){"in"!=o.hoverState&&i.detach(),o.$element.trigger("hidden.bs."+o.type)}var o=this,i=this.tip(),s=t.Event("hide.bs."+this.type);return this.$element.removeAttr("aria-describedby"),this.$element.trigger(s),s.isDefaultPrevented()?void 0:(i.removeClass("in"),t.support.transition&&this.$tip.hasClass("fade")?i.one("bsTransitionEnd",e).emulateTransitionEnd(150):e(),this.hoverState=null,this)},o.prototype.fixTitle=function(){var t=this.$element;(t.attr("title")||"string"!=typeof t.attr("data-original-title"))&&t.attr("data-original-title",t.attr("title")||"").attr("title","")},o.prototype.hasContent=function(){return this.getTitle()},o.prototype.getPosition=function(e){e=e||this.$element;var o=e[0],i="BODY"==o.tagName;return t.extend({},"function"==typeof o.getBoundingClientRect?o.getBoundingClientRect():null,{scroll:i?document.documentElement.scrollTop||document.body.scrollTop:e.scrollTop(),width:i?t(window).width():e.outerWidth(),height:i?t(window).height():e.outerHeight()},i?{top:0,left:0}:e.offset())},o.prototype.getCalculatedOffset=function(t,e,o,i){return"bottom"==t?{top:e.top+e.height,left:e.left+e.width/2-o/2}:"top"==t?{top:e.top-i,left:e.left+e.width/2-o/2}:"left"==t?{top:e.top+e.height/2-i/2,left:e.left-o}:{top:e.top+e.height/2-i/2,left:e.left+e.width}},o.prototype.getViewportAdjustedDelta=function(t,e,o,i){var s={top:0,left:0};if(!this.$viewport)return s;var n=this.options.viewport&&this.options.viewport.padding||0,r=this.getPosition(this.$viewport);if(/right|left/.test(t)){var a=e.top-n-r.scroll,l=e.top+n-r.scroll+i;a<r.top?s.top=r.top-a:l>r.top+r.height&&(s.top=r.top+r.height-l)}else{var p=e.left-n,h=e.left+n+o;p<r.left?s.left=r.left-p:h>r.width&&(s.left=r.left+r.width-h)}return s},o.prototype.getTitle=function(){var t,e=this.$element,o=this.options;return t=e.attr("data-original-title")||("function"==typeof o.title?o.title.call(e[0]):o.title)},o.prototype.getUID=function(t){do t+=~~(1e6*Math.random());while(document.getElementById(t));return t},o.prototype.tip=function(){return this.$tip=this.$tip||t(this.options.template)},o.prototype.arrow=function(){return this.$arrow=this.$arrow||this.tip().find(".tooltip-arrow")},o.prototype.validate=function(){this.$element[0].parentNode||(this.hide(),this.$element=null,this.options=null)},o.prototype.enable=function(){this.enabled=!0},o.prototype.disable=function(){this.enabled=!1},o.prototype.toggleEnabled=function(){this.enabled=!this.enabled},o.prototype.toggle=function(e){var o=this;e&&(o=t(e.currentTarget).data("bs."+this.type),o||(o=new this.constructor(e.currentTarget,this.getDelegateOptions()),t(e.currentTarget).data("bs."+this.type,o))),o.tip().hasClass("in")?o.leave(o):o.enter(o)},o.prototype.destroy=function(){clearTimeout(this.timeout),this.hide().$element.off("."+this.type).removeData("bs."+this.type)};var i=t.fn.tooltip;t.fn.tooltip=e,t.fn.tooltip.Constructor=o,t.fn.tooltip.noConflict=function(){return t.fn.tooltip=i,this}}(jQuery);
// jQuery Easing Plugin
jQuery.easing["jswing"]=jQuery.easing["swing"];jQuery.extend(jQuery.easing,{def:"easeOutQuad",swing:function(e,t,n,r,i){return jQuery.easing[jQuery.easing.def](e,t,n,r,i)},easeInQuad:function(e,t,n,r,i){return r*(t/=i)*t+n},easeOutQuad:function(e,t,n,r,i){return-r*(t/=i)*(t-2)+n},easeInOutQuad:function(e,t,n,r,i){if((t/=i/2)<1)return r/2*t*t+n;return-r/2*(--t*(t-2)-1)+n},easeInCubic:function(e,t,n,r,i){return r*(t/=i)*t*t+n},easeOutCubic:function(e,t,n,r,i){return r*((t=t/i-1)*t*t+1)+n},easeInOutCubic:function(e,t,n,r,i){if((t/=i/2)<1)return r/2*t*t*t+n;return r/2*((t-=2)*t*t+2)+n},easeInQuart:function(e,t,n,r,i){return r*(t/=i)*t*t*t+n},easeOutQuart:function(e,t,n,r,i){return-r*((t=t/i-1)*t*t*t-1)+n},easeInOutQuart:function(e,t,n,r,i){if((t/=i/2)<1)return r/2*t*t*t*t+n;return-r/2*((t-=2)*t*t*t-2)+n},easeInQuint:function(e,t,n,r,i){return r*(t/=i)*t*t*t*t+n},easeOutQuint:function(e,t,n,r,i){return r*((t=t/i-1)*t*t*t*t+1)+n},easeInOutQuint:function(e,t,n,r,i){if((t/=i/2)<1)return r/2*t*t*t*t*t+n;return r/2*((t-=2)*t*t*t*t+2)+n},easeInSine:function(e,t,n,r,i){return-r*Math.cos(t/i*(Math.PI/2))+r+n},easeOutSine:function(e,t,n,r,i){return r*Math.sin(t/i*(Math.PI/2))+n},easeInOutSine:function(e,t,n,r,i){return-r/2*(Math.cos(Math.PI*t/i)-1)+n},easeInExpo:function(e,t,n,r,i){return t==0?n:r*Math.pow(2,10*(t/i-1))+n},easeOutExpo:function(e,t,n,r,i){return t==i?n+r:r*(-Math.pow(2,-10*t/i)+1)+n},easeInOutExpo:function(e,t,n,r,i){if(t==0)return n;if(t==i)return n+r;if((t/=i/2)<1)return r/2*Math.pow(2,10*(t-1))+n;return r/2*(-Math.pow(2,-10*--t)+2)+n},easeInCirc:function(e,t,n,r,i){return-r*(Math.sqrt(1-(t/=i)*t)-1)+n},easeOutCirc:function(e,t,n,r,i){return r*Math.sqrt(1-(t=t/i-1)*t)+n},easeInOutCirc:function(e,t,n,r,i){if((t/=i/2)<1)return-r/2*(Math.sqrt(1-t*t)-1)+n;return r/2*(Math.sqrt(1-(t-=2)*t)+1)+n},easeInElastic:function(e,t,n,r,i){var s=1.70158;var o=0;var u=r;if(t==0)return n;if((t/=i)==1)return n+r;if(!o)o=i*.3;if(u<Math.abs(r)){u=r;var s=o/4}else var s=o/(2*Math.PI)*Math.asin(r/u);return-(u*Math.pow(2,10*(t-=1))*Math.sin((t*i-s)*2*Math.PI/o))+n},easeOutElastic:function(e,t,n,r,i){var s=1.70158;var o=0;var u=r;if(t==0)return n;if((t/=i)==1)return n+r;if(!o)o=i*.3;if(u<Math.abs(r)){u=r;var s=o/4}else var s=o/(2*Math.PI)*Math.asin(r/u);return u*Math.pow(2,-10*t)*Math.sin((t*i-s)*2*Math.PI/o)+r+n},easeInOutElastic:function(e,t,n,r,i){var s=1.70158;var o=0;var u=r;if(t==0)return n;if((t/=i/2)==2)return n+r;if(!o)o=i*.3*1.5;if(u<Math.abs(r)){u=r;var s=o/4}else var s=o/(2*Math.PI)*Math.asin(r/u);if(t<1)return-.5*u*Math.pow(2,10*(t-=1))*Math.sin((t*i-s)*2*Math.PI/o)+n;return u*Math.pow(2,-10*(t-=1))*Math.sin((t*i-s)*2*Math.PI/o)*.5+r+n},easeInBack:function(e,t,n,r,i,s){if(s==undefined)s=1.70158;return r*(t/=i)*t*((s+1)*t-s)+n},easeOutBack:function(e,t,n,r,i,s){if(s==undefined)s=1.70158;return r*((t=t/i-1)*t*((s+1)*t+s)+1)+n},easeInOutBack:function(e,t,n,r,i,s){if(s==undefined)s=1.70158;if((t/=i/2)<1)return r/2*t*t*(((s*=1.525)+1)*t-s)+n;return r/2*((t-=2)*t*(((s*=1.525)+1)*t+s)+2)+n},easeInBounce:function(e,t,n,r,i){return r-jQuery.easing.easeOutBounce(e,i-t,0,r,i)+n},easeOutBounce:function(e,t,n,r,i){if((t/=i)<1/2.75){return r*7.5625*t*t+n}else if(t<2/2.75){return r*(7.5625*(t-=1.5/2.75)*t+.75)+n}else if(t<2.5/2.75){return r*(7.5625*(t-=2.25/2.75)*t+.9375)+n}else{return r*(7.5625*(t-=2.625/2.75)*t+.984375)+n}},easeInOutBounce:function(e,t,n,r,i){if(t<i/2)return jQuery.easing.easeInBounce(e,t*2,0,r,i)*.5+n;return jQuery.easing.easeOutBounce(e,t*2-i,0,r,i)*.5+r*.5+n}})