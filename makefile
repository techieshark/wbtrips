

# return to the pre-update-6001 state (todo: make drupal not know we did the update. db restore? )
drop-cache:
	drush @wb.test eval '_ogevents_clear_cache()'
	echo 'DROP TABLE ogevents_results_cache' | drush @wb.test sqlc 

