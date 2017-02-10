<?php

/**
 * @file
 * Stores the code for export process.
 */

/**
 * Gets the list of  Content Type for Content type dropdown.
 */
function metatag_import_export_csv_type_list() {
  $typedata = node_type_get_types();
  foreach ($typedata as $values1) {
    $type_machine_name = $values1->type;
    $type_human_name = t("@typename", array('@typename' => $values1->name));
    $func_content_type[$type_machine_name] = $type_human_name;
  }
  $default_type = array('all' => 'All');
  $content_type = array_merge($default_type, $func_content_type);
  return $content_type;
}

/**
 * Form for exporting Meta tags.
 */
function metatag_import_export_csv_download_form($form, &$form_state) {
  $content_type = metatag_import_export_csv_type_list();
  $form['content_type'] = array(
    '#type' => 'select',
    '#options' => $content_type,
    '#title' => t("Content type"),
    '#validated' => TRUE,
    '#required' => TRUE,
    '#default_value' => $content_type['all'],
    '#description' => t("Select the Content type"),
  );
  $form['delimiter'] = array(
    '#type' => 'textfield',
    '#title' => t('Delimiter'),
    '#description' => t('Add your delimiter.'),
    '#default_value' => ',',
    '#maxlength' => 2,
   );
  $form['export'] = array(
    '#type' => 'submit',
    '#value' => t("Export"),
    '#submit' => array('metatag_import_export_csv_download_submit'),
  );
  return $form;
}

/**
 * Submit Handler for Export Operation.
 */
function metatag_import_export_csv_download_submit($form, &$form_state) {
  $type = $form_state['values']['content_type'];
  $delimeter= $form_state['values']['delimiter'];
   
  metatag_import_export_csv_download_nid($type,$delimeter);
}

/**
 * Gets the list of nid's according to content type.
 */
function metatag_import_export_csv_download_nid($type, $delimeter) {
    
    $file_path = "public://metatag_import_export_csv/";
  $filename = "metatag_import_export_csv_" . $type . ".csv";
  
  if (!file_prepare_directory($file_path, FILE_CREATE_DIRECTORY)) {
    drupal_set_message(t('Unable to create directory in file system. Check permissions and try again.'), 'error');
    return;
  }
  $type_name = $type;
  $query = db_select('node', 'n')
      ->fields('n', array('nid'));
  if ($type != 'all') {
    $query->condition('type', $type);
  }
  elseif ($type = 'all') {
    $type = metatag_import_export_csv_type_list();
    $type = array_keys($type);
    $query->condition('type', ($type), 'IN');
  }
  $results = $query->execute()->fetchAll();
  //batch code
  $batch = array(
    'operations' => array(),
    'finished' => 'metatag_import_export_csv_finish_batch_download',
    'title' => t("Processing Nodes for exporting Metatags"),
    'init_message' => t("Starting Metatag export Process"),
    'progress_message' => t("Processed @current out of @total Nodes"),
    'file' => drupal_get_path('module', 'metatag_import_export_csv') . '/metatag_import_export_csv_download_admin.inc',
    'error_message' => t("Metatag import process has encountered an error."),
  );
  //batch code ends
  
  foreach ($results as $values) {
      $count++;
      $batch['operations'][] = array('metatag_import_export_csv_start_batch_upload', array(
          $values->nid,
          $type,
          $delimeter,
      //$count,
    ),
    );
  }
    batch_set($batch);
    if (drupal_is_cli()) {
    $batch = & batch_get();
    $batch['progressive'] = FALSE;
    drush_backend_batch_process();
  }
  else {
    batch_process('admin/config/search/metatags/export');
  }
  //metatag_import_export_csv_process_csv($nid_list, $type_name, $delimeter);
}


