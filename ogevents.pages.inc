<?php
/**
 * @file
 * User page callbacks for the ogevents module.
 */


/**
 * Page callback to create dashboard page (My Schools).
 * Displays user's schools and other useful info/links.
 * Basically, a first stop for logged in users.
 * Only logged in users see this page.
 */
function ogevents_page_dashboard() {
  global $user;
  $output = "";

  $events = _ogevents_get_open_events();

  if (count($events) == 0) {
    $output .= t('There are currently no events open for registration or tallying.  Please return later.');
  }

  foreach ($events as $event_nid) {
    // print event name as header
    $event_node = node_load($event_nid);
    if (count($events) > 1) {
      $output .= "<h2>My Schools for " . $event_node->title . "</h2>";
    } // otherwise no need for title.

    $registration_open = _ogevents_is_registration_open($event_node);
    $tallying_open = _ogevents_is_tallying_open($event_node);

    $schools = _ogevents_get_my_registered_schools($event_nid);
    if (empty($schools)) { // no registered school; allow registration if possible
      if ($registration_open) {
        // print 'Register for this' link.
        $output .= '<p>' . t('You have no registered schools yet.  You can <a href="@register-page">register a school</a>.',
                    array('@register-page' => url("event/$event_nid/register"))) . '</p>';
      }
      else {
        $output .= '<p>' . t('Registration is closed.') . '</p>';
      }
    }
    else { // have 1+ registered schools

      // build table header + output instructions
      $header = array('School');
      if ($tallying_open) {
        array_push($header, 'Weekly Walk Trips', 'Weekly Bike Trips', 'Weekly Other Trips', 'Participation');
        $output .= '<p>' . t('Click the name of a school to tally trips.') . '</p>';
      }
      else {
        $output .= '<p>'
                . t('Trip Tallying will be available on @tally-start.',
                    array('@tally-start' =>
                      format_date(
                        _ogevents_get_tally_start($event_nid),
                        'custom', 'F j, Y')
                    )
                  ) . '</p>';
      }


      $header[] = 'Final Results Submitted?';
      $header[] = 'School Profile';

      $star = '<span class="warning">*</span>';
      $rows = array();

      foreach ($schools as $school_nid) {
        $school_name = _ogevents_get_node_title($school_nid);
        $row = array();

        if ($tallying_open) {

          // get school challenge results for current event.
          $results = _ogevents_get_results($school_nid, $event_nid);
          if (($results['bike_trips'] + $results['walk_trips'] + $results['other_trips']) >
              ($results['final_bike_trips'] + $results['final_walk_trips'] + $results['final_other_trips'])) {
            $warning = TRUE;
            $end_warning = TRUE;
          }
          else {
            $warning = FALSE;
          }

          // link school name -> tally page
          $row[] = l($school_name, "event/$event_nid/tally/$school_nid",
                     array('attributes' => array('title' => "Tally trips for $school_name")))
                   . ($warning ? $star : '');

          array_push($row, $results['walk_trips'],
                           $results['bike_trips'],
                           $results['other_trips'],
                           round($results['student_percentage'],1) . '%');

          $reg_nid = _ogevents_get_registration($event_nid, $school_nid);
          $reg_node = node_load($reg_nid);
          if ($reg_node->field_results_submitted[0]['value'] == OGEVENTS_RESULTS_SUBMITTED) {
            array_push($row, t('Yes') . ($warning ? ' ' . $star : ''));
          }
          else {
            array_push($row, t('No'));
          }

        }
        else {
          // tallying is not open (yet) but registration must be,
          // so nothing to show but the school name and following link(s) (edit, etc).
          $row[] = $school_name;
        }

        // add edit link
        $row[] = l('edit', "node/$school_nid/edit",
                           array('query' => drupal_get_destination(),
                                 'attributes' => array('title' => 'Edit basic info like name and address.')));

        //if ($registration_open) {
          // add unregister link
          // $row[] = l('unregister', //TODO
       //}

        $rows[] = $row;
      }

      $output .= theme('table', $header, $rows);

      if ($registration_open) {
        $output .= '<p><b>' . l('Register another school', "event/$event_nid/register") . '</b></p>';
      }
    }
    $output .= "<br/>";

    if ($end_warning) {
      $output .= '<div class="warning"><span class="warning">*</span>This school\'s weekly log total is greater than the recorded final result.  You may want to update and resubmit your final results.</div>';
      //drupal_set_message("Warning: the total trips logged weekly at one or more schools are greater than their recorded final results.  (You may want to update and resubmit your final results.)");

    }
  }

  return $output;
}


