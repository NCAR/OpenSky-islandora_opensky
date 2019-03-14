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
    $path_parts = explode('/', request_path());
    $tail = array_pop($path_parts);
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

    $parts = explode('/',request_path());
    $term = array_pop($parts);
    //    $term = array_pop(explode('/',request_path()));
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
 * IMplements hook_islandora_solr_query
 *
 * HARD-CODE fields returned by solr query to make solr responses smaller                                                           
 * and thus avoid PHP Out of memory errors 
 */
function opensky_islandora_solr_query($islandora_solr_query) {
    //    dpm ('opensky_islandora_solr_query');
    //    dpm ($islandora_solr_query);

    $islandora_solr_query->solrParams['fl'] = 'PID,mods*,fgs*,dc.*,dsmd_DC*,keyDate*,RELS_EXT*,*Display*,fedora_datastreams_ms';

}

/**
 * $qp is an instance of IslandoraSolrQueryProcessor
 * if there is a collection parameter, add filters to limit search to named
 * collection and it's subcollections if any exist
 */
function opensky_islandora_solr_query_alter($qp) {
    // dsm('opensky_islandora_solr_query_alter');

    // if sort is not set for search pages, then set it to keyDate desc
    if (current_path() == ISLANDORA_SOLR_SEARCH_PATH  && !isset($qp->solrParams['sort'])) {
        $qp->solrParams['sort'] = 'keyDate desc';
    }

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
 * implements hook_islandora_solr_results_alter
 *
 * Set thumbnail for citationCModel objects to OpenSky custom image
 * (BiblioCitOnly.png);
 */
function opensky_islandora_solr_results_alter(&$object_results, $query_processor) {

    $default_path = drupal_get_path('module', 'islandora_solr') . '/images/defaultimg.png';
    foreach ($object_results as $object_index => $object_result) {
        if (isset ($object_results[$object_index]['thumbnail_url']) && $object_results[$object_index]['thumbnail_url'] == $default_path) {
            if (strpos($object_results[$object_index]['content_models'][0], 'citationCModel') !== false) {
                $object_results[$object_index]['thumbnail_url'] = drupal_get_path('module', 'opensky') . '/images/BiblioCitOnly.png';
            }
        }
    }
}

/**
 * hook to insert the collection link and x_link into the breadcrumbs (see
 * islandora_solr_search/includes/results->setBreadcrumbs()

DON'T KNOW how to explain, but this function is getting called TWICE in a row.
You can bracket the drupal_alter call (see islandora_solr_search/includes/results.inc
getBreadcrumbs() with print statements and for whatever evil reason,
opensky_islandora_breadcrumbs_alter gets called TWICE between the prints!?!?

therefore, we have to detect when we're called the second time and
politely refuse to do anything .....
 */
function opensky_islandora_breadcrumbs_alter(&$data, $context) {
    // only make change if there is a collection param
    if (!isset($_GET['collection'])) {
        return;
    }
    $collection = $_GET['collection'];

    // Look for the collection_breadcrumb - it has a "collection-crumb" class so
    // we just look for that string
    foreach ($data as $crumb) {
        if (strpos($crumb, 'collection-crumb') !== false) {
            // our work is done - we've already added the collection breadcrumb
            return;
        }
    }
    $collection_breadcrumb = opensky_collection_breadcrumb($_GET);

    // splice it into existing breadcrums
    array_splice ($data, 1, 0, $collection_breadcrumb);
}


/**
 * creates the collection breadcrumb, which is inserted into
 * breadcrumbs by opensky_islandora_breadcrumbs_alter
 */
function opensky_collection_breadcrumb($params) {
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
    if (isset($exclude) && $exclude) {
        $attr['class'] = 'strikethrough';
    }
    // remove query from path if there is one
    // $attr['href'] = url(ISLANDORA_SOLR_SEARCH_PATH, array('query' => $query_diff));
    $attr['href'] = url('/islandora/search', array('query' => $query_diff));
        
    // Pull out filter (for x link).
    $query_x = array();  // query will be like current except it will not have collection param
    foreach ($params as $key => $value) {
        if ($key != 'collection' && $value == $collection) $query_x[$key] = $value;
    }

    $attr_x = array();
    $attr_x['title'] = t('Remove') . ' ' . $filter_string;
    $attr_x['rel'] = 'nofollow';
    $attr_x['href'] = url(current_path(), array('query' => $query_x));
    
    $breadcrumb = '<a' . drupal_attributes($attr) . '>' . check_plain($filter_string) . '</a>'
                . '<span class="islandora-solr-breadcrumb-super collection-crumb"> <a' . drupal_attributes($attr_x) . '>('
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

/*
 * implements hook_islandora_get_view_alter
 * see islandora_scholar/includes/utilities islandora_scholar_get_view()
 *
 * Insert times viewed and times downloaded for islandora views
 * Customize the Download PDF link slightly
 */
function opensky_islandora_scholar_get_view_alter(&$display, $object) {

    if (module_exists('islandora_usage_stats')) {
       module_load_include('inc', 'islandora_usage_stats', 'includes/db');
        // using the view_count method from db.inc
        $view_count = islandora_usage_stats_get_individual_object_view_count($object);
        $ds_count = islandora_usage_stats_get_datastream_downloads_count($object);
        $times_downloaded = 0;
        if (array_key_exists('PDF',$ds_count)){
            $times_downloaded = $ds_count['PDF'];
        }
        $display['view_count']['#markup'] =t("<br/>Times Viewed on OpenSky: $view_count <br/>" .
                                             "Times Downloaded on OpenSky: $times_downloaded <br/> <br/>");
        /* we have to assign view_count a weight that puts it inbetween
           'preview' and 'pdf_download'
           - the other #weight values are integers but the following seems to
           work to put 'view_count' above 'pdf_download'
           - note that using 'pdf_download''s exact weight also does the same thing ...
        */
        $display['view_count']['#weight'] = $display['pdf_download']['#weight'] - 0.5;
    }

    // Set text on download link for PDFs
    if (isset ($display['pdf_download']['#markup'])) {
        unset($display['pdf_download']['#title']);
        $display['pdf_download']['#markup'] = t('Download ') . l(t('PDF'), 
                                              "islandora/object/$object->id/datastream/PDF/download/citation.pdf") .
                                            "<br/><br/>";
    }
}

function opensky_retrieve_doi_markup(AbstractObject $object) {
    $markup = '';
    $mods_xml = simplexml_load_string($object['MODS']->content);
    $mods_xml->registerXPathNamespace('mods', 'http://www.loc.gov/mods/v3');
    $doi_xpath = $mods_xml->xpath('//mods:identifier[@type="doi"]');
    if ($doi_xpath) {
        $doi_link = (string) $doi_xpath[0];
        $markup = 'Published Version: <a href="http://dx.doi.org/' . $doi_link . '">http://dx.doi.org/' . $doi_link . '</a> <br/> <br/>';
    }
    return $markup;
}

/**
 * Add "in collection" link to metadata display.
 */
function opensky_islandora_scholar_get_metadata_display_alter (&$display, $object, $weight) {

    if (variable_get('islandora_scholar_use_standard_metadata_display', FALSE)) {
        module_load_include('inc', 'islandora', 'includes/metadata');
        // Borrowing core JS for fieldsets.                                                                                              
        drupal_add_js('misc/form.js');
        drupal_add_js('misc/collapse.js');
        $metadata = islandora_retrieve_metadata_markup($object);
        $description = islandora_retrieve_description_markup($object);
        
        $doi_link = opensky_retrieve_doi_markup($object);

        // Insert 'in collections' display element from opensky                                                            
        $in_collections = opensky_get_in_collections_display($object);

        $display['#markup'] = $doi_link . $description . $in_collections . $metadata;
    }

}


/*
 * Islandora_scholar _pdf_upload_form_alter is used to upload PDF in the process
 * of creating a new object with forms. (see includes/pdf_upload.form.inc)

 * Here we simply remove the usage options from the form, and then replace the
 * scholar module's validation function
 * (islandora_scholar_pdf_upload_form_validate) with our own version that
 * doesn't require the usage options.
 */
function opensky_form_islandora_scholar_pdf_upload_form_alter (&$form, &$form_state, $form_id) {
    //    remove $form['upload_document']['usage']
    unset ($form['upload_document']['usage']);

    // now change the validation function
    module_load_include ('inc', 'opensky', 'includes/utilities');

    // insert opensky validate before islandora_scholar_pdf_upload_form_validate
    //    (didn't work)
    //    array_splice ($form['hidden_next']['#validate'], 0, 0, 'opensky_pdf_upload_form_validate'); 
  
    // replace islandora_scholar_pdf_upload_form_validate with opensky version
    $form['hidden_next']['#validate'][0] = 'opensky_pdf_upload_form_validate'; 
}

/*
 * This is a duplicate of islandora_scholar_pdf_upload_form_validate except we
 * don't validate the usage block, since we removed these fields from form (see
 * includes/pdf_upload.form.inc) 
 *
 * NOTE: I tried to do this in a more graceful way, but 
 */
function opensky_pdf_upload_form_validate(&$form, &$form_state) {
    error_log ('islandora_scholar_pdf_upload_form_validate ...');
    if ($form_state['values']['upload_pdf_checkbox']) {
        if (empty($form_state['values']['file'])) {
            form_set_error('file', t('A file must be uploaded!'));
        }
        if (empty($form_state['values']['version'])) {
            form_set_error('version', t('A document version must be selected!'));
        }

        /*
        // OpenSky - comment block
        if (empty($form_state['values']['usage'])) {
            form_set_error('usage', t('A usage permission must be selected!'));
        }
        // end opensky comment
        */

        if (empty($form_state['values']['certifying']['certify'])) {
            form_set_error('certifying', t('You must certify that you have the right to upload this PDF!'));
        }
    } 
}
