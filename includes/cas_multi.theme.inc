<?php

/**
 * Returns the HTML used for a queue on the cas overview listing.
 */
function theme_cas_multi_overview_item($variables) {
  $output = '<strong>' . check_plain($variables['label']) . '</strong>';
  if ($variables['name']) {
    $output .= ' <small> (' . t('Machine name') . ': ' . check_plain($variables['name']) . ')</small>';
  }
  $output = '<div class="cas-title">' . $output . '</div>';
  if ($variables['status']) {
    $output .= '<div class="cas-status">' . theme('cas_multi_status', array('status' => $variables['status'], TRUE)) . '</div>';
  }
  return $output;
}

/**
 * Themes the exportable status of a queue.
 */
function theme_cas_multi_status($variables) {
  $status = $variables['status'];
  $html = $variables['html'];
  if (($status & (EXPORT_IN_CODE | EXPORT_IN_DATABASE)) == (EXPORT_IN_CODE | EXPORT_IN_DATABASE)) {
    $label = t('Database overriding code');
    $help = t('This configuration is provided by a module, but has been changed.');
    return $html ? "<span class='cas-status-overridden' title='$help'>" . $label . "</span>" : $label;
  }
  elseif ($status & EXPORT_IN_CODE) {
    $label = t('In code');
    $help = t('A module provides this configuration.');
    return $html ? "<span class='cas-status-default' title='$help'>" . $label . "</span>" : $label;
  }
  elseif ($status & EXPORT_IN_DATABASE) {
    $label = t('In database');
    $help = t('A custom configuration by a user.');
    return $html ? "<span class='cas-status-custom' title='$help'>" . $label . "</span>" : $label;
  }
}

/**
 * Themes our overridden login page.
 */
function theme_cas_multi_login_page($variables) {
  $servers = $variables['cas_server_configs'];

  $output = '<div class="cas-login-links">';
  foreach ($servers as $server) {
    if (!$server->login_link_enabled) {
      continue;
    }

    $output .= '<div class="cas-login-link cas-login-link-' . check_plain($server->name) . '">';
    $label = !empty($server->login_link_label) ? $server->login_link_label : $server->label;
    $output .= '<h2>' . check_plain($label) . '</h2>';
    if (!empty($server->login_link_description)) {
      $output .= '<p>' . $server->login_link_description . '</p>';
    }
    $output .= theme('link', array(
      'text' => 'Log in',
      'path' => 'cas/' . $server->name,
      'options' => array(
        'attributes' => array(),
        'html' => TRUE,
      ),
    ));
    $output .= '</div>';
  }
  $output .= '</div>';

  $output .= '<div class="cas-drupal-login">';
  $output .= '<h2>Standard Login</h2>';
  $output .= drupal_render($variables['standard_login_form']);
  $output .= '</div>';

  return $output;
}
