/**
 * @file
 * Provides JavaScript for Klaviyo.
 */

// Must be in the global namespace so that the Klaviyo 3rd party script may
// access this variable.
var _learnq = _learnq || [];

(function ($) {

Drupal.behaviors.klaviyo = {
  attach: function (context) {
    var klaviyo = Drupal.settings.klaviyo || {}

    if (klaviyo.public_api_key) {
      _learnq.push(['account', klaviyo.public_api_key]);
    }

    if (klaviyo.identify) {
      _learnq.push(['identify', klaviyo.identify]);
    };

    var b = document.createElement('script'); b.type = 'text/javascript'; b.async = true;
    b.src = ('https:' == document.location.protocol ? 'https://' : 'http://') + 'a.klaviyo.com/media/js/analytics/analytics.js';
    var a = document.getElementsByTagName('script')[0]; a.parentNode.insertBefore(b, a);
  }
};

})(jQuery);
