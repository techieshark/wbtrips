<?php
/**
 * @file
 * Tally page and its specific supporting functions.
 * @ingroup ogevents
 */


/**
 * Page (Menu item callback) to record trips for the month.
 */
function ogevents_page_tally($event_id, $school_id) {

  $output = '';
  $reg_nid = _ogevents_get_registration($event_id, $school_id);
  $tally_box = '<h3>' . t('1. Weekly Log (Optional)') . '</h3>' . drupal_get_form('ogevents_tally_form', $event_id, $school_id, $reg_nid);
  $month_box = '<h3>' . t('2. Final Results (Required)') . '</h3>' . drupal_get_form('ogevents_participation_form', $event_id, $school_id, $reg_nid);

  $school_node = node_load($school_id);
  $output .= '<h2>' . check_plain($school_node->title) . '</h2>';
  $output .= t('Tally trips for %school-name. Enter the total number of trips for your entire school. '
         . 'You can update the trip log weekly or just enter your totals at the end of the month.',
           array('%school-name' => $school_node->title));

  $output .= '<div id="ogevents-tally-control">';
  $output .= '<div id="ogevents-tallybox">' . $tally_box . '</div>';
  $output .= '<div id="ogevents-monthbox">' . $month_box . '</div>';
  $output .= '</div>';

  $output .= '<p>' . l(t('Return to My Schools'), "dashboard") . '<br/>';
  $output .= l(t('Compare your progress to other schools'), "event/$event_id/results") . '</p>';

  // output date - TODO

  drupal_add_js(drupal_get_path('module', 'ogevents') . '/tally.js');
  return $output;
}

/**
 * Page for confirmation after submitting monthly tally.
 */
function ogevents_page_tally_thanks($event_id, $school_id) {

  //TODO - the variables should be set (as they are) and passed to a 
  // theme function which will use the output html below.

  $name = _ogevents_get_node_title($school_id);
  $results = _ogevents_get_results($school_id, $event_id);

  $walk = $results['final_walk_trips'];
  $bike = $results['final_bike_trips'];
  $other = $results['final_other_trips'];
  $staff = $results['staff'];
  $parents = $results['parents'];

  $participants = $results['participants'];
  $studentbody = $results['studentbody'];
  $percent = round($results['student_percentage'],1);

  $output .= '<p>';
  $output = t('Thanks for submitting your Walk + Bike Challenge final results! ') . '<p/>';
  $output .= '<p>';
  $output .= t('The following data has been recorded for %name:', array('%name' => $name)) . '<br/>';
  $output .= t('<strong>Totals:</strong> @walk walk trips, @bike bike trips, @other other trips.', 
               array('@walk' => $walk, '@bike' => $bike, '@other' => $other)) . '<br/>';
  $output .= t('<strong>Participation:</strong> @participants of @studentbody students (@percent%), ' . 
               'plus @staff staff and @parents parents.', 
               array('@participants' => $participants, '@studentbody' => $studentbody, 
                     '@percent' => $percent,
                     '@staff' => $staff, '@parents' => $parents));
  $output .= '</p>';

  $output .= '<p>';
  $output .= t('If you need to you can still <a href="@tally-page">go back</a> and make changes. ',
             array('@tally-page' => url("event/$event_id/tally/$school_id")));
  $output .= t('Just remember to resubmit your results when finished.');
  $output .= '</p>';

  $output .= '<p>';
  $output .= t('You can return to <a href="@dashboard-page">My Schools</a> or ' .
             '<a href="@results-page">see Challenge results</a> for all schools.',
             array('@dashboard-page' => url('dashboard'), '@results-page' => url("event/$event_id/results")));
  $output .= '</p>';

  return $output;
}


/**
 * A form for recording each week's results for a month.
 */
