<?php

class Klaviyo {

  public $api;
  private $siteId;
  private $apiKey;
  private static $instance;

  private function __construct() {
    $this->setSiteId();
    $this->apiKey = variable_get('klaviyo_api_key', '');
    $this->api = new KlaviyoAdapter($this->apiKey);
  }

  public static function getInstance() {
    if (NULL === static::$instance) {
      static::$instance = new static();
    }

    return static::$instance;
  }

  private function setSiteId() {
    $this->siteId = variable_get('klaviyo_drupal_site_id', '');

    if (empty($this->siteId)) {
      $this->siteId = base64_encode(variable_get('site_name') . ':' . REQUEST_TIME);
      variable_set('klaviyo_drupal_site_id', $this->siteId);
    }

    return $this;
  }

  public function getSiteId() {
    return $this->siteId;
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

  public function getCachedListOptions() {
    $cache = cache_get('klaviyo:list_options');

    $options = array();
    if ($cache && !empty($cache->data)) {
      $options = $cache->data;
    }
    else {
      foreach ($this->api->getLists() as $list) {
        $options[$list->listType . ':' . $list->id] = check_plain($list->name);
      }
      cache_set('klaviyo:list_options', $options, 'cache', CACHE_TEMPORARY);
    }

    return $options;
  }

  public function preparePersonConfiguration($entity_type, EntityMetadataWrapper $wrapper) {
    $person_configuration = array();

    $person_configuration = $this->preparePersonConfigurationKlaviyoAttributes($entity_type, $wrapper, $person_configuration);

    if (!empty($entity->klaviyo['id'])) {
      $person_configuration['id'] = $entity->klaviyo['id'];
    }

    $person_configuration = $this->preparePersonConfigurationDrupalInfo($entity_type, $wrapper, $person_configuration);

    if ($entity_type === 'user') {
      $person_configuration = $this->preparePersonConfigurationFromAccount($wrapper, $person_configuration);
    }

    return $person_configuration;
  }

  protected function preparePersonConfigurationKlaviyoAttributes($entity_type, EntityMetadataWrapper $wrapper, &$person_configuration) {
    $attributes = variable_get('klaviyo_person_attributes_' . $entity_type, array());

    foreach ($attributes as $field_name => $attribute_key) {
      if (isset($wrapper->{$field_name}) && $value = $wrapper->{$field_name}->value()) {
        $person_configuration[$attribute_key] = $value;
      }
      else {
        $person_configuration['$unset'][] = $attribute_key;
      }
    }

    return $person_configuration;
  }

  protected function preparePersonConfigurationDrupalInfo($entity_type, EntityMetadataWrapper $wrapper, &$person_configuration) {
    $person_configuration['id'] = $this->getKlaviyoIdentifier($entity_type, $wrapper);
    $person_configuration['drupal.site_id'] = $this->getSiteId();
    $person_configuration['drupal.entity_type'] = $entity_type;
    $person_configuration['drupal.entity_bundle'] = $wrapper->getBundle();
    $person_configuration['drupal.entity_id'] = $wrapper->getIdentifier();

    return $person_configuration;
  }

  public function getKlaviyoIdentifier($entity_type, EntityMetadataWrapper $wrapper) {
    return base64_encode($this->getSiteId() . '_' . $entity_type . '_' . $wrapper->getIdentifier());
  }

  protected function preparePersonConfigurationFromAccount(EntityMetadataWrapper $wrapper, &$person_configuration) {
    if (empty($person_configuration['$email'])) {
      $person_configuration['$email'] = $wrapper->mail->value();
    }
    if (empty($person_configuration['$first_name'])) {
      $person_configuration['$first_name'] = $wrapper->label();
    }

    return $person_configuration;
  }

  public function isEnabledOnEntity($entity_type, $bundle = '') {
    $settings = $this->getAllEntitySettings($entity_type, $bundle);
    return $this->isCompatiableEntity($entity_type, $bundle) && !empty($settings['enabled']);
  }

  public function getAllEntitySettings($entity_type, $bundle = '') {
    $variable_name = "klaviyo_entity_settings_$entity_type";

    if ($bundle) {
      $variable_name .= $bundle;
    }

    return variable_get($variable_name, array('enabled' => FALSE, 'settings' => array('list' => '')));
  }

  public function isCompatiableEntity($entity_type, $bundle = '') {
    return $entity_type === 'user';
  }

  public function isFieldMappable($entity_type, $field_type, $bundle = '') {
    $allowable_field_types = array('text', 'list_text');

    return ($this->isEnabledOnEntity($entity_type, $bundle) && in_array($field_type, $allowable_field_types));
  }

  public function isFieldMapped($entity_type, $field_type, $bundle = '') {
    $is_mapped = FALSE;
    $variable_name = "klaviyo_person_attributes_$entity_type";

    if ($bundle) {
      $variable_name .= $bundle;
    }

    if ($entity_type === 'user') {
      $attributes = variable_get($variable_name, array());
      $is_mapped = !empty($attributes[$field_name]);
    }

    return $is_mapped;
  }

  public function saveEntity($entity_type, EntityMetadataWrapper $wrapper) {
    if (!$this->isEnabledOnEntity($entity_type)) {
      return;
    }

    $person_configuration = $this->preparePersonConfiguration($entity_type, $wrapper);

    if (!empty($wrapper->value()->klaviyo['klaviyo_id'])) {
      $person_configuration['id'] = $wrapper->value()->klaviyo['klaviyo_id'];
    }

    $successful = FALSE;
    if (!empty($person_configuration)) {
      $successful = $this->api->savePerson($person_configuration);
    }

    return $successful;
  }

  public function getPersonMappableAttributeOptions($entity_type, $field_name, $bundle = '') {
    $variable_name = "klaviyo_person_attributes_$entity_type";
    if (!empty($bundle)) {
      $variable_name .= "_$bundle";
    }
    $attributes = variable_get($variable_name, array());

    if (isset($attributes[$field_name])) {
      $default_attribute = $attributes[$field_name];
    }

    $mappable_attribute_keys = $this->api->getPersonGetMappableKeys();
    $attributes = array_diff(drupal_map_assoc($mappable_attribute_keys), $attributes);
    if (!empty($default_attribute)) {
      $attributes[$default_attribute] = $default_attribute;
    }

    return $attributes;
  }

  public function lookupKalviyoIdByEmail($entity_type, $email, $bundle = '') {
    $klaviyo_id = 0;
    $settings = $this->getAllEntitySettings($entity_type, $bundle);

    if (!empty($settings['settings']['list']) && $list_id_full = $settings['settings']['list']) {
      list(, $list_id) = $this->parseFullListId($list_id_full);
      if ($person_list = $this->api->lookUpPersonFromList($list_id, $email)) {
        $klaviyo_id = $person_list->person->id;
      }
    }

    return $klaviyo_id;
  }

}
