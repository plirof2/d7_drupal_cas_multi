<?php

$plugin = array(
  'schema' => 'cas_multi_server',
  'access' => 'administer cas_multi',
  'menu' => array(
    'menu prefix' => CAS_CONFIG_BASE_PATH,
    'menu item' => 'server',
    'menu title' => 'Servers',
    'menu description' => 'CAS Server Configurations',
  ),
  'title singular' => t('CAS server config'),
  'title plural' => t('CAS server configs'),
  'title singular proper' => t('CAS Server Config'),
  'title plural proper' => t('CAS Server Configs'),
  'form' => array(
    'settings' => '_cas_multi_server_config_form',
    'validate' => '_cas_multi_server_config_form_validate',
    'submit' => '_cas_multi_server_config_form_submit',
  ),
  'handler' => array(
    'class' => 'cas_multi_ctools_export_ui',
    'parent' => 'ctools_export_ui',
  ),
);

/**
 * Build and return the add/edit form for CAS server configs.
 *
 * $data is a stdClass containing the default values for each form field.
 */
function _cas_multi_server_config_form(&$form, &$form_state) {
  if (!_cas_multi_load_phpcas_library()) {
    drupal_set_message(t("You must first configure the location of the phpCAS library."), 'error');
    drupal_goto(CAS_CONFIG_BASE_PATH . '/settings');
  }

  $server_config = $form_state['item'];

  $form['server'] = array(
    '#type' => 'fieldset',
    '#title' => t('Basic Settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['server']['hostname'] = array(
    '#type' => 'textfield',
    '#title' => t('Hostname'),
    '#maxlength' => 255,
    '#description' => t('The hostname of the CAS server. Do not include the protocol or port. If your CAS server is https://mycasserver.com/, put "casserver.com" here.'),
    '#required' => TRUE,
    '#default_value' => $server_config->hostname,
  );
  $form['server']['port'] = array(
    '#type' => 'textfield',
    '#title' => t('Port'),
    '#maxlength' => 5,
    '#description' => t('Only necessary if your CAS server is not accessible over the standard TLS port of 443.'),
    '#default_value' => $server_config->port,
  );
  $form['server']['path'] = array(
    '#type' => 'textfield',
    '#title' => t('Path'),
    '#maxlength' => 255,
    '#description' => t('If CAS is not at the root of the host, include the path here (e.g., /cas).'),
    '#default_value' => $server_config->path,
  );
  $form['server']['version'] = array(
    '#type' => 'radios',
    '#title' => t('CAS Protocol Version'),
    '#description' => t('Select the protocol version your CAS server implements.'),
    '#options' => array(
      CAS_VERSION_1_0 => t('1.0'),
      CAS_VERSION_2_0 => t('2.0'),
      CAS_VERSION_3_0 => t('3.0 or higher (requires phpCAS >= 1.3.3)'),
      SAML_VERSION_1_1 => t('SAML Version 1.1'),
    ),
    '#default_value' => $server_config->version,
    '#required' => TRUE,
  );
  $form['server']['tls_cert_ca_path'] = array(
    '#type' => 'textfield',
    '#title' => 'Certificate Authority PEM Certificate',
    '#description' => t('The PEM certificate of the Certificate Authority that issued the certificate of the CAS server. If omitted, the certificate authority will not be verified.'),
    '#required' => FALSE,
    '#default_value' => $server_config->tls_cert_ca_path,
  );

  $form['user_accounts'] = array(
    '#type' => 'fieldset',
    '#title' => t('User Accounts'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['user_accounts']['auto_register'] = array(
    '#type' => 'checkbox',
    '#title' => t('Automatic Registration'),
    '#description' => t('Enable to automatically register users who log into your site via CAS for the first time. If this is disabled, you must manually associate existing or new Drupal accounts with CAS usernames before CAS users can authenticate.'),
    '#default_value' => $server_config->auto_register,
  );
  $form['user_accounts']['auto_register_domain'] = array(
    '#type' => 'textfield',
    '#title' => t('Email Domain'),
    '#maxlength' => 255,
    '#description' => t('Drupal user accounts need an email address when they are registered. Users that are automatically registered will be given an email constructed from their CAS username and this hostname.'),
    '#states' => array(
      'visible' => array(
        ':input[name="auto_register"]' => array('checked' => TRUE),
      ),
    ),
    '#default_value' => $server_config->auto_register_domain,
  );
  $roles = array_map('check_plain', user_roles(TRUE));
  unset($roles[DRUPAL_AUTHENTICATED_RID]);
  if (!empty($roles)) {
    $form['user_accounts']['auto_assigned_roles'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Automatic Role Assignment'),
      '#description' => t('The selected roles will be automatically assigned to each CAS user on login. Use this to automatically give CAS users additional privileges or to identify CAS users to other modules.'),
      '#options' => $roles,
      '#access' => user_access('administer permissions'),
      '#default_value' => !empty($server_config->auto_assigned_roles) ? unserialize($server_config->auto_assigned_roles) : array(),
    );
  }
  $form['user_accounts']['hide_email'] = array(
    '#type' => 'checkbox',
    '#title' => t('Users cannot change email'),
    '#description' => t('Prevent users associated with CAS from changing their email on the user edit page.'),
    '#default_value' => $server_config->hide_email,
  );
  $form['user_accounts']['hide_password'] = array(
    '#type' => 'checkbox',
    '#title' => t('Users cannot change password'),
    '#description' => t("Prevent users associated with CAS from changing their password on the user edit page. If this is not enabled, users will be able to change the password for their Drupal account and login without using CAS."),
    '#default_value' => $server_config->hide_password,
  );

  $form['login'] = array(
    '#type' => 'fieldset',
    '#title' => t('Login Behavior'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['login']['login_success_message'] = array(
    '#type' => 'textfield',
    '#title' => t('Login Success Message'),
    '#description' => t("Notice to show to users after they have successfully logged in via this CAS configuration. You can use the token %cas_username in the message which will be replaced with the user's username."),
    '#rows' => 3,
    '#default_value' => $server_config->login_success_message,
  );
  $form['login']['login_link_enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Login Link Enabled'),
    '#description' => t('Include a link to login via this CAS server on the Drupal login page.'),
    '#default_value' => $server_config->login_link_enabled,
  );
  $form['login']['login_link_label'] = array(
    '#type' => 'textfield',
    '#title' => t('Login Link Label'),
    '#maxlength' => 255,
    '#description' => t("The text to display along with the login link. Defaults to admin title if not set."),
    '#default_value' => $server_config->login_link_label,
    '#states' => array(
      'visible' => array(
        ':input[name="login_link_enabled"]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['login']['login_link_description'] = array(
    '#type' => 'textarea',
    '#title' => t('Login Link Description'),
    '#description' => t('An optional description to display along with the login link.'),
    '#default_value' => $server_config->login_link_description,
    '#rows' => 3,
    '#states' => array(
      'visible' => array(
        ':input[name="login_link_enabled"]' => array('checked' => TRUE),
      ),
    ),
  );

  $form['single_sign_out'] = array(
    '#type' => 'fieldset',
    '#title' => t('Single Sign Out Support'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['single_sign_out']['single_sign_out'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enabled'),
    '#description' => t('When enabled, a user signing out of a properly configured CAS server will also be logged out of your Drupal site.'),
    '#default_value' => $server_config->single_sign_out,
  );

  $form['misc'] = array(
    '#type' => 'fieldset',
    '#title' => t('Miscellaneous'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['misc']['weight'] = array(
    '#type' => 'textfield',
    '#title' => t('Weight'),
    '#description' => t('When multiple CAS servers are displayed on the user login page, this number controls the order they appear.'),
    '#default_value' => $server_config->weight,
  );
}

/**
 * Set default values for each field on the settings form.
 *
 * Each form item that has a corresponding property in $defaults will have it's
 * default value set to that $defaults value.
 */
function _cas_multi_set_form_defaults(&$form, $defaults) {
  foreach ($form as $key => $val) {
    // If we're dealing with a fieldset, recurse into it's children.
    if (isset($val['#type']) && $val['#type'] === 'fieldset') {
      $form[$key] = _cas_multi_set_form_defaults($val, $defaults);
    }
    elseif (substr($key, 0, 1) !== '#') {
      if (isset($defaults->$key)) {
        if ($key === 'auto_assigned_roles') {
          $form[$key]['#default_value'] = unserialize($defaults->$key);
        }
        else {
          $form[$key]['#default_value'] = $defaults->$key;
        }
      }
    }
  }

  return $form;
}

/**
 * Validate _cas_multi_server_config_form.
 */
function _cas_multi_server_config_form_validate($form, &$form_state) {
  if (!preg_match('/^[0-9]+$/', $form_state['values']['port'])) {
    form_set_error('port', t('Invalid port number.'));
  }

  if ($form_state['values']['auto_register']) {
    if (empty($form_state['values']['auto_register_domain'])) {
      form_set_error('auto_register_domain', t('Auto registration email domain is required.'));
    }
    else {
      if (!preg_match('/^(?!\-)(?:[a-zA-Z\d\-]{0,62}[a-zA-Z\d]\.){1,126}(?!\d+)[a-zA-Z\d]{1,63}$/', $form_state['values']['auto_register_domain'])) {
        form_set_error('auto_register_domain', t('Not a valid domain.'));
      }
    }
  }
}
