<?php

require_once('../inc/forum.inc');
require_once('../inc/util.inc');
require_once('../inc/subscribe.inc');

$logged_in_user = get_logged_in_user(true);
$logged_in_user = getForumPreferences($logged_in_user);


if (!empty($_GET['id']) && !empty($_POST['title']) && !empty($_POST['content'])) {
    $_GET['id'] = stripslashes(strip_tags($_GET['id']));


    if ($_POST['add_signature']=="add_it") {
        //$forum_signature = "\n".$logged_in_user->signature;
	$add_signature=true;
    } else {
	$add_signature=false;
    }
    $threadID = createThread($_GET['id'], $logged_in_user->id, $_POST['title'], $_POST['content'],$add_signature);
    if (!$threadID) {
        page_head("Can't create thread");
        echo "Title is possibly missing";
        page_tail();
        exit();
    }

    $thread->id=$threadID;
    setThreadLastVisited($logged_in_user,$thread);
	header('Location: forum_thread.php?id=' . $threadID);
}

if (!empty($_GET['id'])) {
	$forum = getForum($_GET['id']);
	$category = getCategory($forum->category);
} else {
	echo "Forum ID missing.<br>";
	exit();
}


// TODO: Write a function to do this.

if ($category->is_helpdesk) {
	page_head('Help Desk');
} else {
	page_head('Forum');
}

show_forum_title($forum, NULL, $category->is_helpdesk);

if ($category->is_helpdesk) {
	echo "<p>The <b>Questions and problems</b> area is designed to help you
        get questions answered and problems solved by other users.
        If you have a question or problem:
        <ul>
        <li>
        Read the existing list of questions.
        If your question is already there,
        click on the <b>I also have this question or problem</b> button.
        If answers to the question have been submitted, read them.
        If one of them answers your question, click the
        <b>This answered my question</b> button.
        <li>
        If your question has not already been asked,
        fill out and submit this form.
        </ul>
        The goal is to prevent questions from being asked repeatedly.
    ";
}

echo "<form action=\"forum_post.php?id=", $_GET['id'], "\" method=POST>";

start_table();
if ($category->is_helpdesk) {
	row1("Submit a new question/problem");
} else {
	row1("Create a new thread");
}

$x = "Title".
    "<br><a href=html.php><font size=-2>May contain HTML tags</font></a>";

if ($category->is_helpdesk) {
	$x .="<br>
        Describe your question in a few words.
        A brief, clear summary will help others with the same
        question (or an answer) find it.
    ";
}

$y = "<input type=text name=title size=62>";
row2($x, $y);
$x = "Message".
    "<br><a href=html.php><font size=-2>May contain HTML tags</font></a>";

if ($category->is_helpdesk) {
	$x .= " If you are having software problems,
        mention the version number of the software,
        your computer type and operating system.
    ";
}
    

$y = "<textarea name=content rows=12 cols=54></textarea>";
if ($logged_in_user->no_signature_by_default==0){$enable_signature="checked=\"true\"";} else {$enable_signature="";}
row2($x, $y);
row2("", "<input name=add_signature value=add_it ".$enable_signature." type=checkbox>Add my signature to this post");
row2("", "<input type=submit value=\"OK\">");

end_forum_table();

echo "</form>\n";

page_tail();
?>