function metatag_import_export_csv_finish_batch_download($success, $results, $operations)
{
    if ($success) {
        $type=$results['type'];
        $delimeter=$results['delimeter'];
    drupal_set_message(t("Finished Importing Metatags"));
     $file_path = "public://metatag_import_export_csv/";
  $filename = "metatag_import_export_csv_" . $type . ".csv";
  $file_url = $file_path . $filename;
  drupal_add_http_header('Content-Type', 'text/csv');
  drupal_add_http_header('Content-Disposition', 'attachment;filename=' . $filename);
  $file = file_save_data('', $file_url, FILE_EXISTS_REPLACE);
  if (!($file)) {
    drupal_set_message(t('Unable to write to file system. Check permissions and try again.'), 'error');
    return;
  }
  $fp = fopen($file->uri, 'w');
  if (!$fp) {
    drupal_set_message(t('Unable open file for writing. Check permissions and try again.'), 'error');
    return;
  }
  $header = array(
    'Node Title',
    'Path',
    ' Meta Title',
    'Meta Description',
    'Meta Abstract',
    ' Meta Keywords',
  );
  fputcsv($fp, $header, $delimeter);
  foreach($results['row'] as $row)
  {
   fputcsv($fp, $row, $delimeter);   
  }
  fclose($fp);
  $file->filesize = filesize($file->uri);
  file_save($file);
  if (drupal_is_cli()) {
    drupal_set_message(dt("A File is created at  %file_url ", array('%file_url' => file_create_url(($file_url)))));
    return ($file);
  }
    drupal_set_message(t("A File is created at  %file_url ", array('%file_url' => file_create_url(($file_url)))));
    if (!drupal_is_cli()) {
  drupal_set_message(l('Download',file_create_url(($file_url))));
  }
   drupal_set_message(l('Download',file_create_url(($file_url))));
  }
  else {
    $error_operation = reset($operations);
    drupal_set_message(t("An error occurred while processing @operation with arguments : @args", array(
      '@operation' => $error_operation[0],
      '@args' => print_r($error_operation[0], TRUE),
    )));
  }
    
}

/**
 * Creates CSV file and downloads it to the System.
 */
function metatag_import_export_csv_start_batch_upload($nid,$type_name,$delimter,&$context)
{

    $node = node_load($nid);
    $node_title = $node->title;
    $path = "node/" . $nid;
    $meta_title = (isset($node->metatags[$node->language]['title']['value']) ? metatag_import_export_csv_token_change($node,$node->metatags[$node->language]['title']['value']) :metatag_import_export_csv_default_token($node,$type,'title') );
    $meta_description = (isset($node->metatags[$node->language]['description']['value']) ? metatag_import_export_csv_token_change($node,$node->metatags[$node->language]['description']['value']) : metatag_import_export_csv_default_token($node,$type,'description'));
    $meta_abstract = (isset($node->metatags[$node->language]['abstract']['value']) ? metatag_import_export_csv_token_change($node,$node->metatags[$node->language]['abstract']['value']) :metatag_import_export_csv_default_token($node,$type,'abstract')) ;
    $meta_keywords = (isset($node->metatags[$node->language]['keywords']['value']) ? metatag_import_export_csv_token_change($node,$node->metatags[$node->language]['keywords']['value']) :metatag_import_export_csv_default_token($node,$type,'keywords') );
    if (!empty($meta_title) || !empty($meta_description) || !empty($meta_abstract) || !empty($meta_keywords)) {
      $row = array(
        $node_title,
        $path,
        $meta_title,
        $meta_description,
        $meta_abstract,
        $meta_keywords,
      );
      $context['results']['row'][]=$row;
      $context['results']['delimeter']=$delimter;
      $context['results']['type']=$type_name;
    }

}

/**
 * Function to convert token values 
 */
function metatag_import_export_csv_token_change($node,$meta_value)
{
     global $user;
     $find=array(
'0'=>'/\[current-page:title\]/',
 '1'=>'/\[current-page:url\]/',
);
$replace=array(
    '0'=>'[node:title]',
 '1'=>'[node:url]',
);
$token_change=preg_replace($find,$replace,$meta_value);
$options=array(
   'clear' => TRUE,
);
  $val = token_replace($token_change, array('node' => $node,'current-user'=>$user),$options);
 $sanitized_value=filter_xss($val);
 $final_meta_value=preg_replace( "/\r|\n/", "", $sanitized_value );
 return $final_meta_value;
}

/*
 * Function to get the value of metatag grom global configuration.
 */
function metatag_import_export_csv_default_token($node,$type,$fieldname)
{
$token_value=metatag_config_load_with_defaults("node:".$type)[$fieldname]['value'];
$default_meta_value=metatag_import_export_csv_token_change($node,$token_value);
return $default_meta_value;

}

