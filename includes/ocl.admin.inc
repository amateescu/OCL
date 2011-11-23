<?php

/**
 * UI controller.
 */
class OclUIController extends EntityDefaultUIController {

  /**
   * Overrides EntityDefaultUIController::hook_menu().
   */
  public function hook_menu() {
    $items = parent::hook_menu();

    $id_count = count(explode('/', $this->path));
    $wildcard = isset($this->entityInfo['admin ui']['menu wildcard']) ? $this->entityInfo['admin ui']['menu wildcard'] : '%entity_object';
    $plural_label = isset($this->entityInfo['plural label']) ? $this->entityInfo['plural label'] : $this->entityInfo['label'] . 's';

    $items[$this->path]['description'] = 'Manage orderable content lists.';

    $items[$this->path . '/lists'] = array(
      'title' => $plural_label,
      'type' => MENU_DEFAULT_LOCAL_TASK,
      'weight' => -10,
    );

    $items[$this->path . '/types'] = array(
      'title' => $this->entityInfo['label'] . ' types',
      'page callback' => 'ocl_overview_types',
      'page arguments' => array($this->entityType),
      'access arguments' => array('administer list types'),
      'type' => MENU_LOCAL_TASK,
      'file' => $this->entityInfo['admin ui']['file'],
      'file path' => drupal_get_path('module', $this->entityInfo['module']),
    );
    $items[$this->path . '/types/manage/%'] = array(
      'title' => 'Manage list types',
      'description' => 'Manage settings for the type of lists used on your site.',
    );

    // View form, a special case for the overview form that now shows
    // the sublists.
    $items[$this->path . '/manage/' . $wildcard . '/view'] = array(
      'title callback' => 'ocl_get_page_title',
      'title arguments' => array('view', $this->entityType, $id_count + 1),
      'page callback' => 'drupal_get_form',
      'page arguments' => array($this->entityType . '_overview_form', $this->entityType, $id_count + 1),
      'load arguments' => array($this->entityType),
      'access callback' => 'entity_access',
      'access arguments' => array('view', $this->entityType),
      'file' => $this->entityInfo['admin ui']['file'],
      'file path' => drupal_get_path('module', $this->entityInfo['module']),
    );

    // Configure form, a special form for editing the properties of a list.
    // Fields are handled in the regular edit form.
    $items[$this->path . '/manage/' . $wildcard . '/configure'] = array(
      'title' => 'Configure',
      'page callback' => 'entity_ui_get_form',
      'page arguments' => array($this->entityType, $id_count + 1, 'configure'),
      'load arguments' => array($this->entityType),
      'access callback' => 'entity_access',
      'access arguments' => array('edit', $this->entityType),
      'file' => $this->entityInfo['admin ui']['file'],
      'file path' => drupal_get_path('module', $this->entityInfo['module']),
    );

    return $items;
  }

  /**
   * Overrides EntityDefaultUIController::hook_forms().
   */
  public function hook_forms() {
    $forms = parent::hook_forms();

    $forms[$this->entityType . '_overview_form'] = array(
      'callback' => $this->entityType . '_overview_form',
      'wrapper_callback' => 'entity_ui_form_defaults',
    );

    return $forms;
  }

  /**
   * Overrides EntityDefaultUIController::hook_forms().
   */
  public function overviewForm($form, &$form_state) {
    $conditions = array();
    if (isset($form_state[$this->entityType])) {
      $conditions = array(
        'parent_name' => $form_state[$this->entityType]->name,
      );
    }
    $form['table'] = $this->overviewTable($conditions);
    return $form;
  }

  /**
   * Overrides EntityDefaultUIController::overviewTable().
   */
  public function overviewTable($conditions = array()) {
    $entities = ocl_overview_lists($conditions);
    ksort($entities);

    $rows = array();
    foreach ($entities as $entity) {
      $rows[] = $this->overviewTableRow($conditions, entity_id($this->entityType, $entity), $entity);
    }
    // Assemble the right table header.
    $header = array(t('Label'));
    $header[] = t('Type');
    $colspan = 5;
    $header[] = array('data' => t('Operations'), 'colspan' => $colspan);

    $render = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
      '#empty' => t('None.'),
    );
    return $render;
  }

  /**
   * Overrides EntityDefaultUIController::overviewTableRow().
   */
  protected function overviewTableRow($conditions, $name, $entity, $additional_cols = array()) {
    $entity_uri = entity_uri($this->entityType, $entity);

    $row[] = array('data' => array(
      '#theme' => 'ocl_overview_item',
      '#label' => entity_label($this->entityType, $entity),
      '#name' => !empty($this->entityInfo['exportable']) ? entity_id($this->entityType, $entity) : FALSE,
      '#status' => $entity->{$this->statusKey},
    ));

    // Add in any passed additional cols.
    foreach ($additional_cols as $col) {
      $row[] = $col;
    }

    // Add a row for the exportable status.
    $types = ocl_type_get_name();
    $row[] = $types[$entity->type];

    // Add operations depending on the status.
    if (entity_has_status($this->entityType, $entity, ENTITY_FIXED)) {
      $row[] = array('data' => l(t('clone'), $this->path . '/manage/' . $name . '/clone'), 'colspan' => 5);
    }
    else {
      if ($entity->parent_name == NULL && $entity->sublist == 0) {
        $row[] = l(t('view sublists'), $this->path . '/manage/' . $name . '/view');
      }
      else {
        $row[] = l(t('edit items'), $this->path . '/manage/' . $name);
      }
      $row[] = l(t('configure'), $this->path . '/manage/' . $name . '/configure');
      $row[] = l(t('clone'), $this->path . '/manage/' . $name . '/clone');
      if (empty($this->entityInfo['exportable']) || !entity_has_status($this->entityType, $entity, ENTITY_IN_CODE)) {
        $row[] = l(t('delete'), $this->path . '/manage/' . $name . '/delete', array('query' => drupal_get_destination()));
      }
      elseif (entity_has_status($this->entityType, $entity, ENTITY_OVERRIDDEN)) {
        $row[] = l(t('revert'), $this->path . '/manage/' . $name . '/revert', array('query' => drupal_get_destination()));
      }
      else {
        $row[] = '';
      }
    }
    $row[] = l(t('export'), $this->path . '/manage/' . $name . '/export');
    return $row;
  }

  /**
   * Generates the render array for a overview table of list types (bundles).
   *
   * @return Array
   *   A renderable array.
   */
  public function overviewTypesTable() {
    drupal_set_title(t('@label types', array('@label' => $this->entityInfo['label'])));

    $types = ocl_type_get_name();
    $field_ui = module_exists('field_ui');

    $header = array(t('Name'));
    if ($field_ui) {
      $header[] = array('data' => t('Operations'), 'colspan' => '2');
    }

    $rows = array();
    foreach ($types as $type => $name) {
      $row = array($name);

      if ($field_ui) {
        $row[] = array('data' => l(t('manage fields'), $this->path . '/types/manage/' . $type . '/fields'));
        $row[] = array('data' => l(t('manage display'), $this->path . '/types/manage/' . $type . '/display'));
      }

      $rows[] = $row;
    }

    $render = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
      '#empty' => t('None.'),
    );
    return $render;
  }
}