/**
 * Returns array of nids of schools registered for the given event,
 * where the user is a member of those school OGs.
 */
function _ogevents_get_my_registered_schools($event_nid) {
  global $user;

  $all_registered_schools = _ogevents_get_registered_schools($event_nid);
  if (empty($user->og_groups) || empty($all_registered_schools)) {
    return array();
  } else {
    $registered_groups = array_intersect(array_keys($user->og_groups), $all_registered_schools);

    // Since og_groups may contain schools we registered for a previous event,
    // make sure the current event's registration is owned by us (otherwise it is
    // someone else's event).

    $my_registered_schools = array();
    foreach ($registered_groups as $group_id) {
      $reg_nid = _ogevents_get_registration($event_nid, $group_id);
      $registration = node_load($reg_nid);
      if ($registration->uid == $user->uid) {
        $my_registered_schools[] = $group_id;
      }
    }

    return $my_registered_schools;
  }

  // filter my og schools by event:
  // look for registrations owned by a given user uid.
  //filter_registered_by_user_for_event($my_schools, $event_nid, $user->uid);
  // This SQL does what this function does.  Consider using it instead.
  //SELECT oga.group_nid FROM og_ancestry oga JOIN content_type_registration r ON oga.nid = r.nid JOIN node n ON r.vid = n.vid JOIN content_field_eventid e ON e.vid = r.vid WHERE n.uid = %user AND e.field_eventid_nid = %event
}


/*
 * Return nids of events open for either registration or tallying.
 * TODO !! sort by tally start date, sooner -> later (in another funct.)
 */
function _ogevents_get_open_events() {
  return array_unique(array_merge(_ogevents_get_current_reg_events(), _ogevents_get_current_tally_events()));
}


/**
 * Page to show duplicate schools (name = name).
 */
function ogevents_page_duplicates() {
  $query = 'SELECT n.nid, n.title, '
         . 'l.street, l.city, l.province AS state, l.postal_code AS zip,'
         . 'n2.nid AS nid2, l2.street AS street2 , l2.city AS city2, l2.province AS state2, l2.postal_code AS zip2 '
         . 'FROM {node} n JOIN {node} n2 ON n.title = n2.title '
         . 'JOIN {location_instance} li ON n.vid = li.vid '
         . 'JOIN {location_instance} li2 ON n2.vid = li2.vid '
         . 'JOIN {location} l ON li.lid = l.lid '
         . 'JOIN {location} l2 ON li2.lid = l2.lid '
         . 'WHERE n.type = "school" AND n.nid <> n2.nid';
  $res = db_query($query);

  $header = array('nid', 'title', 'street', 'city', 'state', 'zip', 'nid2', 'street2', 'city2', 'state2', 'zip2');
  $samefields = array('street', 'city', 'state', 'zip');

  $inspected = array();
  $rows = array();

  while ($db_row = db_fetch_array($res)) {

    // if we've already seen this pair of nids, we don't want to reprint it.
    list($small_nid, $large_nid) = _ogevents_util_smallbig($db_row['nid'], $db_row['nid2']);
    if (isset($inspected[$small_nid]) && isset($inspected[$small_nid][$large_nid])) {
      continue;
    }
    else { // if not yet seen this pair, add to the set of inspected pairs
      $inspected[$small_nid][$large_nid] = TRUE;
    }

    $row = array();
    array_push($row, $db_row['nid'], $db_row['title']);

    foreach ($samefields as $f) {
      $row[] = "'" . $db_row[$f] . "'";
    }

    $row[] = $db_row['nid2'];

    foreach ($samefields as $f) {
      $first = $db_row[$f];
      $second = $db_row[$f . '2'];

      if ($second == $first) {
        $row[] = "[same]"; // make similarities clear
      }
      else {
        $row[] = "'" . $second . "'";
      }
    }

    $rows[] = $row;
  }

  return theme('table', $header, $rows);
}


