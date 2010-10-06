// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2008 University of California
//
// BOINC is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// BOINC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with BOINC.  If not, see <http://www.gnu.org/licenses/>.

// BOINC client simulator.
//
// usage:
// sim [--duration x] [--delta x] [--dirs dir ...]
//  duration = simulation duration (default 86400)
//  delta = simulation time step (default 10)
//
// If no dirs are specified:
// reads input files
//    sim_projects.xml, sim_host.xml, sim_prefs.xml, cc_config.xml
// and does simulation, generating output files
//    sim_log.txt, sim_out.html
//
// If dirs are specified, chdir into each directory in sequence,
// do the above for each one, and write summary info to stdout

#ifdef _MSC_VER
#define chdir _chdir
#endif

#include "error_numbers.h"
#include "str_util.h"
#include "util.h"
#include "log_flags.h"
#include "filesys.h"
#include "client_msgs.h"
#include "client_state.h"
#include "../sched/edf_sim.h"
#include "sim.h"

#define SCHED_RETRY_DELAY_MIN    60                // 1 minute
#define SCHED_RETRY_DELAY_MAX    (60*60*4)         // 4 hours

#ifdef _WIN32
#define SIM_EXEC "..\\boincsim"
#else
#define SIM_EXEC "../sim"
#endif

bool user_active;
double duration = 86400, delta = 60;
FILE* logfile;
bool running;
double running_time = 0;
bool server_uses_workload = false;
bool dcf_dont_use;
bool dcf_stats;
bool dual_dcf;
bool cpu_sched_rr_only;
bool work_fetch_old;
int line_limit = 1000000;

SIM_RESULTS sim_results;

void PROJECT::update_dcf_stats(RESULT* rp) {
    double raw_ratio = rp->final_cpu_time/rp->estimated_duration_uncorrected();
    // see http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Algorithm_III
    ++completed_task_count;
    double delta = raw_ratio - completions_ratio_mean;
    completions_ratio_mean += delta / completed_task_count;
    completions_ratio_s += delta * ( raw_ratio - completions_ratio_mean);
    if (completed_task_count > 1) {
        completions_ratio_stdev = sqrt(completions_ratio_s / (completed_task_count - 1));
        double required_stdev = (raw_ratio - completions_ratio_mean) / completions_ratio_stdev;
        if (required_stdev > completions_required_stdevs) {
            completions_required_stdevs = std::min(required_stdev, 7.0);
        }
    }
    duration_correction_factor = completions_ratio_mean + 
        completions_required_stdevs * completions_ratio_stdev;
    return;
}

APP* choose_app(vector<APP*>& apps) {
    double x = drand();
    double sum = 0;
    unsigned int i;

    for (i=0; i<apps.size(); i++) {
        sum += apps[i]->weight;
    }
    for (i=0; i<apps.size(); i++) {
        APP* app = apps[i];
        x -= app->weight/sum;
        if (x <= 0) {
            return app;
        }
    }
    return apps.back();
}

bool app_version_needs_work(APP_VERSION* avp) {
    if (avp->ncudas) {
        return (cuda_work_fetch.req_secs>0 || cuda_work_fetch.req_instances>0);
    }
    if (avp->natis) {
        return (ati_work_fetch.req_secs>0 || ati_work_fetch.req_instances>0);
    }
    return (cpu_work_fetch.req_secs>0 || cpu_work_fetch.req_instances>0);
}

bool has_app_version_needing_work(APP* app) {
    for (unsigned int i=0; i<gstate.app_versions.size(); i++) {
        APP_VERSION* avp = gstate.app_versions[i];
        if (avp->app != app) continue;
        if (app_version_needs_work(avp)) return true;
    }
    return false;
}

// choose a version for this app for which we need work
//
APP_VERSION* choose_app_version(APP* app) {
    APP_VERSION* best_avp = NULL;
    for (unsigned int i=0; i<gstate.app_versions.size(); i++) {
        APP_VERSION* avp = gstate.app_versions[i];
        if (avp->app != app) continue;
        if (!app_version_needs_work(avp)) continue;
        if (!best_avp) {
            best_avp = avp;
        } else if (avp->flops > best_avp->flops) {
            best_avp = avp;
        }
    }
    return best_avp;
}

