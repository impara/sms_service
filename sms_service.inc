<?php

/**
 * @file
 * Implements missing alma for checking status on which alerts the user is registered to.
 * 
 */

/**
 * missing define in alma client
 */
define('ALMA_SERVICE_METHOD_EMAIL', 'email');

/**
 * Instead of copying alma to sites/all and maintain it,
 * we defind our own methods to implement missing alma integration
 */
class Smsservice extends AlmaClient {

  /**
   * Implements to check messages service in patron
   * (not implemented in original alma module
   * only implemented add and remove messages).
   */
  function GetPatronInfo($borr_card, $pin_code, $extended = FALSE) {
    $path = ($extended) ? 'patron/informationExtended' : 'patron/information';
    $info_node = ($extended) ? 'patronInformationExtended' : 'patronInformation';

    $doc = $this->request($path, array('borrCard' => $borr_card, 'pinCode' => $pin_code));

    $info = $doc->getElementsByTagName($info_node)->item(0);

    $data = array(
      'sendMethods' => array(),
    );
    // The missing part.
    foreach ($info->getElementsByTagName('sendMethod') as $type) {
      $data['sendMethods'][] = array(
        'value' => $type->getAttribute('value'),
      );
    }

    return $data;
  }

}

/**
 * New client
 */
function sms_service_client() {
  // This is basically a singleton. We also wait until right before
  // instantiating to include our required classes. That seems like a
  // decent performance tradeoff instead of loading everything on every
  // page load.
  static $client;
  if (!isset($client)) {
    $path = drupal_get_path('module', 'alma');
    try {
      $client = new Smsservice(variable_get('alma_base_url', ''));
    } catch (Exception $e) {
      watchdog('alma', 'Constructor error: “@message”', array('@message' => $e->getMessage(), WATCHDOG_ERROR));
      return NULL;
    }
  }
  return $client;
}

/**
 * New client to invoke custom changes
 */
function sms_service_client_invoke($method) {
  $args = func_get_args();
  // Lose the method.
  array_shift($args);
  $client = sms_service_client();

  try {
    $result = call_user_func_array(array($client, $method), $args);
  } catch (Exception $e) {
    watchdog('alma', '@method error: “@message”', array('@method' => $method, '@message' => $e->getMessage()), WATCHDOG_ERROR);
    throw $e;
  }

  return $result;
}

/**
 * messages/sendMethods for patron
 */
function sms_service_get_patron($creds = NULL, $reset = FALSE, $as_array = FALSE) {
  if (is_null($creds)) {
    // Get creds, which may throw an exception that login is required.
    global $user;
    $creds = ding_user_get_creds($user);
  }

  static $patron;
  if (!$patron || $reset) {
    $info = sms_service_client_invoke('GetPatronInfo', $creds['name'], $creds['pass'], TRUE);
    $patron = array(
      // The missing part.
      'messages' => isset($info['sendMethods']) ? $info['sendMethods'] : '',
    );
  }
  if ($as_array) {
    return $patron;
  }

  return (object) $patron;
}
