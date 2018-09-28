<?php
/**
 * Assumes PID is last segment of request path
 * e.g., https://osstage2cl.dls.ucar.edu/islandora/object/technotes:546
 */
function opensky_get_pid_from_request () {
    $pat = 'islandora/object';

    if (substr (request_path(), 0, strlen($pat)) != $pat) {
        return null;
    }
    $tail = array_pop(explode('/',request_path()));
    $parts = explode (':', $tail);
    if (count($parts) == 2) {
        return $tail;
    }
    return null;
}

function opensky_get_search_term_from_request() {
    $pat = ISLANDORA_SOLR_SEARCH_PATH;

    $path = request_path();

    if (strlen($path) <= strlen($pat) || substr (request_path(), 0, strlen($pat)) != $pat) {
        return null;
    }

    $term = array_pop(explode('/',request_path()));
    return trim($term);
}

function opensky_truncate($str) {
    $truncated = truncate_utf8($str, 50, $wordsafe=TRUE);
    if (strlen ($str) != strlen($truncated)) {
        $truncated = "$truncated ...";
    }
    return $truncated;
}

/** 
 * Get the fedora object for provided pid.
 */
function opensky_get_fedora_object($pid) {
    $my_islandora_tuque = new IslandoraTuque();
    $repository = $my_islandora_tuque->repository;
    try {
        return $repository->getObject($pid);
    } catch (Exception $e) {
        // dsm ('object ('.$pid.') not in repository');
        return null;
    }
}

/**
 * return array representing DC metadata for provided $pid, or null if not found 
 */
function opensky_get_dc_array ($pid) {
    $fedora_object = opensky_get_fedora_object($pid);
    if ($fedora_object == null) {
        // dsm ('object ('.$pid.') not in repository');
        return null;
    }

    $dc = $fedora_object['DC'];
    $dc_object = DublinCore::importFromXMLString($dc->content);
    $dc_array = $dc_object->asArray();
    // $title = $dc_array['dc:title']['value'];
    return $dc_array;
}


/**
 *
 */
function opensky_get_pid_subcollections_pids ($parent_pid) {

    $key = 'subcollections_pids_'.$parent_pid;
    if ($cache = cache_get($key)) {
        $pids = $cache->data;
    }
    else {
        $obj = opensky_get_fedora_object($parent_pid);
        if ($obj != null) {
            $pids =  opensky_get_obj_subcollection_pids($obj);
        } else {
            $pids = array();
        }

        cache_set ($key, $pids);
    }
    return $pids;
}

/**
 * Gets a list of ALL subcollections under parent collection (by recursively calling 
 * $opensky_get_subcollections.
 *
 * @param string $parent_pid
 *   The parent collection object.
 *
 * @return array
 *   The list of child PIDs.
 */

function opensky_get_subcollections_recursive($parent_pid){

    $key = 'subcollections_recursive_'.$parent_pid;

    if ($cache = cache_get($key)) {
        $subs = $cache->data;
    } else {
        $subs = array($parent_pid);
        $children = opensky_get_pid_subcollections_pids($parent_pid);
        if (count($children)) {
            foreach ($children as $child) {
                $subs = array_merge($subs, opensky_get_subcollections_recursive($child));
            }
        }
        cache_set($key, $subs);
    }

    return $subs;
    
}

/**
 * Gets a list of all immediate child PIDs.
 *
 * @param AbstractObject $object
 *   The parent collection object.
 *
 * @return array
 *   The list of child PIDs.
 */
function opensky_get_obj_subcollection_pids(AbstractObject $object) {
    $get_pid = function($o) {
        return $o['pid'];
    };

    return array_map($get_pid, opensky_get_obj_subcollection_infos ($object));
}

function opensky_is_item_pid($pid) {
    $suffix = explode(':', $pid)[1];
    return $suffix != '*' && is_numeric($suffix);
}

function opensky_is_collection_pid ($pid) {
    $suffix = explode(':', $pid)[1];
    return $suffix != '*' && !is_numeric($suffix);
}


function opensky_has_subcollections ($pid) {
    return (count(opensky_get_pid_subcollections_pids($pid)) > 0);
}

/**
 * Returns list of opensky_collection_info objects:
 * - pid
 * - title
 */
