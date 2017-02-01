<?php

/**
 * Create settings form.
 */
function _cas_multi_settings_form() {
  ctools_include('export');

  $default_server_config_options = array();
  $cas_server_configs = ctools_export_crud_load_all('cas_multi_server');
  foreach ($cas_server_configs as $server_config) {
    $default_server_config_options[$server_config->name] = check_plain($server_config->label) . ' (/cas/' . check_plain($server_config->name) . ')';
  }

  $form['library'] = array(
    '#type' => 'fieldset',
    '#title' => t('Library (phpCAS)'),
    '#collapsible' => TRUE,
  );
  if (module_exists('libraries')) {
    // If Libraries API is enabled, print an information item.
    $form['library']['cas_multi_library_dir'] = array(
      '#type' => 'item',
      '#title' => t('Library directory'),
      '#value' => t('Using <a href="@url">Libraries API</a>.', array('@url' => 'http://drupal.org/project/libraries')),
      '#description' => t('Please ensure phpCAS is installed in a location compatible with Libraries API. For example, install phpCAS so that <em>sites/all/libraries/CAS/CAS.php</em> exists. See README.txt for more information.'),
      '#after_build' => array('_cas_multi_library_version_check'),
    );
  }
  else {
    // If Libraries API is not installed, display path settings.
    $form['library']['cas_multi_library_dir'] = array(
      '#type' => 'textfield',
      '#title' => t('Library directory'),
      '#default_value' => variable_get('cas_multi_library_dir', 'CAS'),
      '#description' => t('Specify the path to the directory the CAS.php file resides in. Leave blank to load CAS from your phpinclude path.'),
      '#after_build' => array('_cas_multi_library_version_check'),
    );
  }

  $form['cas_multi_default_server_config_name'] = array(
    '#type' => 'select',
    '#title' => t('Default CAS server configuration'),
    '#options' => $default_server_config_options,
    '#default_value' => variable_get('cas_multi_default_server_config_name'),
    '#description' => t('The CAS server configuration to use when a visitor goes to /cas instead of /cas/[cas_server_machine_name].'),
    '#empty_option' => t('None, return a 404 page for /cas)'),
  );

  $form['cas_multi_forgot_password_redirect'] = array(
    '#type' => 'textfield',
    '#title' => t('Forgot Password Form Redirect'),
    '#description' => t('Enter a local path or absolute URL to redirect users to who visit the "forgot password" form. Leave blank to disable.'),
    '#default_value' => variable_get('cas_multi_forgot_password_redirect', ''),
  );
  $form['cas_multi_user_registration_redirect'] = array(
    '#type' => 'textfield',
    '#title' => t('Registration Form Redirect'),
    '#description' => t('Enter a local path or absolute URL to redirect users to who visit the user registration form. Leave blank to disable.'),
    '#default_value' => variable_get('cas_multi_user_registration_redirect', ''),
  );

  return system_settings_form($form);
}

/**
 * Create form for bulk registering CAS users.
 */
function _cas_multi_bulk_register_users_form() {
  ctools_include('export');
  $form = array();

  $options = array();
  $cas_server_configs = ctools_export_crud_load_all('cas_multi_server');
  foreach ($cas_server_configs as $server_config) {
    $options[$server_config->name] = check_plain($server_config->label);
  }

  $form['info'] = array(
    '#markup' => '<p>Use the form below to bulk register users and associate them with the specified CAS server. Note that registering adding users this way, any data from your CAS server that you may rely on (like attributes) will not be available.</p>',
  );

  $form['cas_names'] = array(
    '#type' => 'textarea',
    '#title' => t('CAS username(s)'),
    '#required' => TRUE,
    '#default_value' => '',
    '#description' => t('Enter one CAS username per line. A new Drupal account will be created using this username.'),
  );
  $form['cas_server_config_name'] = array(
    '#type' => 'select',
    '#title' => t('CAS Server Config'),
    '#description' => t('What server configuraton are these usernames associated with?'),
    '#options' => $options,
    '#required' => TRUE,
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create new account(s)'),
  );

  $form['#submit'][] = '_cas_multi_bulk_register_users_form_submit';

  return $form;
}

/**
 * Submit handler for _cas_multi_bulk_register_users_form.
 */
function _cas_multi_bulk_register_users_form_submit($form, &$form_state) {
  $cas_names = preg_split('/[\n\r|\r|\n]+/', $form_state['values']['cas_names']);

  $cas_server_config = ctools_export_crud_load('cas_multi_server', $form_state['values']['cas_server_config_name']);

  $operations = array();
  foreach ($cas_names as $cas_name) {
    $cas_name = trim($cas_name);
    if (empty($cas_name)) {
      continue;
    }
    $operations[] = array(
      '_cas_multi_batch_user_add',
      array($cas_name, $cas_server_config),
    );
  }

  $batch = array(
    'title' => t('Creating users...'),
    'operations' => $operations,
    'finished' => '_cas_multi_batch_user_finished',
    'progress_message' => t('Processed @current out of @total.'),
    'file' => drupal_get_path('module', 'cas_multi') . '/includes/cas_multi.batch.inc',
    'progressive' => FALSE,
  );

  batch_set($batch);
}

/**
 * After-build callback to append info about the loading of the phpCAS library.
 */
function _cas_multi_library_version_check($element, &$form_state) {
  $path = module_exists('libraries') ? NULL : $element['#value'];
  // Suppress errors if phpCAS cannot be loaded.
  if ($version = @_cas_multi_load_phpcas_library($path)) {
    $element['#suffix'] = '<div class="ok messages">' . t('phpCAS version %version successfully loaded.', array('%version' => $version)) . '</div>';
  }
  else {
    $element['#suffix'] = '<div class="error messages">' . t('The phpCAS library was not found or could not be loaded.') . '</div>';
  }
  return $element;
}
