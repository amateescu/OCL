<?php

/**
 * UI controller.
 */
class OclUIController extends EntityDefaultUIController {

  /**
   * Overrides hook_menu() defaults.
   */
  public function hook_menu() {
    $items = parent::hook_menu();
    $items[$this->path]['description'] = 'Manage orderable content lists.';
    return $items;
  }
}

/**
 * Generates the list editing form.
 */
function ocl_form($form, &$form_state, $list, $op = 'edit') {
  global $user;

  if ($op == 'clone') {
    $list->label .= ' (cloned)';
    $list->name .= '_clone';
    $list->status = ENTITY_CUSTOM;
  }

  $form['label'] = array(
    '#title' => t('Label'),
    '#type' => 'textfield',
    '#default_value' => $list->label,
  );

  // Machine-readable list name.
  $form['name'] = array(
    '#type' => 'machine_name',
    '#default_value' => isset($list->name) ? $list->name : '',
    '#disabled' => $list->isLocked(),
    '#machine_name' => array(
      'exists' => 'ocl_load',
      'source' => array('label'),
    ),
    '#description' => t('A unique machine-readable name for this list. It must only contain lowercase letters, numbers, and underscores.'),
  );

  $form['sublist_label'] = array(
    '#title' => t('Sublist label'),
    '#type' => 'textfield',
    '#default_value' => $list->sublist_label,
  );

  $form['module'] = array(
    '#type' => 'hidden',
    '#value' => isset($list->module) ? 'ocl' : '',
  );

  $form['uid'] = array(
    '#type' => 'value',
    '#value' => $user->uid,
  );

  $types = ocl_type_get_name();
  $form['type'] = array(
    '#title' => t('Type'),
    '#type' => 'select',
    '#options' => $types,
    '#default_value' => $list->type,
  );

  if (!isset($list->is_new)) {
    field_attach_form('ocl', $list, $form, $form_state);
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save list'),
    '#weight' => 40,
  );
  return $form;
}

/**
 * Validation callback.
 */
function ocl_form_validate($form, &$form_state) {
  entity_form_field_validate('ocl', $form, $form_state);
}

/**
 * Form API submit callback for the list form.
 */
function ocl_form_submit(&$form, &$form_state) {
  $list = entity_ui_form_submit_build_entity($form, $form_state);
  // Save and go back.
  $list->save();
  $form_state['redirect'] = 'admin/structure/ocl';
}
