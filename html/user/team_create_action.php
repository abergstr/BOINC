<?php

    require_once("db.inc");
    require_once("util.inc");
    require_once("team.inc");

    db_init();

    $user = get_logged_in_user();
    if (!strlen($_POST["name"])) {
        page_head("Error");
        echo "You must specify a name for your team.";
        exit();
    }
    $query = sprintf(
        "insert into team (userid, create_time, name, name_lc, url, type, name_html, description, country, nusers) values(%d, %d, '%s', '%s', '%s', %d, '%s', '%s', '%s', %d)",
        $user->id,
        time(),
        $_POST["name"],
        strtolower($_POST["name"]),
        $_POST["url"],
        $_POST["type"],
        $_POST["name_html"],
        $_POST["description"],
        $_POST["country"],
        0
    );

    $result = mysql_query($query);
    if ($result) {
        $teamid = mysql_insert_id();
        $team_result = mysql_query("select * from team where id = $teamid");
        $new_team = mysql_fetch_object($team_result);
        mysql_free_result($team_result);
        user_join_team($new_team,$user);
        Header("Location: team_display.php?teamid=$teamid");
    } else {
        page_head("Error");
        echo "Couldn't create team - please try later.<br>\n";
        echo "You may need to try a different team name.\n";
        page_tail();
    }
?>