function ogevents_tally_form($form_state, $event_id, $school_id, $reg_nid) {

  $form['trips'] = array(
      '#theme' => 'ogevents_tally_table', '#attributes' => array('summary' => 'Use this table to input weekly trips'),
  );

  $num_weeks = _ogevents_get_event_weeks($event_id);
  foreach (range(1,$num_weeks) as $week) {

    // set form defaults
    $default_bike = $default_walk = $default_other = 0;
    $tally_nid = _ogevents_get_nid_triptally($event_id, $school_id, $week);
    if ($tally_nid) {
      $tally_node = node_load($tally_nid);
      // assert tally_node is valid result (s/be, since t_nid is valid).
      $default_bike = $tally_node->field_biketrips[0]['value'];
      $default_walk = $tally_node->field_walktrips[0]['value'];
      $default_other = $tally_node->field_othertrips[0]['value'];
    }

    $form['trips']["Week $week"] = array();
    $form['trips']["Week $week"]["walktrips$week"] = _ogevents_form_input($default_walk);
    $form['trips']["Week $week"]["biketrips$week"] = _ogevents_form_input($default_bike);
    $form['trips']["Week $week"]["othertrips$week"] = _ogevents_form_input($default_other);
  }

  $form['event'] = array('#type' => 'hidden', '#value' => $event_id);
  $form['school'] = array('#type' => 'hidden', '#value' => $school_id);
  $form['reg'] = array('#type' => 'hidden', '#value' => $reg_nid);

  $form['update-totals'] = array(
      '#type' => 'submit',
      '#value' => 'Save & Copy to Results',
       );

  $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Save',
      );

  return $form;
}


/** Helper function that returns an input form element with the default settings. */
function _ogevents_form_input($default) {
  return array(
    '#type' => 'textfield',
    '#default_value' => $default,
    '#size' => 4,
    '#maxlength' => 4,
  );
}


/**
 * Theme the tally table (grid of inputs).
 */
function theme_ogevents_tally_table($form) {

  $weeks = element_children($form);
  foreach ($weeks as $week) {
    $modes = element_children($form[$week]);
    $row = array(array('data' => $week, 'header' => TRUE));
    foreach ($modes as $mode) {
      $row[] = drupal_render($form[$week][$mode]);
    }
    $rows[] = $row;
  }
  $rows[] = array(
    'data' => array(
      array('data' => t('Totals'), 'header' => TRUE),
      array('data' => '0', 'class' => 'walk'),
      array('data' => '0', 'class' => 'bike'),
      array('data' => '0', 'class' => 'other'),
    ),
    'class' => 'sum'
  );

  $header = array(
    array('data' => '', 'class' => 'emptyheader'),
    array('data' => '<div><span class="logo"></span><span>' . t('Walk') . '</span></div>', 'class' => 'walk'),
    array('data' => '<div><span class="logo"></span><span>' . t('Bike') . '</span></div>', 'class' => 'bike'),
    array('data' => '<div><span class="logo"></span><span>' . t('Other') . '</span></div>', 'class' => 'other')
  );

  $attr = $form['#attributes'];
  if (!$attr) {
    $attr = array('class' => 'wb-inputgrid');
  }
  else { // should be an array
    $attr['class'] = isset($attr['class']) ? $attr['class'] . ' wb-inputgrid' : 'wb-inputgrid';
  }

  $output = theme('table', $header, $rows, $attr);

  return $output;
}


/**
 * Submit tally for the month.
 */
