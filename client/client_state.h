// The contents of this file are subject to the BOINC Public License
// Version 1.0 (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://boinc.berkeley.edu/license_1.0.txt
//
// Software distributed under the License is distributed on an "AS IS"
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
// License for the specific language governing rights and limitations
// under the License.
//
// The Original Code is the Berkeley Open Infrastructure for Network Computing.
//
// The Initial Developer of the Original Code is the SETI@home project.
// Portions created by the SETI@home project are Copyright (C) 2002
// University of California at Berkeley. All Rights Reserved.
//
// Contributor(s):
//

#ifndef _CLIENT_STATE_
#define _CLIENT_STATE_

#ifndef _WIN32
#include <vector>
#include <ctime>
#endif

#include "app.h"
#include "client_types.h"
#include "file_xfer.h"
#include "gui_rpc_server.h"
#include "hostinfo.h"
#include "http.h"
#include "language.h"
#include "miofile.h"
#include "net_stats.h"
#include "net_xfer.h"
#include "pers_file_xfer.h"
#include "prefs.h"
#include "scheduler_op.h"
#include "ss_logic.h"
#include "time_stats.h"

extern int add_new_project();

#define USER_RUN_REQUEST_ALWAYS     1
#define USER_RUN_REQUEST_AUTO       2
#define USER_RUN_REQUEST_NEVER      3

#define DONT_NEED_WORK 0
#define NEED_WORK 1
#define NEED_WORK_IMMEDIATELY 2

enum SUSPEND_REASON_t {
    SUSPEND_REASON_BATTERIES = 1,
    SUSPEND_REASON_USER_ACTIVE = 2,
    SUSPEND_REASON_USER_REQ = 4,
    SUSPEND_REASON_TIME_OF_DAY = 8,
    SUSPEND_REASON_BENCHMARKS = 16,
    SUSPEND_REASON_DISK_SIZE = 32
};

#define CPU_HALF_LIFE (86400*7)

// CLIENT_STATE encapsulates the global variables of the core client.
// If you add anything here, initialize it in the constructor
//
class CLIENT_STATE {
public:
  std::vector<PROJECT*> projects;
  std::vector<APP*> apps;
  std::vector<FILE_INFO*> file_infos;
  std::vector<APP_VERSION*> app_versions;
  std::vector<WORKUNIT*> workunits;
  std::vector<RESULT*> results;

    NET_XFER_SET* net_xfers;
    PERS_FILE_XFER_SET* pers_file_xfers;
    HTTP_OP_SET* http_ops;
    FILE_XFER_SET* file_xfers;
    ACTIVE_TASK_SET active_tasks;
    HOST_INFO host_info;
    GLOBAL_PREFS global_prefs;
    NET_STATS net_stats;
    SS_LOGIC ss_logic;
    GUI_RPC_CONN_SET gui_rpcs;
    LANGUAGE language;
    TIME_STATS time_stats;
    PROXY_INFO proxy_info;

    int core_client_major_version;
    int core_client_minor_version;
    int file_xfer_giveup_period;
    int user_run_request;
        // values above (USER_RUN_REQUEST_*)
    int user_network_request;
        // same, just for network
    bool started_by_screensaver;
    bool exit_when_idle;
    bool check_all_logins;
    bool return_results_immediately;
    bool allow_remote_gui_rpc;
    bool show_projects;
    bool requested_exit;
    char detach_project_url[256];
        // stores URL for -detach_project option
    char reset_project_url[256];
        // stores URL for -reset_project option
    char update_prefs_url[256];
        // stores URL for -update_prefs option
    char host_venue[256];    // venue, as reported by project that sent us
        // most recent global prefs
    bool exit_before_upload;
        // exit when about to upload a file
    // exponential backoff variables
    int master_fetch_period, retry_base_period, retry_cap;
    int master_fetch_retry_cap, master_fetch_interval;
    int sched_retry_delay_min, sched_retry_delay_max;
    int pers_retry_delay_min, pers_retry_delay_max, pers_giveup;
    bool activities_suspended;
	bool previous_activities_suspended;
		// if activities were suspended in the previous check_suspend();
		// this is needed to update GUI windows after suspension and close transfers/files.
    bool network_suspended;
	bool executing_as_windows_service;
    bool size_overflow;

private:
    bool client_state_dirty;
    int old_major_version;
    int old_minor_version;
    char* platform_name;
    bool skip_cpu_benchmarks;
        // if set, use hardwired numbers rather than running benchmarks
    bool run_cpu_benchmarks;
        // if set, run benchmarks on client startup
    int exit_after_app_start_secs;
        // if nonzero, exit this many seconds after starting an app
    time_t app_started;
        // when the most recent app was started

