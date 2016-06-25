<?php

/**
 * The main Klaviyo/Drupal integration class.
 */
class Klaviyo {

  /**
   * The Kalviyo adapter used for communicating with the Klaviyo API Library.
   *
   * @var KlaviyoAdapter
   */
  public $api;

  private $apiKey;
  private $siteId;

  /**
   * @var Klaviyo
   *   The reference to Klaviyo instance of this class.
   */
  private static $instance;

  /**
   * The constructor for the Klaviyo integration class.
   *
   * @see Klaviyo::getInstance().
   */
  protected function __construct() {
    $this->setSiteId();
    $this->apiKey = variable_get('klaviyo_api_key', '');
    $this->api = new KlaviyoAdapter($this->apiKey);
  }

  /**
   * Retrieve an instance of the Klaviyo/Drupal integration class.
   *
   * @return Klaviyo
   *   The single available instance of the Klaviyo/Drupal integration class.
   */
  public static function getInstance() {
    if (NULL === static::$instance) {
      static::$instance = new static();
    }

    return static::$instance;
  }

  /**
   * Set the site id used to identify this Drupal site to Klaviyo.
   *
   * @return $this
   */
  protected function setSiteId() {
    $this->siteId = variable_get('klaviyo_drupal_site_id', '');

    if (empty($this->siteId)) {
      $this->siteId = base64_encode(variable_get('site_name') . ':' . REQUEST_TIME);
      variable_set('klaviyo_drupal_site_id', $this->siteId);
    }

    return $this;
  }

  /**
   * Retrieve the site id used to identify this Drupal site to Klaviyo.
   *
   * @return string
   *   A unique identifier of this Drupal site for for use with Klaviyo.
   */
  public function getSiteId() {
    return $this->siteId;
  }

  /**
   * Retrieve the unique Klaviyo identifier for a specific entity.
   *
   * @todo: Rename this method.
   *
   * @param string $entity_type
   *   The entity type to use for building the unique identifier.
   * @param int $entity_id
   *   The entity id to use for building the unique identifier.
   *
   * @return string
   *   A unique identifier of this Drupal entity for for use with Klaviyo.
   */
  public function getKlaviyoIdentifier($entity_type, $entity_id) {
    return base64_encode($this->getSiteId() . '_' . $entity_type . '_' . $entity_id);
  }

  /**
   * Create a full list id that can be used to identify the list's type and id.
   *
   * @param ListModel $list
   *   The list for which to create the id.
   *
   * @return string
   *   A string representing the list's type and id. Example: TYPE:ID
   */
  public function createFullListId($list) {
    return $list->listType . ':' . $list->id;
  }

  /**
   * Parse and retrieve a full list id.
   *
   * @param string $list_full_id
   *   The full list id to be parsed.
   *
   * @return array
   *   The complete array of identifiers for the list.
   *   Example: array(0 => TYPE, 1 => ID)
   */
  public function parseFullListId($list_full_id) {
    $list_options = $this->getCachedListOptions();

    $parsed_list_full_id = array();
    if (isset($list_options[$list_full_id])) {
      $parsed_list_full_id = explode(':', $list_full_id);
    }

    return $parsed_list_full_id + array(0 => '', 1 => '');
  }

