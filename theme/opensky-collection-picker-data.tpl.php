<?php
/**
 * Created by IntelliJ IDEA.
 * User: ostwald
 * Date: 11/30/17
 * Time: 1:59 PM
 */

//dsm('theme/opensky-collection-picker.tpl.php');
//dsm($_GET);

?>

<script>

    var COLLECTION_SELECT_OPTIONS = {};
    <?php foreach ($option_data as $key => $value): ?>
        COLLECTION_SELECT_OPTIONS["<?php print $key ?>"] = "<?php print $value ?>";
    <? endforeach ?>
</script>
<div style="font-size:9pt;color:gray">hello from opensky-collection-picker</div>
