<?php {
    require_once("../inc/cache.inc");
    require_once("../inc/util.inc");

    $n = 20;

    $sort_by = $_GET["sort_by"];
    if (!$sort_by) $sort_by = "expavg_credit";
    $offset = $_GET["offset"];
    if (!$offset) $offset=0;

    if ($offset % $n) $offset = 0;

    if ($offset < 1000) {
        $cache_args = "sort_by=$sort_by&offset=$offset";
        start_cache(TOP_PAGES_TTL, $cache_args);
    } else {
        page_head("Limit exceeded");
        echo "Sorry - first 1000 only.";
        page_tail();
        exit();
    }

    require_once("../inc/db.inc");
    require_once("../inc/user.inc");

    db_init();
    page_head("Top participants");
    if ($sort_by == "total_credit") {
        $sort_order = "total_credit desc";
    } else {
        $sort_order = "expavg_credit desc";
    }

    $result = mysql_query("select * from user order by $sort_order limit $offset,$n");
    user_table_start($sort_by);
    $i = 1 + $offset;
    while ($user = mysql_fetch_object($result)) {
        show_user_row($user, $i);
        $i++;
    }
    echo "</table>\n";

    if ($offset > 0) {
        $new_offset = $offset - $n;
        echo "<a href=top_users.php?sort_by=$sort_by&offset=$new_offset>Previous $n</a> | ";

    }
    $new_offset = $offset + $n;
    echo "<a href=top_users.php?sort_by=$sort_by&offset=$new_offset>Next $n</a>";
    page_tail();

    if ($offset < 1000) {
        end_cache(TOP_PAGES_TTL,$cache_args);
    }
} ?>
