#! /usr/local/bin/php
<?php
    // test the concat application
    //
    // You must have done "make" in all source directories

    include_once("init.inc");

    check_env_vars();
    clear_db();
    clear_data_dirs();
    create_keys();
    init_client_dirs("prefs1.xml");
    copy_to_download_dir("input");
    add_platform(null);
    add_core_client(null);
    add_user(null);
    add_app("concat",null,null);
    create_work("-appname concat -wu_name concat_wu -wu_template concat_wu -result_template concat_result -nresults 2 input input");
    start_feeder();
    run_client("-exit_when_idle");
    stop_feeder();
    check_results_done();
    compare_file("concat_wu_0_0", "concat_correct_output");
    compare_file("concat_wu_1_0", "concat_correct_output");
?>
