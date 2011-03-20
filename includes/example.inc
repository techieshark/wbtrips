<?php
// $Id: example.inc,v 0.1 2010/7/5


/*Custom Form - 

Many Hidden Fields... need to 
1) Add New Node => create the register node

2) Subscribe => Make the user a member of the Group
og_save_subscription($join_group_id, $user->uid, array('is_active' => 1));
*/

//page call back
function ogevents_build_form($args) {
	//WHY ISN:T THIS BEING CALLED????????
	echo 'hi there';
	return drupal_get_form('_create_reg_form');
}

function _create_reg_form(&$form_state) {
	$var = 615;
	//change $var to %d
	$query = '
		SELECT node.nid AS nid, og_ancestry.nid AS og_ancestry_nid, og_ancestry.group_nid AS og_ancestry_group_nid FROM node node  
		LEFT JOIN og_ancestry og_ancestry ON node.nid = og_ancestry.nid 
		LEFT JOIN content_type_registration node_data_field_eventreg ON node.vid = node_data_field_eventreg.vid 
		LEFT JOIN node node_node_data_field_eventreg ON node_data_field_eventreg.field_eventreg_nid = node_node_data_field_eventreg.nid 
		WHERE node_node_data_field_eventreg.nid = %d';

	$result = db_query($query,$var);
	$regd_array = array();
	while ( $obj = db_fetch_object ($result) ) {
	 // print $obj->node_og_ancestry_nid;
	  $regd_array[] = $obj->og_ancestry_group_nid;
	}
	
	$query2 = 'SELECT node.nid AS nid, node.language AS node_language, node.title AS node_title FROM node node  LEFT JOIN og og ON node.nid = og.nid WHERE (node.type in (\'school\')) AND (node.status <> 0) AND (og.og_directory <> 0) ORDER BY node_title ASC';
	$result2 = db_query($query2);
	$allgroups = array();
	while ( $obj = db_fetch_object ($result2) ) {
	 // print $obj->node_og_ancestry_nid;
	  $allgroups[] = $obj->nid;
	}

	$nids = array_diff($allgroups, $regd_array);
	$titles = array();
	foreach ($nids as $item) {
			$node = node_load($item);
			$titles[] = t($node->title);
	}
	$options = array_combine($nids, $titles);

    //$form['og_nodeapi']['visible']['og_groups']['#multiple'] = FALSE;
	$form['School'] = array(
		'#type' => 'select',
		'#title' => t('Select a School'),	
		'#options' => $options;
	);
	
	$form['submit'] = array(
		'#type' => 'submit',
		'#value' => 'Register',
	  );
	
	//for some reason when the form is being submited, it is not being added to the og_ancestry table...?

    // Add own submit handler so we can cast the groups back to an array.
    //$form['#submit'][] = 'og_form_alters_audience_form_alter_submit';
  
  return $form;
}
