<?php

/**
 * @file
 * Contains all blocks related functionality
 */

// see https://api.drupal.org/api/examples/block_example%21block_example.module/7.x-1.x

/**
 * these "info" definitions are necessary to get the blocks on the blocks UI page
 */
function opensky_block_info() {

    $blocks['opensky_browse_collection_search'] = array (
        'info' => t('Collection PID Search'),
    );
    $blocks ['opensky_search_collection_info'] = array (
        'info' => t('Opensky search collection information'),
    );
//
//    $blocks ['opensky_in_collections'] = array (
//        'info' => t('OpenSky in Collections'),
//    );
    $blocks ['opensky_simple_search'] = array (
        'info' => t('OpenSky simple search'),
    );
    $blocks ['opensky_advanced_search'] = array (
        'info' => t('OpenSky advanced search'),
    );
    $blocks ['opensky_collection_picker_data'] = array (
        'info' => t('Opensky collection select data'),
    );
    return $blocks;
}



/**
 * implements hook_block_view
 *
 * OpenSky Blocks
 */
function opensky_block_view($delta = '') {
    $block = array();
    $pid = opensky_get_pid_from_request();
    switch($delta) {
        case 'opensky_browse_collection_search':
            $block['subject'] = t ('Collection Search for Browse view');
            // $block['content'] = drupal_render(drupal_get_form('opensky_browse_collection_search_form'));
            $d_form = drupal_get_form('opensky_browse_collection_search_form');
            $block['content'] = drupal_render($d_form);
            break;
        case 'opensky_search_collection_info':
            $block['subject'] = '';
            $block['content'] = opensky_search_collection_info();
            break;
        case 'opensky_search_collection_search':
            $block['subject'] = t ('Opensky Collection Search title');
            //            $block['content'] = drupal_render(drupal_get_form('opensky_search_collection_search_form'));
            $dform = drupal_get_form('opensky_search_collection_search_form');
            $block['content'] = drupal_render($dform);
            break;
        case 'opensky_advanced_search':
            $block['subject'] = t('OpenSky advanced search');
            $d_form = drupal_get_form('opensky_advanced_search_form');
            $block['content'] = drupal_render($d_form);
            //             $block['content'] = drupal_render(drupal_get_form('opensky_advanced_search_form'));
            break;
        case 'opensky_simple_search':
            $block['subject'] = t('OpenSky simple search');
            //            $block['content'] = drupal_render(drupal_get_form('opensky_simple_search_form'));
            $form = drupal_get_form('opensky_simple_search_form');
            $block['content'] = drupal_render($form);
            break;
        case 'opensky_collection_picker_data':
            $block['subject'] = t('Opensky collection picker data');
            $block['content'] = get_collection_select_options_js();
    }
    return $block;
}

/**
 * Generate javascript data for the collections select in simple and advanced search.
 * Show the current collection (if any), then 'All Collections' and then
 * all children (if any) of current collection (if any).
 *
 * see opensky_collection_picker_data theme
 *
 * @return string
 *   NOTE: this block must be rendered as a region block for the request_uri
 *   be current. If we try to just display the block directly from template
 *   request_uri will be for PREVIOUS page and we won't get current collection
 *
 */
function get_collection_select_options_js () {

    $parent_pid = 'opensky:root';
    if (isset ($_GET['collection'])) {
        $parent_pid =  $_GET['collection'];
    } else {
        $pid = opensky_get_pid_from_request();
        if ($pid && opensky_is_collection_pid($pid)) {
            $parent_pid = $pid;
        }
    }

    $option_data = array ();
     if ($parent_pid != 'opensky:root') {
         $dc_array = opensky_get_dc_array($parent_pid);
         if ($dc_array != null) {
             $title = html_entity_decode($dc_array['dc:title']['value']);
             $option_data[$parent_pid] = opensky_truncate($title);
         }
     }

    $option_data[''] = 'All Collections';

    if (isset ($parent_pid)) {
        $children_info = opensky_get_pid_subcollection_infos($parent_pid);
        foreach ($children_info as $child) {
            $option_data[$child['pid']] = opensky_truncate($child['title']);
        }
    }

    return theme('opensky_collection_picker_data', array(
        'option_data' => $option_data,
    ));
}