// generate a job; pick a random app for this project,
// and pick a FLOP count from its distribution
//
void make_job(
    PROJECT* p, WORKUNIT* wup, RESULT* rp, vector<APP*>app_list
) {
    APP* app = choose_app(app_list);
    APP_VERSION* avp = choose_app_version(app);
    rp->clear();
    rp->avp = avp;
    rp->app = app;
    if (!rp->avp) {
        printf("ERROR - NO APP VERSION\n");
        exit(1);
    }
    rp->project = p;
    rp->wup = wup;
    sprintf(rp->name, "%s_%d", p->project_name, p->result_index++);
    wup->project = p;
    wup->rsc_fpops_est = app->fpops_est;
    strcpy(wup->name, rp->name);
    strcpy(wup->app_name, app->name);
    wup->app = app;
    double ops = app->fpops.sample();
    if (ops < 0) ops = 0;
    rp->final_cpu_time = ops/avp->flops;
    rp->report_deadline = gstate.now + app->latency_bound;
}

// process ready-to-report results
//
void CLIENT_STATE::handle_completed_results(PROJECT* p) {
    char buf[256];
    vector<RESULT*>::iterator result_iter;

    result_iter = results.begin();
    while (result_iter != results.end()) {
        RESULT* rp = *result_iter;
        if (rp->project == p && rp->ready_to_report) {
            sprintf(buf, "result %s reported; %s<br>",
                rp->name,
                (gstate.now > rp->report_deadline)?
                "<font color=#cc0000>MISSED DEADLINE</font>":
                "<font color=#00cc00>MADE DEADLINE</font>"
            );
            PROJECT* spp = rp->project;
            if (gstate.now > rp->report_deadline) {
                sim_results.cpu_wasted += rp->final_cpu_time;
                sim_results.nresults_missed_deadline++;
                spp->project_results.nresults_missed_deadline++;
                spp->project_results.cpu_wasted += rp->final_cpu_time;
            } else {
                sim_results.cpu_used += rp->final_cpu_time;
                sim_results.nresults_met_deadline++;
                spp->project_results.nresults_met_deadline++;
                spp->project_results.cpu_used += rp->final_cpu_time;
            }
            gstate.html_msg += buf;
            delete rp;
            result_iter = results.erase(result_iter);
        } else {
            result_iter++;
        }
    }
}

// convert results in progress to IP_RESULTs,
// and get an initial schedule for them
//
void CLIENT_STATE::get_workload(vector<IP_RESULT>& ip_results) {
    for (unsigned int i=0; i<results.size(); i++) {
        RESULT* rp = results[i];
        double x = rp->estimated_time_remaining(false);
        if (x == 0) continue;
        IP_RESULT ipr(rp->name, rp->report_deadline, x);
        ip_results.push_back(ipr);
    }
    init_ip_results(work_buf_min(), ncpus, ip_results);
}

void get_apps_needing_work(PROJECT* p, vector<APP*>& apps) {
    apps.clear();
    for (unsigned int i=0; i<gstate.apps.size(); i++) {
        APP* app = gstate.apps[i];
        if (app->project != p) continue;
        if (!has_app_version_needing_work(app)) continue;
        apps.push_back(app);
    }
}

void decrement_request_rsc(
    RSC_WORK_FETCH& rwf, double ninstances, double est_runtime
) {
    rwf.req_secs -= est_runtime * ninstances;
    rwf.req_instances -= ninstances;
}

void decrement_request(RESULT* rp) {
    APP_VERSION* avp = rp->avp;
    double est_runtime = rp->wup->rsc_fpops_est/avp->flops;
    decrement_request_rsc(cpu_work_fetch, avp->avg_ncpus, est_runtime);
    decrement_request_rsc(cuda_work_fetch, avp->ncudas, est_runtime);
    decrement_request_rsc(ati_work_fetch, avp->natis, est_runtime);
}

