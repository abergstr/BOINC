<?php {
    require_once("../inc/cache.inc");
    $sort_by = $_GET["sort_by"];
    if (!$sort_by) $sort_by = "expavg_credit";
    $cache_args = "sort_by=$sort_by";
    start_cache(3600, $cache_args);

    require_once("../inc/db.inc");
    require_once("../inc/util.inc");
    require_once("../inc/user.inc");
    require_once("../inc/team.inc");

    db_init();

    page_head("Top teams");
    if ($sort_by == "total_credit") {
        $sort_clause = "total_credit desc";
    } else {
        $sort_clause = "expavg_credit desc";
    }
    $result = mysql_query("select * from team order by $sort_clause");
    start_table();
    row1("Teams", 6);
    team_table_start($sort_by);
    $i = 1;
    while ($team = mysql_fetch_object($result)) {
        show_team_row($team, $i);
        $i++;
    }
    mysql_free_result($result);
    echo "</table>\n<p>\n";
    page_tail();

    end_cache($cache_args);

} ?>