function ogevents_tally_form_submit($form, &$form_state) {

  _ogevents_debug('tally_form_submit...');
  _ogevents_debug($form_state);

  $event_id = $form_state['values']['event'];
  $school_id = $form_state['values']['school'];
  $reg_nid = $form_state['values']['reg'];

  // Update triptally node with results for each week
  $num_weeks = _ogevents_get_event_weeks($event_id);
  foreach (range(1, $num_weeks) as $week) {
    $walk = $form_state['values']["walktrips$week"];
    $bike =  $form_state['values']["biketrips$week"];
    $other =  $form_state['values']["othertrips$week"];
    _ogevents_update_triptally($event_id, $school_id, $week, $bike, $walk, $other);

    $walk_total += $walk;
    $bike_total += $bike;
    $other_total += $other;
  }

  $ret = _ogevents_save_results($school_id, $event_id,
    array(
      "walk_total" => $walk_total,
      "bike_total" => $bike_total,
      "other_total" => $other_total,
      "sum_total" => $walk_total + $bike_total + $other_total,
      )
  );



  // Additionally, if user clicked button to copy totals to final results, do that
  if ($form_state['clicked_button']['#value'] == $form_state['values']['update-totals']) {
    $reg_node = node_load($reg_nid);
    $reg_node->field_walktrips[0]['value'] = $walk_total;
    $reg_node->field_biketrips[0]['value'] = $bike_total;
    $reg_node->field_othertrips[0]['value'] = $other_total;
    node_save($reg_node);
  }

  if (!$ret) {
    drupal_set_message(t('Failed to cache results.'), 'error');
  }
}


/**
 * Create or update a triptally node.
 * @param school_id The school nid.
 * @param week The week we're updating
 * @param bike Number of trips by bike.
 * @param walk Number of trips on foot.
 * @parm other Number of other trips.
 */
function _ogevents_update_triptally($event_id, $school_id, $week, $bike, $walk, $other) {
  // look for the node
  $tally_nid = _ogevents_get_nid_triptally($event_id, $school_id, $week);

  if ($tally_nid) { // use existing node if exists
    $tally_node = node_load($tally_nid);
  }
  else { // create new node
    if ($bike == 0 && $walk == 0 && $other == 0) { // don't save useless nodes.
      return;
    }

    $is_new_node = TRUE;

    // add node properties
    $tally_node = (object) NULL;
    $tally_node->type = 'triptally';
    $tally_node->title = 'Total Trips';
    $tally_node->uid = $GLOBALS['user']->uid;
    $tally_node->created = strtotime("now");
    $tally_node->changed = strtotime("now");
    $tally_node->status = 1;
    $tally_node->comment = 0;
    $tally_node->promote = 0;
    $tally_node->moderate = 0;
    $tally_node->sticky = 0;

    $tally_node->field_eventid[0]['nid'] = $event_id;
  }

  // update relevant node bits from function arguments.
  $tally_node->field_week[0]['value'] = $week;
  $tally_node->field_biketrips[0]['value'] = $bike;
  $tally_node->field_walktrips[0]['value'] = $walk;
  $tally_node->field_othertrips[0]['value'] = $other;

  // save.
  node_save($tally_node);
  if (! $tally_node->nid) {
    drupal_set_message("Error saving bike/walk/other values ($bike/$walk/$other) " .
                       "for event/school/week ($event_id,$school_id,$week).", 'error');
  }

  // First time we save the node we need to put in the Org. Group ancestry table too.
  // Note - if we ever get rid of the OG bits we could just store groupid as a node field. (TODO?)
  if ($is_new_node) {
    $sql = "INSERT INTO {og_ancestry} (nid, group_nid) VALUES (%d, %d)";
    db_query($sql, $tally_node->nid, $school_id);
  }

  return;
}


