<?php

/**
 * @file
 * Convert mods data into JSON for citeproc-js.
 */


/**
 * Add the MODS namespace to the given element, if it isn't already there.
 */
function add_mods_namespace(SimpleXMLElement &$mods) {
  static $used_namespace = NULL;
  static $mods_namespace = 'http://www.loc.gov/mods/v3';

  $namespaces = $mods->getNamespaces();

  if (is_null($used_namespace)) {
    // The namespace is there; possibly default, though.
    if (array_search($mods_namespace, $namespaces) !== FALSE) {
      $used_namespace = $mods_namespace;
    }
    else {
      $used_namespace = '';
    }
  }

  if (array_key_exists('mods', $namespaces) === FALSE) {
    $mods->registerXPathNamespace('mods', $used_namespace);
  }
}

/**
 * Perform a number of XPath queries to extract CSL info.
 *
 * Despite name, actually returns a PHP array. Run results through
 * _citeproc_array_to_object() before passing off to CiteProc-PHP.
 *
 * @param mixed $mods_in
 *   Either a DOMNode or a string representing a MODS record.
 *
 * @return array
 *   An associative array mapping CSL variable names to values we have
 *   extracted for them from the provided MODS record.
 */
function opensky_convert_mods_to_citeproc_jsons_alter(array &$output, SimpleXMLElement $mods) {

  if ($mods instanceof SimpleXMLElement) {
      add_mods_namespace($mods);

      $gtype = convert_mods_to_citeproc_json_genre($mods);
      switch (trim($gtype)) {
          case 'paper-conference': {
              $container_title = convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:name[@type="conference"]/mods:namePart');
              break;
          }
          default: {
              $container_title = convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:relatedItem[@type="host"]/mods:titleInfo[not(@type)]/mods:title');
          }
      }

      // replace selected values with opensky values
      $output['container-title'] = $container_title;

      $output['volume'] = (int)convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:relatedItem[@type="host"]/mods:part/mods:detail[@type="volume"]/mods:number');

      $output['issue'] = (int)convert_mods_to_citeproc_json_query($mods, variable_get('islandora_scholar_xpaths_issue', '//mods:mods[1]/mods:part/mods:detail[@type="issue"]/mods:number'));

      $output['publisher'] = opensky_convert_mods_to_citeproc_json_publisher($mods);

      $output['publisher-place'] = opensky_convert_mods_to_citeproc_json_publisher_place($mods);
      $output['URL'] = convert_mods_to_citeproc_json_query($mods, variable_get('islandora_scholar_xpaths_URL', '//mods:mods[1]/mods:location/mods:url'));
      $output['number'] = opensky_convert_mods_to_citeproc_json_number($mods);
      $output['number-of-pages'] = convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:part/mods:extent[@unit="pages"]/mods:total');
      $output['type'] = convert_mods_to_citeproc_json_genre($mods);
  }

}


