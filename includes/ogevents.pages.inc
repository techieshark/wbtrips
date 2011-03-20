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

  // Welcome, Joe User.

  /*
  $name = $user->name;
  profile_load_profile($user);
  if (!empty($user->profile_fullname)) {
    $name = $user->profile_fullname;
  }
  $output .= '<p>' . "Welcome, $name.";
  */

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
        array_push($header, 'Walk Trips', 'Bike Trips', 'Other Trips', 'Participation');
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


      $header[] = 'School Profile';
      $rows = array();

      foreach ($schools as $school_nid) {
        $school_name = _get_node_title($school_nid);
        $row = array();

        if ($tallying_open) {
          // link school name -> tally page
          $row[] = l($school_name, "event/$event_nid/tally/$school_nid",
                     array('attributes' => array('title' => "Tally trips for $school_name")));

          // get school challenge results for current event.
          $results = _ogevents_get_results($school_nid, $event_nid);
          array_push($row, $results['walk_trips'],
                           $results['bike_trips'],
                           $results['other_trips'],
                           _ogevents_percentify($results['student_percentage']) . '%');
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
        $output .= '<p>' . l('Register another school.', "event/$event_nid/register") . '</p>';
      }
    }
    $output .= "<br/>";
  }

  return $output;
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
 * Output a page with results for all schools in the specified event in a table.
 */
function ogevents_page_results($event_nid) {
  $output = '<p>' . 'You may ' . l('download these results', "event/$event_nid/results/csv") . '.' . '</p>';

  _dm("Event: $event_nid"); //DEBUG

  $header = _ogevents_get_challenge_results_header(FALSE);
  $rows = _ogevents_get_challenge_results_data($event_nid, TRUE, FALSE);

  $output .= theme('table', $header, $rows);
  return $output;
}

function _ogevents_get_results_page_title($event_nid) {
  if (count(_get_current_reg_events()) > 1) {
    return t('Results for @event', array('@event' => _get_node_title($event_nid)));
  }
  else { // Keep things simple if only one event running
    return t('See Challenge results');
  }
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
      array_push($row, $results['participants'],
                       $results['studentbody'],
                       $results['student_percentage']);
    }
    else { //human friendly format for web only
      $students = sprintf("%d of %d (%s%%)",
                        $results['participants'],
                        $results['studentbody'],
                        _ogevents_percentify($results['student_percentage']));
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

  _dpm($rows); //DEBUG

  $output .= theme('table', $header, $rows);

  return $output;
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
