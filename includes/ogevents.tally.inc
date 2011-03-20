<?php 
/**
 * @file 
 * Tally page and its specific supporting functions.
 * @ingroup ogevents
 */

/**
 * Page (Menu item callback) to record trips for the month.
 */
function ogevents_page_tally($event_nid, $gid) {
  $school_nid = $gid;

  $output = '';
  $school_node = node_load($gid); // load school

  // Sanity checks:
  // - does school exist?
  if (!$school_node) {
    drupal_set_message(t('The school ID ("!school-id") does not exist.', array('!school-id' => $gid)), 'error');
    return $output;
  }
  //   - is school registered for this event?
  $reg_nid = _ogevents_get_registration($event_nid, $gid);
  if (!$reg_nid) {
    drupal_set_message(t('No registration was found for the given school ("!school-id") & event ("!event-id").',
                         array('!school-id' => $gid, '!event-id' => $event_nid)), 'error');
    return '';
  }
  else {
    // - is this our school?
    $reg_node = node_load($reg_nid);
    global $user;
    if ($user->uid != $reg_node->uid) {
      drupal_set_message(t('You cannot edit the tally for this school because you did not register it.'), 'error');
      return '';
    }
  }
  // is tallying open?
  if (! _ogevents_is_tallying_open($event_nid)) {
    drupal_set_message(t('Challenge is not yet open for tallying.'), 'error');
    return '';
  }



  $output .= '<h2>' . check_plain($school_node->title) . '</h2>';
  $output .= t('Tally trips for %school-name. Enter the total number of trips for your entire school. '
         . 'You can update the trip log weekly or just enter your totals at the end of the month in the last row.',
           array('%school-name' => $school_node->title));


  $header = array('Walk', 'Bike', 'Other');
  $rows = array();
  $results = _ogevents_get_results($school_nid, $event_nid);
  $bike = $results['bike_trips'];
  $walk = $results['walk_trips'];
  $other = $results['other_trips'];
  $rows[] = array($walk, $bike, $other);

  $tally_box = '<h3>' . t('Weekly Totals') . '</h3>' . drupal_get_form('ogevents_tally_form', $event_nid, $gid);
  $month_box = '<h3>' . t('Monthly Totals') . '</h3>' . theme('table', $header, $rows) . drupal_get_form('ogevents_participation_form', $event_nid, $gid);

  $output .= '<div id="ogevents-tally-control">';
  $output .= '<div id="ogevents-tallybox">' . $tally_box . '</div>';
  $output .= '<div id="ogevents-monthbox">' . $month_box . '</div>';
  $output .= '</div>';

  $output .= '<div class="ogevents-after-form">';
  $output .= '<p>' . l(t('Return to My Schools'), "dashboard") . '</p>';
  $output .= '<p>' . l(t('Compare your progress to other schools'), "event/$event_nid/results") . '</p>';
  $output .= '</div>';

  // output date - TODO

  return $output;
}

/**
 * Page for confirmation after submitting monthly tally.
 */
function ogevents_page_tally_thanks($event_nid, $gid) {
  $output .= '<p>';
  $output = t('Thanks for submitting your Walk + Bike Challenge final results!');
  $output .= '</p>';
  $output .= '<p>';
  $output .= t('If you need to you can still <a href="@tally-page">go back</a> and change them. ',
             array('@tally-page' => url("event/$event_nid/tally/$gid")));
  $output .= t('Just make sure to submit your results again when finished.');
  $output .= '</p>';

  $output .= '<p>';
  $output .= t('Go back to <a href="@dashboard-page">My Schools</a> or ' .
             '<a href="@results-page">see Challenge results</a> for all schools.',
             array('@dashboard-page' => url('dashboard'), '@results-page' => url("event/$event_nid/results")));
  $output .= '</p>';

  return $output;
}


/**
 * A form for recording each week's results for a month.
 */
