<?php

require_once('../inc/forum.inc');
require_once('../inc/util.inc');

db_init();

if (empty($_GET['id'])) {
    // TODO: Standard error page
    echo "No thread was specified.<br>";
    exit();
}

$_GET['id'] = stripslashes(strip_tags($_GET['id']));

$sort_style = $_GET['sort'];

$filter = $_GET['filter'];
if ($filter != "false"){
    $filter = true;
} else {
    $filter = false;
}

$thread = getThread($_GET['id']);
incThreadViews($thread->id);

$forum = getForum($thread->forum);
$category = getCategory($forum->category);

$logged_in_user = get_logged_in_user(false);
$logged_in_user = getForumPreferences($logged_in_user);

$title = cleanup_title($thread->title);
if ($category->is_helpdesk) {
    if (!$sort_style) {
        $sort_style = getSortStyle($logged_in_user,"answer");
    } else {
        setSortStyle($logged_in_user,"answer", $sort_style);
    }
    page_head(PROJECT.': Questions and problems : '.$title);
} else {
    if (!$sort_style) {
        $sort_style = getSortStyle($logged_in_user,"thread");
    } else {
        setSortStyle($logged_in_user,"thread", $sort_style);
    }
    if ($logged_in_user->jump_to_unread){
        page_head(PROJECT.': Message boards : '.$title, 'jumpToUnread();');
    } else {
        page_head(PROJECT.': Message boards : '.$title);
    }
}

// TODO: Constant for default sort style and filter values.
if ($sort_style == NULL) {
    $sort_style = "timestamp";
}

$is_subscribed = false;

if ($logged_in_user) {
    $result = mysql_query("SELECT * FROM subscriptions WHERE userid = " . $logged_in_user->id . " AND threadid = " . $thread->id);
    if ($result) {
        $is_subscribed = (mysql_num_rows($result) > 0);
    }
}

show_forum_title($forum, $thread, $category->is_helpdesk);

if ($thread->hidden) {

    echo "
        <b>This thread has been hidden for administrative purposes</b>
    ";

} else {
	
	echo "
		<form action=\"forum_thread.php\">
		<input type=\"hidden\" name=\"id\" value=\"", $thread->id, "\">
		<table width=\"100%\" cellspacing=0 cellpadding=0>
		<tr>
		<td align=\"left\">
	";

	$link = "<a href=\"forum_reply.php?thread=" . $thread->id;
	if ($category->is_helpdesk) {
		$link = $link . "&helpdesk=1#input\">Answer this question";
	} else {
		$link = $link . "#input\">Reply to this thread";
	}

	echo $link, "</a><br>";

	if ($is_subscribed) {
		if ($category->is_helpdesk) {
			echo "You are subscribed to this question.  ";
		} else {
			echo "You are subscribed to this thread.  ";
		}
		echo "<a href=\"forum_subscribe.php?action=unsubscribe&amp;thread=$thread->id\">Click here to unsubscribe</a>.";
	} else {
		if ($category->is_helpdesk) {
			echo "<a href=\"forum_subscribe.php?action=subscribe&amp;thread=$thread->id\">Subscribe to this question</a>";
		} else {
			echo "<a href=\"forum_subscribe.php?action=subscribe&amp;thread=$thread->id\">Subscribe to this thread</a>";
		}
	}

	echo "</td>";

	echo "<td align=right style=\"border:0px\">";
	if ($category->is_helpdesk) {
		show_select_from_array("sort", $answer_sort_styles, $sort_style);
	} else {
		echo "Sort ";
		show_select_from_array("sort", $thread_sort_styles, $sort_style);
		//show_select_from_array("filter", $thread_filter_styles, $filter_min);
	}
	echo "<input type=submit value=OK>\n</td>";

	echo "</tr>\n</table>\n</form>\n";

	if ($category->is_helpdesk) {
		$headings = array("Author", "Question");
	} else {
		$headings = array("Author", "Message");
	}

	start_forum_table($headings);
	show_posts($thread, $sort_style, $filter, true, true, $category->is_helpdesk);
	end_forum_table();

	echo "<p>";

	$link = "<a href=\"forum_reply.php?thread=" . $thread->id;
	if ($category->is_helpdesk) {
		$link = $link . "&helpdesk=1#input\">Answer this question";
	} else {
		$link = $link . "#input\">Reply to this thread";
	}

	echo $link, "</a><br>\n</p>";
	show_forum_title($forum, $thread, $category->is_helpdesk);

}

page_tail();
?>
