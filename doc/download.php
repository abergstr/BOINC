<?php

require_once("docutil.php");

function version_start($num, $date, $xml, $comment=null) {
    if ($xml) return;
    list_start();
    list_bar("Version $num (released $date)");
    if ($comment) {
        list_bar($comment);
    }
    list_heading("Platform", "click to download", "MD5 checksum of download file");
}

function version($platform, $filename, $xml) {
    $path = "dl/$filename";
    $dlink = "<a href=http://boinc.berkeley.edu/$path>Download</a>";
    $md = md5_file($path);
    if ($xml) {
        echo "
<version>
    <platform>$platform</platform>
    <url>http://boinc.berkeley.edu/$path</url>
    <filename>$filename</filename>
    <md5>$md</md5>
</version>
";
    } else {
        list_item($platform, $dlink, $md);
    }
}

function version_end($xml) {
    if ($xml) return;
    list_end();
}

function show_425($xml=false) {
    version_start("4.25", "3 Mar 2005", $xml);
    version("Windows", "boinc_4.25_windows_intelx86.exe", $xml);
    version("Mac OS X", "boinc_4.25_powerpc-apple-darwin.gz", $xml);
    version("Linux/x86", "boinc_4.25_i686-pc-linux-gnu.sh", $xml);
    version("Solaris/SPARC", "boinc_4.25_sparc-sun-solaris2.7.sh", $xml);
    version_end($xml);
}

function show_424($xml=false) {
    version_start("4.24", "23 Feb 2005", $xml);
    version("Linux/x86", "boinc_4.24_i686-pc-linux-gnu.sh", $xml);
    version_end($xml);
}

function show_423($xml=false) {
    version_start("4.23", "21 Feb 2005", $xml);
    version("Windows", "boinc_4.23_windows_intelx86.exe", $xml);
    version("Linux/x86", "boinc_4.23_i686-pc-linux-gnu.sh", $xml);
    version("Solaris/SPARC", "boinc_4.23_sparc-sun-solaris2.7.sh", $xml);
    version_end($xml);
}

function show_422($xml=false) {
    version_start("4.22", "18 Feb 2005", $xml);
    version("Windows", "boinc_4.22_windows_intelx86.exe", $xml);
    version_end($xml);
}

function show_421($xml=false) {
    version_start("4.21", "11 Feb 2005", $xml);
    version("Windows", "boinc_4.21_windows_intelx86.exe", $xml);
    version_end($xml);
}

function show_420($xml=false) {
    version_start("4.20", "8 Feb 2005", $xml);
    version("Windows", "boinc_4.20_windows_intelx86.exe", $xml);
    version_end($xml);
}

function show_419($xml=false) {
    version_start("4.19", "25 Jan 2005", $xml,
        "This version doesn't work with some HTTP proxies.
        If you use a proxy and experience problems,
        please use version 4.23 or later,
        in which this problem is fixed."
    );
    version("Windows", "boinc_4.19_windows_intelx86.exe", $xml);
    version("Linux/x86", "boinc_4.19_i686-pc-linux-gnu.gz", $xml);
    version("Mac OS X", "boinc_4.19_powerpc-apple-darwin.gz", $xml);
    version("Solaris/SPARC", "boinc_4.19_sparc-sun-solaris2.7.gz", $xml);
    version_end($xml);
}

function show_418($xml=false) {
    version_start("4.18", "24 Jan 2005", $xml);
    version("Windows", "boinc_4.18_windows_intelx86.exe", $xml);
    version("Linux/x86", "boinc_4.18_i686-pc-linux-gnu.gz", $xml);
    version("Mac OS X", "boinc_4.18_powerpc-apple-darwin.gz", $xml);
    version("Solaris/SPARC", "boinc_4.18_sparc-sun-solaris2.7.gz", $xml);
    version_end($xml);
}

function show_stable($xml) {
    show_419($xml);
}

function show_dev($xml) {
    show_425($xml);
}

if ($_GET["no_header"]) {
    show_stable(false);
    if ($_GET["dev"]) {
        show_dev(false);
    }
    exit();
}

if ($_GET["xml"]) {
    header ("Content-Type: text/xml");
    echo "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>
<core_versions>
    ";
    echo "<stable_versions/>\n";
    show_stable(true);
    echo "<unstable_versions/>\n";
    show_dev(true);
    echo "</core_versions>\n";
    exit();
}

page_head("Download BOINC client software");

echo "

<h2>Stable version (recommended)</h2>
";
show_stable(false);
echo "
    After the download is finished:
    <ul>
    <li><b>Windows</b>:
        open the downloaded file.
        This will install BOINC on your computer.
    <li><b>Macintosh OS X, Unix and Linux</b>:
        Use gunzip to uncompress if your browser has not done it for you.
        Then chmod +x the executable and run it.
    </ul>
    <p>

    If your computer is not of one of the above types, you can
    <ul>
    <li> <a href=anonymous_platform.php>download and compile the BOINC software yourself</a> or
    <li> <a href=download_other.php>download from a third-party site</a>.
    </ul>
    <p>
    There are no specific hardware requirements
    (CPU speed, memory, disk space, etc.).
    However, these factors may limit the amount or type
    of work that is sent to your computer.
    <p>
    The Windows version can be customized for
    <a href=http://boinc.berkeley.edu/language.php>languages other than English</a>

";

echo "
    <a name=dev>
    <h2>Development versions (latest features, possibly buggy)</h2>
";
show_dev(false);

echo "
    <p>
    Get data in <a href=download.php?xml=1>XML format</a>.
";


page_tail();

?>