// simulate trying to do an RPC;
// return true if we actually did one
//
bool CLIENT_STATE::simulate_rpc(PROJECT* p) {
    char buf[256], buf2[256];
    static double last_time=-1e9;
    vector<IP_RESULT> ip_results;
    int infeasible_count = 0;
    vector<RESULT*> new_results;

    double diff = now - last_time;
    if (diff && diff < connection_interval) {
        msg_printf(NULL, MSG_INFO,
            "simulate_rpc: too soon %f < %f",
            diff, connection_interval
        );
        return false;
    }
    last_time = now;

    // save request params for WORK_FETCH::handle_reply
    double save_cpu_req_secs = cpu_work_fetch.req_secs;
    host_info.coprocs.cuda.req_secs = cuda_work_fetch.req_secs;
    host_info.coprocs.ati.req_secs = ati_work_fetch.req_secs;


    work_fetch.request_string(buf2);
    sprintf(buf, "RPC to %s: %s<br>", p->project_name, buf2);
    html_msg += buf;

    msg_printf(0, MSG_INFO, buf);

    handle_completed_results(p);

    if (server_uses_workload) {
        get_workload(ip_results);
    }

    bool sent_something = false;
    while (1) {
        vector<APP*> apps;
        get_apps_needing_work(p, apps);
        if (apps.empty()) break;
        RESULT* rp = new RESULT;
        WORKUNIT* wup = new WORKUNIT;
        make_job(p, wup, rp, apps);

        if (server_uses_workload) {
            IP_RESULT c(rp->name, rp->report_deadline, rp->final_cpu_time);
            if (check_candidate(c, ncpus, ip_results)) {
                ip_results.push_back(c);
            } else {
                delete rp;
                delete wup;
                if (++infeasible_count > p->max_infeasible_count) {
                    p->min_rpc_time = now + 1;
                    break;
                }
            }
        }

        sent_something = true;
        rp->set_state(RESULT_FILES_DOWNLOADED, "simulate_rpc");
        results.push_back(rp);
        new_results.push_back(rp);
        sprintf(buf, "got job %s: CPU time %.2f, deadline %s<br>",
            rp->name, rp->final_cpu_time, time_to_string(rp->report_deadline)
        );
        html_msg += buf;
        decrement_request(rp);
    }

    msg_printf(0, MSG_INFO, "Got %d tasks", new_results.size());

    SCHEDULER_REPLY sr;
    cpu_work_fetch.req_secs = save_cpu_req_secs;
    work_fetch.handle_reply(p, &sr, new_results);
    p->nrpc_failures = 0;
    if (sent_something) {
        request_schedule_cpus("simulate_rpc");
        request_work_fetch("simulate_rpc");
    }
    return true;
}

SCHEDULER_REPLY::SCHEDULER_REPLY() {
    cpu_backoff = 0;
    cuda_backoff = 0;
    ati_backoff = 0;
}
SCHEDULER_REPLY::~SCHEDULER_REPLY() {}

void PROJECT::backoff() {
    nrpc_failures++;
    double backoff = calculate_exponential_backoff(
        nrpc_failures, SCHED_RETRY_DELAY_MIN, SCHED_RETRY_DELAY_MAX
    );
    min_rpc_time = gstate.now + backoff;
}

bool CLIENT_STATE::scheduler_rpc_poll() {
    PROJECT *p;
    bool action = false;
    static double last_time=0;
    static double last_work_fetch_time = 0;
    double elapsed_time;

    // check only every 5 sec
    //
    if (now - last_time < SCHEDULER_RPC_POLL_PERIOD) {
        msg_printf(NULL, MSG_INFO, "RPC poll: not time %f - %f < %f",
            now, last_time, SCHEDULER_RPC_POLL_PERIOD
        );
        return false;
    }
    last_time = now;

    msg_printf(NULL, MSG_INFO, "RPC poll start");
    while (1) {
        p = next_project_sched_rpc_pending();
        if (p) {
            work_fetch.compute_work_request(p);
            action = simulate_rpc(p);
            break;
        }
    
        p = find_project_with_overdue_results();
        if (p) {
            work_fetch.compute_work_request(p);
            action = simulate_rpc(p);
            break;
        }

        // should we check work fetch?  Do this at most once/minute

        if (must_check_work_fetch) {
            last_work_fetch_time = 0;
        }
        elapsed_time = now - last_work_fetch_time;
        if (elapsed_time < WORK_FETCH_PERIOD) {
            return false;
        }
        must_check_work_fetch = false;
        last_work_fetch_time = now;

        p = work_fetch.choose_project();
        if (p) {
            action = simulate_rpc(p);
            break;
        }
        break;
    }
    if (action) {
        msg_printf(p, MSG_INFO, "RPC poll: did an RPC");
    } else {
        msg_printf(0, MSG_INFO, "RPC poll: didn't do an RPC");
    }
    return action;
}