/**
 *  Returns an array with $a and $b sorted small then large.
 */
function _ogevents_util_smallbig($a, $b) {
  return $a < $b ? array($a,$b) : array($b,$a);
}


/**
 * Output a page with results for all schools in the specified event in a table.
 */
function ogevents_page_results($event_nid) {
  $output = '<p>' . 'You may ' . l('download these results', "event/$event_nid/results/csv") . '.' . '</p>';

  _ogevents_debug("Event: $event_nid"); //DEBUG

  $header = _ogevents_get_challenge_results_header(FALSE);
  $rows = _ogevents_get_challenge_results_data($event_nid, TRUE, FALSE);

  $output .= theme('table', $header, $rows);
  return $output;
}

function ogevents_page_results_csv($event_nid) {
  // set headers
  header('Content-Type: text/csv');
  header('Content-Disposition: attachment; filename="results-' . $event_nid . '.csv"');

  // print CSV
  $csv = implode(",", _ogevents_get_challenge_results_header(TRUE)) . "\n";
  $rows = _ogevents_get_challenge_results_data($event_nid, FALSE, TRUE);
  foreach ($rows as $row) {
    $line = implode(",", $row);
    $csv .= $line . "\n";
  }
  print($csv);

  exit(); //don't let Drupal do anything else.
}

/**
 * @param csv - if for CSV output; includes more columns if user has permission
 */
function _ogevents_get_challenge_results_header($csv = FALSE) {
  // Note - everything before 'Bike' is the same as in the
  // registered school export. consider consolidating.
  $columns = array(
    // header field => array (is_access_controlled, show on page, show on csv)
    'School'               => array(FALSE, TRUE,  TRUE),
    'Address'              => array(FALSE, FALSE, TRUE),
    'City'                 => array(FALSE, TRUE,  TRUE),
    'State'                => array(FALSE, FALSE, TRUE),
    'Zip'                  => array(FALSE, FALSE, TRUE),
    'Coordinator'          => array(FALSE, TRUE,  TRUE),
    'Coordinator Email'    => array(TRUE,  FALSE, TRUE),
    'Coordinator Phone'    => array(TRUE,  FALSE, TRUE),
    'Walk'                 => array(FALSE, TRUE,  TRUE),
    'Bike'                 => array(FALSE, TRUE,  TRUE),
    'Other'                => array(FALSE, TRUE,  TRUE),
    'Total'                => array(FALSE, TRUE,  TRUE),
    'Final Walk'           => array(FALSE, FALSE,  TRUE),
    'Final Bike'           => array(FALSE, FALSE,  TRUE),
    'Final Other'          => array(FALSE, FALSE,  TRUE),
    'Final Total'          => array(FALSE, FALSE,  TRUE),
    'Student Participants' => array(FALSE, FALSE, TRUE),
    'Student Body'         => array(FALSE, FALSE, TRUE),
    'Student Percentage'   => array(FALSE, FALSE, TRUE),
    'Students'             => array(FALSE, TRUE,  FALSE),
    'Staff'                => array(FALSE, TRUE,  TRUE),
    'Parents'              => array(FALSE, TRUE,  TRUE),
    'Last Tally Activity'  => array(FALSE, FALSE, TRUE),
  );

  $header = array();
  foreach (array_keys($columns) as $field) {
    $access_controlled = $columns[$field][0];
    $on_page = $columns[$field][1];
    $on_csv = $columns[$field][2];

    if ( (!$access_controlled) || ($access_controlled && user_access('access ogevents reports')) ) {
      if (($on_csv && $csv) || ($on_page && !$csv)) {
        array_push($header, $field);
      }
    }
  }

  return $header;
}