function opensky_convert_mods_to_citeproc_json_number(SimpleXMLElement $mods) {
    $genre = trim(convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:genre'));
    if (($genre == 'report') or ($genre == 'technical report') or ($genre == 'workshop report') or ($genre == 'progress report') or ($genre == 'white paper')) {
#      $output = $mods->xpath("//mods:mods[1]/mods:relatedItem[@type='series']/mods:part[not(@type)]/mods:detail[@type='report']/mods:number");
        $output = convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:relatedItem[@type="series"]/mods:part[not(@type)]/mods:detail[@type="report"]/mods:number');
    }
    else {
        $output = $mods->xpath("//mods:mods[1]/mods:relatedItem[@type='series']/mods:titleInfo/mods:partNumber");
    }
    return !empty($output) ? $output : NULL;
}


/**
 * Gets the publisher property for the Citation.
 *
 * @param SimpleXMLElement $mods
 *   A MODS document.
 *
 * @return string
 *   The publisher property for the Citation.
 */
function opensky_convert_mods_to_citeproc_json_publisher(SimpleXMLElement $mods) {

    $gtype = convert_mods_to_citeproc_json_genre($mods);
    switch (trim($gtype)) {
        case 'paper-conference': {
            $publisher = convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:originInfo/mods:place/mods:placeTerm[@type="text"]');
            $publisher2 = explode ("-", convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:originInfo/mods:place/mods:placeTerm[@authority="iso3166"]'));
            $publisher3 = convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:originInfo/mods:place/mods:placeTerm[@authority="marccountry"]');
            if (array_key_exists('1', $publisher2)) {
                $publisher = $publisher . ', ' . $publisher2[1];
            }
            if (isset($publisher3)) {
                $publisher = $publisher . ', ' . $publisher3;
            }
            return $publisher;
        }
        default: {
            $publisher = convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:originInfo/mods:publisher');
            return $publisher;
        }
    }
}

/**
 * Gets the publisher-place property for the Citation.
 *
 * @param SimpleXMLElement $mods
 *   A MODS document.
 *
 * @return string
 *   The publisher-place property for the Citation.
 */
function opensky_convert_mods_to_citeproc_json_publisher_place(SimpleXMLElement $mods) {

    $gtype = convert_mods_to_citeproc_json_genre($mods);
    switch (trim($gtype)) {
        case 'paper-conference': {
            $publisher_place = convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:name[@type="corporate"]/mods:namePart');
            $article_number = convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:relatedItem[@type="host"]/mods:part/mods:detail[@type="citation/article"]/mods:number');
            return $publisher_place;
        }
        default: {
            $publisher_place = convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:originInfo/mods:place/mods:placeTerm');
            return $publisher_place;
        }
    }
}


// ---------- Changed Functions ------------------

/**
 * Convert the given MODS genre to JSON repersentation.
 *
 * @param mixed $mods
 *   A MODS document.
 *
 * @return string
 *   The JSON repersentation of the MODS genre.
 */
function convert_mods_to_citeproc_json_genre($mods) {
    $genre = convert_mods_to_citeproc_json_query($mods, variable_get('islandora_scholar_xpaths_genre', '//mods:mods[1]/mods:genre'));
    switch (trim($genre)) {
        case 'journal article':
            return 'article-journal';

        case 'chapter':
        case 'book chapter':
        case 'book section':
            return 'chapter';

        case 'book':
            return 'book';

        case 'technical report':
        case 'progress report':
        case 'workshop report':
        case 'white paper':
            return 'report';

        case 'conference material':
            return 'paper-conference';

        case 'proceedings article':
            return 'paper-conference';


        default:
            return 'article-journal';
    }
}

// ============= Replicated Functions =====================
/**
 * Gets the first result of the provided $path.
 *
 * @param SimpleXMLElement $mods
 *   A MODS document.
 * @param string $path
 *   An XPath.
 *
 * @return string
 *   The first results of the query if found NULL otherwise.
 */
function convert_mods_to_citeproc_json_query(SimpleXMLElement $mods, $path) {
    $property = $mods->xpath($path);
    return !empty($property) ? (string) $property[0] : NULL;
}

/**
 * Gets the publisher property for the Citation.
 *
 * @param SimpleXMLElement $mods
 *   A MODS document.
 *
 * @return string
 *   The publisher property for the Citation.
 */
function convert_mods_to_citeproc_json_publisher(SimpleXMLElement $mods) {

    $gtype = convert_mods_to_citeproc_json_genre($mods);
    switch (trim($gtype)) {
        case 'paper-conference': {
            $publisher = convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:originInfo/mods:place/mods:placeTerm[@type="text"]');
            $publisher2 = explode ("-", convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:originInfo/mods:place/mods:placeTerm[@authority="iso3166"]'));
            $publisher3 = convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:originInfo/mods:place/mods:placeTerm[@authority="marccountry"]');
            if (array_key_exists('1', $publisher2)) {
                $publisher = $publisher . ', ' . $publisher2[1];
            }
            if (isset($publisher3)) {
                $publisher = $publisher . ', ' . $publisher3;
            }
            return $publisher;
        }
        default: {
            $publisher = convert_mods_to_citeproc_json_query($mods, '//mods:mods[1]/mods:originInfo/mods:publisher');
            return $publisher;
        }
    }
}

// ========================= KLUDGES =================

if (!function_exists ('variable_get')) {
    function variable_get($pathname, $default_value) {
        print ("variable_get()");
        return $default_value;
    }

}