bool ACTIVE_TASK_SET::poll() {
    unsigned int i;
    char buf[256];
    bool action = false;
    static double last_time = 0;
    double diff = gstate.now - last_time;
    if (diff < 1.0) return false;
    last_time = gstate.now;
    PROJECT* p;

    if (!running) return false;

    for (i=0; i<gstate.projects.size(); i++) {
        p = gstate.projects[i];
        p->idle = true;
        sprintf(buf, "%s STD: %f LTD %f<br>",
            p->project_name, p->cpu_pwf.short_term_debt,
            p->pwf.overall_debt
        );
        gstate.html_msg += buf;
    }

    int n=0;
    for (i=0; i<active_tasks.size(); i++) {
        ACTIVE_TASK* atp = active_tasks[i];
        switch (atp->task_state()) {
        case PROCESS_EXECUTING:
            atp->cpu_time_left -= diff;
            atp->current_cpu_time += diff;
            RESULT* rp = atp->result;

            double cpu_time_used = rp->final_cpu_time - atp->cpu_time_left;
            atp->fraction_done = cpu_time_used/rp->final_cpu_time;
            atp->checkpoint_wall_time = gstate.now;

            if (atp->cpu_time_left <= 0) {
                atp->set_task_state(PROCESS_EXITED, "poll");
                rp->exit_status = 0;
                rp->ready_to_report = true;
                gstate.request_schedule_cpus("ATP poll");
                gstate.request_work_fetch("ATP poll");
                sprintf(buf, "result %s finished<br>", rp->name);
                gstate.html_msg += buf;
                action = true;
            }
            rp->project->idle = false;
            n++;
        }
    }
    if (n < gstate.ncpus) {
        sim_results.cpu_idle += diff*(gstate.ncpus-n);
    }
    if (n > gstate.ncpus) {
        sprintf(buf, "TOO MANY JOBS RUNNING");
        gstate.html_msg += buf;
    }

    for (i=0; i<gstate.projects.size(); i++) {
        p = gstate.projects[i];
        if (p->idle) {
            p->idle_time += diff;
            p->idle_time_sumsq += diff*(p->idle_time*p->idle_time);
        } else {
            p->idle_time = 0;
        }
    }
    running_time += diff;

    return action;
}

// return the fraction of CPU time that was spent in violation of shares
// i.e., if a project got X and it should have got Y,
// add up |X-Y| over all projects, and divide by total CPU
//
double CLIENT_STATE::share_violation() {
    unsigned int i;

    double tot = 0, trs=0;
    for (i=0; i<projects.size(); i++) {
        PROJECT* p = projects[i];
        tot += p->project_results.cpu_used + p->project_results.cpu_wasted;
        trs += p->resource_share;
    }
    double sum = 0;
    for (i=0; i<projects.size(); i++) {
        PROJECT* p = projects[i];
        double t = p->project_results.cpu_used + p->project_results.cpu_wasted;
        double rs = p->resource_share/trs;
        double rt = tot*rs;
        sum += fabs(t - rt);
    }
    return sum/tot;

}