/**
 * Return array of rows, each row containing results for one school in the given event.
 * @param coordinator_as_link TRUE if the coordinator name should be a link to their contact form
 * @param csv - if data is intended for CSV output, in which case extra fields are included (if user has permission).
 */
function _ogevents_get_challenge_results_data($event_nid, $coordinator_as_link, $csv = FALSE) {
  $rows = array();

  // foreach school, add results to table..
  foreach (_ogevents_get_registered_schools($event_nid) as $school_nid) {
    $row = array();

    $school_node = node_load($school_nid);

    $results = _ogevents_get_results($school_nid, $event_nid);

    $bike = $results['bike_trips'];
    $walk = $results['walk_trips'];
    $other = $results['other_trips'];
    $total = $bike + $walk + $other;

    $final_bike = $results['final_bike_trips'];
    $final_walk = $results['final_walk_trips'];
    $final_other = $results['final_other_trips'];
    $final_total = $final_bike + $final_walk + $final_other;


    // TODO - idea - make a filter function to keep certain keys out.
    // Instead of the following somewhat complicated/ugly if/else code (could use similar code for registered schools csv)
    array_push($row, $school_node->title);

    if ($csv) {
      array_push($row, $school_node->locations[0]['street']);
    }

    array_push($row,$school_node->locations[0]['city']);

    if ($csv) {
      array_push($row, $school_node->locations[0]['province'],
                       $school_node->locations[0]['postal_code']);
    }

    array_push($row, _ogevents_get_school_coordinator($school_nid, $event_nid, $coordinator_as_link));

    if ($csv && user_access('access ogevents reports')) {
      $coordinator_uid = _ogevents_get_school_coordinator_uid($school_nid, $event_nid);
      $coordinator = user_load($coordinator_uid);
      profile_load_profile($coordinator);
      array_push($row, $coordinator->mail, $coordinator->profile_phone);
    }

    array_push($row, $walk, $bike, $other, $total);

    if ($csv) {
      array_push($row, $final_walk, $final_bike, $final_other, $final_total);
      array_push($row, $results['participants'],
                       $results['studentbody'],
                       $results['student_percentage']);
    }
    else { //human friendly format for web only
      $students = sprintf("%d of %d (%s%%)",
                        $results['participants'],
                        $results['studentbody'],
                        round($results['student_percentage'],1));
      array_push($row, $students);
    }

    array_push($row, $results['staff'], $results['parents']);

    if ($csv) { // date of last tally activity
      $timestamp = _ogevents_get_latest_tally_timestamp($school_nid, $event_nid);
      array_push($row, $timestamp ? format_date($timestamp, 'custom', 'Y-m-d H:i:s') : '');
    }

    $rows[] = $row;
  }
  return $rows;
}


/**
 * Get a unix timestamp for the most recently updated tally or registration
 * for a given school and event.
 * Returns false if no tally data (trips, participation) has been recorded.
 */
function _ogevents_get_latest_tally_timestamp($school_nid, $event_nid) {
  $timestamp = 0;

  $reg_nid = _ogevents_get_registration($event_nid, $school_nid);
  if (!$reg_nid) {
    return FALSE; // no registration, so no tallies either
  }

  $reg_node = node_load($reg_nid);
  if ($reg_node->nid) {
    $staff = $reg_node->field_staff[0]['value'];
    $parents = $reg_node->field_parents[0]['value'];
    $participants = $reg_node->field_participants[0]['value'];
    $studentbody = $reg_node->field_studentbody[0]['value'];
  }

  // consider participation info as tallying activity, use for timestamp
  if ($staff + $parents + $participants + $studentbody > 0) {
    $timestamp = $reg_node->changed;
  }

  // pick out the latest timestamp from the tallies:
  $query = "SELECT n.changed FROM {node} n
  JOIN {content_type_triptally} t ON n.vid = t.vid
  JOIN {og_ancestry} oga ON t.nid = oga.nid
  JOIN {content_field_eventid} e ON t.vid = e.vid
  WHERE oga.group_nid = %d AND e.field_eventid_nid = %d
  ORDER BY n.changed DESC LIMIT 1";
  $tally_time = db_result(db_query($query, $school_nid, $event_nid));

  if ($tally_time > $timestamp) {
    $timestamp = $tally_time;
  }

  return $timestamp;
}