/**
 * Form builder function for the overview form.
 *
 * @see EntityDefaultUIController::overviewForm()
 */
function ocl_overview_form($form, &$form_state, $entity_type, $entity = NULL) {
  // Store the entity in $form_state so we're able to get its sublists in the
  // overview table.
  if ($entity) {
    $form_state[$entity_type] = $entity;
  }
  return entity_ui_controller($entity_type)->overviewForm($form, $form_state);
}

/**
 * Gets the page title for the passed operation.
 */
function ocl_get_page_title($op, $entity_type, $entity = NULL) {
  $label = entity_label($entity_type, $entity);
  switch ($op) {
    case 'view':
      return t('@label', array('@label' => $label));
  }
  return '';
}

/**
 * Generates the list add/configure/clone form.
 */
function ocl_form($form, &$form_state, $list, $op = 'edit') {
  global $user;

  // We're using a different form the 'edit' operation because it only provides
  // access to entity fields.
  if ($op == 'edit') {
    return ocl_edit_form($form, $form_state, $list);
  }

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

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 40,
  );
  return $form;
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

/**
 * Generates the list edit form.
 */
function ocl_edit_form($form, &$form_state, $list) {
  field_attach_form('ocl', $list, $form, $form_state);

  // Entity type (bundle) is needed by entity_form_submit_build_entity().
  $form['type'] = array(
    '#type' => 'value',
    '#default_value' => $list->type,
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 40,
  );

  $form['#validate'][] = 'ocl_edit_form_validate';
  $form['#submit'][] = 'ocl_form_submit';

  return $form;
}

/**
 * Validation callback.
 */
function ocl_edit_form_validate($form, &$form_state) {
  entity_form_field_validate('ocl', $form, $form_state);
}

function ocl_overview_lists($conditions = array()) {
  // Set some defaults for the $conditions array.
  $conditions += array(
    'parent_name' => NULL,
  );
  return ocl_load_multiple(array(), $conditions);
}

function ocl_overview_sublists($list) {
  
}

/**
 * Displays the list types overview page.
 *
 * @see OclUIController::overviewTypesTable()
 */
function ocl_overview_types($entity_type) {
  return entity_ui_controller($entity_type)->overviewTypesTable();
}

/**
 * Returns HTML for a list on the entity overview listing.
 *
 * @ingroup themeable
 */
function theme_ocl_overview_item($variables) {
  $output = '<strong>' . check_plain($variables['label']) . '</strong>';
  if ($variables['name']) {
    $output .= ' <small> (' . t('Machine name') . ': ' . check_plain($variables['name']) . ')</small>';
  }
  $output = '<div class="ocl-title">' . $output . '</div>';
  if ($variables['status']) {
    $output .= '<div class="ocl-status">' . theme('ocl_status', array('status' => $variables['status'], TRUE)) . '</div>';
  }
  return $output;
}

/**
 * Themes the exportable status of a list.
 */
function theme_ocl_status($variables) {
  $status = $variables['status'];
  $html = $variables['html'];
  if (($status & ENTITY_FIXED) == ENTITY_FIXED) {
    $label = t('Fixed');
    $help = t('The configuration is fixed and cannot be changed.');
    return $html ? "<span class='ocl-status-fixed' title='$help'>" . $label . "</span>" : $label;
  }
  elseif (($status & ENTITY_OVERRIDDEN) == ENTITY_OVERRIDDEN) {
    $label = t('Database overriding code');
    $help = t('This configuration is provided by a module, but has been changed.');
    return $html ? "<span class='ocl-status-overridden' title='$help'>" . $label . "</span>" : $label;
  }
  elseif ($status & ENTITY_IN_CODE) {
    $label = t('In code');
    $help = t('A module provides this configuration.');
    return $html ? "<span class='ocl-status-default' title='$help'>" . $label . "</span>" : $label;
  }
  elseif ($status & ENTITY_CUSTOM) {
    $label = t('In database');
    $help = t('A custom configuration by a user.');
    return $html ? "<span class='ocl-status-custom' title='$help'>" . $label . "</span>" : $label;
  }
}