function ogevents_participation_form($form_state, $event_id, $school_id, $reg_nid) {
  $form = array();

  $reg_node = node_load($reg_nid);
  _ogevents_debug("reg_node in populate_participation_form:");
  _ogevents_debug($reg_node);

  $studentbody = $reg_node->field_studentbody[0]['value'];
  $participants = $reg_node->field_participants[0]['value'];
  $staff = $reg_node->field_staff[0]['value'];
  $parents = $reg_node->field_parents[0]['value'];

  $results = _ogevents_get_results($school_id, $event_id);
  $default_walk = $results['final_walk_trips'];
  $default_bike = $results['final_bike_trips'];
  $default_other = $results['final_other_trips'];
  if (!isset($default_walk)) $default_walk = 0;
  if (!isset($default_bike)) $default_bike = 0;
  if (!isset($default_other)) $default_other = 0;

  $form['totals'] = array('#theme' => 'ogevents_tally_table', '#attributes' => array('summary' => 'Use this table to record final results'));
  $form['totals']['Totals'];
  $form['totals']['Totals']['walk'] = _ogevents_form_input($default_walk);
  $form['totals']['Totals']['bike'] = _ogevents_form_input($default_bike);
  $form['totals']['Totals']['other'] = _ogevents_form_input($default_other);

  $form['participation'] = array(
      '#type' => 'fieldset',
      '#attributes' => array('class' => 'ogevents-participation'),
      );

  $form['participation']['participants'] = array(
      '#type' => 'textfield',
      '#title' => t('How many students participated (at least one active trip) in the Challenge?'),
      '#default_value' => $participants,
      '#size' => 30,
      '#maxlength' => 12,
      '#required' => TRUE,
      );

  $form['participation']['studentbody'] = array(
      '#type' => 'textfield',
      '#title' => t('What is the total student body population at your school?'),
      '#default_value' => $studentbody,
      '#size' => 30,
      '#maxlength' => 12,
      '#required' => TRUE,
      );

  $form['participation']['parents'] = array(
      '#type' => 'textfield',
      '#title' => t('How many parents helped or participated?'),
      '#default_value' => $parents,
      '#size' => 30,
      '#maxlength' => 12,
      '#required' => TRUE,
      );

  $form['participation']['staff'] = array(
      '#type' => 'textfield',
      '#title' => t('How many teachers/staff helped or participated?'),
      '#default_value' => $staff,
      '#size' => 30,
      '#maxlength' => 12,
      '#required' => TRUE,
      );

  $form['event'] = array('#type' => 'hidden', '#value' => $event_id);
  $form['school'] = array('#type' => 'hidden', '#value' => $school_id);
  $form['reg'] = array('#type' => 'hidden', '#value' => $reg_nid);

  $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Submit my Challenge results',
      );

  return $form;
}

/**
 * Form validation function.
 */
function ogevents_participation_form_validate($form, &$form_state) {
  // # participating students cannot be > # student body
  if ($form_state['values']['participants'] > $form_state['values']['studentbody']) {
    form_set_error('studentbody', 
      t('You have more students participating than you have in the school. How is that possible?'));
  }
  
  // value must be >= 0 (numbers)
  $keys = array('parents', 'staff', 'participants', 'studentbody');
  foreach ($keys as $key) {
    $value = $form_state['values'][$key];
    if (! is_numeric($value) || $value < 0) {
      form_set_error($key, t('Expected a numeric value greater than or equal to zero.'));
    }
  }
}


/**
 * Submit month participation totals.
 */
function ogevents_participation_form_submit($form, &$form_state) {
  _ogevents_debug("form_state: "); //DEBUG
  _ogevents_debug($form_state); //DEBUG

  $event_id = $form_state['values']['event'];
  $school_id = $form_state['values']['school'];
  $reg_nid = $form_state['values']['reg']; // the registration for the previously given school+event
  $reg_node = node_load($reg_nid);

  $studentbody = $form_state['values']['studentbody'];
  $participants = $form_state['values']['participants'];

  $reg_node->field_biketrips[0]['value'] = $form_state['values']['bike'];
  $reg_node->field_walktrips[0]['value'] = $form_state['values']['walk'];
  $reg_node->field_othertrips[0]['value'] = $form_state['values']['other'];
  $reg_node->field_participants[0]['value'] = $participants;
  $reg_node->field_parents[0]['value'] = $form_state['values']['parents'];
  $reg_node->field_staff[0]['value'] = $form_state['values']['staff'];
  $reg_node->field_studentbody[0]['value'] = $studentbody;
  $reg_node->field_results_submitted[0]['value'] = 1;

  _ogevents_save_results($school_id, $event_id,
    array("participation" => $studentbody ? ($participants / $studentbody * 100) : 0));

  node_save($reg_node);

  //DEBUG
  _ogevents_debug("reg node after saving: ");
  _ogevents_debug($reg_node);

  $form_state['redirect'] = "event/$event_id/thanks/$school_id";
}