// "monotony" is defined as follows:
// for each project P, maintain R(P), the time since P last ran,
// let S(P) be the RMS of R(P).
// Let X = mean(S(P))/(sched_interval*nprojects)
//  (the *nprojects reflects the fact that in the limit of nprojects,
//   each one waits for a time to run proportional to nprojects)
//  X varies from zero (no monotony) to infinity.
//   X is one in the case of round-robin on 1 CPU.
// Let monotony = 1-1/(x+1)
//
double CLIENT_STATE::monotony() {
    double sum = 0;
    double schedint = global_prefs.cpu_scheduling_period();
    unsigned int i;
    for (i=0; i<projects.size(); i++) {
        PROJECT* p = projects[i];
        double avg_ss = p->idle_time_sumsq/running_time;
        double s = sqrt(avg_ss);
        sum += s;
    }
    int n = (int)projects.size();
    double x = sum/(n*schedint*n);
    double m = 1-(1/(x+1));
    //printf("sum: %f; x: %f m: %f\n", sum, x, m);
    return m;
}

// the CPU totals are there; compute the other fields
//
void SIM_RESULTS::compute() {
    double total = cpu_used + cpu_wasted + cpu_idle;
    cpu_wasted_frac = cpu_wasted/total;
    cpu_idle_frac = cpu_idle/total;
    share_violation = gstate.share_violation();
    monotony = gstate.monotony();
}

// top-level results (for aggregating multiple simulations)
//
void SIM_RESULTS::print(FILE* f, const char* title) {
    if (title) {
        fprintf(f, "%s: ", title);
    }
    fprintf(f, "wasted_frac %f idle_frac %f share_violation %f monotony %f\n",
        cpu_wasted_frac, cpu_idle_frac, share_violation, monotony
    );
}

void SIM_RESULTS::parse(FILE* f) {
    fscanf(f, "wasted_frac %lf idle_frac %lf share_violation %lf monotony %lf",
        &cpu_wasted_frac, &cpu_idle_frac, &share_violation, &monotony
    );
}

void SIM_RESULTS::add(SIM_RESULTS& r) {
    cpu_wasted_frac += r.cpu_wasted_frac;
    cpu_idle_frac += r.cpu_idle_frac;
    share_violation += r.share_violation;
    monotony += r.monotony;
}

void SIM_RESULTS::divide(int n) {
    cpu_wasted_frac /= n;
    cpu_idle_frac /= n;
    share_violation /= n;
    monotony /= n;
}

void SIM_RESULTS::clear() {
    memset(this, 0, sizeof(*this));
}

void PROJECT::print_results(FILE* f, SIM_RESULTS& sr) {
    double t = project_results.cpu_used + project_results.cpu_wasted;
    double gt = sr.cpu_used + sr.cpu_wasted;
    fprintf(f, "%s: share %.2f total CPU %2f (%.2f%%)\n"
        "   used %.2f wasted %.2f\n"
        "   met %d missed %d\n",
        project_name, resource_share,
        t, (t/gt)*100,
        project_results.cpu_used,
        project_results.cpu_wasted,
        project_results.nresults_met_deadline,
        project_results.nresults_missed_deadline
    );
}

const char* colors[] = {
    "#ffffdd",
    "#ffddff",
    "#ddffff",
    "#ddffdd",
    "#ddddff",
    "#ffdddd",
};

static int outfile_num=0;

bool uses_coproc(RESULT*, COPROC*) {
    // TODO
    return false;
}

bool using_instance(RESULT*, int) {
    return false;
}

void gpu_header_aux(COPROC* cp) {
    for (int i=0; i<cp->count; i++) {
        fprintf(gstate.html_out, "<th>%s %d</th>", cp->type, i);
    }
}

void gpu_header() {
    gpu_header_aux(&gstate.host_info.coprocs.cuda);
    gpu_header_aux(&gstate.host_info.coprocs.ati);
}

void gpu_off_aux(COPROC* cp) {
    for (int i=0; i<cp->count; i++) {
        fprintf(gstate.html_out, "<td bgcolor=#aaaaaa>OFF</td>");
    }
}

void gpu_off() {
    gpu_off_aux(&gstate.host_info.coprocs.cuda);
    gpu_off_aux(&gstate.host_info.coprocs.ati);
}