/* ---------------
   Populate a block that shows collection info and options for
   search
*/
function opensky_search_collection_info() {
    $output = '';
    $collection = 'unknown';
    $params = $_GET;
//    dsm('opensky_search_collection_info');
//    dsm($params);

    // is there a search term??
    $query = opensky_get_search_term_from_request();
//    dsm('query: '.$query .' ('. strlen($query) .')');

    if (isset($params['collection'])) {
        // Decided to always show collection info - opensky-dev 12/13/17 
        if (true || !$query) {
            $collection = $params['collection'];

            $title = $description = '';
            $dc_array = opensky_get_dc_array($collection);
            if ($dc_array != null) {
                $title = $dc_array['dc:title']['value'];
                $description = $dc_array['dc:description']['value'];
            }
            $output = theme('opensky_search_collection_info', array(
                'collection_pid' => $collection,
                'collection_title' => $title,
                'collection_description' => $description
            ));
        } else {
            $output = '<div id="opensky-search-collection-info-OFF" style="text-align:right;background:white;">';
            $output .= l('Collection home', '/islandora/object/'.$params['collection']);
            $output .= '</div>';
        }
    }
    return $output;
}

/**
 * Opensky item collection search form.
 *
 * @param array $form
 *   An associative array containing form structure.
 * @param array $form_state
 *   An associative array containing form state.
 *
 * @return array
 *   An associative array containing the fully built form structure.
 */
// function opensky_search_collection_search_form($form, &$form_state) {

function opensky_search_collection_search_form() {
    $collection_pid = '';
    if (isset ($_GET['collection'])) {
        $collection_pid = $_GET['collection'];
    }
    return get_opensky_search_collection_form($collection_pid);
}


/**
 * Islandora Solr collection search form submit callback.
 *
 * @param array $form
 *   An associative array containing form structure.
 * @param array $form_state
 *   An associative array containing form state.
 */

function opensky_search_collection_search_form_submit($form, &$form_state) {
  module_load_include('inc', 'islandora_solr', 'includes/utilities');
  $search_string =
      islandora_solr_replace_slashes($form_state['values']['opensky_collection_search_query']);

  $query = array(
      'type'=>'dismax',
      'collection'=>$form_state['values']['collection']
  );

  $form_state['redirect'] = array(
    ISLANDORA_SOLR_SEARCH_PATH . "/$search_string",
    array(
      'query' => $query,
    ),
  );
}

/**
 * Provides search form for collection BROWSE pages
 */
function opensky_browse_collection_search_form() {
    return get_opensky_search_collection_form(opensky_get_pid_from_request());
}

/**
 * Create form for "search this collection"
 */
function get_opensky_search_collection_form($collection_pid) {
    //    dsm ('get_opensky_search_collection_form');
    // dsm (request_uri());
    $form = array(
        '#type' => 'container',
        '#attributes' => array(
            'class' => array(
                'container-inline',
            ),
        ),
    );

    $form['opensky_collection_search_query'] = array(
        '#size' => '30',
        '#type' => 'textfield',
        '#title' => '<none>',
        // @todo Should this be the searched value?
        '#default_value' => '',
    );

    $form['collection'] = array (
        '#type' => 'hidden',
        '#default_value' => $collection_pid,
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('search'),
    );
    return $form;
}

/**
 * Islandora Solr collection search form submit callback.
 *
 * @param array $form
 *   An associative array containing form structure.
 * @param array $form_state
 *   An associative array containing form state.
 */
