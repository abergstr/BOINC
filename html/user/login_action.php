<?php
    require_once("db.inc");
    require_once("util.inc");
    require_once("user.inc");

    init_session();
    db_init();
    $authenticator = trim($_POST["authenticator"]);
    if (strlen($authenticator)) {
        $query = "select * from user where authenticator='$authenticator'";
    }
    $result = mysql_query($query);
    if ($result) {
        $user = mysql_fetch_object($result);
        mysql_free_result($result);
    }
    if (!$user) {
        page_head("Log in");
        echo "
            We have no account with the account key '$authenticator'.
            <br>Click <b>Back</b> to try again.
        ";
        echo "<<".MASTER_URL.">>\n";
        page_tail();
    } else {
        if (split_munged_email_addr($user->email_addr, $authenticator, $email)) {
            mysql_query("update user set email_addr='$email' where id=$user->id");
        }
        $_SESSION["authenticator"] = $authenticator;
        $next_url = $_POST["next_url"];
        if (strlen($next_url) == 0) $next_url = "home.php";
        if ($_GET['send_cookie']) {
            setcookie('auth', $authenticator, time()+3600*24*365);
        }
        Header("Location: $next_url");
    }
?>