void gpu_on_aux(COPROC* cp) {
    for (int j=0; j<cp->count; j++) {
        for (unsigned int k=0; k<gstate.active_tasks.active_tasks.size(); k++) {
            ACTIVE_TASK* atp = gstate.active_tasks.active_tasks[k];
            RESULT* rp = atp->result;
            if (!uses_coproc(rp, cp)) continue;
            if (atp->task_state() != PROCESS_EXECUTING) continue;
            if (!using_instance(rp, j)) continue;
            PROJECT* p = rp->project;
            fprintf(gstate.html_out, "<td bgcolor=%s>%s%s: %.2f</td>",
                colors[p->index],
                atp->result->rr_sim_misses_deadline?"*":"",
                atp->result->name, atp->cpu_time_left
            );
        }
    }
}
void gpu_on() {
    gpu_on_aux(&gstate.host_info.coprocs.cuda);
    gpu_on_aux(&gstate.host_info.coprocs.ati);
}

void CLIENT_STATE::html_start(bool show_prev) {
    char buf[256];

    sprintf(buf, "sim_out_%d.html", outfile_num++);
    html_out = fopen(buf, "w");
    if (!html_out) {
        fprintf(stderr, "can't open %s for writing\n", buf);
        exit(1);
    }
    setbuf(html_out, 0);
    fprintf(html_out, "<h2>Simulator output</h2>\n");
    if (show_prev) {
        fprintf(html_out,
            "<a href=sim_out_%d.html>Previous file</a><p>\n",
            outfile_num-2
        );
    }
    fprintf(html_out,
        "<a href=sim_log.txt>message log</a><p>"
        "<table border=1><tr><th>Time</th>\n"
    );
    for (int i=0; i<ncpus; i++) {
        fprintf(html_out,
            "<th>CPU %d<br><font size=-2>Job name and estimated time left<br>color denotes project<br>* means EDF mode</font></th>", i
        );
    }
    gpu_header();
    fprintf(html_out, "<th>Notes</th></tr>\n");
}

void CLIENT_STATE::html_rec() {
    static int line_num=0;

    fprintf(html_out, "<tr><td>%s</td>", time_to_string(now));

    if (!running) {
        for (int j=0; j<ncpus; j++) {
            fprintf(html_out, "<td bgcolor=#aaaaaa>OFF</td>");
        }
        gpu_off();
    } else {
        int n=0;
        for (unsigned int i=0; i<active_tasks.active_tasks.size(); i++) {
            ACTIVE_TASK* atp = active_tasks.active_tasks[i];
            int np = atp->result->avp->avg_ncpus;
            if (np < 1) np = 1;
            if (atp->task_state() == PROCESS_EXECUTING) {
                PROJECT* p = atp->result->project;
                fprintf(html_out, "<td colspan=%d bgcolor=%s>%s%s: %.2f</td>",
                    np, colors[p->index],
                    atp->result->rr_sim_misses_deadline?"*":"",
                    atp->result->name, atp->cpu_time_left
                );
                n += np;
            }
        }
        while (n<ncpus) {
            fprintf(html_out, "<td>IDLE</td>");
            n++;
        }
    }
    fprintf(html_out,
        "<td><font size=-2>%s</font></td></tr>\n", html_msg.c_str()
    );
    html_msg = "";

    if (++line_num == line_limit) {
        line_num = 0;
        html_end(true);
        html_start(true);
    }
}

void CLIENT_STATE::html_end(bool show_next) {
    fprintf(html_out, "</table>");
    if (show_next) {
        fprintf(html_out,
            "<p><a href=sim_out_%d.html>Next file</a>\n",
            outfile_num
        );
    } else {
        fprintf(html_out, "<pre>\n");
        sim_results.compute();
        sim_results.print(html_out);
        print_project_results(html_out);
        fprintf(html_out, "</pre>\n");
    }
    if (show_next) {
        fprintf(html_out, "<p><a href=sim_out_last.html>Last file</a>\n");
    } else {
        char buf[256];
        sprintf(buf, "sim_out_%d.html", outfile_num-1);
#ifndef _WIN32
        symlink(buf, "sim_out_last.html");
#endif
    }
    fclose(html_out);
}