function opensky_browse_collection_search_form_submit($form, &$form_state) {
    module_load_include('inc', 'islandora_solr', 'includes/utilities');
    $search_string = islandora_solr_replace_slashes($form_state['values']['opensky_collection_search_query']);

    $query = array(
        'type'=>'dismax',
      'collection'=>$form_state['values']['collection']
    );

    $form_state['redirect'] = array(
        ISLANDORA_SOLR_SEARCH_PATH . "/$search_string",
        array(
            'query' => $query,
        ),
    );
}

//--------------------------
/**
 * Islandora Solr simple search form.
 *
 * @param array $form
 *   An associative array containing form structure.
 * @param array $form_state
 *   An associative array containing form state.
 *
 * @return array
 *   An associative array containing the fully built form structure.
 */

function opensky_simple_search_form($form, &$form_state) {

    $form['simple'] = array(
        '#type' => 'container',
        '#attributes' => array(
            'class' => array(
                'container-inline',
            ),
        ),
    );

    //---------------
    $form['simple']['collection'] = array(
        '#title' => t('Search in'),
        '#type' => 'select',
        '#default_value' => '',
        '#options' => array (),
        '#validated' => 'TRUE',
    );
    // ------------------------


    $form['simple']["opensky_simple_search_query"] = array(
        '#size' => '46',
        '#type' => 'textfield',
        '#title' => 'for',
        // @todo Should this be the searched value?
        '#default_value' => '',
    );
    $form['simple']['submit'] = array(
        '#type' => 'submit',
        '#value' => t('  Go  '),
    );
    return $form;
}

/**
 * Islandora Solr simple search form submit callback.
 *
 * @param array $form
 *   An associative array containing form structure.
 * @param array $form_state
 *   An associative array containing form state.
 */
function opensky_simple_search_form_submit($form, &$form_state) {
    module_load_include('inc', 'islandora_solr', 'includes/utilities');
    $search_string = islandora_solr_replace_slashes($form_state['values']['opensky_simple_search_query']);

    $query = array('type' => 'dismax');


    if (isset ($form_state['values']['collection'])) {
        $col = $form_state['values']['collection'];
        if ($col) {
            $query['collection'] = $col;
        }
    }

    $form_state['redirect'] = array(
        ISLANDORA_SOLR_SEARCH_PATH . "/$search_string",
        array(
            'query' => $query,
        ),
    );
}

/**
 * Islandora Solr advanced search block form.
 *
 * Check form states:
 * 1: Form update using AJAX.
 * 2: Populate with current query on search results page.
 * 3: Anywhere else: empty form.
 *
 * @link
 * http://drupal.stackexchange.com/questions/14855/how-do-i-dynamically-fill-a-textfield-with-ajax/16576#16576
 * Some exam\
ple AJAX. @endlink
*
 * @global IslandoraSolrQueryProcessor $_islandora_solr_queryclass
 *   The IslandoraSolrQueryProcessor object which includes the current query
 *   settings and the raw Solr results.
 *
 * @param array $form
 *   An associative array containing form structure.
 * @param array $form_state
 *   An associative array containing form state.
 *
 * @return array
 *   An associative array containing the fully built form structure.
 */