/**
 * Page to register a school.
 * @param $event_nid the event to register for
 */
function ogevents_page_register($event_nid) {
  global $user;

  /* Print header.
   * If ! logged in, suggest logging in.  Print bit about registering sch.
   * Else, print rest of message.
   * Print dropdown.
   * Print currently registered schools list.
   */

  if (! $user->uid) { // anonymous user
    $output .= '<p>If your school isn’t listed below, '
            .  l('log in', 'user/login', array('query' => drupal_get_destination()))
            .  ' or '
            .  l('create an account', '/user/register', array('query' => drupal_get_destination()))
            .  ' to register your school.</p>';
  }
  else { // logged in
    $output .= '<p><strong>Please read these important instructions before you register!</strong></p>';
    $output .= '<p><ol><li>';
    $output .= 'Check if your school already appears on the list of registered schools below. ';
    $output .= 'If it has, click the coordinator\'s name to contact them through this site and learn how you can help.';
    $output .= '</li><li>';
    $output .= 'If your school has participated in the past but is not yet registered for Challenge 2011, it will appear in the drop-down menu. ';
    $output .= 'Select your school and click the Register button to register and identify yourself as coordinator.';
    $output .= '</li><li>';
    $output .= 'If you don’t see your school in either the drop-down menu or the list below it, you’ll need to '
            .  l('add and register your school', 'node/add/school', array('query' => "event=$event_nid")) . '.';
    $output .= '</li></ol></p>';

    $output .= '<h2>Unregistered schools</h2>';
    $output .= drupal_get_form('ogevents_register_form', $event_nid);
  }

  // from registration-form theme file...
  $output .= '<h2 style="margin-top:20px;">Currently Registered Schools</h2>';

  // for BTA Admins, insert a 'Download Registered Schools List' link.
  if (user_access('access ogevents reports')) {
    $output .= l('Download Registered Schools List', "event/$event_nid/register/csv");
  }

  $header = array('School', 'City', 'Coordinator');
  $rows = array();

  $school_nids = _ogevents_get_registered_schools($event_nid);
  foreach ($school_nids as $school_nid) {
    $school = node_load($school_nid);
    $rows[] = array($school->title, $school->locations[0]['city'],
                    _ogevents_get_school_coordinator($school_nid, $event_nid, TRUE));
  }

  _ogevents_debug($rows); //DEBUG

  $output .= theme('table', $header, $rows);

  return $output;
}


/**
 * Build a form to register a school for an event.
 * Form contains a single dropdown of unregistered schools.
 */
function ogevents_register_form($form_state, $event_nid) {
  // get nids of schools registered for $event_nid
  $regd_array = _ogevents_get_registered_schools($event_nid);

  $query = "SELECT node.nid AS nid
    FROM {node}
    LEFT JOIN {og} ON node.nid = og.nid
    WHERE node.type = 'school' AND node.status <> 0 AND og.og_directory <> 0
    ORDER BY node.title ASC";
  $result = db_query($query);
  $allgroups = array(); // will be all schools published and listed in the directory
  while ( $obj = db_fetch_object ($result) ) {
    $allgroups[] = $obj->nid;
  }

  $nids = array_diff($allgroups, $regd_array); // array of unregistered schools
  $titles = array();
  foreach ($nids as $nid) {
    $node = node_load($nid);
    $titles[] = t($node->title . " - " . $node->locations[0]['city']) ;
  }

  // build array like (nid -> "<school name> - <city>")
  $first_option = array(-1 => "Select a School...");
  $normal_options = array_combine($nids, $titles);
  $options = $first_option + $normal_options;

  $form['og_register'] = array(
      '#type' => 'select',
      '#title' => t('Select a School'),
      '#multiple' => FALSE,
      '#options' => $options,
      '#required' => TRUE,
      );
  $form['event'] = array('#type' => 'hidden', '#value' => $event_nid);

  $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Register',
      );

  $form['register_link'] = array(
      '#value' => ' &nbsp; or &nbsp; ' . l('My school doesn\'t appear in the menu or the list below.', 'node/add/school', array('query' => "event=$event_nid")),
      );

  return $form;
}

