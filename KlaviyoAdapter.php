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
    if (empty($this->siteId)) {
      $this->siteId = base64_encode(variable_get('site_name') . ':' . REQUEST_TIME);
      variable_set('klaviyo_drupal_site_id', $this->siteId);
    }

    return $this->siteId;
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
    if (!empty($person_configuration)) {
      $person_configuration = $this->preparePersonConfigurationDrupalInfo($entity_type, $wrapper, $person_configuration);

      if ($entity_type === 'user') {
        $person_configuration = $this->preparePersonConfigurationFromAccount($wrapper, $person_configuration);
      }
    }

    return $person_configuration;
  }

  private function preparePersonConfigurationKlaviyoAttributes($entity_type, EntityMetadataWrapper $wrapper, &$person_configuration) {
    $attributes = variable_get('klaviyo_person_attributes_' . $entity_type, array());

    foreach ($attributes as $field_name => $attribute_key) {
      if (isset($wrapper->{$field_name}) && $field_value = $wrapper->{$field_name}->value()) {
        $person_configuration[$attribute_key] = $field_value;
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

  public function isFieldMappable($entity_type, $bundle, $field_type) {
    $allowable_field_types = array('text', 'list_text');

    return ($entity_type === 'user' && $bundle === 'user' && in_array($field_type, $allowable_field_types));
  }

  public function isFieldMapped($entity_type, $bundle, $field_type) {
    $is_mapped = FALSE;

    if ($entity_type === 'user' && $bundle === 'user') {
      $attributes = variable_get('klaviyo_person_attributes_user', array());
      $is_mapped = !empty($attributes[$field_name]);
    }

    return $is_mapped;
  }


  public function __clone() {}
  public function __wakeup() {}

}
