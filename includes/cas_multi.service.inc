<?php

/**
 * Main page callback for the /cas path.
 *
 * The optional parameter contains the machine name of the CAS server config.
 * If no machine name is passed in, then we load the default if one is
 * specified in configuration.
 */
function _cas_multi_service($server_name = NULL) {
  global $user;
  ctools_include('export');

  // Redirect away if user is already logged in.
  if ($user->uid) {
    drupal_goto();
  }

  // Load the server configuration data for the provided server. If there is
  // no provided server config machine name, load the default if set, or return
  // a 404.
  if (empty($server_name)) {
    $server_name = variable_get('cas_multi_default_server_config_name');
    if (empty($server_name)) {
      return MENU_NOT_FOUND;
    }
  }
  $cas_server_config = ctools_export_crud_load('cas_multi_server', $server_name);

  if (!$cas_server_config) {
    return MENU_NOT_FOUND;
  }

  if (isset($cas_server_config->disabled)) {
    return MENU_NOT_FOUND;
  }

  if (!_cas_multi_load_phpcas_library() || !_cas_multi_initialize_phpcas($cas_server_config)) {
    drupal_set_message(t('There was an error trying to authenticate using CAS. Please contact your site administrator.'), 'error');
    drupal_goto();
  }

  drupal_session_start();

  // phpCAS will check if this is a request for a single-sign-out. If so, it
  // will extract the CAS Ticket from the request and run our callback
  // for logging a CAS user out.
  // TODO: Change this back to TRUE to enable check_clients again.
  phpCAS::handleLogoutRequests(FALSE);

  phpCAS::forceAuthentication();

  // Under normal circumstances we should never get this far. The method call to
  // phpCAS::forceAuthentication() should either redirect to the CAS server for
  // authentication or invoke our _cas_multi_authentication_handler after successful
  // authentication. Just in case this is reached, redirect to the front page.
  drupal_goto();
}

/**
 * Called via phpCAS after a user has been authenticated.
 *
 * Here we log the user in and optionally register them if they are new.
 */
function _cas_multi_authentication_handler($cas_ticket, $cas_server_config) {
  global $user;

  $cas_user = array(
    'name' => phpCAS::getUser(),
    'login' => TRUE,
    'register' => TRUE,
    'attributes' => phpCAS::getAttributes(),
    'server_config' => $cas_server_config,
  );
  drupal_alter('cas_multi_user', $cas_user);

  // Bail out if a module denied login access for this user or unset the user
  // name.
  if (empty($cas_user['login']) || empty($cas_user['name'])) {
    drupal_set_message(t('Unable to login.', array('%name' => $cas_user['name'])), 'error');
    unset($_SESSION['phpCAS']);
    drupal_goto();
  }

  // Proceed with the login process, using the altered CAS username.
  $username = $cas_user['name'];

  $account = _cas_multi_load_cas_user($username, $cas_server_config->name);

  // If no Drupal user account exists for this CAS username and server config,
  // register it if we're configured to do so.
  if (!$account && $cas_server_config->auto_register && $cas_user['register']) {
    if (!_cas_multi_username_is_taken($username)) {
      $email = NULL;
      if (isset($cas_user['register_email']) && !empty($cas_user['register_email'])) {
        $email = $cas_user['register_email'];
      }
      $account = _cas_multi_register_account($username, $email, $cas_server_config);
      if (!$account) {
        drupal_set_message(t('Unable to create an account for your username. Please contact a site administrator.'), 'error');
        unset($_SESSION['phpCAS']);
        drupal_goto();
      }
    }
    else {
      drupal_set_message(t('An account with your username already exists but is not associated with your CAS username. Please contact a site administrator to correct this.'), 'error');
      unset($_SESSION['phpCAS']);
      drupal_goto();
    }
  }

  if ($account && $account->uid > 0) {
    if ($account->status == 0) {
      $blocked_message = 'The user %username has been blocked.';
      drupal_set_message(t($blocked_message, array('%username' => $username)), 'error');
      watchdog('cas_multi', $blocked_message, array('%username' => $username), WATCHDOG_WARNING);
      unset($_SESSION['phpCAS']);
      drupal_goto();
    }

    // Create basic user $edit array that will be passed to modules that
    // implement our presave hook. These modules will have an opportunity to
    // alter the user before they login.
    $edit['cas_user'] = $cas_user;
    $edit['roles'] = $account->roles + _cas_multi_auto_assigned_roles($cas_server_config);
    if (module_exists('persistent_login') && !empty($_SESSION['cas_multi_remember'])) {
      $edit['values']['persistent_login'] = 1;
    }
    _cas_multi_user_module_invoke('presave', $edit, $account, $cas_server_config);

    // Save user if the presave hook made any alterations.
    $user = user_save($account, $edit);

    // Log the user in. This is the only way to do it really - replace the
    // global $user and call this helper function which among other things will
    // regenerate the user session ID.
    user_login_finalize();

    // Save an association of this user's Drupal session with the CAS ticket
    // if single sign out is enabled for this server config.
    if ($cas_server_config->single_sign_out && isset($cas_ticket)) {
      _cas_multi_save_login_data_for_single_sign_out($user->uid, $cas_ticket);
    }

    $login_message = $cas_server_config->login_success_message;
    if (!empty($cas_server_config->login_success_message)) {
      drupal_set_message(t($login_message, array('%cas_username' => $username)));
    }
    drupal_goto();
  }
  else {
    unset($_SESSION['phpCAS']);
    drupal_set_message(t('An account does not exist for your username.'), 'error');
    drupal_goto();
  }
}

