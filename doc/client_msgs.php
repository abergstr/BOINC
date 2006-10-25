<?php
require_once("docutil.php");
page_head("Core client configuration");
echo "
The core client reads configuration options from a file
<b>cc_config.xml</b>
(create this file yourself, with a text editor).
This file has the following format: 
".html_text("
<cc_config>
    <options>
        [ <save_stats_days>N</save_stats_days> ]
        [ <dont_check_file_sizes>0|1</dont_check_file_sizes> ]
        [ <ncpus>N</ncpus> ]
        [ <max_file_xfers>N</max_file_xfers> ]
        [ <max_file_xfers_per_project>N</max_file_xfers_per_project> ]
    </options>
    <log_flags>
        [ flags ]
    </log_flags>
</cc_config>
")."
The following options control the behavior of BOINC:
";
list_start();
list_item_func("<save_stats_days>",
    "How many days to save the per-project credit totals
    that are displayed in the Statistics tab of the BOINC Manager.
    Default is 30."
);
list_item_func("<dont_check_file_sizes>",
    "Normally, the size of application and input files
    are compared with the project-supplied values
    after the files are downloaded,
    and just before starting an application.
    If this flag is set, this check is skipped.
    Use it if you need to modify files locally for some reason."
); 
list_item_func("<ncpus>",
    "Act as if there were N CPUs: run N tasks at once.
    This is for debugging, i.e. to simulate 2 CPUs
    on a machine that has only 1.
    Don't use it to limit the number of CPUs used by BOINC;
    use general preferences instead."
);
list_item_func("<max_file_xfers>",
    "Maximum number of simultaneous file transfers (default 8)."
);
list_item_func("<max_file_xfers_per_project>",
    "Maximum number of simultaneous file transfers per project (default 2)."
);
list_end();
echo "
The flags within &lt;log_flags&gt;
control the messages generated by the client.
These messages appear in the Messages tab of the BOINC Manager
(informational messages in black, error messages in red).
On Windows, they are written to a file (stdoutdae.txt).
On Unix, they are written to standard output.
Different types of messages can selectively be turned on and off 
(&lt;tag>0&lt;/tag> for off, &lt;tag>1&lt;/tag> for on):
<p>
The following messages are enabled by default:
";
list_start();
list_item_func("<task>",
    "The start and completion of compute jobs
    (should get two messages per job)."
);
list_item_func("<file_xfer>",
    "The start and completion of file transfers."
);
list_item_func("<sched_ops>",
    "Connections with scheduling servers."
);
list_end();
echo "
The following messages are disabled by default
(typically they generate lots of output,
and are used for specific debugging purposes):
";
list_start();
list_item_func("<cpu_sched>",
    "CPU scheduler actions (preemption and resumption)"
);
list_item_func("<cpu_sched_debug>",
    "Explain CPU scheduler decisions"
);
list_item_func("<rr_simulation>",
    "Results of the round-robin simulation used by CPU scheduler and work-fetch"
);
list_item_func("<debt_debug>",
    "Changes to project debt"
);
list_item_func("<task_debug>",
    "Low-level details of process start/end (status codes, PIDs etc.),
    and when applications checkpoint."
);
list_item_func("<work_fetch_debug>",
    "Work fetch policy decisions"
);
list_item_func("<unparsed_xml>",
    "Show any unparsed XML"
);
list_item_func("<state_debug>",
    "Show summary of client state after scheduler RPC and garbage collection;
    also show garbage collection actions, and when state file is read/written."
);
list_item_func("<file_xfer_debug>",
    "Show completion status of file transfers"
);
list_item_func("<sched_op_debug>",
    "Details of scheduler RPCs"
);
list_item_func("<http_debug>",
    "Debugging information about HTTP operations"
);
list_item_func("<proxy_debug>",
    "Debugging information about HTTP proxy operations"
);
list_item_func("<time_debug>",
    "Updates to on_frac, active_frac, connected_frac."
);
list_item_func("<http_xfer_debug>",
    "Debugging information about network communication"
);
list_item_func("<measurement_debug>",
    "Debugging information about CPU benchmarks, platform, disk space, etc. "
);
list_item_func("<poll_debug>",
    "Show what poll functions do"
);
list_item_func("<guirpc_debug>",
    "Debugging information about GUI RPC operations"
);
list_item_func("<scrsave_debug>",
    "Debugging information about the screen saver."
);
list_item_func("<app_msg_debug>",
    "Shared-memory messages from applications."
);
list_item_func("<mem_usage_debug>",
    "Application memory usage."
);
list_item_func("<network_status_debug>",
    "Network status (whether need physical connection)."
);
list_end();

page_tail();
?>