  /**
   * Retrieve an array of list options that can be used in form fields.
   *
   * @param bool $reset
   *   Determines if the cached array of options should be reset. When TRUE, the
   *   lists will be retrieved from Klaviyo and re-cached.
   *
   * @return array
   *   An array of list options keyed by the lists' full id that can be used in
   *   form fields.
   */
  public function getCachedListOptions($reset = FALSE) {
    $cache = cache_get('klaviyo:list_options');

    $options = array();
    if (!$reset && $cache && !empty($cache->data)) {
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

  /**
   * Prepare the person configuration array for writing to Klaviyo.
   *
   * @param string $entity_type
   *   The entity type for which to prepare the person configuration.
   * @param EntityMetadataWrapper $wrapper
   *   The entity EMW to use for preparing the person configuration.
   *
   * @return array
   *   The prepared person configuration ready for writing to Klaviyo.
   */
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

  /**
   * Prepare the mapped attributes for a person.
   *
   * @param string $entity_type
   *   The entity type for which to prepare the mapped attributes of the person
   *   configuration.
   * @param EntityMetadataWrapper $wrapper
   *   The entity EMW to use for preparing the mapped attributes of the person
   *   configuration.
   * @param array $person_configuration
   *   The person configuration to which configurations should be appended.
   *
   * @return array
   *   The person configuration containing the mapped attribute values.
   */
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

  /**
   * Prepare the Drupal specific custom attributes for a person.
   *
   * @param string $entity_type
   *   The entity type for which to prepare the Drupal site and entity specific
   *   custom attributes.
   * @param EntityMetadataWrapper $wrapper
   *   The entity EMW to use for preparing the Drupal site and entity specific
   *   custom attributes.
   * @param array $person_configuration
   *   The person configuration to which configurations should be appended.
   *
   * @return array
   *   The person configuration containing the Drupal site and entity specific
   *   custom attributes.
   */
  protected function preparePersonConfigurationDrupalInfo($entity_type, EntityMetadataWrapper $wrapper, &$person_configuration) {
    $person_configuration['id'] = $this->getKlaviyoIdentifier($entity_type, $wrapper->getIdentifier());
    $person_configuration['drupal.site_id'] = $this->getSiteId();
    $person_configuration['drupal.entity_type'] = $entity_type;
    $person_configuration['drupal.entity_bundle'] = $wrapper->getBundle();
    $person_configuration['drupal.entity_id'] = $wrapper->getIdentifier();

    return $person_configuration;
  }

  /**
   * Prepare the Drupal Account entity custom attributes for a person.
   *
   * @param EntityMetadataWrapper $wrapper
   *   The account EMW to use for preparing the account entity specific custom
   *   attributes.
   * @param array $person_configuration
   *   The person configuration to which configurations should be appended.
   *
   * @return array
   *   The person configuration containing the Account specific custom
   *   attributes.
   */
  protected function preparePersonConfigurationFromAccount(EntityMetadataWrapper $wrapper, &$person_configuration) {
    if (empty($person_configuration['$email'])) {
      $person_configuration['$email'] = $wrapper->mail->value();
    }
    if (empty($person_configuration['$first_name'])) {
      $person_configuration['$first_name'] = $wrapper->label();
    }

    return $person_configuration;
  }

  /**
   * Determine if Klaviyo integration has been enabled for the entity bundle.
   *
   * @param string $entity_type
   *   The entity type of the bundle to check.
   * @param string $bundle
   *   (Optional) The bundle to check.
   *
   * @return bool
   *   Returns TRUE if Klaviyo integration has been enabled for the specified
   *   entity bundle.
   */
  public function isEnabledOnEntity($entity_type, $bundle = '') {
    $settings = $this->getAllEntitySettings($entity_type, $bundle);
    return $this->isCompatiableEntity($entity_type, $bundle) && !empty($settings['enabled']);
  }

  /**
   * Retrieve the Klaviyo integration settings for the specified entity bundle.
   *
   * @param string $entity_type
   *   The entity type of the bundle to retrieve the settings.
   * @param string $bundle
   *   (Optional) The bundle to retreive the settings.
   *
   * @return array
   *   The Klaviyo settings associated with the Drupal entity.
   */
  public function getAllEntitySettings($entity_type, $bundle = '') {
    $variable_name = "klaviyo_entity_settings_$entity_type";

    if ($bundle) {
      $variable_name .= $bundle;
    }

    return variable_get($variable_name, array('enabled' => FALSE, 'settings' => array('list' => '')));
  }

  /**
   * Determine if the entity bundle is compatiable with Klaviyo.
   *
   * @param string $entity_type
   *   The entity type of the bundle to check for compatibility.
   * @param string $bundle
   *   (Optional) The bundle to check for compatibility.
   *
   * @return bool
   *   Returns TRUE if the specified entity is compatiable with Klaviyo
   *   integration and FALSE otherwise.
   */
  public function isCompatiableEntity($entity_type, $bundle = '') {
    return $entity_type === 'user';
  }

  /**
   * Determine if the field type is mappable to Klaviyo attributes.
   *
   * @param string $entity_type
   *   The entity type of the bundle to check if Klaviyo integration has been
   *   enabled.
   * @param string $field_type
   *   The field type to check.
   * @param string $bundle
   *   (Optional) The bundle to check if Klaviyo integration has been
   *   enabled.
   *
   * @return bool
   *   Returns TRUE if the field type for the specified entity bundle can be
   *   mapped to Klaviyo attributes and FALSE otherwise.
   */
  public function isFieldMappable($entity_type, $field_type, $bundle = '') {
    $allowable_field_types = array('text', 'list_text');

    return ($this->isEnabledOnEntity($entity_type, $bundle) && in_array($field_type, $allowable_field_types));
  }

  /**
   * Determine if the field has been mapped to a Klaviyo attribute.
   *
   * @param string $entity_type
   *   The entity type of the bundle to check if Klaviyo integration has been
   *   enabled.
   * @param string $field_type
   *   The name of the field to check.
   * @param string $bundle
   *   (Optional) The bundle to check if Klaviyo integration has been
   *   enabled.
   *
   * @return bool
   *   Returns TRUE if the field for the specified entity bundle has be mapped
   *   to Klaviyo attributes and FALSE otherwise.
   */
  public function isFieldMapped($entity_type, $field_name, $bundle = '') {
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

  /**
   * Write person to Klviyo based on mapped Drupal Entity.
   *
   * @param string $entity_type
   *   The type of the entity for which to prepare the person configuration.
   * @param EntityMetdataWrapper $wrapper
   *   The EMW for the entity to use for populating attributes of the person
   *   configuration.
   *
   * @return bool
   *   Returns TRUE if the person was written to Klaviyo and FALSE otherwise.
   */
  public function savePersonFromEntity($entity_type, EntityMetadataWrapper $wrapper) {
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

  /**
   * Retrieve an array of field options for mappable Klaviyo attributes.
   *
   * @param string $entity_type
   *   The entity type for which to retrieve the mapping options.
   * @param string $field_name
   *   The current field name to retrieve the attribute options.
   * @param string $bundle
   *   (Optional) The bundle for which to retrieve the mapping options.
   *
   * @return array
   *   An array of form options for the available mappable attribute options.
   */
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

  /**
   * Retrieve the Klaviyo id associated with a person.
   *
   * @param string $entity_type
   *   The entity type to use for determining which list to perform the lookup
   *   against.
   * @param string $email
   *   The email of the person to be looked up.
   * @param string $bundle
   *   (Optional) The entity bundle to use for determining which list to
   *   perform the lookup against.
   *
   * @return string
   *   The unique Klaviyo identifier of the person looked up.
   */
  public function lookupKalviyoIdByEmail($entity_type, $email, $bundle = '') {
    $klaviyo_id = '';
    $settings = $this->getAllEntitySettings($entity_type, $bundle);

    if (!empty($settings['settings']['list']) && $list_id_full = $settings['settings']['list']) {
      list(, $list_id) = $this->parseFullListId($list_id_full);
      if ($person_list = $this->api->lookUpPersonFromList($list_id, $email)) {
        $entity_id = $person_list->person->getCustomAttribute('drupal.entity_id');

        if (!empty($entity_id)) {
          $klaviyo_id = $this->getKlaviyoIdentifier($entity_type, $entity_id);
        }
        else {
          $klaviyo_id = $this->person->id;
        }
      }
    }

    return $klaviyo_id;
  }

  /**
   * Prevent cloning of the instance of the Klaviyo instance.
   *
   * @return void
   */
  private function __clone() {}

  /**
   * Prevent cloning of the unserializing of the Klaviyo instance.
   *
   * @return void
   */
  private function __wakeup() {}

}
