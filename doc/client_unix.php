<?php
require_once("docutil.php");
page_head("Core client configuration files, command-line options, and environment variables");
echo "
The BOINC core client can be controlled by command-line options,
configuration files, and environment variables.
<p>
<b>These features are generally useful only for
advanced users.</b>

<h3>Configuration files</h3>
<p>
BOINC uses the following configuration files.
You can pre-populate these files to control BOINC in various ways.
";
list_start();
list_item("<a href=client_files.php>Account files</a>",
    "each one describes an attached project."
);
list_item("<a href=prefs_override.php>global_prefs_override.xml</a>",
    "override global preferences"
);
list_item("<a href=client_msgs.php>cc_config.xml</a>",
    "turn log messages on/off by category,
    and other core client configuration options."
);
list_item("<a href=gui_rpc_auth.php>remote_hosts.cfg</a>",
    "hosts from which remote GUI RPCs is allowed"
);
list_item("<a href=gui_rpc_auth.php>gui_rpc_auth.cfg</a>",
    "GUI RPC password"
);
list_item("<a href=client_startup.php>project_init.xml</a>",
    "startup project"
);
list_item("<a href=client_startup.php>acct_mgr_url.xml</a>",
    "account manager URL"
);
list_item("<a href=client_startup.php>acct_mgr_login.xml</a>",
    "account manager login"
);
list_end();
echo "
<h3>Command-line options</h3>
<p>
The core client has
command-line options that provide minimal control
(e.g. the ability attach and detach projects).
<p>
More detailed control, and the ability to interact
with a running client, is provided by the
<a href=boinc_cmd.php>BOINC command tool</a>.
<p>
";
list_start();
list_item("--help",
    "Show client options."
);
list_item("--version",
    "Show client version."
);
list_item("<nobr>--attach_project URL account_key</nobr>",
    "Attach this computer to a new project."
);
list_item("--show_projects",
    "Print a list of projects to which this computer is attached."
);

list_item("--detach_project URL",
    "Detach this computer from a project."
);

list_item("--reset_project URL",
    "Clear pending work for a project.
    Use this if there is a problem that is preventing
    your computer from working."
);
list_item("--update_prefs URL",
    "Contact a project's server to obtain new preferences.
    This will also report completed results
    and get new work if needed."
);
list_item("--return_results_immediately",
    "Report each result as soon as it's done."
);
list_item("--run_cpu_benchmarks",
    "Run CPU benchmarks.
    Do this if you have modified your computer's hardware."
);
list_item("--check_all_logins",
    "(Unix) If 'run if user active' preference is off,
    check for input activity on all current logins;
    default is to check only local mouse/keyboard"
);
list_item("--exit_when_idle",
    "Get, process and report work, then exit."
);
list_item("--gui_rpc_port N",
    "Specify port for GUI RPCs"
);
list_item("--allow_remote_gui_rpc",
    "Allow GUI RPCs from remote hosts"
);
list_item("--dir abs_path",
    "Use the given directory as BOINC home"
);
list_item("--detach",
    "Detach from console (Windows only)"
);
list_item("--no_gui_rpc",
    "Don't allow GUI RPCs."
);
list_item("--daemon",
    "Linux: detach from controlling terminal; Windows: run as service"
);
list_end();
echo "
<h2>Environment variables</h2>
";
list_start();
list_item("HTTP_PROXY", "URL of HTTP proxy");
list_item("HTTP_USER_NAME", "User name for proxy authentication");
list_item("HTTP_USER_PASSWD", "Password for proxy authentication");
list_item("SOCKS4_SERVER", "URL of SOCKS 4 server");
list_item("SOCKS5_SERVER", "URL of SOCKS 5 server");
list_item("SOCKS5_USER", "User name for SOCKS authentication");
list_item("SOCKS5_PASSWD", "Password for SOCKS authentication");
list_end();
echo "
<h2>Implementation and debugging command-line options</h2>
";
list_start();
list_item(" --exit_when_idle ",
    " Exit when we have no work and a scheduling server
gives a 'no work' return."
);
list_item(" --no_time_test",
    " Don't run performance benchmarks; used fixed numbers instead."
);
list_item(" --exit_after_app_start N",
    " Exit about N seconds after first application starts"
);
list_item(" --min",
    " Put client in the background after starting up"
);
list_item(" --skip_cpu_benchmarks",
    " Don't run CPU benchmarks"
);
list_item(" --file_xfer_giveup_period N",
    " Specify giveup period for file transfers"
);
list_item(" --started_by_screensaver N",
    " Passed by screensaver when it launches client"
);
list_end();
page_tail();
?>
