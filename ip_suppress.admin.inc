<?php
/**
 * Created by PhpStorm.
 * User: michaellander
 * Date: 2/24/15
 * Time: 3:24 PM
 */


/**
 * @param $form
 * @param $form_state
 * @return mixed
 */
function ip_suppress_settings_form($form, &$form_state) {

  $form['ip_suppress_redirect'] = array(
    '#type' => 'textfield',
    '#title' => t('Redirect path'),
    '#required' => TRUE,
    '#description' => t('Use <front> for front page. This path will override Whitelist/Blacklist'),
    '#default_value' => variable_get('ip_suppress_redirect')
  );
  $options = array(
    IP_SUPPRESS_WHITELIST => t('Only the listed pages'),
    IP_SUPPRESS_BLACKLIST => t('All pages except those listed'),
  );

 $form['ip_suppress_visibility'] = array(
    '#type' => 'radios',
    '#title' => t('Allow access to specific pages'),
    '#options' => $options,
    '#default_value' => variable_get('ip_suppress_visibility', IP_SUPPRESS_WHITELIST),
  );

  $description = t("Specify pages by using their paths. Enter one path per line. The '*' character is a wildcard. Example paths are %blog for the blog page and %blog-wildcard for every personal blog. %front is the front page.", array('%blog' => 'blog', '%blog-wildcard' => 'blog/*', '%front' => '<front>'));
  $form['ip_suppress_pages'] = array(
    '#type' => 'textarea',
    '#title' => t('Pages'),
    '#default_value' => variable_get('ip_suppress_pages', "user\nuser/*"),
    '#description' => $description,
  );
  //@todo: allow for custom message
  return system_settings_form($form);
}

/**
 * Menu callback. Display suppressed IP addresses.
 *
 * @param $default_ip
 *   Optional IP address to be passed on to drupal_get_form() for
 *   use as the default value of the IP address form field.
 */
function ip_suppress_list($default_ip = '') {
  $rows = array();
  $header = array(t('Suppressed IP addresses'), t('Operations'));
  $result = db_query('SELECT * FROM {ip_suppress}');
  foreach ($result as $ip) {
    $rows[] = array(
      $ip->ip,
      l(t('delete'), "admin/config/people/ip-blocking/delete/$ip->iid"),
    );
  }

  $build['ip_suppress_suppress_form'] = drupal_get_form('ip_suppress_suppress_form', $default_ip);

  $build['ip_suppress_suppress_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No suppressed IP addresses available.'),
  );

  return $build;
}

/**
 * Define the form for blocking IP addresses.
 *
 * @ingroup forms
 * @see ip_suppress_suppress_form_validate()
 * @see ip_suppress_suppress_form_submit()
 */
function ip_suppress_suppress_form($form, $form_state, $default_ip) {
  $form['ip'] = array(
    '#title' => t('IP address'),
    '#type' => 'textfield',
    '#size' => 48,
    '#maxlength' => 40,
    '#default_value' => $default_ip,
    '#description' => t('Enter a valid IP address.'),
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add'),
  );
  $form['#submit'][] = 'ip_suppress_suppress_form_submit';
  $form['#validate'][] = 'ip_suppress_suppress_form_validate';
  return $form;
}

function ip_suppress_suppress_form_validate($form, &$form_state) {
  $ip = trim($form_state['values']['ip']);
  if (ip_suppress_ip_load_by_ip($ip)) {
    form_set_error('ip', t('This IP address is already suppressed.'));
  }
  elseif ($ip == ip_address()) {
    form_set_error('ip', t('You may not suppress your own IP address.'));
  }
  elseif (filter_var($ip, FILTER_VALIDATE_IP, FILTER_FLAG_NO_RES_RANGE) == FALSE) {
    form_set_error('ip', t('Enter a valid IP address.'));
  }
}

function ip_suppress_suppress_form_submit($form, &$form_state) {
  $ip = trim($form_state['values']['ip']);
  ip_suppress_ip_insert($ip);
  drupal_set_message(t('The IP address %ip has been suppressed.', array('%ip' => $ip)));
  $form_state['redirect'] = 'admin/config/people/ip-suppress/list';
  return;
}

/**
 * IP deletion confirm page.
 *
 * @see ip_suppress_suppress_delete_submit()
 */
function ip_suppress_suppress_delete($form, &$form_state, $iid) {
  $form['suppressed_ip'] = array(
    '#type' => 'value',
    '#value' => $iid,
  );
  return confirm_form($form, t('Are you sure you want to delete %ip?', array('%ip' => $iid['ip'])), 'admin/config/people/ip-blocking', t('This action cannot be undone.'), t('Delete'), t('Cancel'));
}

/**
 * Process ip_suppress_suppress_delete form submissions.
 */
function ip_suppress_suppress_delete_submit($form, &$form_state) {
  $suppressed_ip = $form_state['values']['suppressed_ip'];
  ip_suppress_ip_delete($suppressed_ip['iid']);
  watchdog('user', 'Deleted %ip', array('%ip' => $suppressed_ip['ip']));
  drupal_set_message(t('The IP address %ip was deleted.', array('%ip' => $suppressed_ip['ip'])));
  $form_state['redirect'] = 'admin/config/people/ip-blocking';
}