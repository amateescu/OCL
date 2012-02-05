<?php

class EntityReferenceInstanceBehaviorEntityQueue extends EntityReference_BehaviorHandler_Abstract {

  public function load($entity_type, $entities, $field, $instances, $langcode, &$items) {
    drupal_set_message('Do something on load, on the instance level!');
  }

  public function insert($entity_type, $entity, $field, $instance, $langcode, &$items) {
    drupal_set_message('Do something on insert, on the instance level!');
  }

  public function update($entity_type, $entity, $field, $instance, $langcode, &$items) {
    drupal_set_message('Do something on update, on the instance level!');
  }

  public function delete($entity_type, $entity, $field, $instance, $langcode, &$items) {
    drupal_set_message('Do something on delete, on the instance level!');
  }

  /**
   * Generate a settings form for this handler.
   */
  public function settingsForm() {
    $form['entityqueue_instance'] = array(
      '#type' => 'checkbox',
      '#title' => t('Entityqueue behavoir setting'),
    );
    return $form;
  }
}
