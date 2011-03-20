<?php
  // run in drush: drush -u root @wb.test scr delete_schools.php

  // bad: test schools, etc
  $bad_nodes = array(58,52,1025,60,159,145,334,123,333,920,932,938);

  // duplicates: identical names
  $duplicates = array(49,55,882,63,69,70,76,260,143,174,188,226,236,934,936);

  // similar: small diffs
  $similar = array(256,205,197,241,198,224,98,666,83,909,96,719,204,65,258,59);

  $ns = array_merge($bad_nodes, $duplicates, $similar);
  print 'Nodes to delete: ' . implode(',', $ns) . "\n";

  foreach ($ns as $n) {
    print_node($n);
    node_delete($n);
  }

/* print given node or nid */
function print_node($n) {
  $node = is_numeric($n) ? node_load($n) : $n;
  if (!$node->nid) {
    print "NID $n not found.\n";
    return;
  }
  print  $node->nid . ' Title: ' . $node->title . ' ';
  print 'Location: ' . $node->locations[0]['street'] . ' '
                     . $node->locations[0]['city'] . ' '
                     . $node->locations[0]['province'] . ' '
                     . $node->locations[0]['postal_code'] . "\n";
}


?>