void CLIENT_STATE::simulate() {
    bool action;
    double start = START_TIME;
    now = start;
    html_start(false);
    msg_printf(0, MSG_INFO,
        "starting simulation. delta %f duration %f", delta, duration
    );
    while (1) {
        running = available.sample(now);
        while (1) {
            action = active_tasks.poll();
            if (running) {
                action |= handle_finished_apps();
                action |= possibly_schedule_cpus();
                action |= enforce_schedule();
                action |= scheduler_rpc_poll();
            }
            msg_printf(0, MSG_INFO, action?"did action":"did no action");
            if (!action) break;
        }
        now += delta;
        msg_printf(0, MSG_INFO, "took time step");
        for (unsigned int i=0; i<active_tasks.active_tasks.size(); i++) {
            ACTIVE_TASK* atp = active_tasks.active_tasks[i];
            if (atp->task_state() == PROCESS_EXECUTING) {
                atp->elapsed_time += delta;
            }
        }
        html_rec();
        if (now > start + duration) break;
    }
    html_end(false);
}

void parse_error(char* file, int retval) {
    printf("can't parse %s: %d\n", file, retval);
    exit(1);
}

void help(char* prog) {
    fprintf(stderr, "usage: %s\n"
        "[--duration X]\n"
        "[--delta X]\n"
        "[--server_uses_workload]\n"
        "[--dcf_dont_user]\n"
        "[--dcf_stats]\n"
        "[--dual_dcf]\n"
        "[--cpu_sched_rr_only]\n"
        "[--work_fetch_old]\n"
        "[--dirs ...]\n",
        prog
    );
    exit(1);
}

char* next_arg(int argc, char** argv, int& i) {
    if (i >= argc) {
        fprintf(stderr, "Missing command-line argument\n");
        help(argv[0]);
    }
    return argv[i++];
}

// get application FLOPS params.
// These can be specified manually in the <app>.
// If not they are taken from a WU if one exists for the app.
// If not they default to 1 GFLOPS/hour
//
void get_app_params() {
    unsigned int i, j;
    for (i=0; i<gstate.apps.size(); i++) {
        APP* app = gstate.apps[i];
        if (!app->fpops_est) {
            for (j=0; j<gstate.workunits.size(); j++) {
                WORKUNIT* wup = gstate.workunits[j];
                if (wup->app != app) continue;
                app->fpops_est = wup->rsc_fpops_est;
                break;
            }
            if (!app->fpops_est) {
                app->fpops_est = 3600 * 1e9;
            }
        }
        if (!app->fpops.mean) {
            app->fpops.mean = app->fpops_est;
        }
        if (!app->weight) {
            app->weight = 1;
        }
        fprintf(stderr, "app %s: fpops_est %f fpops mean %f\n",
            app->name, app->fpops_est, app->fpops.mean
        );
    }
}

// fill in APP.latency_bound
//
void assign_latency_bounds() {
    unsigned int i;
    for (i=0; i<gstate.results.size(); i++) {
        RESULT* rp = gstate.results[i];
        APP* app = rp->app;
        app->latency_bound = rp->report_deadline - rp->received_time;
    }
}

// zero backoffs and debts.
//
void clear_backoff() {
    unsigned int i;
    for (i=0; i<gstate.projects.size(); i++) {
        PROJECT* p = gstate.projects[i];
        p->cpu_pwf.reset();
        p->cuda_pwf.reset();
        p->ati_pwf.reset();
        p->min_rpc_time = 0;
    }
}

// remove apps with no app versions,
// then projects with no apps
//
void cull_projects() {
    unsigned int i;

    for (i=0; i<gstate.apps.size(); i++) {
        APP* app = gstate.apps[i];
        app->has_version = false;
    }
    for (i=0; i<gstate.app_versions.size(); i++) {
        APP_VERSION* avp = gstate.app_versions[i];
        avp->app->has_version = true;
    }
    for (i=0; i<gstate.projects.size(); i++) {
        PROJECT* p = gstate.projects[i];
        p->dont_request_more_work = true;
    }
    for (i=0; i<gstate.apps.size(); i++) {
        APP* app = gstate.apps[i];
        if (app->has_version) {
            app->project->dont_request_more_work = false;
        }
    }
}