/**
 * Gets a list of all immediate child PIDs.
 *
 * @param AbstractObject $object
 *   The parent collection object.
 *
 * @return array
 *   The list of opensky_collection_info objects:
 *    - pid
 *    - title
 */
function opensky_get_obj_subcollection_infos(AbstractObject $object) {
    module_load_include('inc', 'islandora_basic_collection', 'includes/utilities');

    $params = array(
        'object' => $object,
        'page_size' => -1,
        'model' => "<info:fedora/islandora:collectionCModel>",
    );

    $map_results = function($o) {
        return array (
            'pid' => $o['object']['value'],
            'uri' => $o['object']['uri'],
            'title' => $o['title']['value'],
        );
    };
    $query_info = islandora_basic_collection_get_query_info($params);
    $results = $object->repository->ri->query($query_info['query'], $query_info['type']);
    return array_map($map_results, $results);
}

function opensky_get_pid_subcollection_infos ($parent_pid) {

    $key = 'subollection_infos_'.$parent_pid;

    if ($cached = cache_get($key)) {
        $infos = $cached->data;
    } else {
        $obj = opensky_get_fedora_object($parent_pid);
        if ($obj != null) {
            $infos = opensky_get_obj_subcollection_infos($obj);
        }
        else {
            $infos = array();
        }
        cache_set ($key, $infos);
    }

    return $infos;
}


/**
 * Create a mapping from each collection to the (non-recursive) subcollections under it.
 * Currently only used on scratch (demo) page but could be used to cache the tree
 * structure for faster lookup.
 * @return array
 */
function opensky_get_collection_tree () {
    $pid = 'opensky:root';
    $tree = array();
    foreach (opensky_get_subcollections_recursive('opensky:root') as $pid) {
        $tree[$pid] = opensky_get_pid_subcollections_pids($pid);
    }
    return $tree;
}

/**
 * GEt list of searchable subcollections, which are those subcollections (recursively)
 * that contain items (and not subcollections)
 *
 * @param $pid
 * @return array
 */
function opensky_get_searchable_subcollections ($pid) {
    $descendants = opensky_get_subcollections_recursive($pid);
    $tree = array();
    foreach($descendants as $child) {
        if (!opensky_has_subcollections($child)) {
            $tree[] = $child;
        }
    }
    return $tree;
}

/*
View this collection (identified by $pid) in search view.
Called in islandora_sulution_pack_collection/theme/theme.inc to redirect
collections to search view
*/
function opensky_redirect_to_search_view ($pid) {
    // dsm('opensky_redirect_to_search_view redirecting to search: '. $pid);
    $options = array('query' => array(
        'type'=>'dismax',
        'collection'=>$pid
    ));
    drupal_goto('/islandora/search/',$options);
}

/**
 * hook to insert the collection link and x_link into the breadcrumbs (see
 * islandora_solr_search/includes/results->setBreadcrumbs()

the PROBLEM with this alter is that we don't have access to the info we need
(i.e., $params). So instead of using hook_alter we use conventional function
(opensky_collection_breadcrumb) which we call from within setBreadcrumbs..

 */
function opensky_islandora_breadcrumbs_alter(&$data, $context) {
    if (!isset($_GET['collection'])) {
        // dsm('no collection');
        return;
    }
    $collection = $_GET['collection'];
}

/**
 * $qp is an instance of IslandoraSolrQueryProcessor
 * if there is a collection parameter, add filters to limit search to named
 * collection and it's subcollections if any exist
 */
function opensky_islandora_solr_query_alter($qp) {
    // dsm('opensky_islandora_solr_query_alter');
    if (isset($qp->internalSolrParams['collection'])) {
        $collection = $qp->internalSolrParams['collection'];

        // create $fq_new by removing all RELS_EXT_isMemberOfCollection_uri_ms
        // clauses
        $fq_new = array();
        $pat = 'RELS_EXT_isMemberOfCollection_uri_ms';
        if (isset($qp->solrParams['fq'])) {
            $fq_old = $qp->solrParams['fq'];
            unset($qp->solrParams['fq']);
            foreach ($fq_old as $item) {
                if (substr($item, 0, strlen($pat)) !== $pat) {
                    $fq_new[] = $item;
                }
            }
        }

        // construct a filter to search all searchable sub-collections (those
        // that have no sub-collections).
        $searchable_collections = opensky_get_searchable_subcollections($collection);

        $children_params = array();
        foreach ($searchable_collections as $searchable) {
            $children_params[] = 'RELS_EXT_isMemberOfCollection_uri_ms:"info:fedora/'.$searchable.'"';
        }

        $searchable_collection_clause = implode(' OR ', $children_params);

        $qp->solrParams['fq'] = array_merge($fq_new, array($searchable_collection_clause));
        // dsm($qp->solrParams);
    }
}

