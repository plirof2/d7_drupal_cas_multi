<?php

/**
 * Batch process for registering CAS users.
 */
function _cas_multi_batch_user_add($cas_name, $cas_server_config, &$context) {
  if (_cas_multi_username_is_taken($cas_name)) {
    $context['results']['messages']['already_exist'][] = check_plain($cas_name);
  }
  else {
    $account = _cas_multi_register_account($cas_name, NULL, $cas_server_config, TRUE);
    if (!$account) {
      $context['results']['messages']['error'][] = check_plain($cas_name);
    }
    else {
      $context['results']['messages']['newly_created'][] = check_plain($cas_name);
    }
  }
}

/**
 * The "finished" callback for CAS user registration batch process.
 */
function _cas_multi_batch_user_finished($success, $results, $operations) {
  if ($success) {
    if (!empty($results['messages']['error'])) {
      drupal_set_message(t('%count CAS user(s) not created due to errors: !usernames', array(
        '%count' => count($results['messages']['error']),
        '!usernames' => implode(', ', $results['messages']['error']),
      )), 'warning');
    }
    if (!empty($results['messages']['already_exist'])) {
      drupal_set_message(t('%count username(s) already in use: !usernames', array(
        '%count' => count($results['messages']['already_exist']),
        '!usernames' => implode(', ', $results['messages']['already_exist']),
      )), 'warning');
    }
    if (!empty($results['messages']['newly_created'])) {
      drupal_set_message(t('%count CAS user(s) created: !usernames', array(
        '%count' => count($results['messages']['newly_created']),
        '!usernames' => implode(', ', $results['messages']['newly_created']),
      )));
    }
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    drupal_set_message(t('An error occurred while processing @operation with arguments : @args', array(
      '@operation' => $error_operation[0],
      '@args' => print_r($error_operation[0], TRUE),
    )));
  }
}
