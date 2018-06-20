<?php

function _eatlas_branding_overview() {
	$branding_vocabulary = taxonomy_vocabulary_machine_name_load(EATLAS_BRANDING_VOCABULARY);
	if ($branding_vocabulary === NULL) {
		watchdog('eatlas_branding', 'WARNING: ' . EATLAS_BRANDING_VOCABULARY . ' vocabulary missing!', array(), WATCHDOG_WARNING);
		// Throw exception ?
	}
	// Load all terms for that vocabulary
	$terms = taxonomy_term_load_multiple(array(), array('vid' => $branding_vocabulary->vid));

	$output = '<h3>' . t('eAtlas brandings overview') . '</h3>';

	$header = array(t('Title'), t('Operations'));
	$rows = array();
	foreach($terms as $term) {
		$rows[] = array(
			$term->name,
			l(t('edit'), "admin/config/eatlas/eatlas_branding/$term->tid"),
		);
	}
	$output .= theme('table', array('header' => $header, 'rows' => $rows, 'empty' => t('No branding available.')));

	return $output;
}

/**
 * Module configuration callback
 */
function _eatlas_branding_admin($form, &$form_state, stdClass $branding = NULL) {
	$values = eatlas_branding_branding_load($branding ? $branding->tid : NULL);

	// Get contact categories
	$categories = db_select('contact', 'c')
		->addTag('translatable')
		->fields('c', array('cid', 'category'))
		->orderBy('weight')
		->orderBy('category')
		->execute()
		->fetchAllKeyed();

	// Get brandings
	$branding_vocabulary = taxonomy_vocabulary_machine_name_load(EATLAS_BRANDING_VOCABULARY);
	if ($branding_vocabulary === NULL) {
		watchdog('eatlas_branding', 'WARNING: ' . EATLAS_BRANDING_VOCABULARY . ' vocabulary missing!', array(), WATCHDOG_WARNING);
	}
	// Load all terms for that vocabulary
	$branding_terms = taxonomy_term_load_multiple(array(), array('vid' => $branding_vocabulary->vid));
	$branding_options = array();
	if (!empty($branding_terms)) {
		foreach ($branding_terms as $bid => $term) {
			$branding_options[$bid] = $term->name;
		}
	}

	// Get themes
	$theme_options = array();
	// https://api.drupal.org/api/drupal/includes%21theme.inc/function/list_themes/7
	$all_themes = list_themes();
	foreach($all_themes as $theme_name => $theme_obj) {
		// If the theme is active
		if ($theme_obj->status) {
			$theme_options[$theme_name] = $theme_obj->info['name'];
		}
	}

	// If there are no contact categories, do not display the form.
	if (!$categories) {
		if (user_access('administer contact forms')) {
			drupal_set_message(t('The contact form has not been configured. <a href="@add">Add one or more categories</a> to the form.', array('@add' => url('admin/structure/contact/add'))), 'error');
		} else {
			drupal_not_found();
			drupal_exit();
		}
	}

	$default_category = property_exists($values, 'cid') ? $values->cid : NULL;
	if (!$default_category) {
		$default_category = db_query("SELECT cid FROM {contact} WHERE selected = 1")->fetchField();
	}
	// If there is more than one category available and no default category has
	// been selected, prepend a default placeholder value.
	if (!$default_category) {
		if (count($categories) > 1) {
			$categories = array(0 => t('- Please choose -')) + $categories;
		} else {
			$default_category = key($categories);
		}
	}

	// https://api.drupal.org/api/drupal/developer%21topics%21forms_api_reference.html/7
	$form = array();
	$form['home'] = array(
		'#type' => 'textfield',
		'#title' => t('Home page'),
		'#description' => t('Home page for this branding. Aliases are allowed.'),
		'#default_value' => property_exists($values, 'home') ? $values->home : 'home',
		'#required' => TRUE
	);
	$form['site_name'] = array(
		'#type' => 'textfield',
		'#title' => t('Site name'),
		'#description' => t('The site name display on the page header title. Leave empty to use the default one. Edit the theme configuration to disable it.'),
		'#default_value' => property_exists($values, 'site_name') ? $values->site_name : ''
	);

	if (!empty($theme_options)) {
		$form['theme'] = array(
			'#columns' => array('theme'),
			'#title' => 'Theme',
			'#description' => 'Select the theme for this branding.',
			'#type' => 'select',
			'#default_value' => property_exists($values, 'theme') ? $values->theme : array(variable_get('theme_default', 'none')),
			'#options' => $theme_options,
			'#value_key' => 'theme',
			'#required' => TRUE
		);
	}

	$form['cid'] = array(
		'#type' => 'select',
		'#title' => t('The contact details'),
		'#description' => t('The contact details for this branding, to determine which email address ' .
				'to use in the contact form. <a href="@manage">[manage contacts]</a>',
				array('@manage' => url('admin/structure/contact'))),
		'#default_value' => $default_category,
		'#options' => $categories,
		'#required' => TRUE
	);
	$form['host'] = array(
		'#type' => 'textfield',
		'#title' => t('Domain name'),
		'#description' => t('The HTTP domain name for that branding. It must be the complete host + domaine name.<br/>' .
				'This field must be <strong>UNIQUE</strong> across all brandings.<br/>' .
				'Example: "ts.eatlas.org.au".<br/>' .
				'The eAtlas branding module will fix links when the ' .
				'URL do not match the domain name. For example, a Torres Strait page with links to a page that only ' .
				'support the default branding will be changed so the domain name will match the branding.<br/>' .
				'<strong>NOTE</strong>: If you are using multiple top domain name, ' .
				'you MUST NOT set the $cookie_domain variable in www/sites/default/settings.php config file. ' .
				'Setting that variable will make the login only possible within one top domain.'),
		'#default_value' => property_exists($values, 'host') ? $values->host : '',
		'#required' => TRUE
	);

	$form['geonetwork_categories'] = array(
		'#title' => t('GeoNetwork categories'),
		'#description' => t('List of GeoNetwork categories associated with this branding, one category per line.<br/>' .
				'GeoNetwork records are associated with categories. ' .
				'Since GeoNetwork do not have a concept of branding, this module use those categories to link ' .
				'the records with a given branding. If at least one category "name" in the GeoNetwork record ' .
				'match one of the categories listed here, the record will support this branding.<br/>' .
				'<strong>WARNING:</strong> Categories are case sensitives.'),
		'#type' => 'textarea',
		'#default_value' => property_exists($values, 'geonetwork_categories') ? $values->geonetwork_categories : '',
	);

	// NOTE: $branding_options should never be empty, there will always
	//   be at least the branding associated with the current form.
	if (!empty($branding_options)) {
		$form['pid'] = array(
			'#columns' => array('pid'),
			'#title' => t('Branding policy'),
			'#description' => t('Default value for the "Supported branding" field.'),
			'#required' => TRUE,
			'#type' => 'checkboxes',
			'#default_value' => property_exists($values, 'pid') ? $values->pid : array(),
			'#options' => $branding_options,
			'#value_key' => 'pid'
		);
	}


	// Get a list of most of the pages, as an example
	//   Inspired from: https://gist.github.com/dooug/b102a683d4e4854defba
	//   as suggested here: http://drupal.stackexchange.com/questions/156164/how-can-i-list-all-the-pages-of-a-drupal-site-in-csv-format
	$page_options = array();
	$result = db_query('select path, title from menu_router')->fetchAll();
	foreach ($result as $obj) {
		$page_options[$obj->path] = array(
			'source' => 'menu_router',
			'title' => $obj->title
		);
	}

	$result = db_query('select link_path, link_title from menu_links')->fetchAll();
	foreach ($result as $obj) {
		$page_options[$obj->link_path] = array(
			'source' => 'menu_links',
			'title' => $obj->link_title
		);
	}

	// Filter out pages that are cluttering the list
	$filtered_page_options = array();
	foreach ($page_options as $url => $obj) {
		if (
				// Remove "pattern" URLs
				strpos($url, '%') === FALSE &&
				// The front page is brandable
				$url !== '<front>' &&
				// Admin pages are ALWAYS allowed
				$url !== 'admin' && !eatlas_commons_starts_with($url, 'admin/') &&
				// All nodes should have branding fields
				!preg_match("/^node\/[0-9]+$/", $url) &&
				// External URL
				!eatlas_commons_starts_with($url, 'http://')) {
			$filtered_page_options[$url] = $obj;
		}
	}

	// Order by key (URL)
	uksort($filtered_page_options, 'strnatcasecmp');

	// Generate a table that will be shown in the field description.
	// NOTE: This is a hack because I can't find a "'#type' => 'display'"
	//   equivalent in the forms API.
	$pages_list_html = '</div></div>'; // Hack to "go out" of the field description
	$pages_list_html .= '<div class="pages_list">';
	$pages_list_html .= '<label>' . t('Some URL examples') . '</label>';
	$pages_list_html .= '<table>';
	$pages_list_html .= '<thead>';
	$pages_list_html .= '<tr>';
	$pages_list_html .= '<th class="url">' . t('Path') . '</th>';
	$pages_list_html .= '<th class="title">' . t('Title') . '</th>';
	$pages_list_html .= '</tr>';
	$pages_list_html .= '</thead>';
	$pages_list_html .= '<tbody>';
	$pages_list_html .= '</table>';
	$pages_list_html .= '<div class="pages_list_content">';
	$pages_list_html .= '<table>';
	$pages_list_row_class = 'odd';
	foreach ($filtered_page_options as $path => $data) {
		$pages_list_html .= '<tr class="'.$pages_list_row_class.'">';
		$pages_list_html .= '<td class="url"><a href="' . url($path) . '" target="_blank">' . htmlspecialchars($path) . '</a></td>';
		$pages_list_html .= '<td class="title">' . (empty($data['title']) ? '<em>' . t('UNTITLED') . '</em>' : htmlspecialchars($data['title'])) . '</td>';
		$pages_list_html .= '</tr>';
		$pages_list_row_class = ($pages_list_row_class == 'odd' ? 'even' : 'odd');
	}
	$pages_list_html .= '</tbody>';
	$pages_list_html .= '</table>';
	// Those are closed at the top
	//$pages_list_html .= '</div></div>';



	$form['pages'] = array(
		'#title' => t('System pages allowed with this branding'),
		'#description' => t('This field is used to configure the "eAtlas Branding system ' .
				'pages filter" module. It list all the pages that can not be branded but ' .
				'should be allowed with this branding, like the contact page, the views ' .
				'pages, etc<br/>.' .
				'The field should list all path, one path per line. You may use * as meta character.<br/>' .
				'<b>NOTE:</b> Pages that can be branded, such as nodes and images, do not need ' .
				'to be listed here. !page_list', array('!page_list' => $pages_list_html)),
		'#type' => 'textarea',
		'#rows'=> 15,
		'#default_value' => property_exists($values, 'pages') ? $values->pages : '',
	);

	// TODO
	//$form['pages_list'] = array(
	//	'#markup' => $pages_list_html
	//);

	$form['actions'] = array('#type' => 'actions');
	$form['actions']['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Save')
	);

	if (!empty($branding->tid)) {
		$form['tid'] = array(
			'#type' => 'hidden',
			'#value' => $branding->tid
		);
	}

	return $form;
}


function _eatlas_branding_admin_validate($form, &$form_state) {
	// TODO
	// $form_state['values']
}

function _eatlas_branding_admin_submit($form, &$form_state) {
	if (isset($form_state['values']['tid'])) {
		$branding = eatlas_branding_branding_load($form_state['values']['tid']);
		$branding->tid = $form_state['values']['tid'];
		$branding->home = $form_state['values']['home'];
		$branding->site_name = $form_state['values']['site_name'];
		$branding->theme = $form_state['values']['theme'];
		$branding->cid = $form_state['values']['cid'];
		$branding->host = $form_state['values']['host'];
		$branding->geonetwork_categories = $form_state['values']['geonetwork_categories'];
		$branding->pages = $form_state['values']['pages'];
		$branding->pid = array();
		foreach ($form_state['values']['pid'] as $pid => $pvalue) {
			if ($pvalue) {
				$branding->pid[] = $pid;
			}
		}

		eatlas_branding_branding_save($branding);
	}
}

// Display a personalised title for "Edit branding" pages
function _eatlas_branding_title($branding) {
	return "Edit $branding->name branding settings";
}

?>
