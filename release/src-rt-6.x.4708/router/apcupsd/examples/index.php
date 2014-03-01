<html>
<body link="$0000ff" vlink="$0000ff">
<?php
/* apcupsd monitoring script
  Copyright April 25, 2005, Rob Kroll, rob@killerbob.ca

  Because I don't think I can keep anybody from doing what they want with it, nor would I want to, I'm releasing this script
  to the world without restrictions. There's also no warranty, implied or otherwise. If this script somehow manages to make
  your computer blow up, you're on your own. That said, I sincerely doubt that's even possible.

  User-Configurable variables below:
      $statusfile_location       This is the location of the apcupsd.status file, as found in /etc/apcupsd/apcupsd.conf
                                 If this file exists, the graphical parse will be displayed.
      $logfile_location          This is the location of the apcupsd.events file, as found in /etc/apcupsd/apcupsd.conf
                                 If this file exists, it will be displayed with most recent entries first.
      $showloglines              By default, this script does not display the entire .events file. This is the number of
                                 lines to display by default. The script generates a link that enables displaying the
                                 entire log.
      $goodcolour                Default: "lightgreen". This is the HTML colour for the background of cells where things
                                 are good.
      $warncolour                Default: "yellow". This is the HTML colour for the background of cells where things are
                                 in need of attention.
      $badcolour                 Default: "red". This is the HTML colour for the background of cells where things are bad.

*/


$statusfile_location = "/var/log/apcupsd.status";
$logfile_location = "/var/log/apcupsd.events";

$showloglines = 5;

$goodcolour = "lightgreen";
$warncolour = "yellow";
$badcolour = "red";

/*

  There are no more user-configurable variables beyond this point. You're welcome to browse the code, and submit any bugfixes you
  find. If you don't know much about coding, it can be educational, but you probably don't need to read further.

*/

// There's a logic problem with the display of a set number of log lines. I'm lazy, so rather than fix the logic, I'm just going
// to increase the number of log lines to show. The only problem this should cause is with a very short log.

// $showloglines++;


$showlog = $_GET["showlog"];


echo "<table border=2 width=\"100%\">\n";
echo "<tr><td colspan=2 align=center><b>UPS Status</b></td></tr><tr>\n";
if (file_exists($statusfile_location)) {
	// Open the Status file.
	$statusfile = file($statusfile_location);
	
	// Get the word status ("ONLINE", "ON-BATT", etc.)
	$dumbvar = split(":", $statusfile[10]);
	$status = $dumbvar[1];
	if (eregi('online', $status)) {
		$statusbg = $goodcolour;
	} else $statusbg = $warncolour;

	// Get the current UPS load
	$dumbvar = split(":", $statusfile[12]);
	$load = floatval($dumbvar[1]);
	if ($load > 75) {
		$loadbg = $badcolour;
	} else if ($load > 25) {
		$loadbg = $warncolour;
	} else $loadbg = $goodcolour;

	// Get the battery charge level
	$dumbvar = split(":", $statusfile[13]);
	$charge = floatval($dumbvar[1]);
	if ($charge > 60) {
		$chargebg = $goodcolour;
	} else if ($charge > 25) {
		$chargebg = $warncolour;
	} else $chargebg = $badcolour;

	// Get the estimated time remaining
	$dumbvar = split(":", $statusfile[14]);
	$etl = $dumbvar[1];

	// Draw the table entry.
	echo "<td valign=center>\n";
	echo "<table border=0 width=\"100%\"><tr><td>\n";
	echo "<b>Battery Status:</b></td><td bgcolor=" . $statusbg . " align=center>" . $status . "</td></tr><tr><td>\n";
	echo "<b>UPS Load:</b></td><td bgcolor=" . $loadbg . " align=center>" . $load . "</td></tr><tr><td>\n";
	echo "<b>Battery Charge:</b></td><td bgcolor=" . $chargebg . " align=center>". $charge . "</td></tr><tr><td>\n";
	echo "<b>Estimated time left:</b></td><td align=center>" . $etl . "</td></tr></table></td>\n";
};

if (file_exists($logfile_location)) {
	// Open the log file.
	$logfile = file($logfile_location);
	$loglines = count($logfile);
	if ($loglines < $showloglines) $showloglines = $loglines;
	
	// Draw the table
	echo "<td>";
	if ($showlog == "yes") {   // If the user has clicked the "Show entire log" link, show the whole shebang in reverse order.
		echo "<table border=0 width=\"100%\">";
		for ($i = 0; $i <= $loglines; $i++) {
			$newline = $logfile[($loglines - $i - 1)];
			$notifycolour = "white";
			if (eregi("power is back", $newline)) {
				$notifycolour = $goodcolour;
			} else if (eregi("power failure", $newline)) {
				$notifycolour = $badcolour;
			} else if (eregi("startup succeeded", $newline)) {
				$notifycolour = $goodcolour;
			} else if (eregi("exiting", $newline)) {
				$notifycolour = $warncolour;
			} else if (eregi("shutdown succeeded", $newline)) {
				$notifycolour = $goodcolour;
			} else if (eregi("ups batteries", $newline)) {
				$notifycolour = $warncolour;
			} else if (eregi("exhausted", $newline)) {
				$notifycolour = $badcolour;
			} else {
				$notifycolour = $warncolour;
			};
			echo "<tr><td bgcolor=\"". $notifycolour ."\" width=15></td><td>";
			echo $newline;
			echo "</td></tr>\n";
		}
		echo "</table>\n";
		echo "</td></tr>\n";
		echo "<tr><td colspan=2 align=right>\n";
		
		// Add a link so that the user can switch this view off.
		echo "<a href=\"" . $_SERVER["PHP_SELF"] . "\">Hide old log entries</a></td></tr>\n";
	} else {  // By default, just show the last X lines. Save some screen real estate.
		echo "<table border=0 width=\"100%\">";
		for ($i = 0; $i < $showloglines; $i++) {
			$newline = $logfile[($loglines - $i - 1)];
			$notifycolour = "white";
			if (eregi("power is back", $newline)) {
				$notifycolour = $goodcolour;
			} else if (eregi("power failure", $newline)) {
				$notifycolour = $badcolour;
			} else if (eregi("startup succeeded", $newline)) {
				$notifycolour = $goodcolour;
			} else if (eregi("exiting", $newline)) {
				$notifycolour = $warncolour;
			} else if (eregi("shutdown succeeded", $newline)) {
				$notifycolour = $goodcolour;
			} else if (eregi("ups batteries", $newline)) {
				$notifycolour = $warncolour;
			} else if (eregi("exhausted", $newline)) {
				$notifycolour = $badcolour;
			} else {
				$notifycolour = $warncolour;
			};
			echo "<tr><td bgcolor=\"". $notifycolour ."\" width=15></td><td>";
			echo $newline;
			echo "</td></tr>\n";
		}
		echo "</table>\n";
		echo "</td></tr>\n";
		echo "<tr><td colspan=2 align=right>\n";
		
		// Add a link so the user can see the entire log from the beginning.
		echo "<a href=\"" . $_SERVER["PHP_SELF"] . "?showlog=yes\">Show entire log</a></td>\n";
	}
};
echo "</tr></table>";
?>
</body>
</html>