    // CPU sched state
    //
    time_t cpu_sched_last_time;
    double cpu_sched_work_done_this_period;

// --------------- client_state.C:
public:
    CLIENT_STATE();
    int init();
    bool do_something();
        // Initiates and completes actions (file transfers, process executions)
        // Never blocks.
        // Returns true if it actually did something,
        // in which case it should be called again immediately.
    int net_sleep(double dt);
        // sleeps until either dt seconds have elapsed,
        // or until there's network activity.
    bool time_to_exit();
    PROJECT* lookup_project(const char*);
    APP* lookup_app(PROJECT*, const char*);
    FILE_INFO* lookup_file_info(PROJECT*, const char* name);
    RESULT* lookup_result(PROJECT*, const char*);
    WORKUNIT* lookup_workunit(PROJECT*, const char*);
    APP_VERSION* lookup_app_version(APP*, int);
    ACTIVE_TASK* lookup_active_task_by_result(RESULT*);
    int detach_project(PROJECT*);
    int report_result_error(RESULT &res, int err_num, const char *format, ...);
        // flag a result as having an error
    int reset_project(PROJECT*);
private:
    int link_app(PROJECT*, APP*);
    int link_file_info(PROJECT*, FILE_INFO*);
    int link_file_ref(PROJECT*, FILE_REF*);
    int link_app_version(PROJECT*, APP_VERSION*);
    int link_workunit(PROJECT*, WORKUNIT*);
    int link_result(PROJECT*, RESULT*);
    void print_summary();
    bool garbage_collect();
    bool update_results();
    double total_resource_share();

// --------------- cs_account.C:
public:
    int add_project(const char* master_url, const char* authenticator);
private:
    int parse_account_files();
    int parse_preferences_for_user_files();
// --------------- cs_apps.C:
public:
    bool must_schedule_cpus;
        // set when a result is newly runnable or finished
    int restart_tasks();
    int quit_activities();
    void set_ncpus();
    double estimate_cpu_time(WORKUNIT&);
    double get_fraction_done(RESULT* result);
    bool input_files_available(RESULT*);
    ACTIVE_TASK* get_next_graphics_capable_app();
private:
    int ncpus;
    int nslots;

    int choose_version_num(char*, SCHEDULER_REPLY&);
    int app_finished(ACTIVE_TASK&);
    void assign_results_to_projects();
    bool schedule_largest_debt_project(double expected_pay_off);
    bool start_apps();
    bool schedule_cpus();
    bool handle_finished_apps();
    void handle_file_xfer_apps();
    int schedule_result(RESULT*);

// --------------- cs_benchmark.C:
public:
    bool should_run_cpu_benchmarks();
	void start_cpu_benchmarks();
    bool cpu_benchmarks_poll();
    void abort_cpu_benchmarks();
    bool are_cpu_benchmarks_running();

// --------------- cs_cmdline.C:
public:
    void parse_cmdline(int argc, char** argv);
    void parse_env_vars();
    void do_cmdline_actions();

// --------------- cs_files.C:
public:
    void trunc_stderr_stdout();
    bool start_new_file_xfer(PERS_FILE_XFER&);
private:
    int make_project_dirs();
    bool handle_pers_file_xfers();

// --------------- cs_prefs.C:
public:
    int project_disk_usage(PROJECT*, double&);
    int total_disk_usage(double&);
        // returns the total disk usage of BOINC on this host
    int allowed_disk_usage(double&);
    int allowed_project_disk_usage(double&);
private:
    void check_suspend_activities(int&);
    int suspend_activities(int reason);
    int resume_activities();
    void check_suspend_network(int&);
    int suspend_network(int reason);
    int resume_network();
    void install_global_prefs();
    void show_global_prefs_source(bool);

// --------------- cs_scheduler.C:
public:
    double work_needed_secs();
    PROJECT* next_project_master_pending();
    PROJECT* next_project_need_work(PROJECT*);
    int make_scheduler_request(PROJECT*, double);
    int handle_scheduler_reply(PROJECT*, char* scheduler_url, int& nresults);
    int compute_work_requests();
private:
    SCHEDULER_OP* scheduler_op;
    bool contacted_sched_server;

