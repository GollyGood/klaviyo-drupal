<?php

// @todo: Get all this Klaviyo API specific stuff out of here!
require_once DRUPAL_ROOT . '/sites/all/libraries/klaviyo-api-php/vendor/autoload.php';

class KlaviyoAdapter {

  public $klaviyo;

  public function __construct($api_key) {
    $this->klaviyo = Klaviyo\KlaviyoFacade::create($api_key);
  }

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


  public function getPersonGetMappableKeys() {
    $model_class = $this->klaviyo->getModelClass('person');
    return array_filter(call_user_func("$model_class::getAttributeKeys"), function($attribute_key) {
      return !($attribute_key === 'id' || $attribute_key === 'object');
    });
  }

  public function subscribePersonToList(PersonModel $person, ListModel $list) {
    if (!empty($list)) {
      try {
        $this->klaviyo->service('lists')->addPersonToList($person, $list);
      }
      catch (Exception $e) {
        watchdog_exception('klaviyo', $e);
      }
    }
  }

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

  public function savePerson($person_configuration) {
    $successful = FALSE;

    try {
      $person = $this->klaviyo->model($person_configuration, 'person');
      $this->klaviyo->service('track')->identify($person);
      $successful = TRUE;
    }
    catch(Exception $e) {
      watchdog_exception('klaviyo', $e);
    }

    return $successful;
  }

  public function __clone() {}
  public function __wakeup() {}

}
