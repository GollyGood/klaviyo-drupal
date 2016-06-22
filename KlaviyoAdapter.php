<?php

// @todo: Get all this Klaviyo API specific stuff out of here!
require_once DRUPAL_ROOT . '/sites/all/libraries/klaviyo-api-php/vendor/autoload.php';

class KlaviyoAdapter {

  public $api;
  public $apiKey;
  public $siteId;
  private static $instance;

  public static function getInstance() {
    if (NULL === static::$instance) {
      static::$instance = new static();
    }

    return static::$instance;
  }

  public function __construct() {
    $this->apiKey = variable_get('klaviyo_api_key', '');
    $this->getSiteId();
    $this->api = Klaviyo\KlaviyoFacade::create($this->apiKey);
  }

  public function getSiteId() {
    // @todo: Actually try recieving from variables before re-setting it.
    if (empty($this->siteId)) {
      $this->siteId = base64_encode(variable_get('site_name') . ':' . REQUEST_TIME);
      variable_set('klaviyo_drupal_site_id', $this->siteId);
    }

    return $this->siteId;
  }


  public function getLists() {
    $lists = array();

    try {
      $lists = $this->api->service('lists')->getAllLists();
    }
    catch (Exception $e) {
      watchdog_exception($e);
      drupal_set_message('Unable to retrieve lists from Klaviyo. Please try again later.', 'error');
    }

    return $lists;
  }

  public function createList($list_name) {
    $list = NULL;
    try {
      $list = $this->api->service('lists')->createList($list_name);

      cache_clear_all('klaviyo:list_options', 'cache');
    }
    catch (Exception $e) {
      watchdog_exception('klaviyo', $e);
      drupal_set_message('Unable create list. Please try again later.', 'error');
    }

    return $list;
  }

  public function getCachedListOptions() {
    $cache = cache_get('klaviyo:list_options');

    $options = array();
    if ($cache && !empty($cache->data)) {
      $options = $cache->data;
    }
    else {
      $lists = KlaviyoAdapter::getInstance()->getLists();
      foreach ($lists as $list) {
        $options[$list->listType . ':' . $list->id] = check_plain($list->name);
      }
      cache_set('klaviyo:list_options', $options, 'cache', CACHE_TEMPORARY);
    }

    return $options;
  }

  public function createFullListId($list) {
    return $list->listType . ':' . $list->id;
  }

  public function parseFullListId($list_full_id) {
    $list_options = $this->getCachedListOptions();

    $parsed_list_full_id = array();
    if (isset($list_options[$list_full_id])) {
      $parsed_list_full_id = explode(':', $list_full_id);
    }

    return $parsed_list_full_id + array(0 => '', 1 => '');
  }

  public function getPersonGetMappableKeys() {
    $model_class = $this->api->getModelClass('person');
    return array_filter(call_user_func("$model_class::getAttributeKeys"), function($attribute_key) {
      return !($attribute_key === 'id' || $attribute_key === 'object');
    });
  }

  public function preparePersonConfiguration($entity_type, $entity) {
    $person_configuration = array();

    $wrapper = entity_metadata_wrapper($entity_type, $entity);
    $this->preparePersonConfigurationKlaviyoAttributes($entity_type, $wrapper, $person_configuration);

    $person_configuration = $this->preparePersonConfigurationDrupalInfo($entity_type, $wrapper, $person_configuration);

    if ($entity_type === 'user') {
      $person_configuration = $this->preparePersonConfigurationFromAccount($wrapper, $person_configuration);
    }

    return $person_configuration;
  }

  private function preparePersonConfigurationKlaviyoAttributes($entity_type, EntityMetadataWrapper $wrapper, &$person_configuration) {
    $attributes = variable_get('klaviyo_person_attributes_' . $entity_type, array());

    foreach ($attributes as $field_name => $attribute_key) {
      if (isset($wrapper->{$field_name})) {
        $person_configuration[$attribute_key] = $field_value = $wrapper->{$field_name}->value();
      }
    }

    return $person_configuration;
  }

  private function preparePersonConfigurationDrupalInfo($entity_type, EntityMetadataWrapper $wrapper, &$person_configuration) {
    $person_configuration['drupal.site_id'] = $this->getSiteId();
    $person_configuration['drupal.entity_type'] = $entity_type;
    $person_configuration['drupal.entity_bundle'] = $wrapper->getBundle();
    $person_configuration['drupal.entity_id'] = $wrapper->getIdentifier();

    return $person_configuration;
  }

  private function preparePersonConfigurationFromAccount(EntityMetadataWrapper $wrapper, &$person_configuration) {
    if (empty($person_configuration['$email'])) {
      $person_configuration['$email'] = $wrapper->mail->value();
    }
    if (empty($person_configuration['$first_name'])) {
      $person_configuration['$first_name'] = $wrapper->label();
    }

    return $person_configuration;
  }

  public function isFieldMappable($entity_type, $field_type, $bundle = '') {
    $allowable_field_types = array('text', 'list_text');

    return ($this->isEnabledOnEntity($entity_type, $bundle) && in_array($field_type, $allowable_field_types));
  }

  public function getAllEntitySettings($entity_type, $bundle = '') {
    $variable_name = "klaviyo_entity_settings_$entity_type";

    if ($bundle) {
      $variable_name .= $bundle;
    }

    return variable_get($variable_name, array('enabled' => FALSE, 'list' => ''));
  }

  public function isEnabledOnEntity($entity_type, $bundle = '') {
    $settings = $this->getAllEntitySettings($entity_type, $bundle);
    return $this->isCompatiableEntity($entity_type, $bundle) && !empty($settings['enabled']);
  }

  public function isCompatiableEntity($entity_type, $bundle = '') {
    return $entity_type === 'user';
  }

  public function isFieldMapped($entity_type, $field_type, $bundle = '') {
    $is_mapped = FALSE;
    $variable_name = "klaviyo_person_attributes__$entity_type";

    if ($bundle) {
      $variable_name .= $bundle;
    }

    if ($entity_type === 'user') {
      $attributes = variable_get($variable_name, array());
      $is_mapped = !empty($attributes[$field_name]);
    }

    return $is_mapped;
  }

  public function subscribePersonToList(PersonModel $person, ListModel $list) {
    if (!empty($list)) {
      try {
        $this->api->service('lists')->addPersonToList($person, $list);
      }
      catch (Exception $e) {
        watchdog_error('klaviyo', $e);
      }
    }
  }


  public function __clone() {}
  public function __wakeup() {}

}