/**
 * Form validate: registering a school
 * Ensures user didn't forget to select school.
 */
function ogevents_register_form_validate($form, &$form_state) {
  $school_picked = $form_state['values']['og_register'];
  if ($school_picked == -1) {
    form_set_error('og_register', t('No school was selected.'));
  }
}


/**
 * Form submit function to register a school.
 *
 * A new 'registration' node is saved which records the event
 * we're signing up for and connects (via og_ancestry table) to
 * the school which had been selected.
 */
function ogevents_register_form_submit($form, &$form_state) {
  $newNode = _ogevents_get_blank_regnode();
  $newNode->field_eventid[0]['nid'] = $eid = $form_state['values']['event']; //event nid to signup for

  $gid = $form_state['values']['og_register']; // nid of selected school being registered

  node_save($newNode);
  $sql = "INSERT INTO {og_ancestry} (nid, group_nid) VALUES (%d, %d)";
  db_query($sql, $newNode->nid, $gid);

  // clear form storage, to allow form to submit
  $form_state['storage'] = array();

  og_save_subscription($gid, $GLOBALS["user"]->uid, array('is_active' => 1));

  drupal_set_message(_ogevents_get_post_registration_msg($eid));

  $form_state['redirect'] = 'dashboard';
}


/**
 * Cause a CSV file to be sent to the browser, containing registered schools.
 */
function ogevents_page_registered_csv($event_nid) {
  // set headers
  header('Content-Type: text/csv');
  header('Content-Disposition: attachment; filename="registrations-' . $event_nid . '.csv"');

  // print CSV
  $csv = implode(",", _ogevents_get_registered_schools_header()) . "\n";
  $rows = _ogevents_get_registered_schools_rows($event_nid);
  foreach ($rows as $row) {
    $line = implode(",", $row);
    $csv .= $line . "\n";
  }
  print($csv);

  exit(); //don't let Drupal do anything else.
}

/**
 * Return header for the CSV file.
 */
function _ogevents_get_registered_schools_header() {
  return array('School', 'Address', 'City', 'State', 'Zip', 'Coordinator', 'Coordinator Email', 'Coordinator Phone', 'Registered on');
  //todo ARG. we need row for 'fully entered info'.
}

function _ogevents_get_registered_schools_rows($event_nid) {
  $rows = array();

  $school_nids = _ogevents_get_registered_schools($event_nid);
  foreach ($school_nids as $school_nid) {
    $school = node_load($school_nid);
    $reg_nid = _ogevents_get_registration($event_nid, $school_nid);
    $registration = node_load($reg_nid);
    $coordinator_uid = _ogevents_get_school_coordinator_uid($school_nid, $event_nid);
    $coordinator = user_load($coordinator_uid);
    profile_load_profile($coordinator);

    $rows[] = array($school->title,
                    $school->locations[0]['street'], $school->locations[0]['city'],
                    $school->locations[0]['province'], $school->locations[0]['postal_code'],
                    _ogevents_get_school_coordinator($school_nid, $event_nid, FALSE),
                    $coordinator->mail,
                    $coordinator->profile_phone,
                    '"' . format_date($registration->created, 'custom', 'Y-m-d H:i:s') . '"',
                    );
  }
  return $rows;
}