/**
 * Save an association of the CAS session ticket with the Drupal session ID.
 *
 * This is used later on when processing single sign out requests.
 */
function _cas_multi_save_login_data_for_single_sign_out($uid, $cas_ticket) {
  db_merge('cas_multi_login_data')
    ->key(array('cas_session_id' => $cas_ticket))
    ->fields(array(
      'cas_session_id' => $cas_ticket,
      'drupal_session_id' => session_id(),
      'uid' => $uid,
      'created' => REQUEST_TIME,
    ))
    ->execute();
}

/**
 * Return Drupal user object associated with the passed in CAS name / server.
 */
function _cas_multi_load_cas_user($cas_name, $server_name) {
  // First we need to get the user ID.
  $result = db_query("SELECT uid FROM {cas_multi_user} WHERE server_name = :server_name AND cas_name = :cas_name", array(
    'server_name' => $server_name,
    'cas_name' => $cas_name,
  ));
  if (!$result) {
    return FALSE;
  }

  $uid = (int) $result->fetchField();
  if (!$uid) {
    return FALSE;
  }
  return user_load($uid);
}

/**
 * Handle single-sign-out requests. Invoked from phpCAS.
 */
function _cas_multi_single_sign_out_handler($cas_ticket) {
  $record = db_query_range(
    'SELECT cld.uid, cld.drupal_session_id, u.name FROM {cas_multi_login_data} cld JOIN {users} u ON u.uid = cld.uid WHERE cld.cas_session_id = :cas_ticket',
    0,
    1,
    array('cas_ticket' => $cas_ticket)
  )->fetchObject();

  if ($record) {
    db_delete('cas_multi_login_data')
      ->condition('cas_session_id', $cas_ticket)
      ->execute();

    // Destroy the specific Drupal session. We cannot just delete the session ID
    // from MySQL session table because some sites may use a different session
    // storage. We have to use PHP's session methods which will invoke the
    // proper session storage handler functions.
    session_start();
    session_id($record->drupal_session_id);
    session_destroy();
    session_commit();

    watchdog('user', 'Session closed for %name by CAS single-sign-out request.', array('%name' => $record->name));
  }
}

/**
 * Configure/Initialize the phpCAS library.
 */
function _cas_multi_initialize_phpcas($server_config) {
  // Prevent this function from being called more than once.
  $initialized = &drupal_static(__FUNCTION__, FALSE);
  if ($initialized) {
    return FALSE;
  }
  $initialized = TRUE;

  $server_version = $server_config->version;
  $server_cas_server = $server_config->hostname;
  $server_port = (int) $server_config->port;
  $server_uri = $server_config->path;

  phpCAS::client($server_version, $server_cas_server, $server_port, $server_uri, FALSE);
  if (!empty($server_config->tls_cert_ca_path)) {
    phpCAS::setCasServerCACert($server_config->tls_cert_ca_path);
  }
  else {
    phpCAS::setNoCasServerValidation();
  }
  $service_url = url('cas/' . $server_config->name, array('query' => drupal_get_query_parameters(), 'absolute' => TRUE));
  phpCAS::setFixedServiceURL($service_url);
  phpCAS::setSingleSignoutCallback('_cas_multi_single_sign_out_handler');
  phpCAS::setPostAuthenticateCallback('_cas_multi_authentication_handler', array($server_config));

  return TRUE;
}

/**
 * Returns an array of roles that should be assigned to each CAS user on login.
 */
function _cas_multi_auto_assigned_roles(stdClass $cas_server_config) {
  $auto_assigned_roles = array();
  if (!empty($cas_server_config->auto_assigned_roles)) {
    $auto_assigned_roles = unserialize($cas_server_config->auto_assigned_roles);
  }
  return array_intersect_key(user_roles(), $auto_assigned_roles);
}