function opensky_advanced_search_form($form, &$form_state) {
    module_load_include('inc', 'islandora_solr', 'includes/utilities');
    global $_islandora_solr_queryclass;

    // 1: Form update using AJAX.
    if (isset($form_state['triggering_element'])) {

        unset($form_state['input']['terms']);
        // Check for add.
        if ($form_state['triggering_element']['#value'] == '+') {
            $form_state['values']['terms'][] = array();
        }
        // Check for remove.
        elseif ($form_state['triggering_element']['#value'] == '-') {
            $field = $form_state['triggering_element']['#field'];
            array_splice($form_state['values']['terms'], $field, 1);
        }

        $values = $form_state['values'];
    }
    // 2: Populate with current query on search results page.
    elseif (islandora_solr_results_page($_islandora_solr_queryclass) == TRUE && 
            !isset($_islandora_solr_queryclass->internalSolrParams['type'])) {

        // Get current query.
        $query = $_islandora_solr_queryclass->solrQuery;

        $values['terms'] = array();

        $query_explode = preg_split(ISLANDORA_SOLR_QUERY_SPLIT_REGEX, $query);

        // Break up the solr query to populate the advanced search form.
        $i = 0;
        foreach ($query_explode as $key => $value) {
            $term = array();
            // Check for first colon to split the string.
            if (strpos($value, ':') != FALSE) {
                // Split the filter into field and value.
                $value_split = preg_split(ISLANDORA_SOLR_QUERY_FIELD_VALUE_SPLIT_REGEX, $value, 2);

                $values['terms'][$i]['field'] = stripslashes($value_split[0]);

                // Second part of the split is the query value (or first part of
                // it).
                $value_split[1] = str_replace(array('(', ')'), '', $value_split[1]);

                $values['terms'][$i]['search'] = $value_split[1];

            }
            // If the string does not include a colon or AND/OR/NOT, then it is
            // a
            // part of the query value.
            elseif (!preg_match('/(AND|OR|NOT)/', $value, $matches)) {
                // Trim brackets.
                $value = str_replace(array('(', ')'), '', $value);

                if (isset($values['terms'][$i]['search'])) {
                    // Append to search string.
                    $values['terms'][$i]['search'] .= ' ' . $value;
                }
                else {
                    // Search field is not set, so create new search value.
                    $values['terms'][$i]['search'] = $value;
                }
            }
            // If it matches AND/OR/NOT, then we have the boolean operator.
            else {
                $values['terms'][$i]['boolean'] = $value;

                // XXX: Something about only incrementing here seems... Wrong?
                $i++;
            }
        }
    }
    // 3: Anywhere else: empty form.
    else {
        // Need at least one term to draw the search box.
        $values = array(
            'terms' => array(''),
        );
    }

    $terms = array(
        '#type' => 'markup',
        '#prefix' => '<div id="opensky-advanced-terms">',
        '#suffix' => '</div>',
        '#tree' => TRUE,
    );
    foreach ($values['terms'] as $i => $value) {
        $term = array(
            '#tree' => TRUE,
            '#prefix' => '<div>',
            '#suffix' => '</div>',
        );
        $term['field'] = array(
            '#title' => t('Field'),
            '#type' => 'select',
            '#default_value' => isset($value['field']) ? $value['field'] : 'dc.title',
            '#options' => islandora_solr_get_fields('search_fields'),
        );
        $term['search'] = array(
            '#title' => t('Search terms'),
            '#type' => 'textfield',
            '#size' => 20,
            '#default_value' => isset($value['search']) ? $value['search'] : '',
        );

        // Used for when the user presses enter on the search field.
        $term['hidden_submit'] = array(
            '#type' => 'submit',
            '#value' => t('Search'),
            '#attributes' => array('style' => 'visibility:hidden;position:fixed;top:-1000px;right:-1000px;'),
        );
        $term['add'] = array(
            '#type' => 'button',
            '#value' => '+',
            '#attributes' => array('title' => t('Add field')),
            '#name' => 'add-field-' . $i,
            '#ajax' => array(
                'callback' => '_opensky_advanced_search_terms',
                'wrapper' => 'opensky-advanced-terms',
                'method' => 'replace',
                'effect' => 'fade',
                'progress' => array('type' => 'none'),
            ),
        );
        if (count($values['terms']) > 1) {
            $term['remove'] = array(
                '#type' => 'button',
                '#field' => $i,
                '#value' => '-',
                '#attributes' => array('title' => t('Remove field')),
                '#name' => 'remove-field-' . $i,
                '#ajax' => array(
                    'callback' => '_opensky_advanced_search_terms',
                    'wrapper' => 'opensky-advanced-terms',
                    'method' => 'replace',
                    'effect' => 'fade',
                    'progress' => array('type' => 'none'),
                ),
            );
            if ((variable_get('opensky_search_boolean', 'user') == 'user') && ((count($values['terms']) - 1) != $i)) {
                $term['boolean'] = array(
                    '#type' => 'select',
                    '#prefix' => '<div>',
                    '#suffix' => '</div>',
                    '#default_value' => isset($value['boolean']) ? $value['boolean'] : 'AND',
                    '#options' => array(
                        'AND' => 'AND',
                        'OR' => 'OR',
                        'NOT' => 'NOT',
                    ),
                );
            }
        }

        $terms[] = $term;
    }

    // Add terms.
    $form['terms'] = $terms;

    // ------------------------
    // collections options supplied by opensky_collection_select_options block,
    // and inserted into collection select via javascript (see custom.js in opensky theme)
    // All because we can't get current collection from this function: it is evaluated apparently
    // too early -> request_uri is the PREVIOUS page, not this one!?

    $form['collection'] = array(
        '#title' => t('Collection'),
        '#type' => 'select',
        '#default_value' => '',
        '#options' => array (),
        '#validated' => 'TRUE',
    );
    // ------------------------

    // Add controls.
    $form['controls'] = array(
        '#type' => 'markup',
        '#prefix' => '<div class="islandora-solr-advanced-controls">',
        '#suffix' => '</div>',
    );
    $form['controls']['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Search'),
    );
    return $form;
}

