<?php

/**
 * @file
 * This file contains all theme and preprocess functions
 */

/**
 * Implements template_preprocess_HOOK().
 *
 */
function template_preprocess_opensky_scratch(&$variables) {
    $pid = $variables['pid'];
    $dc_array = opensky_get_dc_array($pid);
    $variables['title'] = $dc_array['dc:title']['value'];
    $variables['children'] = opensky_get_pid_subcollections_pids($pid);
    $variables['descendants'] = opensky_get_subcollections_recursive($pid);
    $variables['searchables'] = opensky_get_searchable_subcollections($pid);
    $obj = opensky_get_fedora_object($pid);
    $variables['tree'] = opensky_get_collection_tree();
}

/**
 * Implements template_preprocess_HOOK().
 *
 */
function template_preprocess_opensky_in_collections(&$variables) {

}

/**
 * Implements template_preprocess_HOOK().
 *
 */
function template_preprocess_opensky_collection_picker(&$variables) {
//    dsm('template_preprocess_opensky_collection_picker');
//    dsm($variables);
//    dsm (request_uri());
//    dsm($_GET);
}