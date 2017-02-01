<?php

/**
 * Displays all CAS usernames for a user and a form for adding more.
 */
function _cas_multi_user_identities($account) {
  drupal_set_title(check_plain(format_username($account)));

  $header = array(t('CAS username'), t('CAS Server'), t('Operations'));
  $rows = array();

  // Load all CAS names for this user.
  $cas_names = db_query(
    'SELECT * FROM {cas_multi_user} WHERE uid = :uid',
    array('uid' => $account->uid)
  )->fetchAll();

  if (!empty($cas_names)) {
    ctools_include('export');
    $cas_servers = ctools_export_crud_load_all('cas_multi_server');

    foreach ($cas_names as $data) {
      $server_label = (array_key_exists($data->server_name, $cas_servers)) ? $cas_servers[$data->server_name]->label : 'Unknown';
      $rows[] = array(
        check_plain($data->cas_name),
        check_plain($server_label),
        l(t('Delete'), 'user/' . $account->uid . '/cas/delete/' . $data->aid),
      );
    }
  }

  $build['cas_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
  );
  $build['description'] = array(
    '#markup' => '<p>Add CAS username association for this user:</p>',
  );
  $build['cas_user_add'] = drupal_get_form('_cas_multi_user_add_cas_name_form', $account);

  return $build;
}

/**
 * Form for adding a CAS username to a user account.
 */
function _cas_multi_user_add_cas_name_form($form, &$form_state, $account) {
  ctools_include('export');

  $form['cas_name'] = array(
    '#type' => 'textfield',
    '#title' => t('CAS username'),
    '#required' => TRUE,
  );

  $options = array();
  $cas_server_configs = ctools_export_crud_load_all('cas_multi_server');
  foreach ($cas_server_configs as $server_config) {
    $options[$server_config->name] = check_plain($server_config->label);
  }

  $form['cas_server_config_name'] = array(
    '#type' => 'select',
    '#title' => t('CAS Server Config'),
    '#description' => t('What server configuraton is CAS username associated with?'),
    '#options' => $options,
    '#required' => TRUE,
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add'),
  );

  $form['#validate'][] = '_cas_multi_user_add_cas_name_validate';
  $form['#submit'][] = '_cas_multi_user_add_cas_name_submit';

  return $form;
}

/**
 * Validation callback for adding a CAS username to an account.
 */
function _cas_multi_user_add_cas_name_validate($form, &$form_state) {
  $existing_assignment = db_query(
    'SELECT aid FROM {cas_multi_user} cu WHERE cas_name = :cas_name AND server_name = :server_name',
    array(
      'cas_name' => $form_state['values']['cas_name'],
      'server_name' => $form_state['values']['cas_server_config_name'],
    )
  );

  if ($existing_assignment->fetchField()) {
    form_set_error('cas_name', t('This CAS username is already asssigned to another user.'));
  }
}

/**
 * Submit callback for adding a CAS username to an account.
 */
function _cas_multi_user_add_cas_name_submit($form, &$form_state) {
  $account = $form_state['build_info']['args'][0];
  $server_name = $form_state['values']['cas_server_config_name'];
  $cas_name = $form_state['values']['cas_name'];

  $cas_user = array(
    'uid' => $account->uid,
    'server_name' => $server_name,
    'cas_name' => $cas_name,
  );
  drupal_write_record('cas_multi_user', $cas_user);

  drupal_set_message(t('CAS username %cas_name has been associated with user %user.', array('%cas_name' => $cas_name, '%user' => $account->name)));
}

/**
 * Menu callback; Delete the specified CAS username from the system.
 */
function _cas_multi_user_delete_cas_name_form($form, $form_state, $account, $aid) {
  $form['#submit'][] = '_cas_multi_user_delete_cas_name_form_submit';

  $cas_name = db_query('SELECT cas_name FROM {cas_multi_user} cu WHERE cu.aid = :aid', array('aid' => $aid))->fetchField();

  return confirm_form($form, t('Are you sure you want to delete the CAS username %cas_name for %user?', array('%cas_name' => $cas_name, '%user' => $account->name)), 'user/' . $account->uid . '/cas');
}

/**
 * Delete callback for removing a CAS username from an account.
 */
function _cas_multi_user_delete_cas_name_form_submit($form, &$form_state) {
  $query = db_delete('cas_multi_user')
    ->condition('aid', $form_state['build_info']['args'][1])
    ->execute();
  if ($query) {
    drupal_set_message(t('CAS username deleted.'));
  }
  $form_state['redirect'] = 'user/' . $form_state['build_info']['args'][0]->uid . '/cas';
}

/**
 * Page callback for displaying the /user and /user/login pages.
 *
 * We override the standard one that Drupal provides (see "user_page" function)
 * so that we can display CAS login links in addition to the standard login
 * form.
 *
 * Most of this function is copied from the user module's "user_page" function.
 */
function _cas_multi_login_page_override() {
  global $user;
  ctools_include('export');

  if ($user->uid) {
    menu_set_active_item('user/' . $user->uid);
    return menu_execute_active_handler(NULL, FALSE);
  }
  else {
    $server_configs = ctools_export_crud_load_all('cas_multi_server');

    // Eliminate any configs that are not enabled or have the login link
    // disabled.
    $enabled_configs = array();
    foreach ($server_configs as $server_config) {
      if (!isset($server_config->disabled) && $server_config->login_link_enabled) {
        $enabled_configs[] = $server_config;
      }
    }

    // Sort the server configurations by their weight.
    usort($enabled_configs, function ($a, $b) {
      return strcmp($a->weight, $b->weight);
    });

    if (!empty($enabled_configs)) {
      return array(
        '#theme' => 'cas_multi_login_page',
        '#standard_login_form' => drupal_get_form('user_login'),
        '#cas_server_configs' => $enabled_configs,
      );
    }
    else {
      return drupal_get_form('user_login');
    }
  }
}