#define SUMMARY_FILE "sim_summary.txt"
#define LOG_FILE "sim_log.txt"

void CLIENT_STATE::do_client_simulation() {
    msg_printf(0, MSG_INFO, "SIMULATION START");
    read_config_file(true);
    config.show();

    add_platform("client simulator");
    parse_state_file();
    read_global_prefs();
    cull_projects();
    int j=0;
    for (unsigned int i=0; i<projects.size(); i++) {
        if (!projects[i]->dont_request_more_work) {
            projects[i]->index = j++;
        }
    }

    get_app_params();
    assign_latency_bounds();
    clear_backoff();

    gstate.workunits.clear();
    gstate.results.clear();

    gstate.set_ncpus();
    work_fetch.init();
    gstate.request_work_fetch("init");
    gstate.simulate();

    sim_results.compute();

    // print machine-readable first
    sim_results.print(stdout);

    // then other
    gstate.print_project_results(stdout);
}

int main(int argc, char** argv) {
    int i, retval;
    vector<std::string> dirs;

    logfile = fopen("sim_log.txt", "w");
    if (!logfile) {
        fprintf(stderr, "Can't open sim_log.txt\n");
        exit(1);
    }
    setbuf(logfile, 0);

    sim_results.clear();
    for (i=1; i<argc;) {
        char* opt = argv[i++];
        if (!strcmp(opt, "--duration")) {
            duration = atof(next_arg(argc, argv, i));
        } else if (!strcmp(opt, "--delta")) {
            delta = atof(next_arg(argc, argv, i));
        } else if (!strcmp(opt, "--dirs")) {
            while (i<argc) {
                dirs.push_back(argv[i++]);
            }
        } else if (!strcmp(opt, "--server_uses_workload")) {
            server_uses_workload = true;
        } else if (!strcmp(opt, "--dcf_dont_use")) {
            dcf_dont_use = true;
        } else if (!strcmp(opt, "--dcf_stats")) {
            dcf_stats = true;
        } else if (!strcmp(opt, "--dual_dcf")) {
            dual_dcf = true;
            dcf_stats = true;
        } else if (!strcmp(opt, "--cpu_sched_rr_only")) {
            cpu_sched_rr_only = true;
        } else if (!strcmp(opt, "--work_fetch_old")) {
            work_fetch_old = true;
        } else if (!strcmp(opt, "--line_limit")) {
            line_limit = atoi(next_arg(argc, argv, i));
        } else {
            help(argv[0]);
        }
    }

    if (duration <= 0) {
        printf("non-pos duration\n");
        exit(1);
    }
    if (delta <= 0) {
        printf("non-pos delta\n");
        exit(1);
    }

    if (dirs.size()) {
        // If we need to do several simulations,
        // use system() to do each one in a separate process,
        // because there are lots of static variables and we need to ensure
        // that they start off with the right initial values
        //
        unsigned int i;
        SIM_RESULTS total_results;
        total_results.clear();
        for (i=0; i<dirs.size(); i++) {
            std::string dir = dirs[i];
            retval = chdir(dir.c_str());
            if (retval) {
                fprintf(stderr, "can't chdir into %s: ", dir.c_str());
                perror("chdir");
                continue;
            }
            char buf[256];
            sprintf(
                buf, "%s --duration %f --delta %f > %s",
                SIM_EXEC, duration, delta, SUMMARY_FILE
            );
            retval = system(buf);
            if (retval) {
                printf("simulation in %s failed\n", dir.c_str());
                exit(1);
            }
            FILE* f = fopen(SUMMARY_FILE, "r");
            sim_results.parse(f);
            fclose(f);
            sim_results.print(stdout, dir.c_str());
            total_results.add(sim_results);
            chdir("..");
        }
        total_results.divide((int)(dirs.size()));
        total_results.print(stdout, "Total");
    } else {
        gstate.do_client_simulation();
    }
}