function opensky_form_opensky_advanced_search_form_alter(&$form, &$form_state, $form_id) {
    /*
    // No help for getting current request
    dsm ('opensky_form_advanced_search_form_alter!! - ' . $form_id);
    dsm('request_uri: '.request_uri());
    dsm($_GET);
    */
}

/**
 * AJAX callback for advanced search form.
 *
 * @param array $form
 *   An associative array containing form structure.
 * @param array $form_state
 *   An associative array containing form state.
 *
 * @return array
 *   An associative array containing the fully built form element.
 *
     * @see opensky_advanced_search_form()
 */
function _opensky_advanced_search_terms($form, &$form_state) {
    // dsm('_opensky_advanced_search_terms');
    // dsm($form_state);
    return $form['terms'];
}


/**
 * Islandora Solr advanced search form submit callback.
 *
 * @param array $form
 *   An associative array containing form structure.
 * @param array $form_state
 *   An associative array containing form state.
 *
 * @see opensky_advanced_search_form()
 */
function opensky_advanced_search_form_submit($form, &$form_state) {
    module_load_include('inc', 'islandora_solr', 'includes/utilities');
    // Collect query values.
    $query_array = array();
    foreach ($form_state['values']['terms'] as $term) {
        $field = $term['field'];

        $search = trim($term['search']);
        $search = islandora_solr_replace_slashes($search);


        $boolean = (isset($term['boolean'])) ? $term['boolean'] : variable_get('opensky_search_boolean', 'user');
        $field = islandora_solr_lesser_escape($field);
        // Add query.
        if (!empty($search)) {
            $query_array[] = array(
                'search' => "$field:($search)",
                'boolean' => $boolean,
            );
        }
    }

    // Create query.
    $query = '';
    $i = 0;
    foreach ($query_array as $term) {
        $query .= $term['search'];
        if (count($query_array) - 1 != $i) {
            $query .= ' ' . $term['boolean'] . ' ';
        }
        $i++;
    }

    // --------------------
    $params = array();

    if (isset ($form_state['values']['collection'])) {
        $col = $form_state['values']['collection'];
        if ($col) {
            $params['collection'] = $col;
//            $params['type'] = 'dismax';
        }
    }

    // -----------------------

    // Check if query is empty.
    if (empty($query) && empty($params)) {
        $query = '*:*';
    }

    // Navigate to results page.
    $form_state['redirect'] = array(
        ISLANDORA_SOLR_SEARCH_PATH . "/$query",
        array (
            'query' => $params,
        ),
    );
}
