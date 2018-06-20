// We define a function that takes one parameter named $.
(function ($) {
	// Use jQuery with the shortcut:
	$(document).ready(
		function() {
			$("iframe").attr('src', function(i, src) {
				//return src + '?ea_branding_iframe=true';
				console.log(i);
				return src;
			});
		}
	);
// Here we immediately call the function with jQuery as the parameter.
}(jQuery));
