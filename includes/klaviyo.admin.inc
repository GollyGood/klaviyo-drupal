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

  if (!libraries_detect('klaviyo-api-php')) {
    drupal_set_message(klaviyo_get_installation_instructions(), 'warning');
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

  return system_settings_form($form);
}
