<?php

/**
 * Base class for export UI.
 */
class cas_multi_ctools_export_ui extends ctools_export_ui {

  /**
   * Overrides list_table_header() to customize data shown in list table.
   */
  public function list_table_header() {
    $header = array(
      array('data' => t('Name'), array('cas-ctools-export-ui-name')),
      array('data' => t('Host'), array('cas-ctools-export-ui-host')),
      array('data' => t('Protocol Version'), array('cas-ctools-export-ui-protocol-version')),
      array('data' => t('Local Login Path'), array('cas-ctools-export-ui-local-path')),
      array(
        'data' => t('Operations'),
        'class' => array('cas-ctools-export-ui-operations', 'ctools-export-ui-operations'),
      ),
    );

    return $header;
  }

  /**
   * Override default implementation so we can customize the data shown.
   */
  function list_build_row($item, &$form_state, $operations) {
    // Set up sorting.
    $name = $item->{$this->plugin['export']['key']};
    $schema = ctools_export_get_schema($this->plugin['schema']);

    // Note: $item->{$schema['export']['export type string']} should have
    // already been set up by export.inc so we can use it safely.
    switch ($form_state['values']['order']) {
      case 'disabled':
        $this->sorts[$name] = empty($item->disabled) . $name;
        break;

      case 'label':
        $this->sorts[$name] = $item->{$this->plugin['export']['admin_title']};
        break;

      case 'name':
        $this->sorts[$name] = $name;
        break;

      case 'storage':
        $this->sorts[$name] = $item->{$schema['export']['export type string']} . $name;
        break;
    }

    $this->rows[$name]['class'] = !empty($item->disabled) ? array('ctools-export-ui-disabled') : array('ctools-export-ui-enabled');

    $label = array(
      '#theme' => 'cas_multi_overview_item',
      '#label' => $item->label,
      '#name' => $name,
      '#status' => $item->export_type,
    );

    $host = 'https://' . $item->hostname;
    if ($item->port != 443) {
      $host .= ':' . $item->port;
    }
    if (!empty($item->path)) {
      $host .= $item->path;
    }

    $this->rows[$name]['data'] = array();
    $this->rows[$name]['data'][] = array(
      'data' => $label,
      'class' => array('ctools-export-ui-name'),
    );
    $this->rows[$name]['data'][] = array(
      'data' => check_plain($host),
      'class' => array('cas-ctools-export-ui-name'),
    );
    $this->rows[$name]['data'][] = array(
      'data' => check_plain($item->version),
      'class' => array('cas-ctools-export-ui-protocol-version'),
    );
    $this->rows[$name]['data'][] = array(
      'data' => '/cas/' . check_plain($name),
      'class' => array('cas-ctools-export-ui-local-path'),
    );

    $ops = theme('links__ctools_dropbutton', array(
      'links' => $operations,
      'attributes' => array('class' => array('links', 'inline')))
    );
    $this->rows[$name]['data'][] = array(
      'data' => $ops,
      'class' => array('cas-ctools-export-ui-operations', 'ctools-export-ui-operations'),
    );

    // Add an automatic mouseover of the description if one exists.
    if (!empty($this->plugin['export']['admin_description'])) {
      $this->rows[$name]['title'] = $item->{$this->plugin['export']['admin_description']};
    }
  }

  /**
   * Our custom submit handler so we can properly serialize the roles data.
   */
  function edit_form_submit(&$form, &$form_state) {
    // Transfer data from the form to the $item based upon schema values.
    $schema = ctools_export_get_schema($this->plugin['schema']);
    foreach (array_keys($schema['fields']) as $key) {
      if (isset($form_state['values'][$key])) {
        // We need to serialize the auto assigned roles before they are saved.
        $val = $form_state['values'][$key];
        if ($key === 'auto_assigned_roles') {
          // Remove roles that were left unchecked.
          foreach ($val as $k => $v) {
            if ($v == 0) {
              unset($val[$k]);
            }
          }
          $val = serialize($val);
        }
        $form_state['item']->{$key} = $val;
      }
    }
  }
}