    PROJECT* find_project_with_overdue_results();
    PROJECT* next_project_sched_rpc_pending();
    bool some_project_rpc_ok();
    bool scheduler_rpc_poll();
    double ettprc(PROJECT*, int);
    double avg_proc_rate(PROJECT*);

// --------------- cs_statefile.C:
public:
    void set_client_state_dirty(char*);
    int parse_state_file();
    int write_state(MIOFILE&);
    int write_state_file();
    int write_state_file_if_needed();
    int parse_venue();
	void check_anonymous();
	int parse_app_info(PROJECT*, FILE*);
    int write_state_gui(MIOFILE&);
    int write_file_transfers_gui(MIOFILE&);
    int write_tasks_gui(MIOFILE&);

// --------------- cs_trickle.C:
private:
    int read_trickle_files(PROJECT*, FILE*);
    int remove_trickle_files(PROJECT*);
public:
    int handle_trickle_down(PROJECT*, FILE*);

// --------------- check_state.C:
// stuff related to data-structure integrity checking
//
public:
    void check_project_pointer(PROJECT*);
    void check_app_pointer(APP*);
    void check_file_info_pointer(FILE_INFO*);
    void check_app_version_pointer(APP_VERSION*);
    void check_workunit_pointer(WORKUNIT*);
    void check_result_pointer(RESULT*);
    void check_pers_file_xfer_pointer(PERS_FILE_XFER*);
    void check_file_xfer_pointer(FILE_XFER*);

    void check_app(APP&);
    void check_file_info(FILE_INFO&);
    void check_file_ref(FILE_REF&);
    void check_app_version(APP_VERSION&);
    void check_workunit(WORKUNIT&);
    void check_result(RESULT&);
    void check_active_task(ACTIVE_TASK&);
    void check_pers_file_xfer(PERS_FILE_XFER&);
    void check_file_xfer(FILE_XFER&);

    void check_all();

// ------------------ cs_data.C:
// mechanisms for managing data saved on host
//
public:   
    bool get_more_disk_space(PROJECT*, double);
    int anything_free(double &);
    int calc_proj_size(PROJECT*);
    int calc_all_proj_size();
    int compute_share_disk_size(PROJECT*);
    int total_potential_offender(PROJECT*, double &);
    int total_potential_self(PROJECT*, double &);
    double select_delete(PROJECT*, double, int);
    double delete_results(PROJECT*, double);
    double compute_resource_share(PROJECT*);
    PROJECT* greatest_offender();
private:
    bool data_manager_poll();
    bool fix_data_overflow(double, double);
    int reset_checks();
    int delete_inactive_results(PROJECT*);
    int unstick_result_files(RESULT*);
    double delete_next_file(PROJECT*, int); 
    double delete_expired(PROJECT*);
    double offender(PROJECT*);
    double proj_potentially_free(PROJECT*);
    FILE_INFO* get_priority_or_lru(PROJECT*, int);
};

extern CLIENT_STATE gstate;

#endif