/**
 * Called from islandora_solr_search/includes/results->setBreadcrumbs(), creates
 * the collection breadcrumb, which is inserted into breadcrumbs by the caller
 */
function opensky_collection_breadcrumb($params) {

    //    module_load_include('inc', 'opensky', 'includes/utilities');
    $collection = $params['collection'];     
    $dc_array = opensky_get_dc_array($collection);
    $filter_string = $dc_array['dc:title']['value'];
    
    // Remove filter array for collection breadcrumb link
    $query = array();
    
    $query_diff = $params;
    
    if (isset($query_diff['f'])) {
        unset($query_diff['f']);
    }
    
    // Set attributes variable.
    $attr = array();
    $attr['title'] = $filter_string;
    $attr['rel'] = 'nofollow';
    if ($exclude) {
        $attr['class'] = 'strikethrough';
    }
    // remove query from path if there is one
    // $attr['href'] = url(ISLANDORA_SOLR_SEARCH_PATH, array('query' => $query_diff));
    $attr['href'] = url('/islandora/search', array('query' => $query_diff));
    
    
    // Pull out filter (for x link).
    $query_x = array_diff($params, array('collection'=>$collection));
    
    $attr_x = array();
    $attr_x['title'] = t('Remove') . ' ' . $filter_string;
    $attr_x['rel'] = 'nofollow';
    $attr_x['href'] = url(current_path(), array('query' => $query_x));
    
    $breadcrumb = '<a' . drupal_attributes($attr) . '>' . check_plain($filter_string) . '</a>'
                . '<span class="islandora-solr-breadcrumb-super"> <a' . drupal_attributes($attr_x) . '>('
                . t('x') . ')</a></span>';
    return $breadcrumb;
}

/**
 * Supply in collections info for islandora_scholar (citation model).
Called from islandora_scholar_get_metadata_display(in islandora_scholar/includes/utilities.inc)
 */
function opensky_get_in_collections_display ($object) {
    $parent_collections = islandora_get_parents_from_rels_ext($object);
    return theme('opensky_in_collections', array(
        'parent_collections' => $parent_collections));
}

// --------- YUK --------------
/**
 * returns a list of subcollections for the given parent_arg, which can be either a
 * collection PID, a opensky_collection_info, or a fedora_obj representing a collection.
 *
 * If the recursive param is true, then recursively find all subcollections under parent
 *
 * @param mixed $parent_arg
 * @param bool $recursive
 */
/*
function opensky_get_subcollections ($parent_arg, $recursive=false) {
    // dsm('opensky_get_subcollections: $parent is a '.gettype($parent_arg));
    $parent_obj = null;
    switch (gettype ($parent_arg)) {
        case 'string':
            $parent_obj = opensky_get_fedora_object($parent_arg);
            break;
        case 'object':
            $parent_obj = $parent_arg;
            break;
        case 'array':
            $parent_obj = opensky_get_fedora_object($parent_arg['pid']);
            break;
        default:
            // dsm('parent_arg not handled');
    }
    
    $ret = array (opensky_collection_obj_to_info ($parent_obj));
    $children = opensky_get_subcollection_infos ($parent_obj);
    if ($recursive) {
        foreach ($children as $child) {
            $ret = array_merge($ret, opensky_get_subcollections($child, true));
        }
    }
    return $ret;

}
*/
/**
 * Here we get current request stuff, but no longer have access to
 * simple/advanced search blocks
 */
function opensky_page_alter(&$page) {
//    dsm ('PAGE ALTER');
//    dsm($_GET);
//    dsm ($page);
}