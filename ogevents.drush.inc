<?php

/**
 * @file Provides drush commands
 * Much thanks to http://codekarate.com/content/drush-integration-your-modules
 */


define('OGEVENTS_DEFAULT_WEEKS', 4);


/**
 * Implements hook_drush_help().
 */
function ogevents_drush_help($command) {
  switch ($command) {
    case 'drush:generate-wbc-event':
      return dt('Create a Walk & Bike event, open for a year from today');
    case 'drush:ogevents-node-print':
      return dt('Debug - print a node');
    case 'drush:ogevents-node-delete':
      return dt('Debug - delete a node');
  }
}

/**
 * Implements hook_drush_command().
 */
function ogevents_drush_command() {
  $items = array();
  $items['generate-wbc-event'] = array(
      'description' => dt('Create a Walk + Bike Event.'),
      'arguments'   => array(
        'weeks'    => dt('(optional) number of weeks in the tally form'),
        'title'    => dt('(optional) event title'),
        ), 
      'examples' => array(
        'drush genw' => 'Create ' . OGEVENTS_DEFAULT_WEEKS . ' week event with generic title',
        'drush genw 8' => 'Create an event with a tallying form showing eight weeks',
        'drush genw 4 "Great Challenge"' => 'Create ' . OGEVENTS_DEFAULT_WEEKS .  
                                            ' week event called "Great Challenge"',
        ),
      'aliases' => array('genw'),
      );
  $items['ogevents-node-print'] = array(
      'description' => dt('Prints a given node.'),
      'arguments'   => array(
       // idea: if no nid given, print last added node. TODO
        'nid'    => dt('Node ID (required)'),
        ), 
      'examples' => array(
        'drush nidp 42' => 'Print node 42',
        ),
      'aliases' => array('nidp'),
      );
  $items['ogevents-node-delete'] = array(
      'description' => dt('Deletes a given node.'),
      'arguments'   => array(
        'nid'    => dt('Node ID (required)'),
        ), 
      'examples' => array(
        'drush nidd 42' => 'Delete node 42',
        ),
      'aliases' => array('nidd'),
      );
  return $items;
}



/**
 * Callback function for drush generate-wbc-event. 
 * Callback is called by using drush_hook_command() where
 * hook is the name of the module (ogevents) and command is the name of
 * the Drush command with all "-" characters converted to "_" characters 
 * 
 * @param weeks 
 *              The number of weeks in the event
 * @param title 
 *              The event name
 */
function drush_ogevents_generate_wbc_event($weeks = OGEVENTS_DEFAULT_WEEKS, $title = NULL) {
  //check if the argument was passed in and just print it out
  if (isset($weeks)) {
    drush_print('Creating event with ' . $weeks . ' week tally form.');
  }

  if ($weeks < 1) {
    drush_set_error('Event cannot run for less than one week.');
    return;
  }

  // Make a new event.
  $event = _ogevents_new_event($weeks, $title);
  node_save($event);
  if (! $event->nid) {
    drush_set_error('Failed to save event node.');
    return;
  }

  //log to the command line with an OK status
  drush_log('Running generate-wbc-event. Saved node ' . $event->nid, 'ok');
}

/**
 * Callback function for drush ogevents_node_print
 * Callback is called by using drush_hook_command() where
 * hook is the name of the module (ogevents) and command is the name of
 * the Drush command with all "-" characters converted to "_" characters 
 * 
 * @param nid 
 *              The nid of the node to print 
 */
function drush_ogevents_node_print($nid = NULL) {
  //check if the argument was passed in and just print it out
  if (!isset($nid)) {
    drush_set_error('Nid argument required');
  }

  $node = node_load($nid);
  if (! $node->nid) {
    drush_set_error('Failed to load node ' . $nid);
    return;
  } 
  else {
    drush_print(print_r($node, TRUE));
  }

  //log to the command line with an OK status
  drush_log('Running ogevents-node-print', 'ok');
}


/**
 * Callback function for drush ogevents_nodedel
 * Callback is called by using drush_hook_command() where
 * hook is the name of the module (ogevents) and command is the name of
 * the Drush command with all "-" characters converted to "_" characters 
 * 
 * @param nid 
 *              The nid of the node to delete 
 */
function drush_ogevents_node_delete($nid = NULL) {
  //check if the argument was passed in and just print it out
  if (!isset($nid)) {
    drush_set_error('Nid argument required');
  }

  $node = node_load($nid);
  if (! $node->nid) {
    drush_set_error('Failed to load node ' . $nid);
    return;
  } 
  node_delete($node->nid);

  //log to the command line with an OK status
  drush_log('Running ogevents-node-delete', 'ok');
}

/**
 * Returns a new event open for a year.
 * @param nweeks how many weeks in the event
 * @param title the event name
 */
function _ogevents_new_event($nweeks, $title) {
  $newNode = (object) NULL;
  $newNode->type = 'event';
  if (isset($title)) {
    $newNode->title = $title;
  } 
  else {
    $newNode->title = $nweeks . ' week event'; 
  }
  $newNode->uid = $GLOBALS['user']->uid;
  $newNode->created = strtotime("now");
  $newNode->changed = strtotime("now");
  $newNode->status = 1;
  $newNode->comment = 0;
  $newNode->promote = 0;
  $newNode->moderate = 0;
  $newNode->sticky = 0;

  $newNode->field_nweeks[0]['value'] = $nweeks;

  // set start date for registration to today.
  $now = _ogevents_getvalue(getdate(), 0);
  $newNode->field_tallydates[0]['value'] = $now; 
  $newNode->field_regdates[0]['value'] = $now;

  // set end dates to a year from today.
  $nextyear = $now + (365 * 24 * 60 * 60);
  $newNode->field_tallydates[0]['value2'] = $nextyear;
  $newNode->field_regdates[0]['value2'] = $nextyear;

  return $newNode;
}

function _ogevents_getvalue($array, $key) {
  return $array[$key];
}

/*
drush_log('Log an event using drush', 'warning');
drush_set_error('Set an error with drush.');
dt('Translate strings with drush');
drush_print('Print to command line with drush');
drush_print_table($rows, TRUE); //print a command line table with drush
drush_confirm('Are you sure you want to continue?', $indent = 0); //Add drush confirmation
*/
