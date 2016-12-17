<?php

/**
 * @file
 * The Klaviyo main module file.
 */

/**
 * Klaviyo API integration settings form.
 *
 * @see system_settings_form()
 */
function klaviyo_api_integration_settings() {
  $form = array();

  if (klaviyo_library_is_installed() == FALSE) {
    drupal_set_message($message = klaviyo_get_installation_instructions(), $type = 'error');
    return $form;
  }
  // Initialize an instance of Klaviyo. This will also initalized our site ID
  // for the first time if necessary.
  $klaviyo = klaviyo_api_get_instance();

  $form['klaviyo_api_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Private API key'),
    '#description' => t('The private key will be used for all communications with Klaviyo. A private get can be retrieved from Account >> Settings >> <a href="!url">API Keys</a> from within Klaviyo.',
      array('!url' => 'https://www.klaviyo.com/account#api-keys-tab')
    ),
    '#default_value' => variable_get('klaviyo_api_key', ''),
    '#size' => 60,
    '#maxlength' => 128,
    '#required' => TRUE,
  );

  if (!empty($form['klaviyo_api_key']['#default_value'])) {
    $form['klaviyo_drupal_site_id'] = array(
      '#type' => 'textfield',
      '#title' => t('Site ID'),
      '#description' => t('The site ID is a universally unique id that is used to identify this site from within Klaviyo and to generate an unique identifier for persons identified to Klaviyo.<br />
        One may be specified manually or one will automatically be generated.<br />
        <span class="warning">Warning: Changing the site ID after data has already been synced to Klaviyo will cause duplicate person records as the unique ID will no longer be the same for any existing person record.</span>
      '),
      '#default_value' => variable_get('klaviyo_drupal_site_id', ''),
      '#size' => 60,
      '#maxlength' => 128,
      '#required' => TRUE,
    );
  }

  $js_settings = variable_get('klaviyo_javascript', array('public_api_key' => '', 'roles' => array()));
  $form['klaviyo_javascript'] = array(
    '#type' => 'fieldset',
    '#title' => t('Javascript API'),
    '#description' => t('The JavaScript API adds automatic tracking of visited pages such as the "Active on site" metric. See <a href="!url">the Klaviyo jasvascript API documentation</a> for more information. Enter the public API key to enable the snippet.',
      array('!url' => 'https://www.klaviyo.com/docs/getting-started')
    ),
    '#collapsible' => TRUE,
    '#tree' => TRUE
  );

  $form['klaviyo_javascript']['public_api_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Public API key'),
    '#description' => t('The javascript will not be added if the public key is missing. The public key can be retrieved from Account >> Settings >> <a href="!url">API Keys</a> from within Klaviyo.',
      array('!url' => 'https://www.klaviyo.com/account#api-keys-tab')
    ),
    '#default_value' => $js_settings['public_api_key'],
    '#size' => 60,
    '#maxlength' => 128,
    '#required' => FALSE,
  );

  // @todo: Add ability for black list vs white list.
  $form['klaviyo_javascript']['roles'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Remove script for specific roles'),
    '#default_value' => $js_settings['roles'],
    '#options' => user_roles(),
    '#description' => t('Remove script only for the selected role(s). If none of the roles are selected, all roles will have the script. Otherwise, any roles selected here will NOT have the script.'),
  );

  // @todo: Add ability to exclude on certain pages.

  return system_settings_form($form);
}
