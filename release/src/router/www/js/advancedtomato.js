// Bind Navigation etc.
function AdvancedTomato() {

    /* First handle page loading, hash change events and other most important tasks
     ************************************************************************************************/
    // Initial Page load, determine what to load
    if ( window.location.hash.match( /#/ ) ) { loadPage( window.location.hash ); } else { loadPage( '#status-home.asp' ); }

    // Bind "Hash Change" - Happens when hash in the "URL" changes (http://site.com/#hash-bind)
    $( window ).bind( 'hashchange', function() {

        // Prevent Mismatch on features page
        ( (location.hash.replace( '#', '' ) != '' ) ? loadPage( location.hash.replace( '#', '' ), true ) : '' );
        return false;

    } );


    /* Misc functions, calls, binds
     ************************************************************************************************/
    // Call navigation function in tomato.js to generate navigation
    navi();

    // Create pre-loader
    $( '#wrapper' ).prepend( '<div id="nprogress"></div>' );

    // Find current active link
    $( '.navigation > ul > li' ).each( function( key ) {

        if ( $( this ).hasClass( 'active' ) ) {

            $( this ).find( 'ul' ).slideDown( 250, 'easeOutCirc' );

        } else {

            $( this ).find( 'ul' ).slideUp( 250, 'easeOutCirc' );

        }

    } );


    /* Click handlers
     ************************************************************************************************/
    // Navigation slides
    $( '.navigation > ul > li > a' ).on( gui.nav_action, function() {

        var elm = $( this );

        // This is just temporary function for navigation
        function reveal_navigation() {

            if ( $( '.navigation' ).hasClass( 'collapsed' ) ) { return; }				// Doesn't work in collapsed state
            if ( $( elm ).parent( 'li' ).hasClass( 'active' ) ) { return false; }      // If already active, ignore click

            $( '.navigation > ul > li' ).removeClass( 'active' ).find( 'ul' ).slideUp( '100' );
            $( elm ).parent( 'li' ).addClass( 'active' );
            $( elm ).closest( 'li' ).find( 'ul' ).slideDown( '100' );

            return false;

        }

        // New since 133 (Allow switch to use navigation as "HOVER" or "CLICK"
        if ( gui.nav_action == 'mouseover' ) {

            clearTimeout( gui.nav_delay );
            gui.nav_delay = setTimeout( reveal_navigation, 100 );

        } else {

            reveal_navigation();

        }

    } );

    // Close click handler for updates
    $( '.ajaxwrap' ).on( 'click', '.alert .close', function() {

        if ( $( this ).attr( 'data-update' ) ) { cookie.set( 'latest-update', $( this ).attr( 'data-update' ) ); }
        $( this ).parent( '.alert' ).slideUp();

        return false;

    } );

    // Handle ajax loading
    $( '.navigation li ul a, .header .links a[href!="#system"]' ).on( 'click', function( e ) {

        if ( $( this ).attr( 'target' ) != '_blank' ) {

            loadPage( $( this ).attr( 'href' ) );
            return false;

        }

    } );

    // Toggle Navigation
    $( '.toggle-nav' ).on( 'click', function() {

        if ( !$( '.navigation' ).hasClass( 'collapsed' ) ) {

            // Collapse the navigation
            $( '#wrapper' ).find( '.container, .top-header, .navigation' ).addClass( 'collapsed' );	// Hide the normal navigation >> animated
            $( '#wrapper' ).find( '.nav-collapse-hide' ).hide(); // Hide elements with class nav-collapse-hide

        } else {

            // Show the normal navigation
            $( '#wrapper' ).find( '.container, .top-header, .navigation' ).removeClass( 'collapsed' );
            setTimeout( function() { $( '#wrapper' ).find( '.nav-collapse-hide' ).show(); }, 300 );

        }

    } );

    // Handle Ajax Class Loading
    $( '.ajaxwrap' ).on( 'click', '.ajaxload', function( e ) {

        loadPage( $( this ).attr( 'href' ) );
        return false;

    } );

    // System Info box
    $( '#system-ui' ).on( 'click', function() {

        if ( $( this ).hasClass( 'active' ) ) {

            $( '#system-ui' ).removeClass( 'active' );
            $( '.system-ui' ).fadeOut( 250 );
            clearInterval( gui.refresh_timer );

        } else {

            $( this ).addClass( 'active' );
            $( '.system-ui' ).fadeIn( 250 );

            // On open
            $( '.system-ui .datasystem' ).html( '<div class="inner-container row"><div style="margin: 45px auto 35px; width: 26px; height:26px;" class="spinner"></div></div>' ).addClass( 'align center' );
            gui.refresh_timer = setInterval( systemUI, 1600 );
            systemUI();

            $( document ).click( function() {

                $( '#system-ui' ).removeClass( 'active' );
                $( '.system-ui' ).fadeOut( 250 );
                clearInterval( gui.refresh_timer );
                $( document ).unbind( 'click' );

            } );

        }

        return false;

    } );


    /* Handle NVRAM global functions and notifications
     ************************************************************************************************/
    if ( typeof nvram == 'undefined' ) { return false; }

    // Check for update
    if ( typeof nvram.at_update !== "undefined" && nvram.at_update != '' ) {

        var n          = cookie.get( 'latest-update' );
        var lastUpdate = nvram[ 'at_update' ].replace( '.', '' );

        if ( n < lastUpdate || n == null ) {

            $updateNotification = $( '<div class="alert alert-info icon"><a href="#" class="close" data-update="' + nvram.at_update.replace( '.', '' ) + '"><i class="icon-cancel"></i></a>\
				<h5>Update Available!</h5>AdvancedTomato version <b>' + nvram.at_update + '</b> has been released and it is available for download.	&nbsp; <a target="_blank" href="https://advancedtomato.com/">Click here to find out more</a>.</div>' );

            $( $updateNotification ).find( '.close' ).on( 'click', function() {
                if ( $( this ).attr( 'data-update' ) ) { cookie.set( 'latest-update', $( this ).attr( 'data-update' ) ); }
                $( this ).parent( '.alert' ).slideUp();
                return false;
            } );

            $( ".container" ).prepend( $updateNotification );

        }
    }

    // Check if tomatoanon is configured
    if ( typeof nvram.tomatoanon_answer !== "undefined" ) {

        if ( nvram.tomatoanon_answer != '1' ) {

            $( '.container' ).prepend( '<div class="alert alert-warning icon"><h5>Attention</h5> You did not configure <b>TomatoAnon project</b> setting.\
				Please go to <a onclick="loadPage(\'admin-tomatoanon.asp\')" href="#">TomatoAnon configuration page</a> and make a choice.</div>' );

        }

    }

    // Check for Navigation State NVRAM value
    if ( typeof nvram.at_nav_state !== 'undefined' ) {

        if ( nvram.at_nav_state == 'collapsed' || $( window ).width() <= 768 ) {

            $( '#wrapper' ).find( '.container, .top-header, .navigation' ).addClass( 'collapsed' );
            $( '#wrapper' ).find( '.nav-collapse-hide' ).hide();

        }

    }

}

// Get status of router and fill system-ui with it
function systemUI() {

    $.ajax(
        {
            url   : 'js/status-data.jsx',
            method: 'POST',
            data  : { '_http_id': escapeCGI( nvram.http_id ) }

        }
    ).done( function( data ) {

        stats = {};
        try { eval( data ); } catch ( ex ) { stats = {}; }

        var wanstatus = '<a title="Go to Status Overview" href="#" onclick="loadPage(\'#status-home.asp\');">' + ( ( stats.wanstatus[ 0 ] == 'Connected' ) ? '<span style="color: green;">' + stats.wanstatus[ 0 ] + '</span>' : stats.wanstatus[ 0 ] ) + '</a>';
        $( '.system-ui .datasystem' ).html(
            '<div class="router-name">' + nvram.t_model_name + ' <small class="pull-right">(' + stats.uptime + ')</small></div>' +
            '<div class="inner-container row">' +
            '<div class="desc">CPU:</div><div class="value">' + stats.cpuload + '</div>' +
            '<div class="desc">RAM:</div><div class="value">' + stats.memory + '<div class="progress small"><div class="bar" style="width: ' + stats.memoryperc + '"></div></div></div>' +
            ((nvram.swap != null) ? '<div class="desc">SWAP:</div><div class="value">' + stats.swap + '<div class="progress small"><div class="bar" style="width: ' + stats.swapperc + '"></div></div></div>' : '') +
            '<div class="desc ">WAN:</div><div class="value">' + wanstatus + ' (' + stats.wanuptime[ 0 ] + ')</div></div>' ).removeClass( 'align center'
        );

    } ).fail( function() { clearInterval( gui.refresh_timer ); } );

}

// Data boxes which allow showing / hiding box content, the behaviour happens here
function data_boxes() {

    $( '[data-box]' ).each( function() {

        var id     = $( this ).attr( 'data-box' );
        var parent = $( this );
        var status = (((hs_cook = cookie.get( id + '_visibility' )) != null && (hs_cook != '1')) && $( this ).is( ':visible' )) ? false : true;
        var html   = $( '<a class="pull-right" href="#" data-toggle="tooltip" title="Hide/Show"><i class="icon-chevron-' + ((status) ? 'down' : 'up') + '"></i></a>' );

        // Hide if hidden
        if ( status ) {

            $( this ).find( '.content' ).show();

        } else { // Set display property no matter the preference (fixes defaults)

            $( this ).find( '.content' ).hide();

        }

        // Now click handler
        $( html ).on( 'click', function() {

            if ( status ) {

                $( parent ).find( '.content' ).stop( true, true ).slideUp( 700, 'easeOutBounce' );
                $( html ).find( 'i' ).removeClass( 'icon-chevron-down' ).addClass( 'icon-chevron-up' );
                cookie.set( id + '_visibility', 0 );
                status = false;

            } else {

                $( parent ).find( '.content' ).stop( true, true ).slideDown( 350, 'easeInQuad' );
                $( html ).find( 'i' ).removeClass( 'icon-chevron-up' ).addClass( 'icon-chevron-down' );
                cookie.set( id + '_visibility', 1 );
                status = true;

            }

            return false;

        } );

        $( parent ).find( '.heading' ).prepend( html );

    } );

}

// We use this for loadPage
var ajax_retries = 1;

// Ajax Function to load pages
function loadPage( page, is_history ) {

    // Some things that need to be done here =)
    page = page.replace( '#', '' );
    if ( page == 'status-home.asp' || page == '/' || page == null ) { page = 'status-home.asp'; }
    if ( window.ajaxLoadingState ) { return false; } else { window.ajaxLoadingState = true; }

    // Since we use ajax, functions and timers stay in memory/cache. Here we undefine & stop them to prevent issues with other pages.
    if ( typeof( ref ) != 'undefined' ) {
        ref.destroy();
        ref = undefined;
        delete ref;
    }
    if ( typeof( wdog ) != 'undefined' ) { clearTimeout( wdog ); } // Delayed function that kills our refreshers!

    // Start page pre-loader
    $( '#nprogress' ).append( '<div class="bar"></div>' );

    // Remove animation class from container, so we reset its anim count to 0
    $( '.container .ajaxwrap' ).removeClass( 'ajax-animation' );


    // Switch to JQUERY AJAX function call (doesn't capture errors allowing much easier debugging)
    $.ajax( { url: page, async: true, cache: false, timeout: 2950 } )
        .done( function( resp ) {

            var dom   = $( resp );
            var title = dom.filter( 'title' ).text();
            var html  = dom.filter( 'content' ).html();

            // Handle pages without title or content as normal (HTTP Redirect)
            if ( title == null || html == null ) {

                window.parent.location.href = page;
                return false;

            }

            // Set page title, current page title and animate page switch
            $( 'title' ).text( window.routerName + title );
            $( 'h2.currentpage' ).text( title );
            $( '.container .ajaxwrap' ).html( html ).addClass( 'ajax-animation' );

            // Push History (First check if using IE9 or not)
            if ( history.pushState && is_history !== true ) {

                // IE9+ function that's awesome for AJAX stuff
                history.pushState(
                    {
                        "html"     : html,
                        "pageTitle": window.routerName + title
                    },
                    window.routerName + title, '#' + page
                );

            }

            // Go back to top
            $( '.container' ).scrollTop( 0 );

            // Handle Navigation
            $( '.navigation li ul li' ).removeClass( 'active' ); // Reset all
            var naviLinks = $( ".navigation a[href='#" + page + "']" );
            $( naviLinks ).parent( 'li' ).addClass( 'active' );

            // Remove existing tooltips
            $('.tooltip').remove();

            // Bind some functions, scripts etc... (Important: after every page change (ajax load))
            $( '[data-toggle="tooltip"]' ).tooltip( { placement: 'top auto', container: 'body' } );
            $( "input[type='file']" ).each( function() { $( this ).customFileInput(); } ); // Custom file inputs
            data_boxes();

            // Stop & Remove Pre-loader
            $( '#nprogress' ).find( '.bar' ).css( { 'animation': 'none' } ).width( '100%' );
            setTimeout( function() { $( '#nprogress .bar' ).remove(); }, 250 );

            // Reset loading state to false.
            window.ajaxLoadingState = false;
            if ( ajax_retries != 1 ) $( '.body-overwrite' ).remove();
            ajax_retries = 1;

        } )
        .fail( function( jqXHR, textStatus, errorThrown ) {

            console.log( jqXHR, errorThrown );

            // We retry few x before showing msg bellow (TBD)
            if ( ajax_retries <= 8 ) {

                // Write only if div doesn't already exist
                if ( $( 'body .body-overwrite' ).length == 0 ) {

                    $( 'body' ).append( '<div class="body-overwrite"><div class="body-overwrite-text text-center"><div class="spinner spinner-large"></div>' +
                                        '<br><br><b>Connection lost!</b><br>Attempting to reconnect...</div></div>' );

                }

                // Try again in 2500ms, when retries reach 3, show error instead
                setTimeout( function() {

                    // Count retries
                    ajax_retries++;

                    // Try again
                    window.ajaxLoadingState = false;
                    loadPage( page, is_history );

                }, 3000 );

                // Don't continue
                return;

            }

            // In case error is 0 it usually means 504, gateway timeout
            if ( jqXHR.status == 0 ) jqXHR.status = 504;

            $( 'h2.currentpage' ).text( jqXHR.status + ' ERROR' );
            $( '.container .ajaxwrap' ).html( '<div class="box"><div class="heading">ERROR - ' + jqXHR.status + '</div><div class="content">\
				<p>The Graphical user interface was unable to communicate with the router!<br>These issues usually occur when a file is missing, web handler is busy or the connection to router is unavailable.</p>\
				<a href="/">Refreshing</a> browser window might help.</div></div>' ).addClass( 'ajax-animation' );

            // Loaded, clear state
            window.ajaxLoadingState = false;
            if ( ajax_retries != 1 ) $( '.body-overwrite' ).remove();

            // Remove Preloader
            $( '#nprogress' ).find( '.bar' ).css( { 'animation': 'none' } ).width( '100%' );
            setTimeout( function() { $( '#nprogress .bar' ).remove(); }, 250 );

        } );

}


/**
 * Custom FileInputs (coded by http://prahec.com)
 */
(function( e ) {
    e.fn.customFileInput = function() {
        var t = e( this ).addClass( "customfile-input" ).mouseover( function() {n.addClass( "customfile-hover" )} ).mouseout( function() {n.removeClass( "customfile-hover" )} ).focus( function() {
            n.addClass( "customfile-focus" );
            t.data( "val", t.val() )
        } ).blur( function() {
            n.removeClass( "customfile-focus" );
            e( this ).trigger( "checkChange" )
        } ).bind( "disable", function() {
            t.attr( "disabled", true );
            n.addClass( "customfile-disabled" )
        } ).bind( "enable", function() {
            t.removeAttr( "disabled" );
            n.removeClass( "customfile-disabled" )
        } ).bind( "checkChange", function() {if ( t.val() && t.val() != t.data( "val" ) ) {t.trigger( "change" )}} ).bind( "change", function() {
            var t = e( this ).val().split( /\\/ ).pop();
            var n = "customfile-ext-" + t.split( "." ).pop().toLowerCase();
            i.html( '<i class="icon-file"></i> ' + t ).removeClass( i.data( "fileExt" ) || "" ).addClass( n ).data( "fileExt", n );
            r.text( "Change" )
        } ).click( function() {
            t.data( "val", t.val() );
            setTimeout( function() {t.trigger( "checkChange" )}, 100 )
        } );
        var n = e( '<div class="customfile"></div>' );
        var i = e( '<span class="customfile-text" aria-hidden="true">No file selected...</span>' ).appendTo( n );
        var r = e( '<a class="btn btn-primary browse" href="#">Browse</a>' ).appendTo( n );
        if ( t.is( "[disabled]" ) ) {t.trigger( "disable" )}
        n.mousemove( function( r ) {t.css( { left: r.pageX - n.offset().left - t.outerWidth() + 20, top: r.pageY - n.offset().top - 15 } )} ).insertAfter( t );
        t.appendTo( n );
        return e( this )
    }
})( jQuery );


/* ========================================================================
 * Bootstrap: tooltip.js v3.3.7
 * http://getbootstrap.com/javascript/#tooltip
 * Inspired by the original jQuery.tipsy by Jason Frame
 * ========================================================================
 * Copyright 2011-2016 Twitter, Inc.
 * Licensed under MIT (https://github.com/twbs/bootstrap/blob/master/LICENSE)
 * ======================================================================== */
+function( $ ) {
    'use strict';

    // TOOLTIP PUBLIC CLASS DEFINITION
    // ===============================

    var Tooltip = function( element, options ) {
        this.type       = null;
        this.options    = null;
        this.enabled    = null;
        this.timeout    = null;
        this.hoverState = null;
        this.$element   = null;
        this.inState    = null;

        this.init( 'tooltip', element, options )
    }

    Tooltip.VERSION = '3.3.7';

    Tooltip.TRANSITION_DURATION = 150;

    Tooltip.DEFAULTS = {
        animation: true,
        placement: 'top',
        selector : false,
        template : '<div class="tooltip" role="tooltip"><div class="tooltip-arrow"></div><div class="tooltip-inner"></div></div>',
        trigger  : 'hover focus',
        title    : '',
        delay    : 0,
        html     : false,
        container: false,
        viewport : {
            selector: 'body',
            padding : 0
        }
    }

    Tooltip.prototype.init = function( type, element, options ) {
        this.enabled   = true
        this.type      = type
        this.$element  = $( element )
        this.options   = this.getOptions( options )
        this.$viewport = this.options.viewport && $( $.isFunction( this.options.viewport ) ? this.options.viewport.call( this, this.$element ) : (this.options.viewport.selector || this.options.viewport) )
        this.inState   = { click: false, hover: false, focus: false }

        if ( this.$element[ 0 ] instanceof document.constructor && !this.options.selector ) {
            throw new Error( '`selector` option must be specified when initializing ' + this.type + ' on the window.document object!' )
        }

        var triggers = this.options.trigger.split( ' ' )

        for ( var i = triggers.length; i--; ) {
            var trigger = triggers[ i ]

            if ( trigger == 'click' ) {
                this.$element.on( 'click.' + this.type, this.options.selector, $.proxy( this.toggle, this ) )
            } else if ( trigger != 'manual' ) {
                var eventIn  = trigger == 'hover' ? 'mouseenter' : 'focusin'
                var eventOut = trigger == 'hover' ? 'mouseleave' : 'focusout'

                this.$element.on( eventIn + '.' + this.type, this.options.selector, $.proxy( this.enter, this ) )
                this.$element.on( eventOut + '.' + this.type, this.options.selector, $.proxy( this.leave, this ) )
            }
        }

        this.options.selector ?
        (this._options = $.extend( {}, this.options, { trigger: 'manual', selector: '' } )) :
        this.fixTitle()
    }

    Tooltip.prototype.getDefaults = function() {
        return Tooltip.DEFAULTS
    }

    Tooltip.prototype.getOptions = function( options ) {
        options = $.extend( {}, this.getDefaults(), this.$element.data(), options )

        if ( options.delay && typeof options.delay == 'number' ) {
            options.delay = {
                show: options.delay,
                hide: options.delay
            }
        }

        return options
    }

    Tooltip.prototype.getDelegateOptions = function() {
        var options  = {}
        var defaults = this.getDefaults()

        this._options && $.each( this._options, function( key, value ) {
            if ( defaults[ key ] != value ) options[ key ] = value
        } )

        return options
    }

    Tooltip.prototype.enter = function( obj ) {
        var self = obj instanceof this.constructor ?
                   obj : $( obj.currentTarget ).data( 'bs.' + this.type )

        if ( !self ) {
            self = new this.constructor( obj.currentTarget, this.getDelegateOptions() )
            $( obj.currentTarget ).data( 'bs.' + this.type, self )
        }

        if ( obj instanceof $.Event ) {
            self.inState[ obj.type == 'focusin' ? 'focus' : 'hover' ] = true
        }

        if ( self.tip().hasClass( 'in' ) || self.hoverState == 'in' ) {
            self.hoverState = 'in'
            return
        }

        clearTimeout( self.timeout )

        self.hoverState = 'in'

        if ( !self.options.delay || !self.options.delay.show ) return self.show()

        self.timeout = setTimeout( function() {
            if ( self.hoverState == 'in' ) self.show()
        }, self.options.delay.show )
    }

    Tooltip.prototype.isInStateTrue = function() {
        for ( var key in this.inState ) {
            if ( this.inState[ key ] ) return true
        }

        return false
    }

    Tooltip.prototype.leave = function( obj ) {
        var self = obj instanceof this.constructor ?
                   obj : $( obj.currentTarget ).data( 'bs.' + this.type )

        if ( !self ) {
            self = new this.constructor( obj.currentTarget, this.getDelegateOptions() )
            $( obj.currentTarget ).data( 'bs.' + this.type, self )
        }

        if ( obj instanceof $.Event ) {
            self.inState[ obj.type == 'focusout' ? 'focus' : 'hover' ] = false
        }

        if ( self.isInStateTrue() ) return

        clearTimeout( self.timeout )

        self.hoverState = 'out'

        if ( !self.options.delay || !self.options.delay.hide ) return self.hide()

        self.timeout = setTimeout( function() {
            if ( self.hoverState == 'out' ) self.hide()
        }, self.options.delay.hide )
    }

    Tooltip.prototype.show = function() {
        var e = $.Event( 'show.bs.' + this.type )

        if ( this.hasContent() && this.enabled ) {
            this.$element.trigger( e )

            var inDom = $.contains( this.$element[ 0 ].ownerDocument.documentElement, this.$element[ 0 ] )
            if ( e.isDefaultPrevented() || !inDom ) return
            var that = this

            var $tip = this.tip()

            var tipId = this.getUID( this.type )

            this.setContent()
            $tip.attr( 'id', tipId )
            this.$element.attr( 'aria-describedby', tipId )

            if ( this.options.animation ) $tip.addClass( 'fade' )

            var placement = typeof this.options.placement == 'function' ?
                            this.options.placement.call( this, $tip[ 0 ], this.$element[ 0 ] ) :
                            this.options.placement

            var autoToken = /\s?auto?\s?/i
            var autoPlace = autoToken.test( placement )
            if ( autoPlace ) placement = placement.replace( autoToken, '' ) || 'top'

            $tip
                .detach()
                .css( { top: 0, left: 0, display: 'block' } )
                .addClass( placement )
                .data( 'bs.' + this.type, this )

            this.options.container ? $tip.appendTo( this.options.container ) : $tip.insertAfter( this.$element )
            this.$element.trigger( 'inserted.bs.' + this.type )

            var pos          = this.getPosition()
            var actualWidth  = $tip[ 0 ].offsetWidth
            var actualHeight = $tip[ 0 ].offsetHeight

            if ( autoPlace ) {
                var orgPlacement = placement
                var viewportDim  = this.getPosition( this.$viewport )

                placement = placement == 'bottom' && pos.bottom + actualHeight > viewportDim.bottom ? 'top' :
                            placement == 'top' && pos.top - actualHeight < viewportDim.top ? 'bottom' :
                            placement == 'right' && pos.right + actualWidth > viewportDim.width ? 'left' :
                            placement == 'left' && pos.left - actualWidth < viewportDim.left ? 'right' :
                            placement

                $tip
                    .removeClass( orgPlacement )
                    .addClass( placement )
            }

            var calculatedOffset = this.getCalculatedOffset( placement, pos, actualWidth, actualHeight )

            this.applyPlacement( calculatedOffset, placement )

            var complete = function() {
                var prevHoverState = that.hoverState
                that.$element.trigger( 'shown.bs.' + that.type )
                that.hoverState = null

                if ( prevHoverState == 'out' ) that.leave( that )
            }

            $.support.transition && this.$tip.hasClass( 'fade' ) ?
            $tip
                .one( 'bsTransitionEnd', complete )
                .emulateTransitionEnd( Tooltip.TRANSITION_DURATION ) :
            complete()
        }
    }

    Tooltip.prototype.applyPlacement = function( offset, placement ) {
        var $tip   = this.tip()
        var width  = $tip[ 0 ].offsetWidth
        var height = $tip[ 0 ].offsetHeight

        // manually read margins because getBoundingClientRect includes difference
        var marginTop  = parseInt( $tip.css( 'margin-top' ), 10 )
        var marginLeft = parseInt( $tip.css( 'margin-left' ), 10 )

        // we must check for NaN for ie 8/9
        if ( isNaN( marginTop ) ) marginTop = 0
        if ( isNaN( marginLeft ) ) marginLeft = 0

        offset.top += marginTop
        offset.left += marginLeft

        // $.fn.offset doesn't round pixel values
        // so we use setOffset directly with our own function B-0
        $.offset.setOffset( $tip[ 0 ], $.extend(
            {
                using: function( props ) {
                    $tip.css( {
                                  top : Math.round( props.top ),
                                  left: Math.round( props.left )
                              } )
                }
            }, offset ), 0 )

        $tip.addClass( 'in' )

        // check to see if placing tip in new offset caused the tip to resize itself
        var actualWidth  = $tip[ 0 ].offsetWidth
        var actualHeight = $tip[ 0 ].offsetHeight

        if ( placement == 'top' && actualHeight != height ) {
            offset.top = offset.top + height - actualHeight
        }

        var delta = this.getViewportAdjustedDelta( placement, offset, actualWidth, actualHeight )

        if ( delta.left ) offset.left += delta.left
        else offset.top += delta.top

        var isVertical          = /top|bottom/.test( placement )
        var arrowDelta          = isVertical ? delta.left * 2 - width + actualWidth : delta.top * 2 - height + actualHeight
        var arrowOffsetPosition = isVertical ? 'offsetWidth' : 'offsetHeight'

        $tip.offset( offset )
        this.replaceArrow( arrowDelta, $tip[ 0 ][ arrowOffsetPosition ], isVertical )
    }

    Tooltip.prototype.replaceArrow = function( delta, dimension, isVertical ) {
        this.arrow()
            .css( isVertical ? 'left' : 'top', 50 * (1 - delta / dimension) + '%' )
            .css( isVertical ? 'top' : 'left', '' )
    }

    Tooltip.prototype.setContent = function() {
        var $tip  = this.tip()
        var title = this.getTitle()

        $tip.find( '.tooltip-inner' )[ this.options.html ? 'html' : 'text' ]( title )
        $tip.removeClass( 'fade in top bottom left right' )
    }

    Tooltip.prototype.hide = function( callback ) {
        var that = this
        var $tip = $( this.$tip )
        var e    = $.Event( 'hide.bs.' + this.type )

        function complete() {
            if ( that.hoverState != 'in' ) $tip.detach()
            if ( that.$element ) { // TODO: Check whether guarding this code with this `if` is really necessary.
                that.$element
                    .removeAttr( 'aria-describedby' )
                    .trigger( 'hidden.bs.' + that.type )
            }
            callback && callback()
        }

        this.$element.trigger( e )

        if ( e.isDefaultPrevented() ) return

        $tip.removeClass( 'in' )

        $.support.transition && $tip.hasClass( 'fade' ) ?
        $tip
            .one( 'bsTransitionEnd', complete )
            .emulateTransitionEnd( Tooltip.TRANSITION_DURATION ) :
        complete()

        this.hoverState = null

        return this
    }

    Tooltip.prototype.fixTitle = function() {
        var $e = this.$element
        if ( $e.attr( 'title' ) || typeof $e.attr( 'data-original-title' ) != 'string' ) {
            $e.attr( 'data-original-title', $e.attr( 'title' ) || '' ).attr( 'title', '' )
        }
    }

    Tooltip.prototype.hasContent = function() {
        return this.getTitle()
    }

    Tooltip.prototype.getPosition = function( $element ) {
        $element = $element || this.$element

        var el     = $element[ 0 ]
        var isBody = el.tagName == 'BODY'

        var elRect = el.getBoundingClientRect()
        if ( elRect.width == null ) {
            // width and height are missing in IE8, so compute them manually; see https://github.com/twbs/bootstrap/issues/14093
            elRect = $.extend( {}, elRect, { width: elRect.right - elRect.left, height: elRect.bottom - elRect.top } )
        }
        var isSvg = window.SVGElement && el instanceof window.SVGElement
        // Avoid using $.offset() on SVGs since it gives incorrect results in jQuery 3.
        // See https://github.com/twbs/bootstrap/issues/20280
        var elOffset  = isBody ? { top: 0, left: 0 } : (isSvg ? null : $element.offset())
        var scroll    = { scroll: isBody ? document.documentElement.scrollTop || document.body.scrollTop : $element.scrollTop() }
        var outerDims = isBody ? { width: $( window ).width(), height: $( window ).height() } : null

        return $.extend( {}, elRect, scroll, outerDims, elOffset )
    }

    Tooltip.prototype.getCalculatedOffset = function( placement, pos, actualWidth, actualHeight ) {
        return placement == 'bottom' ? { top: pos.top + pos.height, left: pos.left + pos.width / 2 - actualWidth / 2 } :
               placement == 'top' ? { top: pos.top - actualHeight, left: pos.left + pos.width / 2 - actualWidth / 2 } :
               placement == 'left' ? { top: pos.top + pos.height / 2 - actualHeight / 2, left: pos.left - actualWidth } :
                   /* placement == 'right' */ { top: pos.top + pos.height / 2 - actualHeight / 2, left: pos.left + pos.width }

    }

    Tooltip.prototype.getViewportAdjustedDelta = function( placement, pos, actualWidth, actualHeight ) {
        var delta = { top: 0, left: 0 }
        if ( !this.$viewport ) return delta

        var viewportPadding    = this.options.viewport && this.options.viewport.padding || 0
        var viewportDimensions = this.getPosition( this.$viewport )

        if ( /right|left/.test( placement ) ) {
            var topEdgeOffset    = pos.top - viewportPadding - viewportDimensions.scroll
            var bottomEdgeOffset = pos.top + viewportPadding - viewportDimensions.scroll + actualHeight
            if ( topEdgeOffset < viewportDimensions.top ) { // top overflow
                delta.top = viewportDimensions.top - topEdgeOffset
            } else if ( bottomEdgeOffset > viewportDimensions.top + viewportDimensions.height ) { // bottom overflow
                delta.top = viewportDimensions.top + viewportDimensions.height - bottomEdgeOffset
            }
        } else {
            var leftEdgeOffset  = pos.left - viewportPadding
            var rightEdgeOffset = pos.left + viewportPadding + actualWidth
            if ( leftEdgeOffset < viewportDimensions.left ) { // left overflow
                delta.left = viewportDimensions.left - leftEdgeOffset
            } else if ( rightEdgeOffset > viewportDimensions.right ) { // right overflow
                delta.left = viewportDimensions.left + viewportDimensions.width - rightEdgeOffset
            }
        }

        return delta
    }

    Tooltip.prototype.getTitle = function() {
        var title
        var $e = this.$element
        var o  = this.options

        title = $e.attr( 'data-original-title' )
                || (typeof o.title == 'function' ? o.title.call( $e[ 0 ] ) : o.title)

        return title
    }

    Tooltip.prototype.getUID = function( prefix ) {
        do prefix += ~~(Math.random() * 1000000)
        while ( document.getElementById( prefix ) )
        return prefix
    }

    Tooltip.prototype.tip = function() {
        if ( !this.$tip ) {
            this.$tip = $( this.options.template )
            if ( this.$tip.length != 1 ) {
                throw new Error( this.type + ' `template` option must consist of exactly 1 top-level element!' )
            }
        }
        return this.$tip
    }

    Tooltip.prototype.arrow = function() {
        return (this.$arrow = this.$arrow || this.tip().find( '.tooltip-arrow' ))
    }

    Tooltip.prototype.enable = function() {
        this.enabled = true
    }

    Tooltip.prototype.disable = function() {
        this.enabled = false
    }

    Tooltip.prototype.toggleEnabled = function() {
        this.enabled = !this.enabled
    }

    Tooltip.prototype.toggle = function( e ) {
        var self = this
        if ( e ) {
            self = $( e.currentTarget ).data( 'bs.' + this.type )
            if ( !self ) {
                self = new this.constructor( e.currentTarget, this.getDelegateOptions() )
                $( e.currentTarget ).data( 'bs.' + this.type, self )
            }
        }

        if ( e ) {
            self.inState.click = !self.inState.click
            if ( self.isInStateTrue() ) self.enter( self )
            else self.leave( self )
        } else {
            self.tip().hasClass( 'in' ) ? self.leave( self ) : self.enter( self )
        }
    }

    Tooltip.prototype.destroy = function() {
        var that = this
        clearTimeout( this.timeout )
        this.hide( function() {
            that.$element.off( '.' + that.type ).removeData( 'bs.' + that.type )
            if ( that.$tip ) {
                that.$tip.detach()
            }
            that.$tip      = null
            that.$arrow    = null
            that.$viewport = null
            that.$element  = null
        } )
    }


    // TOOLTIP PLUGIN DEFINITION
    // =========================

    function Plugin( option ) {
        return this.each( function() {
            var $this   = $( this )
            var data    = $this.data( 'bs.tooltip' )
            var options = typeof option == 'object' && option

            if ( !data && /destroy|hide/.test( option ) ) return
            if ( !data ) $this.data( 'bs.tooltip', (data = new Tooltip( this, options )) )
            if ( typeof option == 'string' ) data[ option ]()
        } )
    }

    var old = $.fn.tooltip

    $.fn.tooltip             = Plugin
    $.fn.tooltip.Constructor = Tooltip


    // TOOLTIP NO CONFLICT
    // ===================

    $.fn.tooltip.noConflict = function() {
        $.fn.tooltip = old
        return this
    }

}( jQuery );


/*
 * jQuery Easing v1.3 - http://gsgd.co.uk/sandbox/jquery/easing/
 *
 * Uses the built in easing capabilities added In jQuery 1.1
 * to offer multiple easing options
 *
 * TERMS OF USE - jQuery Easing
 *
 * Open source under the BSD License.
 *
 * Copyright Â© 2008 George McGinley Smith
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * Neither the name of the author nor the names of contributors may be used to endorse
 * or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

// t: current time, b: begInnIng value, c: change In value, d: duration
jQuery.easing[ 'jswing' ] = jQuery.easing[ 'swing' ];

jQuery.extend(
    jQuery.easing,
    {
        def             : 'easeOutQuad',
        swing           : function( x, t, b, c, d ) {
            //alert(jQuery.easing.default);
            return jQuery.easing[ jQuery.easing.def ]( x, t, b, c, d );
        },
        easeInQuad      : function( x, t, b, c, d ) {
            return c * (t /= d) * t + b;
        },
        easeOutQuad     : function( x, t, b, c, d ) {
            return -c * (t /= d) * (t - 2) + b;
        },
        easeInOutQuad   : function( x, t, b, c, d ) {
            if ( (t /= d / 2) < 1 ) return c / 2 * t * t + b;
            return -c / 2 * ((--t) * (t - 2) - 1) + b;
        },
        easeInCubic     : function( x, t, b, c, d ) {
            return c * (t /= d) * t * t + b;
        },
        easeOutCubic    : function( x, t, b, c, d ) {
            return c * ((t = t / d - 1) * t * t + 1) + b;
        },
        easeInOutCubic  : function( x, t, b, c, d ) {
            if ( (t /= d / 2) < 1 ) return c / 2 * t * t * t + b;
            return c / 2 * ((t -= 2) * t * t + 2) + b;
        },
        easeInQuart     : function( x, t, b, c, d ) {
            return c * (t /= d) * t * t * t + b;
        },
        easeOutQuart    : function( x, t, b, c, d ) {
            return -c * ((t = t / d - 1) * t * t * t - 1) + b;
        },
        easeInOutQuart  : function( x, t, b, c, d ) {
            if ( (t /= d / 2) < 1 ) return c / 2 * t * t * t * t + b;
            return -c / 2 * ((t -= 2) * t * t * t - 2) + b;
        },
        easeInQuint     : function( x, t, b, c, d ) {
            return c * (t /= d) * t * t * t * t + b;
        },
        easeOutQuint    : function( x, t, b, c, d ) {
            return c * ((t = t / d - 1) * t * t * t * t + 1) + b;
        },
        easeInOutQuint  : function( x, t, b, c, d ) {
            if ( (t /= d / 2) < 1 ) return c / 2 * t * t * t * t * t + b;
            return c / 2 * ((t -= 2) * t * t * t * t + 2) + b;
        },
        easeInSine      : function( x, t, b, c, d ) {
            return -c * Math.cos( t / d * (Math.PI / 2) ) + c + b;
        },
        easeOutSine     : function( x, t, b, c, d ) {
            return c * Math.sin( t / d * (Math.PI / 2) ) + b;
        },
        easeInOutSine   : function( x, t, b, c, d ) {
            return -c / 2 * (Math.cos( Math.PI * t / d ) - 1) + b;
        },
        easeInExpo      : function( x, t, b, c, d ) {
            return (t == 0) ? b : c * Math.pow( 2, 10 * (t / d - 1) ) + b;
        },
        easeOutExpo     : function( x, t, b, c, d ) {
            return (t == d) ? b + c : c * (-Math.pow( 2, -10 * t / d ) + 1) + b;
        },
        easeInOutExpo   : function( x, t, b, c, d ) {
            if ( t == 0 ) return b;
            if ( t == d ) return b + c;
            if ( (t /= d / 2) < 1 ) return c / 2 * Math.pow( 2, 10 * (t - 1) ) + b;
            return c / 2 * (-Math.pow( 2, -10 * --t ) + 2) + b;
        },
        easeInCirc      : function( x, t, b, c, d ) {
            return -c * (Math.sqrt( 1 - (t /= d) * t ) - 1) + b;
        },
        easeOutCirc     : function( x, t, b, c, d ) {
            return c * Math.sqrt( 1 - (t = t / d - 1) * t ) + b;
        },
        easeInOutCirc   : function( x, t, b, c, d ) {
            if ( (t /= d / 2) < 1 ) return -c / 2 * (Math.sqrt( 1 - t * t ) - 1) + b;
            return c / 2 * (Math.sqrt( 1 - (t -= 2) * t ) + 1) + b;
        },
        easeInElastic   : function( x, t, b, c, d ) {
            var s = 1.70158;
            var p = 0;
            var a = c;
            if ( t == 0 ) return b;
            if ( (t /= d) == 1 ) return b + c;
            if ( !p ) p = d * .3;
            if ( a < Math.abs( c ) ) {
                a     = c;
                var s = p / 4;
            }
            else var s = p / (2 * Math.PI) * Math.asin( c / a );
            return -(a * Math.pow( 2, 10 * (t -= 1) ) * Math.sin( (t * d - s) * (2 * Math.PI) / p )) + b;
        },
        easeOutElastic  : function( x, t, b, c, d ) {
            var s = 1.70158;
            var p = 0;
            var a = c;
            if ( t == 0 ) return b;
            if ( (t /= d) == 1 ) return b + c;
            if ( !p ) p = d * .3;
            if ( a < Math.abs( c ) ) {
                a     = c;
                var s = p / 4;
            }
            else var s = p / (2 * Math.PI) * Math.asin( c / a );
            return a * Math.pow( 2, -10 * t ) * Math.sin( (t * d - s) * (2 * Math.PI) / p ) + c + b;
        },
        easeInOutElastic: function( x, t, b, c, d ) {
            var s = 1.70158;
            var p = 0;
            var a = c;
            if ( t == 0 ) return b;
            if ( (t /= d / 2) == 2 ) return b + c;
            if ( !p ) p = d * (.3 * 1.5);
            if ( a < Math.abs( c ) ) {
                a     = c;
                var s = p / 4;
            }
            else var s = p / (2 * Math.PI) * Math.asin( c / a );
            if ( t < 1 ) return -.5 * (a * Math.pow( 2, 10 * (t -= 1) ) * Math.sin( (t * d - s) * (2 * Math.PI) / p )) + b;
            return a * Math.pow( 2, -10 * (t -= 1) ) * Math.sin( (t * d - s) * (2 * Math.PI) / p ) * .5 + c + b;
        },
        easeInBack      : function( x, t, b, c, d, s ) {
            if ( s == undefined ) s = 1.70158;
            return c * (t /= d) * t * ((s + 1) * t - s) + b;
        },
        easeOutBack     : function( x, t, b, c, d, s ) {
            if ( s == undefined ) s = 1.70158;
            return c * ((t = t / d - 1) * t * ((s + 1) * t + s) + 1) + b;
        },
        easeInOutBack   : function( x, t, b, c, d, s ) {
            if ( s == undefined ) s = 1.70158;
            if ( (t /= d / 2) < 1 ) return c / 2 * (t * t * (((s *= (1.525)) + 1) * t - s)) + b;
            return c / 2 * ((t -= 2) * t * (((s *= (1.525)) + 1) * t + s) + 2) + b;
        },
        easeInBounce    : function( x, t, b, c, d ) {
            return c - jQuery.easing.easeOutBounce( x, d - t, 0, c, d ) + b;
        },
        easeOutBounce   : function( x, t, b, c, d ) {
            if ( (t /= d) < (1 / 2.75) ) {
                return c * (7.5625 * t * t) + b;
            } else if ( t < (2 / 2.75) ) {
                return c * (7.5625 * (t -= (1.5 / 2.75)) * t + .75) + b;
            } else if ( t < (2.5 / 2.75) ) {
                return c * (7.5625 * (t -= (2.25 / 2.75)) * t + .9375) + b;
            } else {
                return c * (7.5625 * (t -= (2.625 / 2.75)) * t + .984375) + b;
            }
        },
        easeInOutBounce : function( x, t, b, c, d ) {
            if ( t < d / 2 ) return jQuery.easing.easeInBounce( x, t * 2, 0, c, d ) * .5 + b;
            return jQuery.easing.easeOutBounce( x, t * 2 - d, 0, c, d ) * .5 + c * .5 + b;
        }
    } );

/*
 *
 * TERMS OF USE - EASING EQUATIONS
 *
 * Open source under the BSD License.
 *
 * Copyright Â© 2001 Robert Penner
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * Neither the name of the author nor the names of contributors may be used to endorse
 * or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */