

# return to the pre-update-6001 state (todo: make drupal not know we did the update. db restore? )
drop-results:
	drush @wb.test eval '_ogevents_clear_results()'
	echo 'DROP TABLE ogevents_results' | drush @wb.test sqlc

