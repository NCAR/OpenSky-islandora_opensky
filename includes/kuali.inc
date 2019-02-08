<?php
/**
 * Created by IntelliJ IDEA.
 * User: ostwald
 * Date: 2/6/19
 * Time: 9:46 AM
 */


/*
 * Returns object associated with provided award_id if
 * there is a single hit returned by kuali API.
 * othewise, returns null.
 */
function get_kuail_award_info ($award_id) {
    $kuali_resp = get_kuali_response($award_id);
    if (!is_array($kuali_resp)) {
        return null;
    }
    if (count($kuali_resp) != 1) {
        return null;
    }
    return $kuali_resp[0];
}

/*
 * returns kauli response for proviced $award_id as object or array
 */
function get_kuali_response($award_id) {

    $base_url = 'http://fatomcat-test.fin.ucar.edu:8081/kualiapi/awardsbysponsorawardid';
    $username = 'openskykcapi';
    $password = 'W!n+er5now#';

//    $auth_string = 'Basic ' . base64_encode("$username:$password");

    $ch = curl_init();
    $options = array(
        CURLOPT_URL => $base_url.'?sponsorAwardId='.$award_id,
        CURLOPT_USERPWD => $username . ":" . $password,
        CURLOPT_RETURNTRANSFER => 1
    );

    curl_setopt_array($ch, $options);
    $resp = curl_exec($ch);
    curl_close($ch);

    $response = json_decode($resp);

//    print json_encode($response, JSON_PRETTY_PRINT);

    return $response;

}



