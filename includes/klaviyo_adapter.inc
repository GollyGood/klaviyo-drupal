<?php

/**
 * @file
 * Contains the KlaviyoAdapter class.
 */

/**
 * The Klaviyo API PHP Library adapter for Drupal.
 */
class KlaviyoAdapter {

  /**
   * The Klaviyo API PHP Library facade.
   *
   * @var KlaviyoFacade
   */
  public $klaviyo;

  /**
   * Construct a new Klaviyo adapter.
   *
   * @param string $api_key
   *   The Klaviyo API key to use when creating the Klaviyo api service.
   */
  public function __construct($api_key) {
    $this->klaviyo = Klaviyo\KlaviyoFacade::create($api_key);
  }

  /**
   * Retrieve all lists.
   *
   * @return array $lists
   *   An array of ListModels that represent all lists and segments present.
   */
  public function getLists() {
    $lists = array();

    try {
      $lists = $this->klaviyo->service('lists')->getAllLists();
    }
    catch (Exception $e) {
      watchdog_exception('klaviyo', $e);
      drupal_set_message('Unable to retrieve lists from Klaviyo. Please try again later.', 'error');
    }

    return $lists;
  }

  /**
   * Create a new list.
   *
   * @param string $list_name
   *   The name to use for the list.
   *
   * @return ListModel
   *   The ListModel that represents the newly created list.
   */
  public function createList($list_name) {
    $list = NULL;
    try {
      $list = $this->klaviyo->service('lists')->createList($list_name);

      cache_clear_all('klaviyo:list_options', 'cache');
    }
    catch (Exception $e) {
      watchdog_exception('klaviyo', $e);
      drupal_set_message('Unable create list. Please try again later.', 'error');
    }

    return $list;
  }

  /**
   * Retrieve an array of all mappable attributes for a Klaviyo person record.
   *
   * @return array
   *   An array of all attribute keys that are mappable for a Klaviyo person
   *   record.
   */
  public function getPersonGetMappableKeys() {
    $model_class = $this->klaviyo->getModelClass('person');
    return array_filter(call_user_func("$model_class::getAttributeKeys"), function($attribute_key) {
      return !($attribute_key === 'id' || $attribute_key === 'object');
    });
  }

  /**
   * Subscribe a person to an existing list.
   *
   * @param PersonModel $person
   *   The person that should be added to the existing list.
   * @param ListModel $list
   *   The list model to which the person should be added.
   *
   * @return $this
   */
  public function subscribePersonToList(PersonModel $person, ListModel $list) {
    if (!empty($list)) {
      try {
        $this->klaviyo->service('lists')->addPersonToList($person, $list);
      }
      catch (Exception $e) {
        watchdog_exception('klaviyo', $e);
      }

      return $this;
    }
  }

  /**
   * Look up an individual person from a specified list.
   *
   * @param string $list_id
   *   The id of the list to use when looking up the person.
   * @param string $email
   *   The email of the person to lookup.
   *
   * @return PersonListModel
   *   The references of the person that was added and the list to which the
   *   user was added.
   */
  public function lookUpPersonFromList($list_id, $email) {
    $person_list = array();

    try {
      $lists_service = $this->klaviyo->service('lists');
      $list = $lists_service->getList($list_id);
      $person_list = $this->klaviyo->service('lists')->checkMembersAreInList($list, [$email]);
    }
    catch (Exception $e) {
      watchdog_exception('klaviyo', $e);
    }

    return reset($person_list);
  }

  /**
   * Save the person configuration to Klaviyo.
   *
   * @return bool
   *   Returns TRUE if the person was written to Klaviyo and FALSE otherwise.
   */
  public function savePerson($person_configuration) {
    $successful = FALSE;

    try {
      $person = $this->klaviyo->model($person_configuration, 'person');
      $this->klaviyo->service('track')->identify($person);
      $successful = TRUE;
    }
    catch (Exception $e) {
      watchdog_exception('klaviyo', $e);
    }

    return $successful;
  }

  /**
   * Check if the specified attribute is a person custom attribute.
   *
   * @param string $attribute_key
   *   The attribute key for which to check is a custom attribute.
   *
   * @return bool
   *    Returns TRUE if the specified attribute is a custom attribute and FALSE
   *    otherwise.
   */
  public function personAttributeIsCustom($attribute_key) {
    return call_user_func("{$this->klaviyo->getModelClass('person')}::isCustomAttributeKey", $attribute_key);
  }

}