function ogevents_tally_form($form_state, $event_nid, $gid) {

  $form = array();

  $form['trips'] = array(
      '#type' => 'fieldset',
      //'#title' => t('Weekly Totals (optional)'),
      '#attributes' => array('class' => 'ogevents-tally'),
      //'#collapsible' => FALSE,
      );

  foreach (range(1,OGEVENTS_NUM_WEEKS) as $week) {

    $default_bike = $default_walk = $default_other = 0;

    $tally_nid = _ogevents_get_nid_triptally($event_nid, $gid, $week);
    if ($tally_nid) {
      $tally_node = node_load($tally_nid);
      // assert tally_node is valid result (s/be, since t_nid is valid).
      $default_bike = $tally_node->field_biketrips[0]['value'];
      $default_walk = $tally_node->field_walktrips[0]['value'];
      $default_other = $tally_node->field_othertrips[0]['value'];
    }


    // classes needed for CSS grid trick
    $classes = array('ogevents-hack');
    $classes[] = ($week == 1) ? 'fieldset-header' : 'fieldset-body';
    $classes[] = ($week & 1) ? 'odd' : 'even';

    $form['trips']["week$week"] = array(
        '#type' => 'fieldset',
        '#title' => t('Week !week!or-month',
                      array('!week' => $week,
                            '!or-month' =>
                              $week == OGEVENTS_NUM_WEEKS ? ' or Month Total' : '',
                           )),
        '#attributes' => array('class' => implode(' ', $classes)),
        );


    //$form['trips']["week$week"]["name"] = array('#value' => "<div class=\"foo\">Week $week</div>");

    $form['trips']["week$week"]["walktrips$week"] = array(
        '#type' => 'textfield',
        '#title' => t('Walk'),
        '#default_value' => $default_walk,
        '#size' => 5,
        '#maxlength' => 3,
        '#attributes' => array('class' => 'ogevents-hack'),
        //'#required' => TRUE,
        );

    $form['trips']["week$week"]["biketrips$week"] = array(
        '#type' => 'textfield',
        '#title' => t('Bike'),
        '#default_value' => $default_bike,
        '#size' => 5,
        '#maxlength' => 3,
        '#attributes' => array('class' => 'ogevents-hack'),
        //'#required' => TRUE,
        );

    $form['trips']["week$week"]["othertrips$week"] = array(
        '#type' => 'textfield',
        '#title' => t('Other'),
        '#default_value' => $default_other,
        '#size' => 5,
        '#maxlength' => 3,
        '#attributes' => array('class' => 'ogevents-hack'),
        //'#required' => TRUE,
        );
  }

  $form['event'] = array('#type' => 'hidden', '#value' => $event_nid);
  $form['gid'] = array('#type' => 'hidden', '#value' => $gid);

  $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Update',
      );

  return $form;
}


/**
 * Submit tally for the month.
 */
function ogevents_tally_form_submit($form, &$form_state) {

  _dm('tally_form_submit...');
  _dpm($form_state);

  $event_nid = $form_state['values']['event'];
  $gid = $form_state['values']['gid'];

  // for each week, look for info and pass to tally-submit function
  foreach (range(1, OGEVENTS_NUM_WEEKS) as $week) {
    _ogevents_update_triptally($event_nid, $gid, $week,
      $form_state['values']["biketrips$week"],
      $form_state['values']["walktrips$week"],
      $form_state['values']["othertrips$week"]);
  }
}


function ogevents_participation_form($form_state, $event_nid, $school_nid) {
  $form = array();

  $reg_nid = _ogevents_get_registration($event_nid, $school_nid);
  $reg_node = node_load($reg_nid);
  _dm("reg_nid in populate_participation_form: $reg_nid"); //DEBUG
  _dm("reg_node in populate_participation_form: <pre>" . print_r($reg_node, TRUE) . "</pre>");

  $studentbody = $reg_node->field_studentbody[0]['value'];
  $participants = $reg_node->field_participants[0]['value'];
  $staff = $reg_node->field_staff[0]['value'];
  $parents = $reg_node->field_parents[0]['value'];

  if ($studentbody + $participants + $staff + $parents > 0) {
    $collapsed = TRUE; //because fields have already been filled out.
  }

  $form['participation'] = array(
      '#type' => 'fieldset',
      //'#title' => t('Participation'),
      //'#collapsible' => TRUE,
      //'#collapsed' => $collapsed,
      '#attributes' => array('class' => 'ogevents-participation'),
      );

  $form['participation']['participants'] = array(
      '#type' => 'textfield',
      '#title' => t('How many students participated (at least one trip) in the Challenge?'),
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
      '#title' => t('How many parents were involved?'),
      '#default_value' => $parents,
      '#size' => 30,
      '#maxlength' => 12,
      '#required' => TRUE,
      );

  $form['participation']['staff'] = array(
      '#type' => 'textfield',
      '#title' => t('How many teachers/staff were involved?'),
      '#default_value' => $staff,
      '#size' => 30,
      '#maxlength' => 12,
      '#required' => TRUE,
      );

  $form['regnode'] = array('#type' => 'hidden', '#value' => $reg_nid);
  $form['event'] = array('#type' => 'hidden', '#value' => $event_nid);
  $form['school'] = array('#type' => 'hidden', '#value' => $school_nid);

  $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Submit my Challenge results',
      );

  return $form;
}


/**
 * Submit month participation totals.
 */
function ogevents_participation_form_submit($form, &$form_state) {
  _dm("form_state: "); //DEBUG
  _dpm($form_state); //DEBUG

  $event_nid = $form_state['values']['event'];
  $school_nid = $form_state['values']['school'];
  $reg_nid = $form_state['values']['regnode']; // the registration for the previously given school+event
  $reg_node = node_load($reg_nid);

  $reg_node->field_participants[0]['value'] = $form_state['values']['participants'];
  $reg_node->field_parents[0]['value'] = $form_state['values']['parents'];
  $reg_node->field_staff[0]['value'] = $form_state['values']['staff'];
  $reg_node->field_studentbody[0]['value'] = $form_state['values']['studentbody'];

  node_save($reg_node);

  //DEBUG
  _dm("reg node after saving: ");
  _dpm($reg_node);

  $form_state['redirect'] = "event/$event_nid/thanks/$school_nid";
}
